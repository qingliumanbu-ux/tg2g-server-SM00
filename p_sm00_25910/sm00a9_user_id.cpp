/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************	
*  程序名称			: sm00a9_user_id 隶属:SM00
*  程序描述			: 操作者查询
*  备注说明			: 9000
*  修改历史			: 		
*  		2011-12-13 	吴新			(ADD)程序建立
*			... ...
* ***********************************/
#include "stdafx.h"
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;   
//#include "tsmpe01.h"

 
int f_sm00a9_user_id(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);

/*<remark>=========================================================
/// <summary>
/// 操作者查询
/// <para>
/// 1.从配置表信息查询出操作者信息
/// 2.排序方式：ENAME ASC；
/// </para>
/// <para>数据库表：TESUSERINFO(用户信息表) ，TESGROUPINFO(群组信息表)       </para>
/// <para>主调用函数：前台SMN00A9画面（空间点击）调用。   </para>
/// </summary>
/// <returns>用户信息ENAME,CNAME</returns>
===========================================================</remark>*/
// service入口 
BM2F_ENTERACE(sm00a9_user_id)
	/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm00a9_user_id(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0;

   /* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr("");    
	
   /* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn);
  
   /* ***** 给返回块定义列名 ***** */


   /* ***** 应用程序开始处理 ***** */
   try
	{
		bcls_ret->Tables[0].Columns.Add(DT_STRING,"ENAME");
		bcls_ret->Tables[0].Columns.Add(DT_STRING,"CNAME");
     /* ***** 获取库区号  ***** */
	  switch(conn->DatabaseKind)
	   {
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
		sqlstr = CString(
			     " SELECT ENAME , CNAME  FROM TESUSERINFO  union select name ,groupdescription  from tesgroupinfo ORDER BY ENAME ASC" );
		sqlstr = CString(
			" SELECT ENAME , CNAME  FROM TESUSERINFO  ORDER BY ENAME ASC");
			break; 
		}
	 /* ***** 执行SQL   ***** */
	 execute_sql.SetCommandText( sqlstr );
	 // execute_sql.Parameters.Set( "user_id" , "admin" );
	 fetchRowCount = execute_sql.ExecuteQuery(bcls_ret->Tables[0]);
/*	 execute_sql.ExecuteReader();
	 fetchRowCount = 0;
	 while(execute_sql.Read())
	   {  
         	 bcls_ret->Tables[0].Rows.Add();
		 bcls_ret->Tables[0].Rows[fetchRowCount]["ENAME"] = execute_sql.GetString(1); 
		 bcls_ret->Tables[0].Rows[fetchRowCount]["CNAME"] = execute_sql.GetString(2); 
		 fetchRowCount++;
	   }
	   execute_sql.Close();  */ 
  
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
	//	strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应 
		Log::Error("" , __FUNCTION__ , "error=[{0}]", s.msg );  
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
