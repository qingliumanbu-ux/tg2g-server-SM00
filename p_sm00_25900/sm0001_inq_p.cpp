/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: sm0001_inq_p
*  程序描述			: 准发出厂计划查询
*  备注说明			:
*  修改历史			:
*  		2011-12-14 	wuxin			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"


   



int f_sm0001_inq_p(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);

/*<remark>=========================================================
/// <summary>
/// 准发出厂计划查询
/// <para>
/// 1.根据传入的信息参数，查询准发出厂计划。
/// 2.排序方式：prg_send_time ASC；
/// </para>
/// <para>数据库表：TSMPE00(准发单据表)， TSMPE01(准发计划表)， TSMPE02(准发材料表)       </para>
/// <para>主调用函数：前台SM0001画面F2(查询)调用。   </para>
/// </summary>
/// <param name="c_stock_no">库区号    </param>
/// <param name="c_confm_plan_no">准发计划号               </param>
/// <param name="c_ready_bill_no">准发单据号    </param>
/// <param name="c_order_no_from">合同号头               </param>
/// <param name="c_order_no_to">合同号尾    </param>
/// <param name="c_confm_status_from">准发状态头               </param>
/// <param name="c_confm_status_to">准发状态尾    </param>
/// <param name="c_mat_no_from">材料号头               </param>
/// <param name="c_mat_no_to">材料号尾    </param>
/// <returns>准发计划表信息</returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm0001_inq_p)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0001_inq_p(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0; 

	CModel tsmpe01("TSMPE01");

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
		c_stock_no       = bcls_rec->Tables[0].Rows[0]["stock_no"].ToString().TrimOrBlank();
		c_confm_plan_no  = bcls_rec->Tables[0].Rows[0]["confm_plan_no"].ToString().TrimOrBlank();
		c_ready_bill_no  = bcls_rec->Tables[0].Rows[0]["ready_bill_no"].ToString().TrimOrBlank();
		c_order_no_from  = bcls_rec->Tables[0].Rows[0]["order_no_from"].ToString().TrimOrBlank();
		c_order_no_to    = bcls_rec->Tables[0].Rows[0]["order_no_to"].ToString().TrimOrBlank();
		c_confm_status_from  = bcls_rec->Tables[0].Rows[0]["confm_status_from"].ToString().TrimOrBlank();
		c_confm_status_to    = bcls_rec->Tables[0].Rows[0]["confm_status_to"].ToString().TrimOrBlank();
		c_prg_send_time_from = bcls_rec->Tables[0].Rows[0]["prg_send_time_from"].ToString().TrimOrBlank();
		c_prg_send_time_to   = bcls_rec->Tables[0].Rows[0]["prg_send_time_to"].ToString().TrimOrBlank();
		c_mat_no_from        = bcls_rec->Tables[0].Rows[0]["mat_no_from"].ToString().TrimOrBlank();
		c_mat_no_to          = bcls_rec->Tables[0].Rows[0]["mat_no_to"].ToString().TrimOrBlank();
		//c_heat_no            = bcls_rec->Tables[0].Rows[0]["heat_no"].ToString().TrimOrBlank();
		c_factory_div        = bcls_rec->Tables[0].Rows[0]["factory_div"].ToString().TrimOrBlank();

		Log::Info("" , __FUNCTION__ , "stock_no=[{0}]"			, c_stock_no);
		Log::Info("" , __FUNCTION__ , "confm_plan_no=[{0}]"		, c_confm_plan_no);
		Log::Info("" , __FUNCTION__ , "ready_bill_no=[{0}]"		, c_ready_bill_no);
		Log::Info("" , __FUNCTION__ , "order_no_from=[{0}]"		, c_order_no_from);
		Log::Info("" , __FUNCTION__ , "order_no_to=[{0}]"		, c_order_no_to);
		Log::Info("" , __FUNCTION__ , "confm_status_from=[{0}]"	, c_confm_status_from);
		Log::Info("" , __FUNCTION__ , "confm_status_to=[{0}]"	, c_confm_status_to);
		Log::Info("" , __FUNCTION__ , "prg_send_time_from=[{0}]", c_prg_send_time_from);
		Log::Info("" , __FUNCTION__ , "prg_send_time_to=[{0}]"	, c_prg_send_time_to);
		Log::Info("" , __FUNCTION__ , "mat_no_from=[{0}]"		, c_mat_no_from);
		Log::Info("" , __FUNCTION__ , "mat_no_to=[{0}]"			, c_mat_no_to);
		Log::Info("" , __FUNCTION__ , "factory_div=[{0}]"		, c_factory_div);
	
		if	(c_prg_send_time_to == "00010101")	c_prg_send_time_to = " ";

		/* ***** 获取库区号  ***** */
		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
			sqlstr1 = CString(
				" SELECT DISTINCT t1.*,decode(t1.confm_status,1,'计划编制',2,'计划确认',3,'计划释放',4,'计划执行',5,'计划完成','') confm_status2 "
				" FROM tsmpe01 t1, tsmpe00 t2, tsmpe02 t3 "  //准发计划表，准发单据表，准发材料表
				"  WHERE t1.confm_plan_no = t2.confm_plan_no "
				"  AND t2.ready_bill_no = t3.ready_bill_no  "
				"  AND	T1.CONFM_STATUS < 9 "
				); 
			if	(c_stock_no.Trim() != "" )		sqlstr1 += " AND t1.stock_no   = @stock_no ";
			if	(c_confm_plan_no.Trim() != "")	sqlstr1	+= " AND t1.confm_plan_no  LIKE @confm_plan_no || '%' ";
			if	(c_ready_bill_no.Trim() != "")	sqlstr1	+= " AND t2.ready_bill_no  LIKE @ready_bill_no || '%' ";
			if	(c_order_no_from.Trim() != "")	sqlstr1	+= " AND t2.order_no  >= @order_no_from ";
			if	(c_order_no_to.Trim() != "")	sqlstr1	+= " AND t2.order_no  <= @order_no_to ";
			if	(c_confm_status_from.Trim() != "")	sqlstr1	+= " AND t1.confm_status  >= @confm_status_from ";
			if	(c_confm_status_to.Trim() != "")	sqlstr1	+= " AND t1.confm_status  <= @confm_status_to ";
			if	(c_mat_no_from.Trim() != "")	sqlstr1	+= " AND t3.mat_no  >= @mat_no_from ";
			if	(c_mat_no_to.Trim() != "")		sqlstr1	+= " AND t3.mat_no  <= @mat_no_to ";
//			if	(c_factory_div.Trim() != "")	sqlstr1 += " AND t3.factory_div = @factory_div ";
			if	(c_prg_send_time_from.Trim() != "")	sqlstr1 += " AND t1.prg_send_time >= @prg_send_time_from " ;
			if	(c_prg_send_time_to.Trim() != "")	sqlstr1 += " AND t1.prg_send_time <= @prg_send_time_to || '24' " ;
			sqlstr1 += " ORDER BY t1.prg_send_time ASC " ;
			sqlstr2 = CString(
				"select code_desc_1_content from tep0002 where code_class = 'SM28' and code = @code"
				);
			break; 
		}
		/* ***** 执行SQL   ***** */ 
		sqlstr = sqlstr1;   
		execute_sql.SetCommandText( sqlstr ); 
		execute_sql.Parameters.Set( "stock_no", c_stock_no.Trim());
		execute_sql.Parameters.Set( "confm_plan_no", c_confm_plan_no.Trim());
		execute_sql.Parameters.Set( "ready_bill_no", c_ready_bill_no.Trim());
		execute_sql.Parameters.Set( "order_no_from", c_order_no_from.Trim());
		execute_sql.Parameters.Set( "order_no_to", c_order_no_to.Trim());
		execute_sql.Parameters.Set( "confm_status_from", c_confm_status_from.Trim());
		execute_sql.Parameters.Set( "confm_status_to", c_confm_status_to.Trim());
		execute_sql.Parameters.Set( "prg_send_time_from", c_prg_send_time_from.Trim());
		execute_sql.Parameters.Set( "prg_send_time_to", c_prg_send_time_to.Trim());
		execute_sql.Parameters.Set( "mat_no_from", c_mat_no_from.Trim());
		execute_sql.Parameters.Set( "mat_no_to", c_mat_no_to.Trim());
//		execute_sql.Parameters.Set( "heat_no", c_heat_no);
		execute_sql.Parameters.Set( "factory_div", c_factory_div);

		execute_sql.ExecuteQuery( bcls_ret->Tables[0] );
		//for (int i = 0; i < bcls_ret->Tables[0].Rows.get_Count(); i++)
		//{
		//	c_code = bcls_ret->Tables[0].Rows[i]["confm_status"].ToString();
		//	sqlstr = sqlstr2;
		//	execute_sql.Parameters.Clear();
		//	execute_sql.SetCommandText( sqlstr );
		//	execute_sql.Parameters.Set( "code", c_code);
		//	execute_sql.ExecuteReader();  
		//
		//	while(execute_sql.Read())
		//	{   
		//		bcls_ret->Tables[0].Rows[i]["CONFM_STATUS2"]          = execute_sql.GetString(1);
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
