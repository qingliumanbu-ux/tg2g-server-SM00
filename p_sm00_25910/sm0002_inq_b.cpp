/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: sm0002_inq_b
*  程序描述			: 准发单据查询
*  备注说明			:
*  修改历史			:
*  		2008-8-19 	BM2IDE			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"


   
 
 
int f_sm0002_inq_b(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn); 
/*<remark>=========================================================
/// <summary>
/// 准发单据查询
/// <para>
/// 1.根据传入的参数信息，查询发货计划表。
/// </para>
/// <para>数据库表：TSMPE10(发货计划表)         </para>
/// <para>主调用函数：前台SM0002画面F2(查询)调用。   </para>
/// </summary>
/// <param name="c_stock_no">库区号    </param>
/// <param name="c_ready_bill_no">准发单据号               </param>
/// <param name="c_bill_of_lading_no">提货单号    </param>
/// <param name="c_order_no_from">合同号头               </param>
/// <param name="c_order_no_to">合同号尾    </param>
/// <param name="c_mat_no_from">材料号头               </param>
/// <param name="c_mat_no_to">材料号尾    </param>
/// <returns>准发单据信息</returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm0002_inq_b)
	/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0002_inq_b(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0; 
	
	CModel tsmpe02("TSMPE02");

   /* ***** 程序变量 ***** */
   CString c_user=s.userid,c_mat_kind=" ",datetime=" ";
   //CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	 CString c_stock_no=" "; 
	 CString c_ready_bill_no=" ";
     CString c_bill_of_lading_no=" ";
	 CString c_order_no_from=" ";
	 CString c_order_no_to=" "; 
	 CString c_mat_no_from=" ";
	 CString c_mat_no_to=" "; 
	 CString c_factory_div=" ";
	 CString       c_code = " ";
	 CString       c_plan_status = " ";
	 CString       c_trnp_mode_code = " ";
	 CString       c_bill_of_lading_no_from = " ";
	 CString       c_bill_of_lading_no_to = " ";
   /* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3("");    
	
   /* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn); 
	CDbCommand cmd_inq(conn);

   
   /* ***** 应用程序开始处理 ***** */
   try
	{
     /* ***** 获取前台参数  ***** */ 
		/* ***** 获取前台参数  ***** */
	   c_stock_no = bcls_rec->Tables[0].Rows[0]["STOCK_NO"].ToString().TrimOrBlank();// 库区号
	   c_plan_status = bcls_rec->Tables[0].Rows[0]["PLAN_STATUS"].ToString().TrimOrBlank();// 计划状态
	   c_trnp_mode_code = bcls_rec->Tables[0].Rows[0]["TRNP_MODE_CODE"].ToString().TrimOrBlank();// 运输方式
	   c_bill_of_lading_no_from = bcls_rec->Tables[0].Rows[0]["BILL_OF_LADING_NO_FROM"].ToString().TrimOrBlank();// 开始提单号
	   c_bill_of_lading_no_to = bcls_rec->Tables[0].Rows[0]["BILL_OF_LADING_NO_TO"].ToString().TrimOrBlank();// 结束提单号
	   c_order_no_from = bcls_rec->Tables[0].Rows[0]["ORDER_NO_FROM"].ToString().TrimOrBlank();// 开始合同号
	   c_order_no_to = bcls_rec->Tables[0].Rows[0]["ORDER_NO_TO"].ToString().TrimOrBlank();// 结束合同号


	   /* ***** 打印输入参数 ***** */
	   Log::Info("", __FUNCTION__, "c_stock_no=[{0}]", c_stock_no);
	   Log::Info("", __FUNCTION__, "c_plan_status=[{0}]", c_plan_status);
	   Log::Info("", __FUNCTION__, "c_trnp_mode_code=[{0}]", c_trnp_mode_code);
	   Log::Info("", __FUNCTION__, "c_bill_of_lading_no_from=[{0}]", c_bill_of_lading_no_from);
	   Log::Info("", __FUNCTION__, "c_bill_of_lading_no_to=[{0}]", c_bill_of_lading_no_to);
	   Log::Info("", __FUNCTION__, "c_order_no_from=[{0}]", c_order_no_from);
	   Log::Info("", __FUNCTION__, "c_order_no_to=[{0}]", c_order_no_to);


	   /*if (c_stock_no.Trim() == "")
	   {
		   strcpy(s.msg, "库区号不可为空！");
		   throw CApplicationException(-1, s.msg, s.svc_name);
	   }*/

	   sqlstr = " select * from tsmsm01 "
		   " where 1=1 ";

	   if (c_stock_no.Trim() != "") sqlstr += " AND stock_no = @c_stock_no  ";
	   if (c_bill_of_lading_no_from.Trim() != "") sqlstr += " AND bill_of_lading_no >= @c_bill_of_lading_no_from ";
	   if (c_bill_of_lading_no_to.Trim() != "") sqlstr += " AND bill_of_lading_no <= @c_bill_of_lading_no_to  ";
	   if (c_order_no_from.Trim() != "") sqlstr += " AND order_no >= @c_order_no_from ";
	   if (c_order_no_to.Trim() != "") sqlstr += " AND order_no <= @c_order_no_to  ";
	   if (c_plan_status.Trim() != "") sqlstr += " AND plan_status = @c_plan_status  ";
	   if (c_trnp_mode_code.Trim() != "") sqlstr += " AND trnp_mode_code = @c_trnp_mode_code  ";

	   Log::Info("", __FUNCTION__, "sqlstr=[{0}]", sqlstr);



	   // SQL语句中的变量赋值
	   cmd_inq.SetCommandText(sqlstr);
	   cmd_inq.Parameters.Set("c_order_no_from", c_order_no_from);
	   cmd_inq.Parameters.Set("c_order_no_to", c_order_no_to);
	   cmd_inq.Parameters.Set("c_bill_of_lading_no_from", c_bill_of_lading_no_from);
	   cmd_inq.Parameters.Set("c_bill_of_lading_no_to", c_bill_of_lading_no_to);
	   cmd_inq.Parameters.Set("c_stock_no", c_stock_no.Trim());
	   cmd_inq.Parameters.Set("c_plan_status", c_plan_status.Trim());
	   cmd_inq.Parameters.Set("c_trnp_mode_code", c_trnp_mode_code.Trim());
	   cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);
	   cmd_inq.Close();
 
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
