/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      013801
Version:     1.0
Date:        2023-2-24
Description: 车号修改
**************************************************/
#include "stdafx.h"		// 框架头，不可删除

#include "epex.h"


//外部函数声明


BM2_FUNCTION_EXPORT


int f_xxsm02_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序用变量 */
	int	    doFlag = 0;				// 调用本函数的返回值
	int		fetchRowCount = 0;
	int		i = 0;
	int		ret = 0;

	/* 实体类定义 */
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");
	CModel tsmpe10("TSMPE10");

	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_loop(conn);
	CString sqlstr;
	CString tc_no = "7000S4";
	CString	block_name_master = "BODY";
	CString	block_name_detail = "DETAIL";

	try
	{
		CString datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");;

		CString ticket_no="",vehicle_no="";
		CString oper_flag = "";

		/* ***** 创建电文处理对象 ***** */
		EPEX epex(&s);

		for (i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString();	//装车单
			vehicle_no = bcls_rec->Tables[0].Rows[i]["VEHICLE_NO"].ToString();	//车号

			if (bcls_rec->Tables[0].Columns.Contains("oper_flag"))
			{
				oper_flag = bcls_rec->Tables[0].Rows[0]["oper_flag"].ToString();
			}

			Log::Debug("", "", "TICKET_NO=【{0}】，", ticket_no);

			sqlstr = "select * from tsmpe11 where TICKET_NO = '" + ticket_no + "' ";
			cmd_inq_loop.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq_loop.ExecuteReader();
			while (cmd_inq_loop.Read())
			{
				cmd_inq_loop.Fetch(tsmpe11);
				sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = '" + tsmpe11["BILL_OF_LADING_NO"].ToString() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					cmd_inq.Fetch(tsmpe10);
				}
				else
				{
					CFormattable arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, "没有读取到提单号【{0}】的计划", arguments, 1);//格式化字符串
					throw	CApplicationException(-1, s.msg, log.Location);
				}
				cmd_inq.Close();

				tc_no = tsmpe10["REC_CREATOR"].ToString().Substring(2, 2) + tsmpe10["REC_CREATOR"].ToString().Substring(0, 2) + "02";	//电文号


				// 初始化电文格式
				if (epex.Initialize(tc_no) < 0)   //电文号
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0001464")/*初始化电文时失败！ 原因描述：[ {0}]*/, arguments, 1);//格式化字符串
					throw	CApplicationException(-1, s.msg, log.Location);
				}


				if (epex.SetValue(block_name_master, "STACKING_NO", 0, tsmpe11["STACKING_NO"].ToString()) < 0	// 码单号
					|| epex.SetValue(block_name_master, "VEHICLE_NO", 0, vehicle_no) < 0			// 新车号
					|| epex.SetValue(block_name_master, "VEHICLE_NO_OLD", 0, tsmpe11["VEHICLE_NO"].ToString()) < 0 // 原车号
					|| epex.SetValue(block_name_master, "OPERATE_TIME", 0, datetime) < 0	// 操作时间
					|| epex.SetValue(block_name_master, "OPERATOR", 0, s.username) < 0	// 操作者
					)
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, "写入电文数据时出错! 原因描述： [{0}]", arguments, 1);//格式化字符串
					throw	CApplicationException(-1, s.msg, log.Location);
				}

				if (oper_flag.Trim() != "")
				{
					epex.SetValue(block_name_master, "oper_flag", 0, oper_flag);
				}

				// 发送电文
				if (epex.SendTele() < 0)
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
					throw	CApplicationException(-1, s.msg, log.Location);
				}

				// 释放
				epex.Uninitialize();
			}
			cmd_inq_loop.Close();
		}

	}
	catch ( CDbException& ex )  //捕获数据库操作异常
	{
		CFormattable arguments[] ={ ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);

		strncpy(s.sysmsg, (const char*)str, sizeof(s.msg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	if (doFlag < 0)
	{
		//CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0} :{1}", arguments, 2);
	}

	return doFlag;
}
