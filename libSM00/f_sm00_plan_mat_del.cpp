/****************************************************
*	程序功能：	装车材料减少						*
*	编制日期：	2023-2-6							*
*	编制人员：	013801								*
*	传入参数：	材料号								*
*	返回参数：	0	成功	-1	失败				*
*	出错描述：	s.msg								*
*****************************************************
*													*
****************************************************/
//框架公用头文件，勿删
#include "stdafx.h"

using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;


//程序用头文件


// service入口
//BM2F_ENTERACE(smbw07_del)
/* -EP_SYSTEM_HEAD_END */
int f_sm00_plan_mat_del(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);
	/*程序用变量*/
	int blkNum = 0;
	int	ret = 0;
	int	doFlag = 0;
	CString mat_no = "";
	CString vehicle_no = "";

	CString datetime = "";
	/*实体对象*/
	CModel tsmpe02("TSMPE02");
	/* ***** 数据库操作类定义 ***** */
	CString sqlstr;
	CDbCommand cmd_inq(conn);

	try
	{
		int v_count = bcls_rec->Tables[0].Rows.get_Count();
		if (v_count == 0)
		{
			sprintf(s.msg, "没有传入处理的参数！");
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}

		for (int i = 0; i < v_count; i++)
		{
			tsmpe02["MAT_NO"] = bcls_rec->Tables[0].Rows[i]["MAT_NO"].ToString().Trim();
			//tsmpe02["BILL_OF_LADING_NO"] = bcls_rec->Tables[0].Rows[i]["BILL_OF_LADING_NO"].ToString().Trim();
			//tsmpe02["ORDER_NO"] = bcls_rec->Tables[0].Rows[i]["ORDER_NO"].ToString().Trim();
			//tsmpe02["VEHICLE_NO"] = vehicle_no;
			
			sqlstr = "select * from tsmpe02 where MAT_NO ='" + tsmpe02["MAT_NO"].ToString() + "' "; 
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe02);
			}
			else
			{
				sprintf(s.msg, "无此材料号【%s】", (const char *)tsmpe02["MAT_NO"].ToString());
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}
			cmd_inq.Close();

			//liguangyuan 20230908 add 判定材料已经操作装车确认了(F4发货确认)
			if (tsmpe02["TICKET_NO"].ToString().Trim() != "")
			{
				CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString(),tsmpe02["TICKET_NO"].ToString() };
				CMessageFormat::Format(s.msg, "此材料【{0}】已经生成装车单号【{1}】，不能操作卸车！", arguments, 2);
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			CString v_order_no = tsmpe02["ORDER_NO"].ToString();
			CString v_bill_of_lading_no = tsmpe02["BILL_OF_LADING_NO"].ToString();

			if (tsmpe02["MAT_KIND"].ToString().Trim() == "BW"
				&& (tsmpe02["CONFM_STATUS"].ToString().Trim() == "4"
				|| tsmpe02["DELIVY_QTY_FLAG"].ToString().Trim() == "1" 
				|| tsmpe02["DELIVY_QTY_FLAG"].ToString().Trim() == "2"))
			{
				v_bill_of_lading_no = " ";
				v_order_no = tsmpe02["OLD_ORDER_NO"].ToString();
			}
			if (tsmpe02["MAT_KIND"].ToString().Trim() == "HP"
				&& (tsmpe02["DELIVY_QTY_FLAG"].ToString().Trim() == "1"
				|| tsmpe02["DELIVY_QTY_FLAG"].ToString().Trim() == "2"))
			{
				v_bill_of_lading_no = " ";
				v_order_no = tsmpe02["ORDER_NO"].ToString();
			}
			sqlstr = "UPDATE TSMPE02 SET VEHICLE_NO = ' ',ORDER_NO = '" + v_order_no + "' , BILL_OF_LADING_NO = '" + v_bill_of_lading_no + "' "
				" WHERE MAT_NO ='" + tsmpe02["MAT_NO"].ToString() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteNonQuery();


		}

	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		Log::Error("", __FUNCTION__, "error=[{0}]", s.sysmsg);
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch(CApplicationException& ex)  //捕获应用错误
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch(CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	if (doFlag < 0)
	{
		//CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
	}

	return doFlag;

}
