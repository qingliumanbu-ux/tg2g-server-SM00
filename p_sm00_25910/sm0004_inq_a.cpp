/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: sm0004_inq_a
*  程序描述			: 码单汇总查询
*  备注说明			:
*  修改历史			:
*  		2011-12-27 	wuxin			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"


   



int f_sm0004_inq_a(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);

/*<remark>=========================================================
/// <summary>
/// 码单汇总查询
/// <para>
/// 1.根据传入的参数信息，查询当前生成的码单信息。
/// </para>
/// <para>数据库表：TSMPE11(发货码单表) ,TSMPE12(码单材料表)        </para>
/// <para>主调用函数：前台SM0004画面F2(查询)调用。   </para>
/// </summary>
/// <param name="c_delivy_time_from">发货时间起    </param>
/// <param name="c_delivy_time_to">发货时间末               </param>
/// <param name="c_stacking_no_from">码单号头    </param>
/// <param name="c_stacking_no_to">码单号尾               </param>
/// <param name="c_order_no_from">合同号头    </param>
/// <param name="c_order_no_to">合同号尾               </param>
/// <param name="c_bill_of_lading_no_from">提货单号头    </param>
/// <param name="c_bill_of_lading_no_to">提货单号尾               </param>
/// <param name="c_mat_no_from">材料号头    </param>
/// <param name="c_mat_no_to">材料号尾               </param>
/// <param name="c_stock_no">库区号    </param>
/// <param name="c_vehicle_no">车牌号               </param>
/// <returns>发货码单信息</returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm0004_inq_a)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0004_inq_a(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0; 

	//CModel tsmpe01("TSMPE01");

	/* ***** 程序变量 ***** */
	CString c_user=s.userid,c_mat_kind=" ",datetime=" "; 

	CString c_factory_div  = " ";
	CString c_delivy_time_from  = " ";
	CString c_delivy_time_to  = " ";
	CString c_stacking_no_from  = " ";
	CString c_stacking_no_to  = " ";
	CString c_order_no_from  = " ";
	CString c_order_no_to  = " ";
	CString c_bill_of_lading_no_from  = " ";
	CString c_bill_of_lading_no_to  = " ";
	CString c_mat_no_from  = " ";
	CString c_mat_no_to  = " ";
	CString c_stock_no  = " ";
	CString c_vehicle_no  = " ";
	CString c_print_mark = "";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3("");    

	/* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn); 

	/* ***** 应用程序开始处理 ***** */
	try
	{
		/* ***** 获取前台参数  ***** */
		c_factory_div = bcls_rec->Tables[0].Rows[0]["factory_div"].ToString().TrimOrBlank();
		c_delivy_time_from = bcls_rec->Tables[0].Rows[0]["delivy_time_from"].ToString().TrimOrBlank();
		c_delivy_time_to = bcls_rec->Tables[0].Rows[0]["delivy_time_to"].ToString().TrimOrBlank();
		c_stacking_no_from = bcls_rec->Tables[0].Rows[0]["stacking_no_from"].ToString().TrimOrBlank();
		c_stacking_no_to = bcls_rec->Tables[0].Rows[0]["stacking_no_to"].ToString().TrimOrBlank();
		c_order_no_from = bcls_rec->Tables[0].Rows[0]["order_no_from"].ToString().TrimOrBlank();
		c_order_no_to = bcls_rec->Tables[0].Rows[0]["order_no_to"].ToString().TrimOrBlank();
		c_bill_of_lading_no_from = bcls_rec->Tables[0].Rows[0]["bill_of_lading_no_from"].ToString().TrimOrBlank();
		c_bill_of_lading_no_to = bcls_rec->Tables[0].Rows[0]["bill_of_lading_no_to"].ToString().TrimOrBlank();
		c_mat_no_from = bcls_rec->Tables[0].Rows[0]["mat_no_from"].ToString().TrimOrBlank();
		c_mat_no_to = bcls_rec->Tables[0].Rows[0]["mat_no_to"].ToString().TrimOrBlank();
		c_stock_no = bcls_rec->Tables[0].Rows[0]["stock_no"].ToString().TrimOrBlank();
		c_vehicle_no = bcls_rec->Tables[0].Rows[0]["vehicle_no"].ToString().TrimOrBlank();
		c_print_mark = bcls_rec->Tables[0].Rows[0]["print_mark"].ToString().TrimOrBlank();

		if	(c_delivy_time_to.Trim()	== "00010101")	c_delivy_time_to = "";
		if	(c_delivy_time_from.Trim()	== "00010101")	c_delivy_time_from = "";
		Log::Info("", __FUNCTION__, "c_stock_no=[{0}]", c_stock_no);
		/* ***** 获取库区号  ***** */
		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
			sqlstr1 = CString(" SELECT * FROM TSMSM05 a  WHERE  1 = 1 "	); 

			/*if	(c_delivy_time_from.Trim() != "" )	sqlstr1 += " AND SUBSTR(a.REC_CREATE_TIME, 1, 8) >= @delivy_time_from ";
			if	(c_delivy_time_to.Trim() != "" )	sqlstr1 += " AND SUBSTR(a.REC_CREATE_TIME, 1, 8) <= @delivy_time_to ";*/
			if	(c_delivy_time_from.Trim() != "" )	sqlstr1 += " AND a.REC_CREATE_TIME>= '"+ c_delivy_time_from +"' ";
			if	(c_delivy_time_to.Trim() != "" )	sqlstr1 += " AND a.REC_CREATE_TIME <= '"+ c_delivy_time_to +"' ";
			if	(c_stacking_no_from.Trim() != "" )	sqlstr1 += " AND a.stacking_no >= '"+ c_stacking_no_from +"' ";
			if	(c_stacking_no_to.Trim() != "" )	sqlstr1 += " AND a.stacking_no <= '"+ c_stacking_no_to +"' ";
			if	(c_order_no_from.Trim() != "" )		sqlstr1 += " AND a.order_no >= '"+ c_order_no_from +"' ";
			if	(c_order_no_to.Trim() != "" )		sqlstr1 += " AND a.order_no <= '"+ c_order_no_to +"' ";
			if	(c_bill_of_lading_no_from.Trim() != "" )	sqlstr1 += " AND a.bill_of_lading_no >= '"+ c_bill_of_lading_no_from +"' ";
			if	(c_bill_of_lading_no_to.Trim() != "" )	sqlstr1 += " AND a.bill_of_lading_no <= '"+ c_bill_of_lading_no_to +"' ";
			if	(c_stock_no.Trim() != "")			sqlstr1 += " AND A.STOCK_NO = '"+ c_stock_no +"' ";
			if	(c_vehicle_no.Trim() != "")			sqlstr1 += " AND A.VEHICLE_NO = '"+ c_vehicle_no +"' ";
			if	(c_factory_div.Trim() != "")		sqlstr1 += " AND A.FACTORY_DIV = '"+ c_factory_div +"' ";
			//sqlstr1 += " AND exists ( select 1 from tsmpe12 b where a.stacking_no = b.stacking_no " ;
			if	(c_mat_no_from.Trim() != "" )		sqlstr1 += " AND b.mat_no >= '"+ c_mat_no_from +"' ";
			if	(c_mat_no_to.Trim() != "" )			sqlstr1 += " AND b.mat_no <= '"+ c_mat_no_to +"' ";
			//sqlstr1 += " ) ";
			if ( c_print_mark.Trim() == "0" )		sqlstr1 += " AND A.STACKING_PRINTS = 0 ";
			if ( c_print_mark.Trim() == "1" )		sqlstr1 += " AND A.STACKING_PRINTS > 0 ";
			break; 
		}

		Log::Info("" , __FUNCTION__ , "c_delivy_time_from=[{0}]", c_delivy_time_from); 
		Log::Info("" , __FUNCTION__ , "c_delivy_time_to=[{0}]", c_delivy_time_to); 
		/* ***** 执行SQL   ***** */
		sqlstr = sqlstr1;  
		Log::Trace("" , __FUNCTION__ , "sqlstr=[{0}]" , sqlstr); 
		execute_sql.SetCommandText( sqlstr ); 
		/*execute_sql.Parameters.Set("factory_div",c_factory_div);
		execute_sql.Parameters.Set("delivy_time_from", c_delivy_time_from.Trim());
		execute_sql.Parameters.Set("delivy_time_to", c_delivy_time_to.Trim());
		execute_sql.Parameters.Set("stacking_no_from",c_stacking_no_from.Trim());
		execute_sql.Parameters.Set("stacking_no_to",c_stacking_no_to.Trim());
		execute_sql.Parameters.Set("order_no_from",c_order_no_from.Trim());
		execute_sql.Parameters.Set("order_no_to",c_order_no_to.Trim());
		execute_sql.Parameters.Set("bill_of_lading_no_from",c_bill_of_lading_no_from.Trim());
		execute_sql.Parameters.Set("bill_of_lading_no_to",c_bill_of_lading_no_to.Trim());
		execute_sql.Parameters.Set("mat_no_from",c_mat_no_from.Trim());
		execute_sql.Parameters.Set("mat_no_to",c_mat_no_to.Trim());
		execute_sql.Parameters.Set("stock_no",c_stock_no.Trim());
		execute_sql.Parameters.Set("vehicle_no",c_vehicle_no.Trim());*/

		execute_sql.ExecuteQuery( bcls_ret->Tables[0] );
		execute_sql.Close();

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
