/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: sm0001_inq_b
*  程序描述			: 准发单据查询
*  备注说明			:
*  修改历史			:
*  		2008-8-6 	BM2IDE			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"


   
 
 
int f_sm0001_inq_b(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn); 

/*<remark>=========================================================
/// <summary>
/// 准发单据查询
/// <para>
/// 1.根据传入的准发计划号，从准发单据表查询信息。；
/// </para>
/// <para>数据库表：TSMPE00(准发单据表)         </para>
/// <para>主调用函数：前台SM0001画面F2(查询)调用。   </para>
/// </summary>
/// <param name="c_confm_plan_no">准发计划号    </param>
/// <returns>准发单据表信息</returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm0001_inq_b)
	/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0001_inq_b(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0; 
	
	CModel tsmpe00("TSMPE00");

   /* ***** 程序变量 ***** */
   CString c_user=s.userid,c_mat_kind=" ",datetime=" ";
   //CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	 CString c_stock_no=" ";
	 CString c_confm_plan_no=" ";
	 CString c_ready_bill_no=" ";
	 CString c_order_no_from=" ";
	 CString c_order_no_to=" ";
	 CString c_confm_status_from=" ";
	 CString c_confm_status_to=" ";
	 CString c_prg_send_time_from=" ";
	 CString c_prg_send_time_to=" ";
	 CString c_mat_no_from=" ";
	 CString c_mat_no_to=" ";
	 CString c_heat_no=" ";
	 CString c_factory_div=" ";
     CString c_code = " ";
   /* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3("");    
	
   /* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn); 
   
   /* ***** 应用程序开始处理 ***** */
   try
	{
     /* ***** 获取前台参数  ***** */ 

//	c_factory_div        = bcls_rec->Tables[0].Rows[0]["factory_div"].ToString().TrimOrBlank();

	if (bcls_rec->Tables[0].Columns.Contains("CONFM_PLAN_NO"))
	{
		c_confm_plan_no = bcls_rec->Tables[0].Rows[0]["confm_plan_no"].ToString().TrimOrBlank();
	}
	if (bcls_rec->Tables[0].Columns.Contains("STOCK_NO"))
	{
		c_stock_no = bcls_rec->Tables[0].Rows[0]["STOCK_NO"].ToString().TrimOrBlank();
	}
	if (bcls_rec->Tables[0].Columns.Contains("FACTORY_DIV"))
	{
		c_factory_div = bcls_rec->Tables[0].Rows[0]["FACTORY_DIV"].ToString().TrimOrBlank();
	}

	Log::Debug("", "", "confm_plan_no=【{0}】，STOCK_NO=【{1}】，FACTORY_DIV=【{2}】", c_confm_plan_no, c_stock_no, c_factory_div);
     /* ***** 获取库区号  ***** */
	  switch(conn->DatabaseKind)
	   {
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
		sqlstr1 = CString(
			     " SELECT t.*,"
			" (select code_desc_1_content from tep0002 where code_class = 'SM28' and code = t.confm_status) CONFM_STATUS2 "
			"FROM tsmpe00 t "
		         " WHERE 1=1 "
		  //       "   AND factory_div   = @factory_div "
				 ); 
		sqlstr2 = CString(
						"select code_desc_1_content from tep0002 where code_class = 'SM28' and code = @code"
				);

			break; 
		}
	  if (c_confm_plan_no.Trim()!="")
	  {
		  sqlstr1 += " and confm_plan_no = @confm_plan_no ";
	  }
	  //if (c_stock_no.Trim() != "")
	  //{
		 // sqlstr1 += " and stock_no = @stock_no ";
	  //}
	  //if (c_factory_div.Trim() != "")
	  //{
		 // sqlstr1 += " and factory_div = @factory_div ";
	  //}
	 /* ***** 执行SQL   ***** */  
		sqlstr = sqlstr1;  
	    execute_sql.SetCommandText( sqlstr );  
		execute_sql.Parameters.Set( "confm_plan_no", c_confm_plan_no); 
		execute_sql.Parameters.Set( "factory_div", c_factory_div);
		execute_sql.Parameters.Set("stock_no", c_stock_no);
		Log::Debug("", "", "sqlstr={0}", sqlstr);

     execute_sql.ExecuteQuery( bcls_ret->Tables[0] );
	    //bcls_ret->Tables[0].Columns.Add(DT_STRING,"CONFM_STATUS2");
		//for (int i = 0; i < bcls_ret->Tables[0].Rows.get_Count(); i++)
		//{
		//	c_code = bcls_ret->Tables[0].Rows[i]["confm_status"].ToString();
		//	sqlstr = sqlstr2;
		//	execute_sql.Parameters.Clear();
		//	execute_sql.SetCommandText( sqlstr );
		//	execute_sql.Parameters.Set( "code", c_code);
		//	Log::Debug("", "", "sqlstr={0}", sqlstr);
		//	execute_sql.ExecuteReader();
	    //
		//	while(execute_sql.Read())
		//	{   
		//	  bcls_ret->Tables[0].Rows[i]["CONFM_STATUS2"]          = execute_sql.GetString(1);
		//	}
		//       
		//	execute_sql.Close();
		//}
 
  
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
