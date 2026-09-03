/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      013801
Version:     1.0
Date:        2023-1-3 14:32:11
Description: 车皮批复电文接收
**************************************************/
/***** C/C++ 的标准头文件部分 *****/
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"
#include "epex.h"


BM2_FUNCTION_EXPORT
int f_sm00_md_ok(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	// 码单确认


int f_smxx02_rcv(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);
	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, i = 0, ret = 0;

	CString c_datetime = s.datetime;

	CModel tsmpe15("TSMPE15");

	/* ***** 电文变量定义 ***** */
	CString    c_operate_flag;	// 【0-批复，1--批复取消】
	CString    c_bill_of_lading_no;	//提单号

	/* ***** 程序变量 ***** */
	CString c_user = " ", c_mat_kind = " ", datetime = " ";
	CString	block_name_master = "BODY";
	CString	block_name_detail = "DETAIL";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	/* ***** 应用程序开始处理 ***** */
	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		/* ***** 获取电文号 ***** */
		c_user = s.username;

		Log::Trace("", __FUNCTION__, "电文号=[{0}]", c_user);

		/* ***** 解析电文 ***** */
		c_operate_flag = bcls_rec->Tables[block_name_master].Rows[0]["op_flag"].ToString().TrimOrBlank();		// 操作标志 
		tsmpe15["VEHICLE_APP_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["vehicle_app_no"].ToString();	// 车皮申请号
		tsmpe15["BILL_OF_LADING_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["bill_of_lading_no"].ToString();// 提单号
		tsmpe15["VEHICLE_ADMIT"] = bcls_rec->Tables[block_name_master].Rows[0]["vehicle_admit"].ToDecimal().ToInt32();// 日准车数
		tsmpe15["VEHICLE_DATE"] = bcls_rec->Tables[block_name_master].Rows[0]["vehicle_date"].ToString();	// 准车日期
		tsmpe15["REMARK"] = bcls_rec->Tables[block_name_master].Rows[0]["remark"].ToString();	// 备注
		

		/* 读取数据的合理性校验 */
		c_bill_of_lading_no = tsmpe15["BILL_OF_LADING_NO"].ToString();


		if (c_bill_of_lading_no.Trim() == "")
		{
			sprintf(s.msg, _RES("SM00S0000769")/*接收提单号不能为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		Log::Info("", __FUNCTION__, "c_operate_flag=[{0}]", c_operate_flag);

		if (c_operate_flag.Trim() != "0"	// 批复
			&& c_operate_flag.Trim() != "1"	// 批复取消
			)
		{
			CFormattable	arguments[] = { c_operate_flag };
			CMessageFormat::Format(s.msg, "接收下发标记【{0}】出错，不是0或1", arguments, 1);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (c_operate_flag.Trim() == "0")
		{
			tsmpe15["REC_CREATE_TIME"] = datetime;
			tsmpe15["REC_CREATOR"] = c_user;
			tsmpe15.TrimOrBlank();
			sqlstr = "insert into tsmpe15 ";
			Log::Debug("", "", "BILL_OF_LADING_NO = {0}", tsmpe15["BILL_OF_LADING_NO"]);
			tsmpe15.Insert();
		}
		else
		{
			sqlstr = "delete  tsmpe15 ";
			Log::Debug("", "", "BILL_OF_LADING_NO = {0}", tsmpe15["BILL_OF_LADING_NO"]);
			tsmpe15.Delete("BILL_OF_LADING_NO");
		}

		
		// 判已经生成装车单时，调用码单确认函数
		// 根据计划号到码单表上读取是否已经装车，是做码单确认并发送码单给销售
		CString ticket_no = "", stacking_status="";
		sqlstr = "SELECT TICKET_NO,STACKING_STATUS FROM TSMPE11 WHERE BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' ";
		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			ticket_no = cmd_inq.GetString(1);
			stacking_status = cmd_inq.GetString(2);
		}
		else
		{
			return 0;
		}
		cmd_inq.Close();

		// 判装车单不为空，并且没有确认时，调用函数进行确认
		if (ticket_no.Trim() != "" && stacking_status == "1")
		{
			CFormattable	arguments[] = { ticket_no, stacking_status };
			CMessageFormat::Format(s.msg, "无此装车单【{0}】或装车单状态不是未确认", arguments, 1);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}


		CString blkname = "md_ok";
		EIClass bcls_rec_md;
		bcls_rec_md.Tables[0].set_TableName(blkname);

		bcls_rec_md.Tables[blkname].Rows.Clear();
		bcls_rec_md.Tables[blkname].Rows.Add();
		bcls_rec_md.Tables[blkname].Columns.Add(DT_STRING, "TICKET_NO");
		bcls_rec_md.Tables[blkname].Rows[0]["TICKET_NO"] = ticket_no;
		/********************************************************************************
		*****	调用码单确认函数	*****
		********************************************************************************/
		bcls_rec->SetSYS(s);
		ret = 0;
		ret = f_sm00_md_ok(&bcls_rec_md, bcls_ret, conn);
		if (ret != 0)
		{
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		sprintf(s.msg, "装车单确认正确，码单传销售物流！");


		sprintf(s.msg, "处理成功！");
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{

		CFormattable arguments[] = { ex.GetCode(), ex.GetMsg() };// 定义参数列表的数组
		CMessageFormat::Format(s.msg, "数据库处理出错sqlcode=[{0}],msg={1}", arguments, 2);//格式化字符串
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

