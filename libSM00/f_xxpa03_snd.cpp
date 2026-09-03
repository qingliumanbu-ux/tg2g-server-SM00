/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2010
Author:   13801
Version:    1.0
Date:     2023-2-15
Description: 车皮调出通知
传入参数：	VEHICLE_NO	车号
			ID_FLAG		预确报标记
			OK_TIME		预确报时间
**************************************************/
/* C/C++ 的标准头文件部分 */

#include "stdafx.h"		// 框架头，不可删除
#include "epex.h"


//外部函数声明

BM2_FUNCTION_EXPORT
int f_xxpa03_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int i;
	int fetchRowCount = 0;
	int i_vehicle_num = 1;	// 组批车数

	/* 业务变量 */
	CString	datetime("");
	CString tc_no = "XXPA03";	// 电文号
	CString DST_ADDR = " ";	// 去向
	CString status = " ";	// 车皮状态   4-- 集结   9--完成


	CString	ticket_no("");
	CString	vehicle_no("");
	CString	operate_flag = "I";		// I:Insert;U:Update;D:Delete

	/* 实体类定义 */
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");
	CModel tsm00b4("TSM00B4");
	CModel hsm00b4("HSM00B4");
	CModel tsmpe02("TSMPE02");
	CModel ted21("TED21");

	CString		sqlstr("");              // 数据库SQL操作字符串
	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq1(conn);
	CDbCommand cmd_inq2(conn);

	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		/*获得传入参数*/
		int row = bcls_rec->Tables[0].Rows.get_Count();
		if ( row == 0 )
		{
			sprintf(s.msg, "传入的记录数为0");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		tsm00b4.MergeFrom(bcls_rec->Tables[0].Rows[0]);

		CString id_flag = "";	//预确报标记  0--预报，1--确报
		CString	ok_time = "";	// 确认时间
		id_flag = bcls_rec->Tables[0].Rows[0]["ID_FLAG"];
		if (id_flag != "0" && id_flag != "1")
		{
			sprintf(s.msg, "预确报标记不正确！");
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}

		ok_time = bcls_rec->Tables[0].Rows[0]["OK_TIME"];
		datetime = ok_time;

		// 根据车号到车皮进车表上读取电文号
		sqlstr = "select * from tsm00b4 where vehicle_no = @vehicle_no ";
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("vehicle_no", tsm00b4["VEHICLE_NO"].ToString());
		Log::Info("", "", "sqlstr = {0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			cmd_inq.Fetch(tsm00b4);
			tc_no = tsm00b4["REC_CREATOR"].ToString().SubstringNE(2, 2) + tsm00b4["REC_CREATOR"].ToString().SubstringNE(0, 2) + "03";
		}
		else
		{
			sprintf(s.msg, "无此车皮%s信息", (const char *)tsm00b4["VEHICLE_NO"].ToString());
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}
		cmd_inq.Close();

		if (tsm00b4["DIS_SOURCE"].ToString().Trim() != "")
		{
			return 0;
		}

		// 生成电文发送对象
		EPEX epex(&s);

		// 初始化电文格式
		if ( epex.Initialize(tc_no) < 0 )   //电文号
		{
			CFormattable arguments[] ={ epex.GetMsg() };// 定义参数列表的数组
			CMessageFormat::Format(s.msg, "初始化电文时失败！ 原因描述：[ {0}]", arguments, 1);//格式化字符串
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		fetchRowCount = 0;
		/* 读取传入参数 */
		for ( int i = 0; i < row; i++ )
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"];
			vehicle_no	= bcls_rec->Tables[0].Rows[i]["VEHICLE_NO"];

			if ( bcls_rec->Tables[0].Columns.Contains("OPERATE_FLAG") )
			{
				operate_flag = bcls_rec->Tables[0].Rows[0]["OPERATE_FLAG"];
			}

			/* 显示读取的参数 */
			Log::Info("", __FUNCTION__, "装车单号 ticket_no=[{0}]]", ticket_no);
			Log::Info("", __FUNCTION__, "车号 vehicle_no=[{0}]]", vehicle_no);

			/* 数据校验 */
			if ( ticket_no.Trim() == "" )
			{
				sprintf(s.msg, "装车单号不能为空");
				//throw	CApplicationException(-1, s.msg, log.Location);
			}
			if ( vehicle_no.Trim() == "" )
			{
				sprintf(s.msg, "车号不能为空");
				throw	CApplicationException(-1, s.msg, log.Location);
			}


			sqlstr = " SELECT SUM(MAT_WT) FROM TSMPE02 WHERE TICKET_NO = '" + ticket_no.Trim() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();		// 语句执行

			if ( cmd_inq.Read() )
			{
				tsmpe02["MAT_ACT_WT"] = cmd_inq.GetDecimal(1);
			}
			else
			{
				tsmpe02["MAT_ACT_WT"] = 0;
			}
			cmd_inq.Close();



			/* 判车号是否在接收的列表中，无不发送电文 */
			tsm00b4.Reset();
			tsm00b4["VEHICLE_NO"]	= vehicle_no;
			tsm00b4.Query("VEHICLE_NO");



			/* 新增时生成启票序号 */
			CString id_seq = "0000000";
			if ( operate_flag == "I" && i==0 )
			{
				/* 生成装车清单号 */
				//到流水号表按关键字读取记录
				ted21["SEQ_NAME"] = "QPXH";
				int count = ted21.QueryCount("SEQ_NAME");
				if ( count == 0 )
				{
					//新增记录，按年复位
					ted21["SEQ_DESC"] = "启票序号";
					ted21["SEQ_BEGIN"] = 0;
					ted21["SEQ_NOW"] = 0;
					ted21["SEQ_END"] = 9999999;
					ted21["SEQ_PRE"] = "";	// 流水号前缀
					ted21["SEQ_LEN"] = 7;
					ted21["SEQ_RECYCLE_FLAG"] = "0";	// 最大值复位
					ted21["REC_CREATE_TIME"] = datetime;
					ted21["REC_CREATOR"] = s.userid;
					ted21.TrimOrBlank();
					if ( ted21.Insert() == false )
					{
						sprintf(s.msg, "新增流水号记录失败");
						throw CApplicationException(-1, s.msg, log.Location);
					}
				}
				id_seq = EPGetNextSeq(ted21["SEQ_NAME"].ToString(), conn);

				Log::Trace("", __FUNCTION__, "生成的启票序号[{0}]", id_seq);
			}

			////tsmpe02["TICKET_NO"] = ticket_no;
			////int update_count = tsmpe02.Update("SEQ_CODE", "TICKET_NO");
			////if ( update_count == 0 )
			////{
			////	CFormattable arguments[] ={ tsmpe02.SEQ_CODE, tsmpe02["TICKET_NO"].ToString() };// 定义参数列表的数组
			////	CMessageFormat::Format(s.msg, "没有更新成功启票序号：[{0}]，装车单号[{1}]", arguments, 2);
			////	//throw	CApplicationException(-1, s.msg, log.Location);
			////}
			////CString svc_name = s.svc_name;
			////Log::Debug("", "", "svc_name={0}", svc_name);
			////if (svc_name == "sm0012_tyjj_send" )
			////{
			////	DST_ADDR = " ";
			////	status = "4";
			////}
			////else
			////{
			////	DST_ADDR = "010104";
			////	status = "9";

			////	if (tc_no.SubstringNE(2, 2) == "M4")
			////	{
			////		DST_ADDR = "010103";
			////	}

			////	if (ticket_no.Trim() == "")
			////	{
			////		DST_ADDR = " ";
			////	}
			////}


			////// 2022-2-14 down
			////CString V_DST_ADDR = "";
			////if (tsm00b4["MARK"].ToString() == "CZ")
			////{
			////	sqlstr = "select MANUFACTOR_NO FROM TWMQC1A WHERE TRAN_PLAN_NO = '" + tsm00b4["PLAN_NO"].ToString() + "' ";
			////	cmd_inq.SetCommandText(sqlstr);
			////	Log::Debug("", "", "sqlstr={0}", sqlstr);
			////	cmd_inq.ExecuteReader();
			////	if (cmd_inq.Read())
			////	{
			////		V_DST_ADDR = cmd_inq.GetString(1);
			////	}
			////	cmd_inq.Close();

			////	if (V_DST_ADDR.Trim() != "")
			////	{
			////		DST_ADDR = V_DST_ADDR;
			////	}
			////	if (ticket_no.Trim() == "")
			////	{
			////		DST_ADDR = " ";
			////	}
			////}
			////// 2022-2-14 up

			/* 数据压电文 */
			if (epex.SetValue("OPERATION_FLAG", 0, operate_flag) < 0	// 操作标记
				|| epex.SetValue("ID_SEQ"	, 0, id_seq) < 0	// 通知序号
				|| epex.SetValue("ID_FLAG"	, 0, id_flag) < 0	// 预确报标记	0--预报，1--确报
				|| epex.SetValue("RAILWAY_NO", 0, tsm00b4["LANE_NO"].ToString()) < 0	// 股道代码
				|| epex.SetValue("LOAD_UNLOAD"		, 0, "Z") < 0	// 装卸标记 Z--装,X--卸
				|| epex.SetValue("AFFIRM_BY", 0, s.userid) < 0	// 确认者
				|| epex.SetValue("AFFIRM_TIME", 0, datetime) < 0	// 确认时间
				|| epex.SetValue("VEHICLE_NUM", 0, row) < 0	// 车皮数
				|| epex.SetValue("VEHICLE_SEQ", fetchRowCount, tsm00b4["SEQ_NO"].ToDecimal()) < 0	// 顺位
				|| epex.SetValue("VEHICLE_TYPE", fetchRowCount, tsm00b4["VEHICLE_TYPE"].ToString()) < 0	// 车型代码
				|| epex.SetValue("VEHICLE_NO", fetchRowCount, tsm00b4["VEHICLE_NO"].ToString()) < 0	// 车号
				|| epex.SetValue("VEHICLE_ID", fetchRowCount, tsm00b4["VEHICLE_ID"].ToString()) < 0	// 车号ID
				)
			{
				CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "发送电文时失败! 原因描述： [{0}]", arguments, 1);//格式化字符串
				throw CApplicationException(-1, s.msg, log.Location);
			}


			fetchRowCount++;

		}

		// 发送电文
		if ( epex.SendTele() < 0 )
		{
			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
			CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		// 释放
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
	catch ( CApplicationException& ex )  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch ( CException& ex )
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
