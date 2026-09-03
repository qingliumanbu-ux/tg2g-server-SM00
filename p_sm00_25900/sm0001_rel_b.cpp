/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smbw01_rel_b
*  程序描述			: 准发单据释放
*  备注说明			:
*  修改历史			:
*  		2012-05-22 	BM2IDE			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"

BM2F_ENTERACE(sm0001_rel_b)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0001_rel_b(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0,ret = 0; 

	CModel tsmpe02("TSMPE02");

	/* ***** 程序变量 ***** */
	CString c_user=s.userid,c_mat_kind=" ",datetime=" ";
	//CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	CString c_stock_no=" ";
	CString c_confm_plan_no=" "; 
	CString c_ready_bill_no=" "; 
	CString c_confm_status =" ";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3(""),sqlstr4(""),sqlstr5("");      

	/* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn); 

	/* ***** 应用程序开始处理 ***** */
	try
	{
		/* ***** 获取前台参数  ***** */ 
		/**************** 接收准发材料 **********************/ 
		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++) 
		{ 
			c_ready_bill_no              = bcls_rec->Tables[0].Rows[i]["ready_bill_no"].ToString().TrimOrBlank(); 

			if (c_ready_bill_no.Compare(" ") == 0)
			{
				break;
			}

			/* ***** 获取库区号  ***** */
			switch(conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default: 
				sqlstr1 = CString(
					" SELECT confm_status,confm_plan_no from tsmpe00 where ready_bill_no = @ready_bill_no "
				 ); 


				sqlstr2 = CString(
					" UPDATE	tsmpe00 "
					"           SET		rec_revisor     = @c_user, "
					"                      rec_revise_time = to_char(sysdate,'yyyymmddhh24miss'), "
					"                      confm_status = '3' "
					"            WHERE	ready_bill_no = @ready_bill_no "
					"              and confm_status IN ('2', '3') "
				 ); 

				sqlstr3 = CString(
					" UPDATE	tsmpe02 "
					"           SET		rec_revisor     = @c_user, "
					"                      rec_revise_time = to_char(sysdate,'yyyymmddhh24miss'), "
					"                      confm_status = '3' "
					"            WHERE	ready_bill_no = @ready_bill_no "
					"              and confm_status IN ('2', '3') "
				 ); 

				sqlstr4 = CString(
					" UPDATE	tsmpe01 a "
					"           SET		rec_revisor     = @c_user, "
					"                      rec_revise_time = to_char(sysdate,'yyyymmddhh24miss'), "
					"                      confm_status = '3' "
					"            WHERE	confm_plan_no = @confm_plan_no "
					"              and confm_status IN ('2', '3') "
					"              and not exists (select 1 from tsmpe00 b where a.confm_plan_no = b.confm_plan_no and b.confm_status < '3') "
				 ); 

				break; 
			}

			/* ***** 执行SQL   ***** */    
			sqlstr = sqlstr1;
			execute_sql.SetCommandText( sqlstr ); 
			execute_sql.Parameters.Set( "ready_bill_no" , c_ready_bill_no );
			execute_sql.ExecuteReader();  

			if ( execute_sql.Read() )
			{
				c_confm_status                = execute_sql.GetString(1); 
				c_confm_plan_no                = execute_sql.GetString(2); 
			}
			execute_sql.Close();

			/* ***** 判断计划的状态   ***** */  
			//	if (c_confm_status.Compare("2") != 0 && c_confm_status.Compare("3") != 0)
			if (c_confm_status.Compare("2") != 0)
			{
				doFlag = -1;
				{CFormattable arguments[] = { c_ready_bill_no, c_confm_status }; // 定义参数列表的数组
				CMessageFormat::Format (s.msg , _RES("SM00S0001456")/*单据号[{0}]的准发状态为[{1}]， 不能释放。*/ , arguments ,2);
				} 
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			/* ***** 修改准发计划的状态   ***** */  
			sqlstr = sqlstr2;
			execute_sql.SetCommandText( sqlstr ); 
			execute_sql.Parameters.Set( "c_user" , c_user );
			execute_sql.Parameters.Set("ready_bill_no" , c_ready_bill_no ); 
			execute_sql.ExecuteNonQuery();  

			/* ***** 修改准发单据的状态   ***** */  
			sqlstr = sqlstr3;
			execute_sql.SetCommandText( sqlstr ); 
			execute_sql.Parameters.Set( "c_user" , c_user );
			execute_sql.Parameters.Set( "ready_bill_no" , c_ready_bill_no ); 
			execute_sql.ExecuteNonQuery();  

			/* ***** 修改准发材料的状态   ***** */  
			sqlstr = sqlstr4;
			execute_sql.SetCommandText( sqlstr ); 
			execute_sql.Parameters.Set( "c_user" , c_user );
			execute_sql.Parameters.Set( "confm_plan_no" , c_confm_plan_no ); 
			execute_sql.ExecuteNonQuery();  


		} 


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
