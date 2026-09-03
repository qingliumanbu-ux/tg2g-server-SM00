/****************************************************
*	程序功能：	装车材料增加						*
*	编制日期：	2023-2-3							*
*	编制人员：	013801								*
*	传入参数：	计划号、合同号、车号、材料号		*
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

int f_sm00_load_wt_check(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	/* 装车重量超重检查 */

// service入口
//BM2F_ENTERACE(smbw07_add)
/* -EP_SYSTEM_HEAD_END */
int f_sm00_plan_mat_add(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);
	/*程序用变量*/
	int blkNum = 0;
	int	ret = 0;
	int	doFlag = 0;
	CString mat_no = "";
	CString vehicle_no = "";
	CString COLOR_MARK = "";	// 可发货标记1--可发，0--不可发，2--在别的计划中，3--红冲请求中

	CString datetime = "";
	/*实体对象*/
	CModel tsmpe02("TSMPE02");
	/* ***** 数据库操作类定义 ***** */
	CString sqlstr;
	CDbCommand cmd_inq(conn);

	try
	{
		vehicle_no = bcls_rec->Tables[0].Rows[0]["VEHICLE_NO"].ToString().Trim();
		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			tsmpe02["MAT_NO"] = bcls_rec->Tables[0].Rows[i]["MAT_NO"].ToString().Trim();
			tsmpe02["BILL_OF_LADING_NO"] = bcls_rec->Tables[0].Rows[i]["BILL_OF_LADING_NO"].ToString().Trim();
			tsmpe02["ORDER_NO"] = bcls_rec->Tables[0].Rows[i]["ORDER_NO"].ToString().Trim();
			tsmpe02["VEHICLE_NO"] = vehicle_no;
			COLOR_MARK = bcls_rec->Tables[0].Rows[i]["COLOR_MARK"].ToString().Trim();
			
			Log::Debug("", "", "COLOR_MARK=【{0}】", COLOR_MARK);

			// 计划上有车号时取计划上的车号
			sqlstr = "select VEHICLE_NO from tsmpe10 where BILL_OF_LADING_NO = '" + tsmpe02["BILL_OF_LADING_NO"].ToString() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				if (cmd_inq.GetString(1).Trim() != "")
				{
					tsmpe02["VEHICLE_NO"] = cmd_inq.GetString(1);
				}

			}
			cmd_inq.Close();

			if (COLOR_MARK.Trim() != "1")
			{
				CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString(),COLOR_MARK };
				CMessageFormat::Format(s.msg, "此材料【{0}】不符合装车条件，标记【{1}】！", arguments, 2);
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}
			//liguangyuan 20230908 add 判定材料已经操作装车确认了(F4发货确认)
			tsmpe02["TICKET_NO"] = Db::QueryCString(" select ticket_no from tsmpe02 where mat_no ='" + tsmpe02["MAT_NO"].ToString() + "' ");
			if (tsmpe02["TICKET_NO"].ToString().Trim() != "")
			{
				CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString(),tsmpe02["TICKET_NO"].ToString() };
				CMessageFormat::Format(s.msg, "此材料【{0}】已经生成装车单号【{1}】，不能操作装车！", arguments, 2);
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			sqlstr = "UPDATE TSMPE02 WHERE MAT_NO = '" + tsmpe02["MAT_NO"].ToString() + "' ";
			tsmpe02.Update("VEHICLE_NO,BILL_OF_LADING_NO,ORDER_NO", "MAT_NO");
		}


		// 调用装车重量检查函数
		EIClass bcls_rec_check;
		bcls_rec_check.Tables[0].Columns.Add(DT_STRING, "VEHICLE_NO");
		bcls_rec_check.Tables[0].Columns.Add(DT_STRING, "BILL_OF_LADING_NO");

		bcls_rec_check.Tables[0].Rows.Add();
		int ii = bcls_rec_check.Tables[0].Rows.get_Count() - 1;
		bcls_rec_check.Tables[0].Rows[ii]["VEHICLE_NO"] = vehicle_no;
		bcls_rec_check.Tables[0].Rows[ii]["BILL_OF_LADING_NO"] = tsmpe02["BILL_OF_LADING_NO"].ToString();

		doFlag = f_sm00_load_wt_check(&bcls_rec_check, bcls_ret, conn);
		if (doFlag != 0)
		{
			throw CApplicationException(doFlag, s.msg, log.Location);
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
