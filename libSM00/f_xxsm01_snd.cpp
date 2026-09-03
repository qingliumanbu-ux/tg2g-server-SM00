/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      013801
Version:     1.0
Date:        2020-02-27 14:56:22
Description: 发货码单电文发送
2021-11-17	修改出厂日期为码单日期
**************************************************/
#include "stdafx.h"		// 框架头，不可删除

 	/* 发货码单表 */
	/* 码单材料表 */
	/* 发货计划表 */
#include "epex.h"

//名称空间引用




//外部函数声明


BM2_FUNCTION_EXPORT


int f_xxsm01_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
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

		tc_no = bcls_rec->Tables["0"].Rows[0]["tc_no"].ToString().TrimOrBlank();	//电文号
		tc_no = tc_no.SubstringNE(0, 4) + "04";

		CString oper_flag = "";
		if (bcls_rec->Tables["0"].Columns.Contains("oper_flag"))
		{
			oper_flag = bcls_rec->Tables["0"].Rows[0]["oper_flag"].ToString();
		}
		
		/* ***** 创建电文处理对象 ***** */
		EPEX epex(&s);

		// 初始化电文格式
		if (epex.Initialize(tc_no) < 0)   //电文号
		{
			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
			CMessageFormat::Format(s.msg, _RES("SM00S0001464")/*初始化电文时失败！ 原因描述：[ {0}]*/, arguments, 1);//格式化字符串
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		int row = bcls_rec->Tables["0"].Rows.get_Count();
		for (i = 0; i < row; i++)
		{

			/* ***** 获取单据数据块号  ***** */
			tsmpe11["STACKING_NO"] = bcls_rec->Tables["0"].Rows[i]["stacking_no"].ToString().TrimOrBlank();	//码单号

			/* ***** 打印输入参数 ***** */
			Log::Info("", __FUNCTION__, "码单号	stacking_no=[{0}]", tsmpe11["STACKING_NO"].ToString());

			//读取码单表信息
			sqlstr = "SELECT	* 	FROM	tsmpe11 "
				"	WHERE	stacking_no = @stacking_no";
			cmd_inq.Parameters.Set("stacking_no", tsmpe11["STACKING_NO"].ToString());
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe11);
			}
			tsmpe11.TrimOrBlank();
			cmd_inq.Close();

			/* ***** 检查输入参数合法性 ***** */
			if (tsmpe11["STACKING_NO"].ToString() == " ")
			{
				sprintf(s.msg, "输入的码单号[%s]有误，请重新输入！", (const char*)tsmpe11["STACKING_NO"].ToString());
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			if (tsmpe11["BILL_OF_LADING_NO"].ToString() == " ")
			{
				sprintf(s.msg, "输入的提货单号[%s]有误，请重新输入！", (const char*)tsmpe11["BILL_OF_LADING_NO"].ToString());
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			if (tsmpe11["ORDER_NO"].ToString() == " ")
			{
				sprintf(s.msg, "输入的合同号[%s]有误，请重新输入！", (const char*)tsmpe11["ORDER_NO"].ToString());
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			if (tsmpe11["TICKET_NO"].ToString().Trim() == "")
			{
				sprintf(s.msg, "输入的装车单号[%s]有误，请重新输入！", (const char*)tsmpe11["TICKET_NO"].ToString());
				throw	CApplicationException(-1, s.msg, log.Location);
			}

			if (0 >= tsmpe11["STACKING_WT"].ToDecimal())
			{
				sprintf(s.msg, "输入的码单重量[%f]有误，请重新输入！", tsmpe11["STACKING_WT"].ToDecimal().ToDouble());
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			if (0 >= tsmpe11["STACKING_NUM"].ToDecimal())
			{
				sprintf(s.msg, "输入的码单个数[%d]有误，请重新输入！", tsmpe11["STACKING_NUM"].ToDecimal().ToInt32());
				throw	CApplicationException(-1, s.msg, log.Location);
			}

			// 按合同号读取合同上的计重方式，是实重交货时加磅差
			CString WT_METHOD_CODE = "0";	//计重方式
			sqlstr = "select WT_METHOD_CODE from tom01 where order_no = '" + tsmpe11["ORDER_NO"].ToString() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Info("", "", "sqlstr= {0} ", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				WT_METHOD_CODE = cmd_inq.GetString(1);
			}
			cmd_inq.Close();

			if (WT_METHOD_CODE == "0" && tsmpe11["MAT_KIND"].ToString() == "SM")
			{
				tsmpe11["STACKING_WT"] = tsmpe11["STACKING_DISCREP_WT"].ToDecimal() + tsmpe11["STACKING_WT"].ToDecimal();
			}


			// 读取计划表信息
			sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = @BILL_OF_LADING_NO and ORDER_NO in ( @ORDER_NO,' ') ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("ORDER_NO", tsmpe11["ORDER_NO"].ToString());
			cmd_inq.Parameters.Set("BILL_OF_LADING_NO", tsmpe11["BILL_OF_LADING_NO"].ToString());
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe10);
			}
			else
			{
				CFormattable	arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString(), tsmpe11["ORDER_NO"].ToString() };
				CMessageFormat::Format(s.msg, "没有读取到发货计划{0}合同{1}信息", arguments, 2);
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}
			cmd_inq.Close();



			if (epex.SetValue(block_name_master, "stacking_no", 0, tsmpe11["STACKING_NO"].ToString()) < 0	// 码单号
				|| epex.SetValue(block_name_master, "bill_of_lading_no", 0, tsmpe11["BILL_OF_LADING_NO"].ToString()) < 0	// 提单号
				|| epex.SetValue(block_name_master, "order_no", 0, tsmpe11["ORDER_NO"].ToString()) < 0			// 合同号
				|| epex.SetValue(block_name_master, "carrier_no", 0, tsmpe11["TICKET_NO"].ToString()) < 0 // 装车单号
				|| epex.SetValue(block_name_master, "stacking_wt", 0, tsmpe11["STACKING_WT"].ToDecimal()) < 0			// 重量
				|| epex.SetValue(block_name_master, "stacking_num", 0, tsmpe11["STACKING_NUM"].ToDecimal()) < 0			// 件数
				|| epex.SetValue(block_name_master, "delivy_time", 0, tsmpe11["REC_CREATE_TIME"].ToString()) < 0	// 出厂日期
				|| epex.SetValue(block_name_master, "vehicle_no", 0, tsmpe11["VEHICLE_NO"].ToString()) < 0 // 车号
				|| epex.SetValue(block_name_master, "operate_name", 0, tsmpe11["REC_CREATOR"].ToString()) < 0			// 操作者工号
				//|| epex.SetValue(block_name_master, "delivy_time", 0, datetime < 0	// 码单日期	
				//|| epex.SetValue(block_name_master, "TRNP_MODE_CODE", 0, tsmpe11["TRNP_MODE_CODE"].ToString()) < 0 // 运输方式
				//|| epex.SetValue(block_name_master, "PRODUCT_DSCR", 0, tsmpe11["PROD_CNAME"].ToString()) < 0 // 品名
				//|| epex.SetValue(block_name_master, "LOADING_SOLUTION_CODE", 0, tsmpe11.LOADING_SOLUTION_CODE) < 0 // 装载方案号
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


			//读取码单材料表信息
			sqlstr = "SELECT	* 	FROM	tsmpe12	"
				"	WHERE	stacking_no = @stacking_no";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("stacking_no", tsmpe11["STACKING_NO"].ToString());
			cmd_inq.ExecuteReader();
			fetchRowCount = 0;
			Log::Trace("", __FUNCTION__, "STACKING_NO = [{0}]", tsmpe11["STACKING_NO"].ToString());
			Log::Trace("", __FUNCTION__, "sqlstr = [{0}]", sqlstr);
			while (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe12);
				tsmpe12.TrimOrBlank();
				Log::Trace("", __FUNCTION__, "MAT_NO = [{0}]", tsmpe12["MAT_NO"].ToString());
				/************************
				*	码单下材料压入电文	*
				************************/
				if (WT_METHOD_CODE == "0" && tsmpe11["MAT_KIND"].ToString() == "SM")
				{
					tsmpe12["MAT_WT"] = tsmpe12["MAT_WT"].ToDecimal() + tsmpe12["MAT_DISCREP_WT"].ToDecimal();
				}
				if (epex.SetValue(block_name_detail, "cust_mat_no", fetchRowCount, tsmpe12["MAT_NO"].ToString()) < 0	// 材料号
					|| epex.SetValue(block_name_detail, "mat_net_wt", fetchRowCount, tsmpe12["MAT_WT"].ToDecimal()) < 0	// 结算重量
					|| epex.SetValue(block_name_detail, "mat_discrep_wt", fetchRowCount, tsmpe12["MAT_DISCREP_WT"].ToDecimal()) < 0	// 结算重量
					)
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000064")/*写入循环体数据时出错! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
					throw	CApplicationException(-1, s.msg, log.Location);
				}
				fetchRowCount++;
			}
			cmd_inq.Close();

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
