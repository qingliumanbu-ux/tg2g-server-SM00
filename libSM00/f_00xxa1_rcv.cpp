/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      013801
Version:     1.0
Date:        2020-02-13 15:12:06
Description: 准发计划头文件接收
**************************************************/

#include "stdafx.h"
#include "epex.h"
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

BM2_FUNCTION_EXPORT
int f_00xxa1_rcv(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, ret = 0;


	/* ***** 电文变量定义 ***** */
	CString    c_confm_plan_no;	// 准发计划号
	int        i_lot_num = 0;	// 准发单据数
	CDecimal   d_total_mat_wt = 0;	// 合计准发材料重量
	int        i_total_mat_num = 0;	// 合计准发材料件数
	CString    c_prg_send_time;	//准发计划发送时刻
	CString    c_plan_maker;	// 计划责任者
	CString    c_reser_start_time;	// 预定开始执行时刻
	CString    c_reser_end_time;	// 预定结束执行时刻
	CString		c_mat_kind = " ";	// 物料种类
	CString		c_stock_no = " ";	// 库区代码

	/* ***** 程序变量 ***** */
	CString c_user = " ", c_factory_div = " ", c_tc_no = " ", c_event_name = "准发计划接收";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr1(""), sqlstr2(""), sqlstr3("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	CModel tsmpe01 = CModel("TSMPE01");

	/* ***** 应用程序开始处理 ***** */

	CString datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

	try
	{
		/* ***** 获取电文号 ***** */
		c_tc_no = s.username;
		c_user = c_tc_no;

		/* ***** 解析电文 ***** */
		c_confm_plan_no = bcls_rec->Tables[0].Rows[0]["confm_plan_no"].ToString().TrimOrBlank();
		i_lot_num = ((CDecimal)(bcls_rec->Tables[0].Rows[0]["lot_num"])).ToInt32();
		d_total_mat_wt = bcls_rec->Tables[0].Rows[0]["total_mat_wt"];
		i_total_mat_num = ((CDecimal)(bcls_rec->Tables[0].Rows[0]["total_mat_num"])).ToInt32();
		c_prg_send_time = bcls_rec->Tables[0].Rows[0]["prg_send_time"].ToString().TrimOrBlank();
		c_plan_maker = bcls_rec->Tables[0].Rows[0]["plan_maker"].ToString().TrimOrBlank();
		c_reser_start_time = bcls_rec->Tables[0].Rows[0]["reser_start_time"].ToString().TrimOrBlank();
		c_reser_end_time = bcls_rec->Tables[0].Rows[0]["reser_end_time"].ToString().TrimOrBlank();
		c_mat_kind = bcls_rec->Tables[0].Rows[0]["mat_kind"].ToString().TrimOrBlank();
		c_stock_no = bcls_rec->Tables[0].Rows[0]["stock_no"].ToString().TrimOrBlank();

		Log::Info("", __FUNCTION__, "c_confm_plan_no=[{0}]", c_confm_plan_no);

		if (c_confm_plan_no.Compare(" ") < 0)
		{
			strcpy(s.msg, "准发计划号不能为空");
			throw CApplicationException(-2, s.msg, s.svc_name);
		}

		if (i_lot_num <= 0)
		{
			strcpy(s.msg, "单据个数应大于零.");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (i_total_mat_num <= 0)
		{
			strcpy(s.msg, "合计材料个数应大于零.");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (d_total_mat_wt <= 0)
		{
			strcpy(s.msg, "合计材料重量应大于零.");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (c_mat_kind.Trim() == "")
		{
			strcpy(s.msg, "物料类型代码不能为空.");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (c_stock_no.Trim() == "")
		{
			strcpy(s.msg, "准发的库区代码不能为空.");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}



		// 判此准发计划是否已经存在是报错
		sqlstr = CString(" select count(1) from tsmpe01 where confm_plan_no = @confm_plan_no ");
		execute_sql.SetCommandText(sqlstr);
		execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
		row_count = execute_sql.ExecuteScalar().ToInt32();
		execute_sql.Close();

		if (row_count	>	0)
		{
			CFormattable arguments[] = { c_confm_plan_no }; // 定义参数列表的数组
			CMessageFormat::Format(s.msg, "计划号[{0}]已存在.", arguments, 1);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		// 按库区代码读取厂别代码
		sqlstr = "select factory_div from vsmpea9 where stock_no = @stock_no ";
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("stock_no", c_stock_no);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			c_factory_div = cmd_inq.GetString(1);
		}
		cmd_inq.Close();

		tsmpe01.Reset();
		tsmpe01["REC_CREATE_TIME"] = datetime;
		tsmpe01["REC_CREATOR"] = c_user;
		tsmpe01["REC_REVISE_TIME"] = datetime;
		tsmpe01["REC_REVISOR"] = c_user;
		tsmpe01["CONFM_PLAN_NO"] = c_confm_plan_no;
		tsmpe01["MAT_KIND"] = c_mat_kind;
		tsmpe01["FACTORY_DIV"] = c_factory_div;
		tsmpe01["STOCK_NO"] = c_stock_no;
		tsmpe01["CONFM_STATUS"] = "0";
		tsmpe01["PLAN_MAKER"] = c_plan_maker;
		tsmpe01["PRG_SEND_TIME"] = c_prg_send_time;
		tsmpe01["RESER_START_TIME"] = c_reser_start_time;
		tsmpe01["RESER_END_TIME"] = c_reser_end_time;
		tsmpe01["TOTAL_MAT_WT"] = d_total_mat_wt;
		tsmpe01["TOTAL_MAT_NUM"] = i_total_mat_num;
		//tsmpe01["PLAN_BILL_NUM"] = i_lot_num;

		tsmpe01.TrimOrBlank();
		sqlstr = "insert into tsmpe01";
		tsmpe01.Insert();


		sprintf(s.msg ,"准发计划号[%s]头信息已接收成功.",(const char *) c_confm_plan_no);

	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{

		CFormattable arguments[] = { ex.GetCode() };// 定义参数列表的数组
		CMessageFormat::Format(s.msg, "数据库处理出错sqlcode=[{0}]", arguments, 1);//格式化字符串
		CString str = sqlstr + "\r\n" + ex.GetMsg();

		Log::Error("", __FUNCTION__, "error=[{0}]", str);

		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                  //数据库异常时返回-1，事务将被回滚
	}
	catch (const CApplicationException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1;
	}

	catch (const CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1;
	}

	if (doFlag < 0)
	{
		CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		//CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0} :{1}", arguments, 2);
	}

	return doFlag;
}


