/****************************************************
*	程序功能：检查材料封锁标记						*
*	编制日期：2015-2-4								*
*	编制人员：t013801								*
*	传入参数：材料号、物料表名称					*
*	返回参数：0	成功	-1	失败					*
*	出错描述：s.msg									*
*****************************************************
****************************************************/
#include "stdafx.h"		// 框架头，不可删除

//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明

BM2_FUNCTION_EXPORT
 int  f_sm00_hold_flag_chk(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int	blkseq = 0;
	int rows = 0;
	int i=0;

	/* 业务变量 */
	CString	sqlstr("") ;              // 数据库SQL操作字符串
	CString	dateTime("");
	CString	blkname("hold_flag");
	CString	mat_no("");
	CString	table_name("");
	CString v_hold_flag( "0" );

	/* 实体类定义 */

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);

	try
	{
		dateTime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		/* 判块是否存在 */
		blkseq = bcls_rec->Tables.IndexOf( blkname ); 		//读取块名所在的块号
		if	(blkseq < 0)	bcls_rec->Tables[0].set_TableName(blkname);

		rows = bcls_rec->Tables[blkname].Rows.get_Count();
		for (i = 0 ; i < rows  ; i++ )
		{
			mat_no	= bcls_rec->Tables[blkname].Rows[i]["mat_no"].ToString().TrimOrBlank();	// 材料号
			table_name	= bcls_rec->Tables[blkname].Rows[i]["table_name"].ToString().TrimOrBlank();

			//传入的参数打印
			Log::Info("" , __FUNCTION__ ,"第[{0}]条记录，共[{1}]条记录",i+1,rows);
			Log::Info("" , __FUNCTION__ ,"材料号=[{0}]"		, mat_no);
			Log::Info( "" , __FUNCTION__ , "表名=[{0}]" , table_name );

			/* 数据库操作 */
			switch(conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr = " SELECT HOLD_FLAG FROM  " + table_name +	" WHERE mat_no = @mat_no ";

				break;
			}

			v_hold_flag = "0";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("mat_no", mat_no);	// SQL语句中的变量赋值
			cmd_inq.ExecuteReader();
			if	(cmd_inq.Read() )
			{
				v_hold_flag = cmd_inq.GetString( 1 );
			}
			cmd_inq.Close();

			if ( v_hold_flag != "0" )
			{
				Log::Debug( "" , __FUNCTION__ , "封锁标记=[{0}]" , v_hold_flag );
				throw CApplicationException( -1 , s.msg , s.svc_name );
			}

			sprintf	(s.msg , _RES("GCRSS0000002")/*处理成功。*/);
		}
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = {  ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("" , __FUNCTION__ , "error=[{0}]", str );

		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
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
