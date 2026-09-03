/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2010
Author:   13801
Version:    3.0
Date:     2021-3-15 13:12:12
Description: 铁运车皮材料信息给物流运输
2022-2-14	13801	厂装车时去向取请车计划上的卸厂
2022-3-7	13801	厚板发货厂装计划时，收货单位字段写死
2022-3-31	13801	修改3-7日的写法用配置的方法
**************************************************/
/* C/C++ 的标准头文件部分 */

#include "stdafx.h"		// 框架头，不可删除

#include "epex.h"

//名称空间引用




//外部函数声明

BM2_FUNCTION_EXPORT
int f_xxpa04_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int i;
	int fetchRowCount = 0;
	CDecimal v_mat_act_wt = 0;
	CDecimal v_mat_gross_wt = 0;
	int v_count = 0;
	int i_vehicle_num = 1;	// 组批车数

	/* 业务变量 */
	CString	datetime("");
	CString tc_no = "XXPA04";	// 电文号

	CString	ticket_no("");	// 装车单号
	CString	vehicle_no("");
	CString	vehicle_id("");

	CString	operate_flag = "I";		// I:Insert;U:Update;D:Delete

	/* 实体类定义 */
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");
	CModel tsm00b4("TSM00B4");
	CModel hsm00b4("HSM00B4");

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

		/* 读取传人参数 */
		for ( i = 0; i < row; i++ )
		{

			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"];
			vehicle_no = bcls_rec->Tables[0].Rows[i]["VEHICLE_NO"];
			if ( bcls_rec->Tables[0].Columns.Contains("VEHICLE_ID") )
			{
				vehicle_id = bcls_rec->Tables[0].Rows[i]["VEHICLE_ID"];
			}


			if ( bcls_rec->Tables[0].Columns.Contains("OPERATE_FLAG") )
			{
				operate_flag = bcls_rec->Tables[0].Rows[0]["OPERATE_FLAG"];
			}
			if ( operate_flag.Trim() == "" )
			{
				operate_flag = "I";
			}

			/* 显示读取的参数 */
			Log::Info("", __FUNCTION__, "装车单号 ticket_no=[{0}]]", ticket_no);

			/* 数据校验 */
			if ( ticket_no.Trim() == "" )
			{
				Log::Debug("", "", "装车单号为空，跳过");
				continue;

				//sprintf(s.msg, "装车单号不能为空");
				//throw	CApplicationException(-1, s.msg, log.Location);
			}

			/* 按传入的参数读取记录 */
			tsmpe12["VEHICLE_NO"] = " ";
			sqlstr = " SELECT * "
				"	FROM tsmpe12 A "
				" WHERE A.TICKET_NO =   @ticket_no  ";		// SQL语句定义

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("ticket_no", ticket_no);	// SQL语句中的变量赋值
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();		// 语句执行

			if ( cmd_inq.Read() )
			{
				cmd_inq.Fetch(tsmpe12);
			}
			else
			{
				sprintf(s.msg, "无此装车单号[%s]", (const char *)ticket_no);
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			cmd_inq.Close();


			/* 按提单号读取入库库区 */
			CString STOCK_NO_TO = " ";
			CString CONSIGNE_NAME = " ";	// 收货单位
			sqlstr = "SELECT STOCK_NO_TO,CONSIGNE_NAME FROM TSMPE10 WHERE BILL_OF_LADING_NO = '" + tsmpe12["BILL_OF_LADING_NO"].ToString() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				STOCK_NO_TO = cmd_inq.GetString(1);
				CONSIGNE_NAME = cmd_inq.GetString(2);
			}
			cmd_inq.Close();


			/* 判车号是否在接收的列表中，无不发送电文 */
			tsm00b4["VEHICLE_NO"]	= tsmpe12["VEHICLE_NO"];
			if ( tsm00b4.Query("VEHICLE_NO") == false )
			{
				Log::Debug("", "", "无此车号【{0}】，不需要发送电文", tsm00b4["VEHICLE_NO"].ToString());
				tsm00b4["VEHICLE_ID"] = vehicle_id;
				continue;
			}

			if (tsm00b4["DIS_SOURCE"].ToString().Trim() != "")
			{
				return 0;
			}



			tc_no = tsm00b4["REC_CREATOR"].ToString().SubstringNE(2, 2) + tsm00b4["REC_CREATOR"].ToString().SubstringNE(0, 2) + "04";

			// 生成电文发送对象
			EPEX epex(&s);

			// 初始化电文格式
			if ( epex.Initialize(tc_no) < 0 )   //电文号
			{
				CFormattable arguments[] ={ epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "初始化电文时失败！ 原因描述：[ {0}]", arguments, 1);//格式化字符串
				throw	CApplicationException(-1, s.msg, log.Location);
			}



			/////* 2018-11-1 增加轨梁时重量拆分 */
			i_vehicle_num = 1;
			tsmpe12["MAT_ACT_WT"] = 0;
			tsmpe12["MAT_WT"] = 0;

			////Log::Debug("", "", "根据车号读取组批号{0}", vehicle_no);
			////sqlstr = " SELECT * FROM TSM00B4 WHERE VEHICLE_NO = @VEHICLE_NO ";
			////cmd_inq.SetCommandText(sqlstr);
			////cmd_inq.Parameters.Set("VEHICLE_NO", vehicle_no);
			////cmd_inq.ExecuteReader();
			////if (cmd_inq.Read())
			////{
			////	cmd_inq.Fetch(tsm00b4);
			////	Log::Debug("", "", "读取的组批号{0}", tsm00b4["VEHICLE_GROUP"].ToString());
			////	Log::Debug("", "", "读取的ID{0}", tsm00b4["VEHICLE_ID"].ToString());
			////	Log::Debug("", "", "读取的装车单{0}", tsm00b4["TICKET_NO"].ToString());

			////	if (tsm00b4["VEHICLE_GROUP"].ToString().Trim() != "")
			////	{
			////		i_vehicle_num = tsm00b4.QueryCount("VEHICLE_GROUP");
			////		Log::Debug("", "", "此组批号{0}下有{1}个车皮", tsm00b4["VEHICLE_GROUP"].ToString(), i_vehicle_num);

			////		sqlstr = " SELECT TICKET_NO FROM TSM00B4 WHERE VEHICLE_GROUP = @VEHICLE_GROUP AND VEHICLE_KEY = '1' ";
			////		cmd_inq2.SetCommandText(sqlstr);
			////		cmd_inq2.Parameters.Set("VEHICLE_GROUP", tsm00b4["VEHICLE_GROUP"].ToString());
			////		cmd_inq2.ExecuteReader();
			////		while (cmd_inq2.Read())
			////		{
			////			tsm00b4["TICKET_NO"] = cmd_inq2.GetString(1);
			////			Log::Debug("", "", "读取的装车单号{0}", tsm00b4["TICKET_NO"].ToString());
			////			Log::Debug("", "", "读取的ID号{0}", tsm00b4["VEHICLE_ID"].ToString());

			////			if (tsm00b4["TICKET_NO"].ToString().Trim() != "")
			////			{
			////				//sqlstr = " SELECT SUM(MAT_ACT_WT) FROM tsmpe12 WHERE TICKET_NO =   @ticket_no "
			////				//	" AND	STATUS < '5' ";

			////				sqlstr = " SELECT SUM(MAT_ACT_WT),COUNT(1),sum(MAT_GROSS_WT) FROM tsmpe12 WHERE STATUS < '5' "
			////					" AND TICKET_NO IN (SELECT TICKET_NO FROM TSM00B4 WHERE VEHICLE_GROUP IN (SELECT VEHICLE_GROUP FROM TSM00B4 WHERE VEHICLE_NO = @vehicle_no) AND VEHICLE_KEY = '1')";

			////				cmd_inq1.SetCommandText(sqlstr);
			////				cmd_inq1.Parameters.Set("ticket_no", tsm00b4["TICKET_NO"].ToString());	// SQL语句中的变量赋值
			////				cmd_inq1.Parameters.Set("vehicle_no", vehicle_no);	// SQL语句中的变量赋值
			////				cmd_inq1.ExecuteReader();		// 语句执行

			////				if (cmd_inq1.Read())
			////				{
			////					tsmpe12["MAT_ACT_WT"] = cmd_inq1.GetDecimal(1);
			////					v_count = cmd_inq1.GetInt32(2);
			////					v_mat_gross_wt = cmd_inq1.GetDecimal(3);
			////					Log::Debug("", "", "装车单号{0}下有{1} 重量", tsm00b4["TICKET_NO"].ToString(), tsmpe12["MAT_ACT_WT"].ToDecimal());
			////				}
			////				else
			////				{
			////					tsmpe12["MAT_ACT_WT"] = 0;
			////				}
			////				cmd_inq1.Close();
			////				break;
			////			}
			////		}
			////		cmd_inq2.Close();
			////	}
			////	else
			////	{
			////		i_vehicle_num = 1;
			////	}
			////}
			////cmd_inq.Close();

			/////* 2018-11-1 */


			// 2018-11-1 down
			if (tsmpe12["MAT_ACT_WT"].ToDecimal() > 0)
			{
				v_mat_act_wt = tsmpe12["MAT_ACT_WT"];
				tsmpe12["VEHICLE_NO"] = vehicle_no;
				ticket_no = tsm00b4["TICKET_NO"];
			}
			// 2018-11-1 up
			else
			{

				sqlstr = " SELECT SUM(MAT_WT),COUNT(1),sum(MAT_GROSS_WT) FROM tsmpe12 WHERE TICKET_NO =   @ticket_no ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("ticket_no", ticket_no);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					v_mat_act_wt = cmd_inq.GetDecimal(1);
					v_count = cmd_inq.GetInt32(2);
					v_mat_gross_wt = cmd_inq.GetDecimal(3);
					Log::Debug("", "", "重量 = [{0}],件数=[{1}]", v_mat_act_wt, v_count);
				}
				else
				{
					sprintf(s.msg, "没有读取到装车件数");
					throw	CApplicationException(-1, s.msg, log.Location);
				}
				cmd_inq.Close();
			}
			if ( v_count == 0 )
			{
				Log::Debug("","", "读取到装车件数为0");
				cmd_inq.Close();
				continue;
				//throw	CApplicationException(-1, s.msg, log.Location);
			}
			cmd_inq.Close();



			CString svc_name = s.svc_name;
			CString DST_ADDR = " ";	// 去向
			if (svc_name == "sm0012_tyjj_send")
			{
				DST_ADDR = " ";
			}
			else
			{
				DST_ADDR = "010104";
				if (tc_no.SubstringNE(2, 2) == "M4")
				{
					DST_ADDR = "010101";
				}
			}

			// 2022-2-14 down
			CString V_DST_ADDR = "";
			CString V_RECEIVE_UNIT_CODE = " ";	// 收货单位
			if (tsm00b4["MARK"].ToString() == "CZ")
			{
				sqlstr = "select MANUFACTOR_NO FROM TWMQC1A WHERE TRAN_PLAN_NO = '" + tsm00b4["PLAN_NO"].ToString() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					V_DST_ADDR = cmd_inq.GetString(1);
				}
				cmd_inq.Close();

				if (V_DST_ADDR.Trim() != "")
				{
					DST_ADDR = V_DST_ADDR;
				}
				if (ticket_no.Trim() == "")
				{
					DST_ADDR = " ";
				}

				// 2022-3-31 down
				if (DST_ADDR.Trim() != "")
				{
					sqlstr = "SELECT code_desc_3_content FROM TEP0002 WHERE code_class = 'SMAG00' AND  CODE = '" + DST_ADDR.Trim() + "' ";
					cmd_inq.SetCommandText(sqlstr);
					Log::Debug("", "", "sqlstr = {0} ", sqlstr);
					cmd_inq.ExecuteReader();
					if (cmd_inq.Read())
					{
						V_RECEIVE_UNIT_CODE = cmd_inq.GetString(1);
					}
					cmd_inq.Close();
				}
				// 2022-3-31 up


			}
			// 2022-2-14 up



			// 读取预装标记
			CString yz_flag = " ";
			sqlstr = "select * from tsmpe15 where BILL_OF_LADING_NO = '" + tsmpe12["BILL_OF_LADING_NO"].ToString() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (!cmd_inq.Read())
			{
				yz_flag = "1";
			}
			cmd_inq.Close();


			CString LOAD_UNLOAD = "Z";
			if (s.svc_name == "sm0012_confirm1")
			{
				LOAD_UNLOAD = "X";
			}


			/* 数据压电文 */
			fetchRowCount = 0;
			if (epex.SetValue("OPERATION_FLAG", fetchRowCount, operate_flag)<0	// 操作标记
				|| epex.SetValue("IN_OUT_FLAG", fetchRowCount, "O")<0	// 出厂/厂内区分标记   O:出厂   I 厂内
				|| epex.SetValue("LOAD_UNLOAD", fetchRowCount, LOAD_UNLOAD)<0	// 装卸标记 Z:装  X：卸
				|| epex.SetValue("RAILWAY_NO", fetchRowCount, tsm00b4["LANE_NO"].ToString())<0	// 股道代码
				|| epex.SetValue("SEQ_NO", fetchRowCount, tsm00b4["SEQ_NO"].ToDecimal())<0	// 顺位
				|| epex.SetValue("WAGONNO", fetchRowCount, tsm00b4["VEHICLE_NO"].ToString())<0	// 车号
				|| epex.SetValue("VEHICLE_ID", fetchRowCount, tsm00b4["VEHICLE_ID"].ToString())<0	// 车皮号 ID
				|| epex.SetValue("VEHICLE_CODE", fetchRowCount, tsm00b4["VEHICLE_TYPE"].ToString())<0	// 车型代码
				|| epex.SetValue("GOODS_CODE", fetchRowCount, tsmpe12["PROD_CODE"].ToString())<0	// 品名代码
				|| epex.SetValue("GOODS_NAME", fetchRowCount, tsmpe12["PROD_CNAME"].ToString())<0	// 品名名称
				|| epex.SetValue("DEST_RAILWAY", fetchRowCount, " ") <0	// 目的股道
				//|| epex.SetValue("DST_STOCK_CODE", fetchRowCount, STOCK_NO_TO) <0	// 目的库区
				|| epex.SetValue("WT", fetchRowCount, v_mat_act_wt / i_vehicle_num )<0	// 总重量
				|| epex.SetValue("NUM", fetchRowCount, v_count)<0	// 总件数
				|| epex.SetValue("WORK_STAFF_NAME", fetchRowCount, s.userid)<0	// 确认人
				|| epex.SetValue("WORK_TIME", fetchRowCount, datetime)<0	// 确认时间
				|| epex.SetValue("LAOD_NO", fetchRowCount, tsm00b4["TICKET_NO"].ToString())<0	// 装车单号
				|| epex.SetValue("YZ_FLAG", fetchRowCount, yz_flag)<0	// 预装标记
				//|| epex.SetValue("PONDER_FLAG", fetchRowCount, "0")<0	// 是否过磅 0--不过磅  1--过磅
				//|| epex.SetValue("DST_ADDR", fetchRowCount, DST_ADDR)<0	// 去向
				//|| epex.SetValue("UL_UNIT_CODE", fetchRowCount, tsm00b4["FACTORY_DIV"].ToString())<0	// 发货单位
				//|| epex.SetValue("RECEIVE_UNIT_CODE", fetchRowCount, V_RECEIVE_UNIT_CODE)<0	// 收货单位
				//|| epex.SetValue("START_TIME", fetchRowCount, datetime)<0	// 装/卸开始时间
				//|| epex.SetValue("END_TIME", fetchRowCount, datetime)<0	// 装/卸结束时间
				//|| epex.SetValue("SALES_ORDER_NO", fetchRowCount, tsmpe12["ORDER_NO"].ToString())<0	// 销售订单号
				//|| epex.SetValue("ARRI_STATION_NAME", fetchRowCount, tsm00b4["TERMINAL_NAME"].ToString())<0	// 到站
				//|| epex.SetValue("PLAN_NO", fetchRowCount, tsm00b4["PLAN_NO"].ToString())<0	// 计划号
				)
			{
				CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "发送电文主体时失败! 原因描述： [{0}]", arguments, 1);//格式化字符串
				throw CApplicationException(-1, s.msg, log.Location);
			}


			// 循环读取装车材料
			sqlstr = " SELECT B.* FROM tsmpe12 B  "
				" WHERE B.TICKET_NO =   @ticket_no "
				;
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("ticket_no", ticket_no);
			cmd_inq.ExecuteReader();
			while ( cmd_inq.Read() )
			{
				cmd_inq.Fetch(tsmpe12);

				if (epex.SetValue("MAT_NO", fetchRowCount, tsmpe12["MAT_NO"].ToString()) < 0	// 材料号
					|| epex.SetValue("MAT_ACT_WT", fetchRowCount, tsmpe12["MAT_WT"].ToDecimal() / i_vehicle_num ) < 0	// 材料重量
					|| epex.SetValue("PLAN_NO", fetchRowCount, tsmpe12["BILL_OF_LADING_NO"].ToString())<0	// 计划号
					)
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, "发送电文循环体时失败! 原因描述： [{0}]", arguments, 1);//格式化字符串
					throw CApplicationException(-1, s.msg, log.Location);
				}
				fetchRowCount++;
			}
			cmd_inq.Close();

			// 发送电文
			if ( epex.SendTele() < 0 )
			{
				{
					CFormattable arguments[] ={ epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
				}
				//sprintf(s.msg,"发送电文时失败! 原因描述: %s", epex.GetMsg());//转换前
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			// 释放
			epex.Uninitialize();

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
