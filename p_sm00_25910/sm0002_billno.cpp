/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************	
*  程序名称			: sm0002_billno 隶属:SM00
*  程序描述			: 发货单据查询
*  备注说明			: 9000
*  修改历史			: 		
*  		2011-12-13 	吴新			(ADD)程序建立
*			... ...
* ***********************************/
#include "stdafx.h"


   


 
int f_sm0002_billno(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);

/*<remark>=========================================================
/// <summary>
/// 发货单据查询
/// <para>
/// 1.根据传入的库区号，查询当前的准发单据号。
/// </para>
/// <para>数据库表：TSMPE02(准发材料表)         </para>
/// <para>主调用函数：前台SM0002画面调用。   </para>
/// </summary>
/// <param name="c_stock_no">库区号    </param>
/// <returns>准发单据号</returns>
===========================================================</remark>*/
// service入口
 
BM2F_ENTERACE(sm0002_billno)
	/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0002_billno(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0; 
	
	CModel tsmpe01("TSMPE01");

   /* ***** 程序变量 ***** */
   CString c_user=" ",c_mat_kind=" ",datetime=" ";
   CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

   /* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3("");    
	
   /* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn); 
   
   /* ***** 应用程序开始处理 ***** */
   try
	{
     /* ***** 获取前台参数  ***** */
	//  c_query_type                       = bcls_rec->Tables[0].Rows[0]["query_type" ].ToString().TrimOrBlank();
	  c_stock_no                         = bcls_rec->Tables[0].Rows[0][ "stock_no"].ToString().TrimOrBlank(); 
	//  c_stock_no_end                     = bcls_rec->Tables[0].Rows[0][ "stock_no_end"].ToString().TrimOrBlank();

     /* ***** 获取库区号  ***** */
	  switch(conn->DatabaseKind)
	   {
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
		sqlstr1 = CString(
			     " select distinct ready_bill_no from tsmpe02 where stock_no = @stock_no and confm_status > '4' "  
			             );
			break; 
		}
	 /* ***** 执行SQL   ***** */  
     sqlstr = sqlstr1; 
	 execute_sql.SetCommandText( sqlstr ); 
	 execute_sql.Parameters.Set( "stock_no" , c_stock_no );
	 execute_sql.Parameters.Set( "stock_no_end" , c_stock_no_end ); 

     execute_sql.ExecuteQuery( bcls_ret->Tables[0] );

//	 execute_sql.ExecuteReader(); 
//	 while(execute_sql.Read())
//	   {  
//       execute_sql.Fetch(tsmpe01);
//		 tsmpe01.MergeTo(bcls_ret->Tables[0], false);
//	   }
//	   execute_sql.Close();   
  
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
