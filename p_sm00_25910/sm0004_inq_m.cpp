/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: sm0004_inq_m
*  程序描述			: 发货材料查询
*  备注说明			:
*  修改历史			:
*  		2011-12-27 	wuxin			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"


   



int f_sm0004_inq_m(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);

/*<remark>=========================================================
/// <summary>
/// 发货材料查询
/// <para>
/// 1.根据传入的码单号，查询材料表信息。
/// </para>
/// <para>数据库表：TSMPE12(码单材料表)         </para>
/// <para>主调用函数：前台SM0004画面（点击）调用。   </para>
/// </summary>
/// <param name="c_stacking_no">码单号    </param>
/// <returns>对应的材料表信息</returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm0004_inq_m)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0004_inq_m(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0; 

	//CModel tsmpe01("TSMPE01");

	/* ***** 程序变量 ***** */
	CString c_user=s.userid,c_mat_kind=" ",datetime=" ";  
	CString c_stacking_no  = " ";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3("");    

	/* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn); 

	/* ***** 应用程序开始处理 ***** */
	try
	{
		/* ***** 获取前台参数  ***** */
		c_stacking_no = bcls_rec->Tables[0].Rows[0]["stacking_no"].ToString().TrimOrBlank();
		Log::Info("" , __FUNCTION__ , "码单号=[{0}]" , c_stacking_no);
		/* ***** 获取库区号  ***** */
		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
			sqlstr1 = CString(
				" select * from tsmsm03 where stacking_no = @stacking_no "
				); 
			break; 
		}
		/* ***** 执行SQL   ***** */
		sqlstr = sqlstr1;   
		execute_sql.SetCommandText( sqlstr ); 
		execute_sql.Parameters.Set("stacking_no",c_stacking_no);

		Log::Trace("" , __FUNCTION__ , "sqlstr=[{0}]" , sqlstr);
		execute_sql.ExecuteQuery( bcls_ret->Tables[0] );

	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{ 
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;  
	}
	catch(const CApplicationException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应 
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1; 
	}

	catch(const CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1; 
	}

	return doFlag;
}
