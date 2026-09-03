/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2010
Author:   罗宁
Version:    1.0
Date:     2021-4-12
Description: 计量委托发送
1。带包装的卷允许误差范围是   -10到20KG   每卷，根据装车卷数变动。
2.不带包装    -10到10KG   每卷，根据装车卷数变动（此时卷数要求大于等于2卷）。如果只有1卷的时候误差 -10到20KG
**************************************************/


//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件
#include "epex.h"





int f_sm00_xxjlwt_snd(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);

	/* 程序内部变量 */
	int doFlag = 0;
	int fetchRowCount = 0;
	int blknum = 0;
	CString datetime = "";
	CString blk_name = "JLWT";
	CString ponder_app_no = "";	// 计量委托号
	CString oper_flag = "I";
	int ret = 0;

	CDecimal wt_min = 0, wt_max = 0;

	//* 业务变量 */
	int count = 0;
	CString  userid = " ";            /* 登陆用户 */
	CString  tc_no = "P4J104";

	// 创建电文处理对象
	EPEX epex(&s, conn);

	/* 实体类定义 */
	CModel ted21("TED21");
	CModel tsmpe02("TSMPE02");
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");
	CString sqlstr;
	CString	block_name_master = "BODY";
	CString	block_name_detail = "DETAIL";

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq1(conn);
	CDbCommand cmd_inq_loop(conn);

	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		CString datetime_end = CDateTime::Now().AddDays(1).ToString("yyyyMMddHHmmss");
		//CString datetime1 = CDateTime::Parse(datetime).AddHours(24).ToString("yyyyMMddHHmmss");

		//设定返回参数表
		EDLog(1, 1, "//---------------------------------设定返回参数表--------------------------------------//");

		//获得输入参数
		EDLog(1, 1, "//---------------------------------获得输入参数--------------------------------------//");
		//获取输入参数?  列名的大小写区分
		blknum = bcls_rec->Tables.IndexOf(blk_name); //SMBW查询条件
		if (blknum < 0)
		{
			sprintf(s.msg, "没有数据块 JLWT");
			throw CApplicationException(-1, s.msg, __FUNCTION__);
		}

		bcls_rec->prt();

		int	rows = bcls_rec->Tables[blk_name].Rows.get_Count();
		if (rows == 0)
		{
			strcpy(s.msg, "没有传入参数。");
			throw CApplicationException(-1, s.msg, log.Location);
		}


		for (int i = 0; i < rows; i++)
		{
			oper_flag = "I";
			CString TICKET_NO = bcls_rec->Tables[blk_name].Rows[i]["TICKET_NO"].ToString().Trim();
			oper_flag = bcls_rec->Tables[blk_name].Rows[i]["OPER_FLAG"].ToString().Trim();

			/*参数校验*/
			if (TICKET_NO.Trim() == "")
			{
				strcpy(s.msg, "装车单不能为空!");
				throw CApplicationException(-1, s.msg, log.Location);
			}

			EDLog(1, 1, "//---------------------------------从码单表中读取信息--------------------------------------//");
			sqlstr = " SELECT * FROM tsmpe11 WHERE TICKET_NO IN ('" + TICKET_NO + "') ";

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("TICKET_NO", TICKET_NO);
			Log::Info("", "", "sqlstr=[{0}]", sqlstr);
			cmd_inq.ExecuteReader();

			if (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe11);
			}
			else
			{
				sprintf(s.msg, "装车单表中没有TICKET_NO[%s]", (const char*)TICKET_NO);
				throw CApplicationException(-1, s.msg, log.Location);
			}//if码单表中读到数据
			cmd_inq.Close();


			sqlstr = "SELECT sum(STACKING_WT),SUM(STACKING_GROSS_WT) FROM tsmpe11 WHERE TICKET_NO IN ('" + TICKET_NO + "') ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("TICKET_NO", TICKET_NO);
			Log::Info("", "", "sqlstr=[{0}]", sqlstr);
			cmd_inq.ExecuteReader();

			if (cmd_inq.Read())
			{
				tsmpe11["STACKING_WT"] = cmd_inq.GetDecimal(1);
				tsmpe11["STACKING_GROSS_WT"] = cmd_inq.GetDecimal(2);
			}
			cmd_inq.Close();


			CString	CONSIGNE_CODE = " ";//收货用户代码
			sqlstr = " SELECT CONSIGNE_CODE FROM tsmpe10 WHERE BILL_OF_LADING_NO IN ('" + tsmpe11["BILL_OF_LADING_NO"].ToString() + "') ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Info("", "", "sqlstr=[{0}]", sqlstr);
			cmd_inq.ExecuteReader();

			if (cmd_inq.Read())
			{
				CONSIGNE_CODE = cmd_inq.GetString(1);
			}
			cmd_inq.Close();
			Log::Info("", "", "CONSIGNE_CODE=[{0}]", CONSIGNE_CODE);
			////// 计算误差重量
			////// 按装车单号读取材料
			////fetchRowCount = 0;
			////wt_min = 0, wt_max = 0;
			////sqlstr = "select * from tsmpe12 where TICKET_NO IN ('" + TICKET_NO + "') ";
			////cmd_inq_loop.SetCommandText(sqlstr);
			////cmd_inq_loop.Parameters.Set("TICKET_NO", TICKET_NO);
			////Log::Info("", "", "sqlstr=[{0}]", sqlstr);
			////cmd_inq_loop.ExecuteReader();
			////while (cmd_inq_loop.Read())
			////{
			////	cmd_inq_loop.Fetch(tsmpe12);
			////	if (tsmpe12["MAT_WT"].ToDecimal() == tsmpe12["MAT_GROSS_WT"].ToDecimal())
			////	{
			////		wt_min = wt_min + 0.01;
			////		wt_max = wt_max + 0.01;
			////	}
			////	else
			////	{
			////		wt_min = wt_min + 0.01;
			////		wt_max = wt_max + 0.02;
			////	}
			////	fetchRowCount++;
			////}
			////cmd_inq_loop.Close();
			////if (fetchRowCount == 1)
			////{
			////	wt_max = 0.02;
			////	wt_min = 0.01;
			////}


			//取分区函数
			CString pathvar;
			pathvar = getenv("BM2_PART_NAME");
			Log::Info("", "", "pathvar = [{0}]", pathvar);
			if (pathvar == "AGP4Z")
			{
				tc_no = "P4J104";
			}
			else if (pathvar == "AGP5Z")
			{
				tc_no = "P5J104";
			}
			else if (pathvar == "AGP3Z")
			{
				tc_no = "P3J104";
			}
			else if (pathvar == "AGP1Z")
			{
				tc_no = "P1J104";
			}
			else
			{
				CFormattable arguments[] = { pathvar };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "读取系统环境变量分区号错【{0}】！", arguments, 1);//格式化字符串
				throw CApplicationException(-1, s.msg, log.Location);
			}

			//初始化
			Log::Info("", "", "tc_no=[{0}]", tc_no);
			if (epex.Initialize(tc_no) < 0)
			{
				CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "电文初始化失败! 原因描述：{0}", arguments, 1);//格式化字符串
				throw CApplicationException(-1, s.msg, log.Location);
			}


			if (oper_flag == "I")
			{
				// 生成计量委托号
				//到流水号表按关键字读取记录
				ted21["SEQ_NAME"] = "JLWT";
				count = ted21.QueryCount("SEQ_NAME");
				if (count == 0)
				{
					//新增记录，按年复位
					ted21["SEQ_DESC"] = "JLWT";
					ted21["SEQ_BEGIN"] = 0;
					ted21["SEQ_NOW"] = 1;
					ted21["SEQ_END"] = 9999;
					ted21["SEQ_PRE"] = "";	// 流水号前缀
					ted21["SEQ_LEN"] = 4;
					ted21["SEQ_RECYCLE_FLAG"] = "3";	// 1--按年复位，2--按月，3--按天，0--最大值
					ted21["REC_CREATE_TIME"] = datetime;
					ted21["REC_CREATOR"] = s.userid;
					ted21.TrimOrBlank();
					if (ted21.Insert() == false)
					{
						sprintf(s.msg, "新增流水号记录失败");
						throw CApplicationException(-1, s.msg, log.Location);
					}
				}
				ponder_app_no = tc_no.Substring(0, 2) + "Q" + datetime.Substring(2, 6) + EPGetNextSeq(ted21["SEQ_NAME"].ToString(), conn);

				Log::Trace("", __FUNCTION__, "生成的计量委托号[{0}]", ponder_app_no);

			}
			else
			{
				ponder_app_no = tsmpe11["PONDER_NO"];
			}

			if (epex.SetValue(block_name_master, "OPERATION_FLAG", 0, oper_flag) < 0					// 操作标志 I:新增 D：删除
				|| epex.SetValue(block_name_master, "PONDER_APP_NO", 0, ponder_app_no) < 0	// 计量委托号  系统别+Q(汽运)+6位日期+4位流水
				|| epex.SetValue(block_name_master, "APP_TYPE", 0, "1") < 0					// 委托类型  0:固定委托 1:一车一委托
				|| epex.SetValue(block_name_master, "GROSS_TYPE", 0, "1") < 0					// 计量方式	0：定皮；1：先皮后毛；2：先毛后皮
				|| epex.SetValue(block_name_master, "FIXED_TARE_TYPE", 0, " ") < 0			// 定皮方式		0：日皮；1：周皮；2：月皮；3：半年皮；4:1季度；5：1年
				|| epex.SetValue(block_name_master, "PLAN_TYPE", 0, "C") < 0					// 任务类型		A:进厂B:厂内C:出厂
				|| epex.SetValue(block_name_master, "PONDER_APP_TIME", 0, datetime) < 0		// 过磅申请时间
				|| epex.SetValue(block_name_master, "GOODS_NAME", 0, tsmpe11["PROD_CNAME"].ToString()) < 0	// 品名
				|| epex.SetValue(block_name_master, "GOODS_CODE", 0, tsmpe11["PROD_CODE"].ToString()) < 0	// 品名代码
				|| epex.SetValue(block_name_master, "DEL_UNIT", 0, "鞍钢本部") < 0				// 发货单位
				|| epex.SetValue(block_name_master, "DEL_UNIT_CODE", 0, tsmpe11["FACTORY_DIV"].ToString()) < 0				// 发货单位代码
				|| epex.SetValue(block_name_master, "REC_UNIT", 0, tsmpe11["CONSIGNE_NAME"].ToString()) < 0	// 收货单位
				|| epex.SetValue(block_name_master, "REC_UNIT_CODE", 0, CONSIGNE_CODE) < 0	// 收货单位代码
				|| epex.SetValue(block_name_master, "TRUCK_NO", 0, tsmpe11["VEHICLE_NO"].ToString()) < 0	// 车牌号
				|| epex.SetValue(block_name_master, "EFFECTIVE_TIME", 0, datetime) < 0		// 生效时间
				|| epex.SetValue(block_name_master, "EXPIRING_TIME", 0, datetime_end) < 0		// 截止时间
				|| epex.SetValue(block_name_master, "NEED_UNLOAD_FLAG", 0, "0") < 0			// 是否需要卸货确认		0：不需要卸货确认；	1：需要卸货确认
				|| epex.SetValue(block_name_master, "LOAD_FLAG", 0, "1") < 0					// 是否装货		0:未装货；1：已装货
				//|| epex.SetValue(block_name_master, "GROSS_WEIGHT_MAX", 0, tsmpe11["STACKING_GROSS_WT"].ToDecimal() + wt_max)<0		// 毛重上限
				//|| epex.SetValue(block_name_master, "GROSS_WEIGHT_MIN", 0, tsmpe11["STACKING_GROSS_WT"].ToDecimal() - wt_min)<0		// 毛重下限
				|| epex.SetValue(block_name_master, "NET_WEIGHT_MIN", 0, tsmpe11["STACKING_GROSS_WT"].ToDecimal() - wt_max) < 0		// 净重上限
				|| epex.SetValue(block_name_master, "NET_WEIGHT_MAX", 0, tsmpe11["STACKING_GROSS_WT"].ToDecimal() + wt_min) < 0		// 净重下限
				|| epex.SetValue(block_name_master, "CHECK_TARE", 0, "0") < 0				// 是否检查皮重超差	0：否；1：是
				|| epex.SetValue(block_name_master, "SYS_ID1", 0, tc_no.Substring(0, 2)) < 0					// 转发系统1
				)
			{
				CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "发送电文时失败! 原因描述：{0}", arguments, 1);//格式化字符串
				throw CApplicationException(-1, s.msg, log.Location);
			}


			//发送电文
			if (epex.SendTele() < 0)
			{
				CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "发送电文时失败! 原因描述： [{0}]", arguments, 1);//格式化字符串
				throw CApplicationException(-1, s.msg, log.Location);
			}
			//释放
			epex.Uninitialize();


			// 更新码单表上的计量委托号
			if (oper_flag == "I")
			{
				sqlstr = " UPDATE tsmpe11 SET PONDER_NO = @PONDER_NO WHERE TICKET_NO IN ('" + TICKET_NO + "') ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("TICKET_NO", TICKET_NO);
				cmd_inq.Parameters.Set("PONDER_NO", ponder_app_no);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteNonQuery();
			}
			else
			{
				sqlstr = " UPDATE tsmpe11 SET PONDER_NO = ' ' WHERE TICKET_NO IN ('" + TICKET_NO + "') ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("TICKET_NO", TICKET_NO);
				//cmd_inq.Parameters.Set("PONDER_NO", ponder_app_no);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteNonQuery();
			}

		}

		strcpy(s.msg, "处理成功！");//处理成功。
		s.flag = 0;
		return 0;
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, "数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。", arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		EDLog(1, 1, "[%s]", s.sysmsg);
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
	{
		EDLog(1, 1, "#####msg = [%s]", (const char*)ex.GetMsg());
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
