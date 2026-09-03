/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2010
Author:
Version:    1.0
Date:
Description: 产成品厂内库入库/退库入库
**************************************************/

/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除 
#include "epex.h"

//外部函数声明

BM2_FUNCTION_EXPORT
int f_xxsm05_snd(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* ***** 静态变量定义 ***** */
	/*程序用变量*/
	int doFlag = 0, blkNum, fetchRowCount, row_count = 0, ret = 0;
	int blkSeq = 0;
	int i = 0;


	/* 业务变量 */
	CString	blkname("");	/* 块名 */

	CString tc_no = "7000S4";
	CString	block_name_master = "BODY";
	CString	block_name_detail = "DETAIL";

	CString v_userid = "";
	CString datetime = "";
	CString c_mat_kind = "";
	CString c_factory_div = "";

	CString ts_mark = "";

	CString c_flag = "";
	CString c_stacking_no = "";
	CString bill_of_lading_no = "";
	CString stock_no = "";
	CString mat_no = "";
	CString old_stacking_no = "";

	/* ***** 创建电文处理对象 ***** */
	EPEX epex;

	/* 实体类定义 */
	CString sqlstr;
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_tmmxx01(conn);
	CDataTable tmmxx01;

	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		// 初始化电文格式

		c_flag = bcls_rec->Tables[0].Rows[0]["FLAG"];
		stock_no = bcls_rec->Tables[0].Rows[0]["STOCK_NO"];
		c_mat_kind = bcls_rec->Tables[0].Rows[0]["MAT_KIND"];
		c_factory_div = bcls_rec->Tables[0].Rows[0]["FACTORY_DIV"];

		sqlstr = "select code_desc_4_content from tep0002 where code_class='M00F' and code = @c_factory_div ";
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("c_factory_div", c_factory_div);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			ts_mark = cmd_inq.GetString(1);
		}
		cmd_inq.Close();

		//tc_no = tsmpe10["REC_CREATOR"].ToString().Substring(2, 2) + tsmpe10["REC_CREATOR"].ToString().Substring(0, 2) + "05";	//电文号
		tc_no = ts_mark + "SM05";
		Log::Info("", __FUNCTION__, "tc_no=[{0}]", tc_no);

		if (epex.Initialize(tc_no) < 0)
		{
			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
			CMessageFormat::Format(s.msg, _RES("SM00S0001464")/*初始化电文时失败！ 原因描述：[ {0}]*/, arguments, 1);//格式化字符串			
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		Log::Error("", __FUNCTION__, "c_flag=[{0}]", c_flag);
		if (c_flag == "0")
		{
			for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
			{
				if (bcls_rec->Tables[0].Columns.Contains("STACKING_NO"))
					c_stacking_no = bcls_rec->Tables[0].Rows[i]["STACKING_NO"];
				if (bcls_rec->Tables[0].Columns.Contains("BILL_OF_LADING_NO"))
					bill_of_lading_no = bcls_rec->Tables[0].Rows[i]["BILL_OF_LADING_NO"];

				epex.SetValue(block_name_master, "OP_FLAG", 0, c_flag);
				//epex.SetValue(block_name_master, "ORDER_NO_ERP", 0, order_no_erp);
				epex.SetValue(block_name_master, "STACKING_NO", 0, c_stacking_no);
				epex.SetValue(block_name_master, "BILL_OF_LADING_NO", 0, bill_of_lading_no);
				epex.SetValue(block_name_master, "IN_STOCK_CODE", 0, stock_no);
				epex.SetValue(block_name_master, "IN_STOCK_TIME", 0, CDateTime::Now().ToString("yyyyMMddHHmmss"));
				epex.SetValue(block_name_master, "OPERATE_ID", 0, s.userid);
				epex.SetValue(block_name_master, "OPERATE_NAME", 0, s.username);
				//epex.SetValue(block_name_master, "VEHICLE_NO", vehicle_no);
				mat_no = bcls_rec->Tables[0].Rows[i]["MAT_NO"];

				if (bcls_rec->Tables[0].Columns.Contains("OLD_STACKING_NO"))
					old_stacking_no = bcls_rec->Tables[0].Rows[i]["OLD_STACKING_NO"];
				else
					old_stacking_no = "";

				tmmxx01.Clear();
				sqlstr = "SELECT * FROM TMM" + c_mat_kind + "01 WHERE MAT_NO=@MAT_NO ";
				cmd_tmmxx01.Parameters.Set("MAT_NO", mat_no);
				cmd_tmmxx01.SetCommandText(sqlstr);
				cmd_tmmxx01.ExecuteQuery(tmmxx01);
				if (tmmxx01.Rows.get_Count() != 1)
				{
					CFormattable arguments[] = { mat_no };
					CMessageFormat::Format(s.msg, "未找到该材料号{0}", arguments, 1);
					throw	CApplicationException(-1, s.msg, log.Location);
				}
				Log::Trace("", __FUNCTION__, "MAT_NO=[{0}]", mat_no);
				epex.SetValue(block_name_detail, "CUST_MAT_NO", 0, mat_no);
				epex.SetValue(block_name_detail, "MAT_NET_WT", 0, tmmxx01.Rows[0]["MAT_WT"].ToDecimal());
				epex.SetValue(block_name_detail, "STOCK_ROOM_NO", 0, tmmxx01.Rows[0]["STOCK_PLACE_NO"].ToString());
				epex.SetValue(block_name_detail, "LAYER_NO", 0, tmmxx01.Rows[0]["LAYERNO"].ToDecimal());
				epex.SetValue(block_name_detail, "OLD_STACKING_NO", 0, old_stacking_no);
				// 发送电文
				if (epex.SendTele() < 0)
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
					throw	CApplicationException(-1, s.msg, log.Location);
				}
			}
		}
		else
		{
			if (bcls_rec->Tables[0].Columns.Contains("STACKING_NO"))
				c_stacking_no = bcls_rec->Tables[0].Rows[0]["STACKING_NO"];
			if (bcls_rec->Tables[0].Columns.Contains("BILL_OF_LADING_NO"))
				bill_of_lading_no = bcls_rec->Tables[0].Rows[0]["BILL_OF_LADING_NO"];
			Log::Error("", __FUNCTION__, "第二种情况发送前0");
			epex.SetValue(block_name_master, "OP_FLAG", 0, c_flag);
			//epex.SetValue(block_name_master, "ORDER_NO_ERP", 0, order_no_erp);
			epex.SetValue(block_name_master, "STACKING_NO", 0, c_stacking_no);
			epex.SetValue(block_name_master, "BILL_OF_LADING_NO", 0, bill_of_lading_no);
			epex.SetValue(block_name_master, "IN_STOCK_CODE", 0, stock_no);
			epex.SetValue(block_name_master, "IN_STOCK_TIME", 0, CDateTime::Now().ToString("yyyyMMddHHmmss"));
			epex.SetValue(block_name_master, "OPERATE_ID", 0, s.userid);
			epex.SetValue(block_name_master, "OPERATE_NAME", 0, s.username);
			//epex.SetValue(block_name_master, "VEHICLE_NO", vehicle_no);
			//循环电文部分
			for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
			{
				mat_no = bcls_rec->Tables[0].Rows[i]["MAT_NO"];

				if (bcls_rec->Tables[0].Columns.Contains("OLD_STACKING_NO"))
					old_stacking_no = bcls_rec->Tables[0].Rows[i]["OLD_STACKING_NO"];
				else
					old_stacking_no = "";

				tmmxx01.Clear();
				sqlstr = "SELECT * FROM TMM" + c_mat_kind + "01 WHERE MAT_NO=@MAT_NO ";
				cmd_tmmxx01.Parameters.Set("MAT_NO", mat_no);
				cmd_tmmxx01.SetCommandText(sqlstr);
				cmd_tmmxx01.ExecuteQuery(tmmxx01);
				if (tmmxx01.Rows.get_Count() != 1)
				{
					CFormattable arguments[] = { mat_no };
					CMessageFormat::Format(s.msg, "未找到该材料号{0}", arguments, 1);
					throw	CApplicationException(-1, s.msg, log.Location);
				}
				Log::Trace("", __FUNCTION__, "MAT_NO=[{0}]", mat_no);
				epex.SetValue(block_name_detail, "CUST_MAT_NO", i, mat_no);
				epex.SetValue(block_name_detail, "MAT_NET_WT", i, tmmxx01.Rows[0]["MAT_WT"].ToDecimal());
				epex.SetValue(block_name_detail, "STOCK_ROOM_NO", i, tmmxx01.Rows[0]["STOCK_PLACE_NO"].ToString());
				epex.SetValue(block_name_detail, "LAYER_NO", i, tmmxx01.Rows[0]["LAYERNO"].ToDecimal());
				epex.SetValue(block_name_detail, "OLD_STACKING_NO", i, old_stacking_no);
			}
			// 发送电文
			if (epex.SendTele() < 0)
			{
				CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
				throw	CApplicationException(-1, s.msg, log.Location);
			}
		}


		// 释放
		epex.Uninitialize();
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);

		strncpy(s.sysmsg, (const char*)str, sizeof(s.msg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
	{
		Log::Error("", __FUNCTION__, "msg=[{0}]", ex.GetMsg());
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		Log::Error("", __FUNCTION__, "msg=[{0}]", ex.GetMsg());
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
