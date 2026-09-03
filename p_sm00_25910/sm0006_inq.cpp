/// <summary>
/// 功能说明：
/// <version>1.0.0.0</para>
/// <creator>13801 ShenMingQi</creator>
/// <history>2015-7-17 文件创建</history>
/// </summary>

/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2010
Author:   13801
Version:    3.0
Date:     2015-7-17
Description: 磅差处理
**************************************************/

#include "stdafx.h"		// 框架头，不可删除

#include "tsmpe11.h"
#include "tsmpe10.h"

//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明

/*<remark>=========================================================
/// <summary>
/// 准发材料及准发红冲请求查询
/// <para>
/// 到ED54表上读取显示的配置项；
/// 1.根据传入的合同号、发货计划号、材料号、准发计划号、运输方式、红冲标志查询出符合条件的材料。
/// 2.根据查询的材料到物料表上读取当前的库位信息。
/// </para>
/// </summary>
/// <returns>指定连铸机下的炉次制造命令信息</returns>
===========================================================</remark>*/


// Service 入口
BM2F_ENTERACE(sm0006_inq)

int f_sm0006_inq(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int ret = 0;
	int blknum = 0;
	int count = 0;
	int blkseq = 0;
	int i=0;

	/* 业务变量 */
	CString	sqlstr("");              // 数据库SQL操作字符串
	CString	dateTime("");
	CString	blkname("");
	CString tab_name("");

	CString c_vehicle_no = " ";
	CString c_load_vehicle_no = " ";
	CString c_start_date = " ";
	CString c_end_date = " ";

	/* 实体类定义 */
	CTSMPE11	tsmpe11(conn);
	CTSMPE10	tsmpe10(conn);

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq1(conn);
	try
	{
		dateTime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		/* 自定义查询显示项读取 */
		EIClass	in01;
		in01.Tables[0].Columns.Add(DT_STRING , "function_id");	// 传入的功能名
		in01.Tables[0].Rows.Add();   //新增空行
		in01.Tables[0].Rows[0]["function_id"] = "SM0006_INQ1";	// 在EDA2中定义
		bcls_ret->blk_now = 1;										// 返回的块号
		ret = f_edsetcustominfo(&in01 , bcls_ret);			// 读取字段名
		if ( ret != 0 )
		{
			sprintf(s.sysmsg , "调用函数f_edsetcustominfo出错[%s]" , s.msg);
			Log::Trace("" , __FUNCTION__ , s.sysmsg);
			throw	CApplicationException(-1 , s.msg , log.Location);
		}

		/* 读取传入的参数 */
		c_vehicle_no = bcls_rec->Tables[0].Rows[0]["VEHICLE_NO"];	// 按字段名称读取
		c_load_vehicle_no = bcls_rec->Tables[0].Rows[0]["LOAD_VEHICLE_NO"];
		c_start_date = bcls_rec->Tables[0].Rows[0]["START_DATE"];
		c_end_date = bcls_rec->Tables[0].Rows[0]["END_DATE"];

		/* 打印传入的参数 */
		Log::Info("" , __FUNCTION__ , "VEHICLE_NO = [{0}]" , c_vehicle_no);
		Log::Info("" , __FUNCTION__ , "LOAD_VEHICLE_NO = [{0}]" , c_load_vehicle_no);
		Log::Info("" , __FUNCTION__ , "START_DATE = [{0}]" , c_start_date);
		Log::Info("" , __FUNCTION__ , "END_DATE = [{0}]" , c_end_date);

		/* 数据库操作 */

		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr	=	" SELECT	MAX(A.VEHICLE_NO) ,SUBSTR(A.STACKING_NO,1,9),MAX(A.BILL_OF_LADING_NO),SUM(A.STACKING_WT)   "
						" ,	SUM(A.STACKING_DISCREP_WT) , MAX(A.REC_CREATE_TIME) , MAX(A.REC_CREATOR) , MAX(A.FACTORY_DIV) "
						" FROM		tsmpe11 A  "
						" WHERE		A.REC_CREATE_TIME BETWEEN @c_start_date AND @c_end_date "
						" AND		A.STACKING_STATUS  = '1' "
						" AND		A.WT_MODE = '1' ";
	//					" AND		A.STACKING_NO IN (SELECT STACKING_NO FROM TSMPE12 B WHERE A.STACKING_NO = B.STACKING_NO AND		B.WT_MODE = '1') ";

			if ( c_vehicle_no.Trim() != "" )
			{
				sqlstr += " AND A.VEHICLE_NO = @c_vehicle_no ";
			}
			if ( c_load_vehicle_no.Trim() != "" )
			{
				sqlstr += " AND A.STACKING_NO LIKE @c_load_vehicle_no" ;
			}
			sqlstr += " GROUP BY SUBSTR(A.STACKING_NO,1,9) " ;
			break;
		}
		Log::Info("" , __FUNCTION__ , "sqlstr = [{0}]" , sqlstr);

		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("c_start_date" , c_start_date);	// SQL语句中的变量赋值
		cmd_inq.Parameters.Set("c_end_date" , c_end_date);
		cmd_inq.Parameters.Set("c_vehicle_no" , c_vehicle_no);
		cmd_inq.Parameters.Set("c_load_vehicle_no" , c_load_vehicle_no + "%");
		cmd_inq.ExecuteReader();

		/* 循环从游标中读取记录，压回前台 */
		count = 0;
		while( cmd_inq.Read() )
		{
			//cmd_inq.Fetch(tsmpe11);
			i = 0;
			tsmpe11.VEHICLE_NO = cmd_inq.GetString(++i);
			tsmpe11.STACKING_NO = cmd_inq.GetString(++i);
			tsmpe11.BILL_OF_LADING_NO = cmd_inq.GetString(++i);
			tsmpe11.STACKING_WT = cmd_inq.GetDecimal(++i);
			tsmpe11.STACKING_DISCREP_WT = cmd_inq.GetDecimal(++i);
			tsmpe11.REC_CREATE_TIME = cmd_inq.GetString(++i);
			tsmpe11.REC_CREATOR = cmd_inq.GetString(++i);
			tsmpe11.FACTORY_DIV = cmd_inq.GetString(++i);
			// 将结果放入返回块
			//tsmpe11.MergeTo(bcls_ret->Tables[blknum] , true);   //true是以block的定义为准 ,false是以头文件结构覆盖block
			count++ ;

			// 读取提单量和完成量
			if ( tsmpe11.FACTORY_DIV.Trim()=="BW" )
			{
				tab_name = "TSMBW10";
			}
			else if(tsmpe11.FACTORY_DIV.Trim() == "HP")
			{
				tab_name = "TSMHP10";
			}
			else
			{
				tab_name = "TSMPE10";
			}			

			sqlstr = " SELECT SUM(PLAN_WT) FROM " + tab_name + " WHERE BILL_OF_LADING_NO = @tsmpe11.BILL_OF_LADING_NO ";
			cmd_inq1.SetCommandText(sqlstr);
			cmd_inq1.Parameters.Set("tsmpe11.BILL_OF_LADING_NO" , tsmpe11.BILL_OF_LADING_NO);	// SQL语句中的变量赋值
			cmd_inq1.ExecuteReader();
			if ( cmd_inq1.Read() )
			{
				tsmpe10.PLAN_WT = cmd_inq1.GetDecimal(1);
			}
			cmd_inq1.Close();

			// 读取提单完成量+磅差量
			sqlstr = " SELECT SUM( STACKING_WT + STACKING_DISCREP_WT) FROM TSMPE11 WHERE BILL_OF_LADING_NO = @tsmpe11.BILL_OF_LADING_NO "; 
			cmd_inq1.SetCommandText(sqlstr);
			cmd_inq1.Parameters.Set("tsmpe11.BILL_OF_LADING_NO" , tsmpe11.BILL_OF_LADING_NO);	// SQL语句中的变量赋值
			cmd_inq1.ExecuteReader();
			if ( cmd_inq1.Read() )
			{
				tsmpe10.DELIVY_WT = cmd_inq1.GetDecimal(1);
			}
			cmd_inq1.Close();

			CDataRow& row = bcls_ret->Tables[blknum].Rows.Add();   //新增空行
			row.Merge(tsmpe10);   //将实体类的值写入新增行中
			row.Merge(tsmpe11);   //写入第2个实体类到该行中

		}//while
		cmd_inq.Close();	// 关闭游标

		Log::Trace("" , __FUNCTION__ , "query records. [{0}]", bcls_ret->Tables[blknum].Rows.get_Count() );	// 读取了多少条记录
		sprintf	(s.msg , "共读取到[%d]条记录" , count);
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = {  ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("" , __FUNCTION__ , "error=[{0}]", str);  

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
