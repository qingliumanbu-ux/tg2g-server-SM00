#include "stdafx.h"
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;   

 
BM2F_ENTERACE(sm0003ab_inq2)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0003ab_inq2(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	 
	CString bill_of_lading_no = " ";
	CString bill_of_lading_detailno = " ";
	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr("");    

	/* ***** 数据库操作类定义 ***** */  
	CDbCommand cmd_inq(conn);
	/* ***** 应用程序开始处理 ***** */
	try
	{
		bill_of_lading_no = bcls_rec->Tables[0].Rows[0]["bill_of_lading_no"].ToString().TrimOrBlank();
		bill_of_lading_detailno = bcls_rec->Tables[0].Rows[0]["bill_of_lading_detailno"].ToString().TrimOrBlank();
		Log::Trace("", __FUNCTION__, "bill_of_lading_no = [{0}]", bill_of_lading_no);
		Log::Trace("", __FUNCTION__, "bill_of_lading_detailno = [{0}]", bill_of_lading_detailno);
		 
		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	// DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	    // Oracle 数据库
		default: // 所有数据库适用，通用SQL语句	
			sqlstr = "select a.*,b.stock_desc from tsmpe23 a left outer join twm01 b on a.stock_no=b.stock_no "
				" where 1=1 ";

			if (bill_of_lading_no.Trim() != "")
			{
				sqlstr += " and a.bill_of_lading_no=@bill_of_lading_no  ";
			}
			if (bill_of_lading_detailno.Trim() != "")
			{
				sqlstr += " and a.bill_of_lading_detailno=@bill_of_lading_detailno";
			}
			 
			break;
		}

		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("bill_of_lading_no", bill_of_lading_no);
		cmd_inq.Parameters.Set("bill_of_lading_detailno", bill_of_lading_detailno);
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
