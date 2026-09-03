#include "stdafx.h"
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;   


BM2F_ENTERACE(sm0003ab_inq1)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0003ab_inq1(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0; 
 
	CString rma_no = "";
	CString bill_of_lading_no = " "; 
	CString status_h = "";
	CString type = "";
	CString stock_no = "";
	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr("");    

	/* ***** 数据库操作类定义 ***** */  
	CDbCommand cmd_inq(conn);
	/* ***** 应用程序开始处理 ***** */
	try
	{
		bill_of_lading_no = bcls_rec->Tables[0].Rows[0]["bill_of_lading_no"].ToString().TrimOrBlank();
		rma_no = bcls_rec->Tables[0].Rows[0]["rma_no"].ToString().TrimOrBlank();
		status_h = bcls_rec->Tables[0].Rows[0]["status_h"].ToString().TrimOrBlank();
		stock_no = bcls_rec->Tables[0].Rows[0]["stock_no"].ToString().TrimOrBlank();

		switch (conn->DatabaseKind)
		{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	// DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	    // Oracle 数据库
			default: // 所有数据库适用，通用SQL语句	
			sqlstr = " select (select b.bill_of_lading_no from tsmpe11 b where b.stacking_no=a.stacking_no fetch first 1 rows only) bill_of_lading_no_y,a.* from tsmpe22 a "
				    " where 1=1";
			if (bill_of_lading_no.Trim()!="")
			{
				sqlstr += " and bill_of_lading_no=@bill_of_lading_no";
			}
			if (rma_no.Trim()!="")
			{
				sqlstr += " and rma_no=@rma_no";
			}
			if (status_h.Trim() != "")
			{
				sqlstr += " and status_h=@status_h";
			}
			if (type.Trim() == "0")//计划操作区
			{
				sqlstr += " and (delivy_plan_type = '5' or (delivy_plan_type = '4' and trim(trnp_app_no) is not null) ) and status_h = '1', ";
			}
			if (stock_no.Trim()!="")
			{
				sqlstr += " and a.IN_STOCK_CODE = @stock_no ";
			}
			
			sqlstr += " order by rec_create_time desc ";
			break;
		}

		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("bill_of_lading_no", bill_of_lading_no);
		cmd_inq.Parameters.Set("rma_no", rma_no);
		cmd_inq.Parameters.Set("status_h", status_h); 
		cmd_inq.Parameters.Set("stock_no", stock_no);
		Log::Trace("", __FUNCTION__, "sqlstr = [{0}]", sqlstr);
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
