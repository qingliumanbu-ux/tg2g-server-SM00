/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      013801
Version:     1.0
Date:        2023-1-10 12:45:22
Description: 计划结案
**************************************************/
#include "stdafx.h"		// 框架头，不可删除

#include "epex.h"

//名称空间引用




//外部函数声明


BM2_FUNCTION_EXPORT


int f_xxsm04_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
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
	CString sqlstr;
	CString tc_no = "7000S4";
	CString	block_name_master = "BODY";
	CString	block_name_detail = "DETAIL";

	try
	{
		CString datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");;

		CString order_no = "", contract_no = "", bill_of_lading_no = "";
		bill_of_lading_no = bcls_rec->Tables[0].Rows[0]["BILL_OF_LADING_NO"].ToString();	//提单号
		order_no = bcls_rec->Tables[0].Rows[0]["ORDER_NO"].ToString();	//合同号

		CString oper_flag = "";
		if (bcls_rec->Tables[0].Columns.Contains("oper_flag"))
		{
			oper_flag = bcls_rec->Tables[0].Rows[0]["oper_flag"].ToString();
		}

		Log::Debug("", "", "bill_of_lading_no=【{0}】，order_no=【{1}】", bill_of_lading_no, order_no);

		sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = '" + bill_of_lading_no + "' AND '" + order_no + "' IN (ORDER_NO ,CONTRACT_NO) ";
		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			cmd_inq.Fetch(tsmpe10);
		}
		else
		{
			CFormattable arguments[] = { bill_of_lading_no, order_no };// 定义参数列表的数组
			CMessageFormat::Format(s.msg, "没有读取到提单号【{0}】合同【{0}】的计划", arguments, 2);//格式化字符串
			throw	CApplicationException(-1, s.msg, log.Location);
		}
		cmd_inq.Close();

		tc_no = tsmpe10["REC_CREATOR"].ToString().Substring(2, 2) + tsmpe10["REC_CREATOR"].ToString().Substring(0, 2) + "01";	//电文号

		/* ***** 创建电文处理对象 ***** */
		EPEX epex(&s);

		// 初始化电文格式
		if (epex.Initialize(tc_no) < 0)   //电文号
		{
			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
			CMessageFormat::Format(s.msg, _RES("SM00S0001464")/*初始化电文时失败！ 原因描述：[ {0}]*/, arguments, 1);//格式化字符串
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		if (epex.SetValue(block_name_master, "BILL_OF_LADING_NO", 0, bill_of_lading_no) < 0	// 提单号
			|| epex.SetValue(block_name_master, "ORDER_NO_SALE", 0, order_no) < 0			// 合同号
			|| epex.SetValue(block_name_master, "FINISHED_WT", 0, tsmpe10["DELIVY_WT"].ToDecimal()) < 0 // 重量
			|| epex.SetValue(block_name_master, "SENDING_NUM", 0, tsmpe10["DELIVY_NUM"].ToDecimal()) < 0	// 件数
			|| epex.SetValue(block_name_master, "PICK_FINISH_DATE", 0, datetime.Substring(0,8)) < 0	// 出厂日期
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

		Log::Trace("", __FUNCTION__, "111111111111111111 ");
		// 发送电文
		if (epex.SendTele() < 0)
		{
			{
				CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
			}
			//sprintf(s.msg,"发送电文时失败! 原因描述: %s", epex.GetMsg());//转换前
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		// 释放
		Log::Trace("", __FUNCTION__, "2222222222222222222222222 ");
		epex.Uninitialize();
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
