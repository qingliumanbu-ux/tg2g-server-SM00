/****************************************************
*	程序功能：称重处理								*
*	编制日期：2006-9-5 8:56							*
*	编制人员：t013801								*
*	传入参数：装车单号								*
*	返回参数：										*
*	出错描述：s.msg									*
*****************************************************
****************************************************/

#include "stdafx.h"		// 框架头，不可删除

#include "tsmpe11.h"
#include "tsmpe12.h"
#include "tsmpe10.h"

//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明

BM2_FUNCTION_IMPORT
 int f_sm00_record(EIClass *bcls_rec,EIClass *bcls_ret , CDbConnection * conn);					/* 写履历记录 */

BM2F_ENTERACE(sm0006_put)
int	f_sm0006_put( EIClass *bcls_rec , EIClass *bcls_ret , CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int		fetchRowCount	=	0;
	int		ret		=	0;
	int		i = 0;
	double	v_mat_wt = 0;
	int		v_count = 0;
	double	v_stacking_wt = 0 ;

	/* 业务变量 */
	CString	datetime("");
	CString	gh("");															/* 操作者工号 */
	CString	blkname("test");	/* 块名 */
	CString tab_name("");

	/* 实体类定义 */
	CTSMPE11	tsmpe11(conn);
	CTSMPE12	tsmpe12(conn);
	CTSMPE10	tsmpe10(conn);

	CString		sqlstr("");              // 数据库SQL操作字符串
	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq1(conn);

	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");


		//调用 写履历
		EIClass record_bcls_rec;
		record_bcls_rec.Tables[0].Columns.Add(DT_STRING , "mat_no");
		record_bcls_rec.Tables[0].Columns.Add(DT_STRING , "event_mark");
		record_bcls_rec.Tables[0].Columns.Add(DT_STRING , "userid");

		if	(bcls_rec->Tables.Contains(blkname) == false ) 					//指定当前块
		{
			bcls_rec->Tables[0].set_TableName(blkname);
		}

		//读取传入的参数
		int row = bcls_rec->Tables[blkname].Rows.get_Count();
		for	(i = 0 ; i< row ; i++ )
		{
			Log::Info("" , __FUNCTION__ , "第[{0}]条记录，共[{1}]条记录", i+1 , row );
			tsmpe11.STACKING_NO	= bcls_rec->Tables[blkname].Rows[i]["STACKING_NO"];	// 码单号
			tsmpe11.STACKING_WT	= bcls_rec->Tables[blkname].Rows[i]["PONDER_WT"];	// 重量
			tsmpe11.VEHICLE_NO	= bcls_rec->Tables[blkname].Rows[i]["VEHICLE_NO"];	// 车牌号

			Log::Info("" , __FUNCTION__ , "传入的参数1，装车单号=[{0}]", tsmpe11.STACKING_NO);
			Log::Info("" , __FUNCTION__ , "称重重量=[{0}]" , tsmpe11.STACKING_WT.ToDouble() );
			v_stacking_wt	=	tsmpe11.STACKING_WT.ToDouble() ;
			if	(tsmpe11.STACKING_NO.Trim() == "")
			{
				sprintf	(s.msg , _RES("SM00S0001373")/*传入的码单号为空*/);
				throw	CApplicationException(-1 , s.msg , log.Location);
			}

			switch(conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr = " SELECT sum(mat_wt) , count(1) FROM tsmpe12 "
					" WHERE stacking_no	LIKE @tsmpe11.stacking_no || '%' ";		// SQL语句定义
				break;
			}

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("tsmpe11.stacking_no", tsmpe11.STACKING_NO);	// SQL语句中的变量赋值
			cmd_inq.ExecuteReader();		// 语句执行

			if	(cmd_inq.Read())
			{
				v_mat_wt	= cmd_inq.GetDecimal(1).ToDouble();
				v_count		= cmd_inq.GetInt32(2);
			}
			cmd_inq.Close();

			//定义游标
			switch(conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr = " SELECT * FROM tsmpe12 "
					" WHERE stacking_no	LIKE @tsmpe11.stacking_no || '%' ";		// SQL语句定义
				break;
			}

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("tsmpe11.stacking_no", tsmpe11.STACKING_NO);	// SQL语句中的变量赋值
			cmd_inq.ExecuteReader();		// 语句执行

			fetchRowCount = 0;
			while	(cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe12);
				++fetchRowCount ;
				// 计算磅差量=材料重量/总量 * 过磅重量
				if	(fetchRowCount < v_count)
				{
					tsmpe12.MAT_DISCREP_WT = (tsmpe12.MAT_WT / v_mat_wt * tsmpe11.STACKING_WT) - tsmpe12.MAT_WT ;	// 计算前几个的磅差量
					// 由于精度关系对计算出来的磅差进行四舍五入,保留3位小数
					tsmpe12.MAT_DISCREP_WT = tsmpe12.MAT_DISCREP_WT.Round(3);
					v_stacking_wt = v_stacking_wt - tsmpe12.MAT_WT.ToDouble() - tsmpe12.MAT_DISCREP_WT.ToDouble() ;	// 计算剩余量
				}
				if	(fetchRowCount == v_count)	tsmpe12.MAT_DISCREP_WT = v_stacking_wt - tsmpe12.MAT_WT ;	// 计算最后一个材料的差量

				// 更新材料表上的磅差量
				switch(conn->DatabaseKind)
				{
				case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
				case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
				case DB_KIND_MSSQL:	        // MS SQL Server数据库
				case DB_KIND_ORACLE:	        // Oracle 数据库
				default:
					sqlstr = " UPDATE	tsmpe12 "
						" SET	mat_discrep_wt	=	@tsmpe12.mat_discrep_wt "
						" WHERE mat_no			=	@tsmpe12.mat_no ";		// SQL语句定义
					break;
				}

				cmd_inq1.SetCommandText(sqlstr);
				cmd_inq1.Parameters.Set("tsmpe12.mat_discrep_wt" , tsmpe12.MAT_DISCREP_WT);	// SQL语句中的变量赋值
				cmd_inq1.Parameters.Set("tsmpe12.mat_no" , tsmpe12.MAT_NO);	// SQL语句中的变量赋值
				if	(cmd_inq1.ExecuteNonQuery() == 0)		// 语句执行
				{
					CFormattable arguments[] = { tsmpe12.MAT_NO , 1403 };
					CMessageFormat::Format (s.msg , _RES("SM00S0001392")/*更新材料表上的磅差量出错，材料号=[{0}],sqlcode=[{1}]*/ , arguments , 2);
					throw	CApplicationException(-1 , s.msg , log.Location);
				}
	/********************************************************************************
	*			调用函数新增履历记录												*
	********************************************************************************/
				switch(conn->DatabaseKind)
				{
				case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
				case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
				case DB_KIND_MSSQL:	        // MS SQL Server数据库
				case DB_KIND_ORACLE:	        // Oracle 数据库
				default:
					sqlstr = " DELETE	FROM	TSMPEA1 "
						" WHERE mat_no = @tsmpe12.mat_no "
						" AND	EVENT_ID = '9' ";		// SQL语句定义
					break;
				}

				cmd_inq1.SetCommandText(sqlstr);
				cmd_inq1.Parameters.Set("tsmpe12.mat_no" , tsmpe12.MAT_NO);	// SQL语句中的变量赋值
				cmd_inq1.ExecuteNonQuery();		// 语句执行

				/********************************************************************************
				*			调用函数新增履历记录												*
				********************************************************************************/
				bcls_rec->	GetSYS(&s);
				Log::Trace("" , __FUNCTION__ , "★★★★★调用函数函数f_sm00_record开始★★★★★");
				record_bcls_rec.Tables[0].Rows.Clear();
				record_bcls_rec.Tables[0].Rows.Add();
				record_bcls_rec.Tables[0].Rows[0]["mat_no"]	= tsmpe12.MAT_NO;
				record_bcls_rec.Tables[0].Rows[0]["event_mark"]		= "9";
				record_bcls_rec.Tables[0].Rows[0]["userid"]		= s.userid;

				ret =	0;
				ret = f_sm00_record(&record_bcls_rec , bcls_ret , conn);
				if	(ret	!=	0)
				{
					Log::Trace("" , __FUNCTION__ , "★★★★★调用函数函数f_sm00_record 异常结束★★★★★");
					throw	CApplicationException(-1 , s.msg , log.Location);
				}
				Log::Trace("" , __FUNCTION__ , "★★★★★调用函数函数f_sm00_record正常结束★★★★★");


				// 更新码单表上的磅差量
				switch(conn->DatabaseKind)
				{
				case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
				case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
				case DB_KIND_MSSQL:	        // MS SQL Server数据库
				case DB_KIND_ORACLE:	        // Oracle 数据库
				default:
					sqlstr = " UPDATE	tsmpe11 "
						" SET	stacking_discrep_wt = (SELECT SUM(mat_discrep_wt) FROM tsmpe12 WHERE stacking_no	=	@tsmpe12.stacking_no) "
						" WHERE stacking_no			=	@tsmpe12.stacking_no " ;
					break;
				}

				cmd_inq1.SetCommandText(sqlstr);
				cmd_inq1.Parameters.Set("tsmpe12.stacking_no" , tsmpe12.STACKING_NO);	// SQL语句中的变量赋值
				if	(cmd_inq1.ExecuteNonQuery() == 0)		// 语句执行
				{
					CFormattable arguments[] = { tsmpe12.STACKING_NO , 1403 };
					CMessageFormat::Format (s.msg , _RES("SM00S0001377")/*更新码单表上的磅差量出错，码单号=[{0}],sqlcode=[{1}]*/ , arguments , 2);
					throw	CApplicationException(-1 , s.msg , log.Location);
				}
			}
			cmd_inq.Close();
			if	(fetchRowCount == 0)
			{
				CFormattable arguments[] = { tsmpe11.STACKING_NO };
				CMessageFormat::Format (s.msg , _RES("SM00S0001374")/*无此码单[{0}]下的材料*/ , arguments , 1);
				throw	CApplicationException(-1 , s.msg , log.Location);
			}

			// 检查磅差后是否超计划量
			// 读取提单量和完成量
			if ( tsmpe12.FACTORY_DIV.Trim() == "BW" )
			{
				tab_name = "TSMBW10";
			}
			else if ( tsmpe12.FACTORY_DIV.Trim() == "HP" )
			{
				tab_name = "TSMHP10";
			}
			else
			{
				tab_name = "TSMPE10";
			}

			sqlstr = " SELECT SUM(PLAN_WT) FROM " + tab_name + " WHERE BILL_OF_LADING_NO = @tsmpe12.BILL_OF_LADING_NO ";
			cmd_inq1.SetCommandText(sqlstr);
			cmd_inq1.Parameters.Set("tsmpe12.BILL_OF_LADING_NO" , tsmpe12.BILL_OF_LADING_NO);	// SQL语句中的变量赋值
			cmd_inq1.ExecuteReader();
			if ( cmd_inq1.Read() )
			{
				tsmpe10.PLAN_WT = cmd_inq1.GetDecimal(1);
			}
			cmd_inq1.Close();

			// 读取提单完成量+磅差量
			sqlstr = " SELECT SUM( STACKING_WT + STACKING_DISCREP_WT) FROM TSMPE11 WHERE BILL_OF_LADING_NO = @tsmpe12.BILL_OF_LADING_NO ";
			cmd_inq1.SetCommandText(sqlstr);
			cmd_inq1.Parameters.Set("tsmpe12.BILL_OF_LADING_NO" , tsmpe12.BILL_OF_LADING_NO);	// SQL语句中的变量赋值
			cmd_inq1.ExecuteReader();
			if ( cmd_inq1.Read() )
			{
				tsmpe10.DELIVY_WT = cmd_inq1.GetDecimal(1);
			}
			cmd_inq1.Close();

			if ( tsmpe10.DELIVY_WT > tsmpe10.PLAN_WT )
			{
				CFormattable arguments[] = { tsmpe12.BILL_OF_LADING_NO , tsmpe10.DELIVY_WT };
				CMessageFormat::Format(s.msg , "出库总重量大于提单计划量，提单号{0},出库量{1}" , arguments , 2);
				throw	CApplicationException(-1 , s.msg , log.Location);
			}
		}
		sprintf	(s.msg , _RES("SM00S0001378")/*磅差处理成功*/  );
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { _S("TPSSMD1"), ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000017")/*读取数据失败,表[{0}],sqlcode=[{1}]。请联系系统维护人员。*/, arguments, 2);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("" , __FUNCTION__ , "error=[{0}]", str );

		strncpy(s.sysmsg, (const char*)str, sizeof(s.msg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch(CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch(CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
