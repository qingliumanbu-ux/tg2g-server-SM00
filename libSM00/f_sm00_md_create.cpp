/************************************************
*	程序名称：作业单处理——装车确认		*
*	编制日期：2023-1-11 11:07:31                *
*	编 制 人：13801					            *
*************************************************/
#include "stdafx.h"
#include "epex.h"

//程序用头文件








/* ***** 外部函数申明 ***** */
int	f_sm00_md_no(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);		/* 码单号生成 */
int	f_sm00_md_ok(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);		//码单确认
int f_sm00_record(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 写履历记录 */
int f_wm_stock(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 仓库出库函数

int f_sm00_load_wt_check(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	/* 装车重量超重检查 */
int f_sm00_xxjlwt_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	// 计量委托

// service入口

int f_sm00_md_create(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);
	/*程序用变量*/
	int blkNum = 0;
	int	ret = 0;
	int	doFlag = 0;
	int	fetchRowCount = 0;
	int	fetchRowCount1 = 0;
	int	blkSeq = 0;
	int	i = 0;
	//int	blkname1 = 1;
	int	v_flag = 0;
	//CString	blk_name = "md_ok";			/* 定义传入的块名 */
	CString	str = "";
	//CString	blkName = "smbw07_pro";
	CString	record_name = "sm00_record";
	CString	wm_block = "WM_STOCK";
	CString	lsh;															/* 流水号细项 */

	int i_count = 0;
	CString c_factory_div = " ";
	CString c_mat_no = " ";
	CString c_proc_type = " ";
	CString c_userid = " ";
	CString c_stacking_no = " ";	// 装车清单号，码单号前9位
	CString	stacking_no_r = "发货成功，码单号：";
	CString	delivy_remark = " ";
	CString	stacking_no = "";			/* 码单号 */
	CString	c_bill_of_lading_no = "";
	CString	c_vehicle_no = "";
	CString	v_crane_cmd_yn = "";			/* 吊车命令标记 */
	CString v_ponder_mark = "";	// 称重标记
	CString	v_out_stock_code = "";
	CString c_delivy_shift = " ", c_delivy_group = " ";
	char  c_datetime[15];

	CString	datetime = "";
	CString	date = "";
	CString	time = "";
	CString	v_userid = "";
	CDecimal v_wt = 1;
	CString DELIVY_MAKER = " ";	// 发货人

	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	date = datetime.Substring(0, 8);
	time = datetime.Substring(8, 6);

	CModel tsmpe00("TSMPE00");
	CModel tsmpe02("TSMPE02");
	CModel tsmpe02_in("TSMPE02");
	CModel tsmpe02_in1("TSMPE02");
	CModel tsmpe10("TSMPE10");
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");
	CModel tom01("TOM01");
	CModel ted21("TED21");

	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_loop(conn);
	CDbCommand cmd_inq_loop2(conn);
	CDbCommand cmd_upd(conn);
	CString sqlstr;

	try
	{

		/*获得传入参数*/
		c_userid = s.userid;

		EIClass ds_mat_no;
		EIClass ds_mdno_rec, ds_mdno_ret;	//码单号生成块
		ds_mdno_rec.Tables[0].Columns.Add(DT_STRING, "stock_no");
		ds_mdno_rec.Tables[0].Rows.Add();

		/* 仓库函数定义块 */
		//EIClass bcls_stock_out;
		//bcls_stock_out.Tables[0].set_TableName(wm_block);
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_NO");	// 材料号
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_OPER_ORDER");	// 库业务类型
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_NO");
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "TO_STOCK_NO");
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "TO_STOCK_PLACE_NO");	//目标库位号
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "ROWNO");
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "COLUMN_NO");
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "TO_LAYERNO");	// 目标层号
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_PLACE_POSITION");
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "VEHICLE_NO");
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_LINE_TYPE");	//产线类型
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_KIND");	// 物料类型
		//bcls_stock_out.Tables[0].Rows.Clear();

		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}


		/********************************
		*	读取传入的参数				*
		********************************/
		int rows = bcls_rec->Tables[0].Rows.get_Count();
		if (rows == 0)
		{
			sprintf(s.msg, "没有传入装车的材料记录");
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}



		// 读取传入的多记录数据
		for ( i = 0; i < rows; i++)
		{
			tsmpe02_in.MergeFrom(bcls_rec->Tables[0].Rows[i]);

			if (tsmpe02_in["MAT_NO"].ToString().Trim() == "")
			{
				// 按作业单读取材料
				if (tsmpe02_in["WORK_ID"].ToString().Trim() == "")
				{
					sprintf(s.msg, "传入的作业单不能为空.");
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}

				// 检查作业单号是否为确认，否报错
				CString status = "", STOCK_NO = "";
				sqlstr = "select status,STOCK_NO from tsmpe10a where work_id = @work_id ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("work_id", tsmpe02_in["WORK_ID"].ToString());
				Log::Info("", "", "sqlstr = {0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					status = cmd_inq.GetString(1);
					STOCK_NO = cmd_inq.GetString(2);
				}
				cmd_inq.Close();


				if (status != "3")
				{
					CFormattable arguments[] = { tsmpe02_in["WORK_ID"].ToString(), status };
					CMessageFormat::Format(s.msg, "作业单 {0} 的状态{1}不是确认状态 3 不能做装车处理", arguments, 2);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}


				sqlstr = "select * from tsmpe02 t where t.work_id = @work_id ";
				cmd_inq_loop.SetCommandText(sqlstr);
				cmd_inq_loop.Parameters.Set("work_id", tsmpe02_in["WORK_ID"].ToString().Trim());
				Log::Info("", "", "sqlstr = {0}", sqlstr);
				cmd_inq_loop.ExecuteReader();
				while (cmd_inq_loop.Read())
				{
					cmd_inq_loop.Fetch(tsmpe02);
					if (tsmpe02["RED_FLAG"].ToString() == "1")
					{
						sprintf(s.msg, "作业单下材料 %s 提出了红冲请求，请先处理！", (const char *)tsmpe02["MAT_NO"].ToString());
						throw	CApplicationException(-1, s.msg, s.svc_name);
					}
					if (tsmpe02["TICKET_NO"].ToString().Trim() != "")
					{
						sprintf(s.msg, "作业单下材料 %s 已经装车！", (const char *)tsmpe02["MAT_NO"].ToString());
						throw	CApplicationException(-1, s.msg, s.svc_name);
					}

					////// 2021-12-24 down
					////CString order_type_code = "", order_month = "";
					////sqlstr = "select order_type_code,ORDER_MONTH from tom01 where order_no = '" + tsmpe02["ORDER_NO"].ToString().Trim() + "' ";
					////cmd_inq.SetCommandText(sqlstr);
					////cmd_inq.ExecuteReader();
					////if (cmd_inq.Read())
					////{
					////	order_type_code = cmd_inq.GetString(1);	// 合同性质
					////	order_month = cmd_inq.GetString(2);
					////	if (order_type_code == "TZA")		// 一般统货合同
					////	{
					////		if (order_month < datetime.SubstringNE(0,6))
					////		{
					////			CFormattable arguments[] = { tsmpe02["ORDER_NO"].ToString(), order_month };
					////			CMessageFormat::Format(s.msg, "此合同{0}的交货月比当前月小！", arguments, 2);
					////			throw	CApplicationException(-1, s.msg, s.svc_name);
					////		}
					////	}
					////}
					////else
					////{
					////	CFormattable arguments[] = { tsmpe02["ORDER_NO"].ToString() };
					////	CMessageFormat::Format(s.msg, "无此合同{0} 信息！", arguments, 1);
					////	throw	CApplicationException(-1, s.msg, s.svc_name);
					////}
					////cmd_inq.Close();
					////// 2021-12-24 up

					tsmpe02.MergeTo(ds_mat_no.Tables[0], false);
				}
				cmd_inq_loop.Close();
				//continue;
			}
			else
			{
				////sprintf(s.msg, "请选择作业单！");
				////throw	CApplicationException(-1, s.msg, s.svc_name);

				tsmpe02_in.MergeTo(ds_mat_no.Tables[0], false);
				tsmpe02["MAT_NO"] = tsmpe02_in["MAT_NO"];
				tsmpe02["STOCK_NO"] = tsmpe02_in["STOCK_NO"];
				tsmpe02["MAT_KIND"] = tsmpe02_in["MAT_KIND"];

				CString stacking_no = Db::QueryCString("select stacking_no from tsmpe12 where mat_no ='" + tsmpe02["MAT_NO"].ToString() + "'");
				if (stacking_no.Trim() != "")
				{
					CDecimal cnt_0 = Db::QueryCDecimal("select count(1) from tsmpe11 where stacking_no ='" + stacking_no + "' and stacking_prints ='0'");
					if (cnt_0 > 0)
					{
						Log::Info("", "", "stacking_no = {0}", stacking_no);
						sprintf(s.msg, "材料[" + tsmpe02["MAT_NO"].ToString() + "],在码单[" + stacking_no + "]，未确认。不能再操作！");
						throw	CApplicationException(-1, s.msg, s.svc_name);

					}
				}
			}

		}

		CString LOADING_SOLUTION_CODE = "";	// 装载方案号
		CString	STEEL_BRACKET_NO = "", STEEL_BRACKET_NO2 = "", STEEL_BRACKET_NO3 = "", STEEL_BRACKET_NO4 = "";	//钢架号
		CString	DELIVY_SHIFT = "", DELIVY_GROUP = "", DELIVY_MAKER = "";
		CString	VEHICLE_NO = "";
		CString steel_type = " ";	// 钢架类型
		CDecimal u1_loading_wt = 0;	// u1钢架装载重量
		CString CARRY_PLAN_NO = "";	// 承运计划号
		// 数据检查
		tsmpe02_in1.MergeFrom(bcls_rec->Tables[1].Rows[0]);
		tsmpe02_in1.TrimOrBlank();

		//if (tsmpe02_in1["DELIVY_SHIFT"].ToString().Trim() == "")
		//{
		//	sprintf(s.msg, "班次不能为空");
		//	throw	CApplicationException(-1, s.msg, s.svc_name);
		//}
		//if (tsmpe02_in1["DELIVY_GROUP"].ToString().Trim() == "")
		//{
		//	sprintf(s.msg, "班组不能为空");
		//	throw	CApplicationException(-1, s.msg, s.svc_name);
		//}
		//if (tsmpe02_in1["DELIVY_MAKER"].ToString().Trim() == "")
		//{
		//	sprintf(s.msg, "发货工不能为空");
		//	throw	CApplicationException(-1, s.msg, s.svc_name);
		//}
		if (tsmpe02_in1["VEHICLE_NO"].ToString().Trim() == "")
		{
			sprintf(s.msg, "车号不能为空");
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}


		// 读取班次班组
		f_epep_get_shift_group("DEFAULT", datetime, c_delivy_shift, c_delivy_group, conn);
		tsmpe02_in1["DELIVY_SHIFT"] = c_delivy_shift;
		tsmpe02_in1["DELIVY_GROUP"] = c_delivy_group;
		tsmpe02_in1["DELIVY_MAKER"] = s.username;


		// 按材料号读取提单号
		sqlstr = "select BILL_OF_LADING_NO,VEHICLE_NO,CONFM_STATUS,DELIVY_QTY_FLAG from tsmpe02 where mat_no = '" + tsmpe02["MAT_NO"].ToString().Trim() + "' ";
		cmd_inq.SetCommandText(sqlstr);
		Log::Info("", "", "sqlstr = {0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			tsmpe10["BILL_OF_LADING_NO"] = cmd_inq.GetString(1);
			tsmpe02["VEHICLE_NO"] = cmd_inq.GetString(2);
			tsmpe02["CONFM_STATUS"] = cmd_inq.GetString(3);
			tsmpe02["DELIVY_QTY_FLAG"] = cmd_inq.GetString(4);
		}
		cmd_inq.Close();
		Log::Info("", "", "BILL_OF_LADING_NO = {0}", tsmpe10["BILL_OF_LADING_NO"].ToString());
		Log::Info("", "", "VEHICLE_NO = {0}", tsmpe02["VEHICLE_NO"].ToString());

		// 按提单号读取运输方式，物料种类
		sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = @BILL_OF_LADING_NO ";
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("BILL_OF_LADING_NO", tsmpe10["BILL_OF_LADING_NO"].ToString());
		Log::Info("", "", "sqlstr = {0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			cmd_inq.Fetch(tsmpe10);
		}
		cmd_inq.Close();


		//if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1,1) == "2")	// 铁运
		//{
		//	if (tsmpe02_in1["LOADING_SOLUTION_CODE"].ToString().Trim() == "")
		//	{
		//		sprintf(s.msg, "铁运装载方案不能为空！");
		//		throw	CApplicationException(-1, s.msg, s.svc_name);
		//	}
		//}



		////////// 根据库区代码读取系统别
		////////CString sys_code_1 = "", sys_code_2 = "";
		////////sqlstr = "select AREA_CODE,crane_mark,ponder_mark FROM TSI0021 WHERE STOCK_NO = '" + tsmpe10["STOCK_NO"].ToString() + "' ";
		////////cmd_inq.SetCommandText(sqlstr);
		////////Log::Debug("", "", "sqlstr={0}", sqlstr);
		////////cmd_inq.ExecuteReader();
		////////if (cmd_inq.Read())
		////////{
		////////	sys_code_1 = cmd_inq.GetString(1);
		////////	v_crane_cmd_yn = cmd_inq.GetString(2);
		////////	v_ponder_mark = cmd_inq.GetString(3);
		////////}
		////////else
		////////{
		////////	sprintf(s.msg, "读取库区代码表TSI0021上行车标记出错，库区号=%s", (const char *)tsmpe10["STOCK_NO"].ToString());
		////////	throw	CApplicationException(-1, s.msg, s.svc_name);
		////////}
		////////cmd_inq.Close();

		////////if (tsmpe10["STOCK_NO_TO"].ToString().Trim() != "")
		////////{
		////////	sqlstr = "select AREA_CODE FROM TSI0021 WHERE STOCK_NO = '" + tsmpe10["STOCK_NO_TO"].ToString() + "' ";
		////////	cmd_inq.SetCommandText(sqlstr);
		////////	Log::Debug("", "", "sqlstr={0}", sqlstr);
		////////	cmd_inq.ExecuteReader();
		////////	if (cmd_inq.Read())
		////////	{
		////////		sys_code_2 = cmd_inq.GetString(1);
		////////	}
		////////	cmd_inq.Close();
		////////}




		// 生成装车单号
		CString stock_no = tsmpe02["STOCK_NO"];
		int count = 0;
		CString	TICKET_NO = "";	// 装车单

		if (stock_no.Trim() == "")
		{
			sprintf(s.msg, "传人的仓库代码不能为空");
			throw CApplicationException(-1, s.msg, log.Location);
		}

		//到流水号表按关键字读取记录
		ted21["SEQ_NAME"] = "ZCD_" + stock_no;
		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "2")	// 铁运
		{
			ted21["SEQ_NAME"] = "2ZCD_" + stock_no;
		}
		else
		{
			ted21["SEQ_NAME"] = "1ZCD_" + stock_no;
		}

		count = ted21.QueryCount("SEQ_NAME");
		if (count == 0)
		{
			//新增记录，按年复位
			ted21["SEQ_DESC"] = "仓库" + stock_no + "装车单流水号";
			ted21["SEQ_BEGIN"] = 0;
			ted21["SEQ_NOW"] = 0;
			ted21["SEQ_END"] = 999;
			ted21["SEQ_PRE"] = stock_no;	// 流水号前缀
			ted21["SEQ_LEN"] = 3;
			ted21["SEQ_RECYCLE_FLAG"] = "3";	// 1--按年复位,2--按月复位，3--按日复位
			ted21["REC_CREATE_TIME"] = datetime;
			ted21["REC_CREATOR"] = s.userid;
			ted21.TrimOrBlank();
			if (ted21.Insert() == false)
			{
				sprintf(s.msg, "新增流水号记录失败");
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
		//TICKET_NO = stock_no + datetime.Substring(2, 2) + EPGetNextSeq(ted21["SEQ_NAME"].ToString(), conn);
		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "2")	// 铁运
		{
			TICKET_NO = "2" + stock_no + CDateTime::Now().ToString("yyyyMMddHHmmss").Substring(2, 6) + EPGetNextSeq(ted21["SEQ_NAME"].ToString(), conn);
		}
		else
		{
			TICKET_NO = "1" + stock_no + CDateTime::Now().ToString("yyyyMMddHHmmss").Substring(2, 6) + EPGetNextSeq(ted21["SEQ_NAME"].ToString(), conn);
		}

		Log::Trace("", __FUNCTION__, "生成的装车单[{0}]", TICKET_NO);




		// 循环读取压入的材料，更新材料上的装车单号、车号、班次、班组、操作工，钢架号，装载方案号
		rows = ds_mat_no.Tables[0].Rows.get_Count();
		if (rows == 0)
		{
			sprintf(s.msg, "没有读取到材料记录");
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}

		CString mat_no = "";
		for (i = 0; i < rows; i++)
		{
			tsmpe02["MAT_NO"] = ds_mat_no.Tables[0].Rows[i]["MAT_NO"];
			if (i == 0)
			{
				mat_no = "'" + tsmpe02["MAT_NO"].ToString().Trim() + "'";
			}
			else
			{
				mat_no = mat_no + ",'" + tsmpe02["MAT_NO"].ToString().Trim() + "'";
			}
		}



		CString	VEHICLE_ID = " ", VEHICLE_TYPE = " ", STATUS = " ", MARK = "";
		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "2")
		{
			// 按车号读取车皮ID
			sqlstr = "select vehicle_id,VEHICLE_TYPE,STATUS,MARK from tsm00b4 where vehicle_no = @vehicle_no  ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("vehicle_no", tsmpe02_in1["VEHICLE_NO"].ToString());
			Log::Info("", "", "sqlstr = {0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				VEHICLE_ID = cmd_inq.GetString(1);
				VEHICLE_TYPE = cmd_inq.GetString(2);
				STATUS = cmd_inq.GetString(3);
				MARK = cmd_inq.GetString(4);	// 2022-3-31
				if (STATUS != "2")	// 可用状态
				{
					CFormattable arguments[] = { tsmpe02_in1["VEHICLE_NO"].ToString() };
					CMessageFormat::Format(s.msg, "铁运没有此车皮{0}状态为可用‘2’的信息不能发货！", arguments, 1);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
			}
			else
			{
				CFormattable arguments[] = { tsmpe02_in1["VEHICLE_NO"].ToString() };
				CMessageFormat::Format(s.msg, "铁运没有此车皮{0}信息不能发货！", arguments, 1);
				throw	CApplicationException(-1, s.msg, s.svc_name);	// 2021-8-18
			}
		}
		cmd_inq.Close();


		sqlstr = "UPDATE TSMPE02 SET TICKET_NO = @TICKET_NO,VEHICLE_NO = @VEHICLE_NO ,DELIVY_SHIFT=@DELIVY_SHIFT,DELIVY_GROUP=@DELIVY_GROUP,DELIVY_MAKER=@DELIVY_MAKER"
			",VEHICLE_ID = @VEHICLE_ID ,TRNP_MODE_CODE = @TRNP_MODE_CODE "
			" ,DELIVY_TIME = @DELIVY_TIME , LOADING_SOLUTION_CODE = @LOADING_SOLUTION_CODE "
			" WHERE MAT_NO IN (" + mat_no + ") ";
		cmd_upd.SetCommandText(sqlstr);
		cmd_upd.Parameters.Set("TICKET_NO", TICKET_NO);
		cmd_upd.Parameters.Set("VEHICLE_NO", tsmpe02_in1["VEHICLE_NO"].ToString());
		cmd_upd.Parameters.Set("VEHICLE_ID", VEHICLE_ID.TrimOrBlank());
		cmd_upd.Parameters.Set("DELIVY_SHIFT", tsmpe02_in1["DELIVY_SHIFT"].ToString());
		cmd_upd.Parameters.Set("DELIVY_GROUP", tsmpe02_in1["DELIVY_GROUP"].ToString());
		cmd_upd.Parameters.Set("DELIVY_MAKER", tsmpe02_in1["DELIVY_MAKER"].ToString());
		cmd_upd.Parameters.Set("TRNP_MODE_CODE", tsmpe10["TRNP_MODE_CODE"].ToString().TrimOrBlank());
		cmd_upd.Parameters.Set("DELIVY_TIME", datetime);
		cmd_upd.Parameters.Set("LOADING_SOLUTION_CODE", tsmpe02_in1["LOADING_SOLUTION_CODE"].ToString().TrimOrBlank());
		Log::Info("", "", "sqlstr = {0}", sqlstr);
		int update_rows;
		update_rows = cmd_upd.ExecuteNonQuery();
		if (update_rows != rows)
		{
			CFormattable arguments[] = { update_rows,rows };
			CMessageFormat::Format(s.msg, "更新材料数 {0} 和实际的材料数 {1} 不符", arguments, 2);
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}


		//// 清除没有勾选材料上的车号、计划号
		//if (tsmpe02["CONFM_STATUS"].ToString()=="4" && tsmpe02["VEHICLE_NO"].ToString().Trim()!= "")
		//{
		//	sqlstr = "UPDATE TSMPE02 SET VEHICLE_NO = ' ' ,BILL_OF_LADING_NO = ' ' WHERE VEHICLE_NO ='" + tsmpe02["VEHICLE_NO"].ToString() + "' ";
		//	cmd_upd.SetCommandText(sqlstr);
		//	Log::Debug("", "", "sqlstr={0}", sqlstr);
		//	cmd_upd.ExecuteNonQuery();
		//}

		//if (tsmpe02["CONFM_STATUS"].ToString() == "6" && tsmpe02["VEHICLE_NO"].ToString().Trim() != "")
		//{
		//	sqlstr = "UPDATE TSMPE02 SET VEHICLE_NO = ' ' WHERE VEHICLE_NO ='" + tsmpe02["VEHICLE_NO"].ToString() + "' ";
		//	cmd_upd.SetCommandText(sqlstr);
		//	Log::Debug("", "", "sqlstr={0}", sqlstr);
		//	cmd_upd.ExecuteNonQuery();
		//}



		sqlstr = " select BILL_OF_LADING_NO,ORDER_NO,sum(mat_wt),count(1),sum(MAT_GROSS_WT),sum(MAT_TUBE) from tsmpe02 "
			" where TICKET_NO = @TICKET_NO group by BILL_OF_LADING_NO,ORDER_NO ";
		cmd_inq_loop.SetCommandText(sqlstr);
		cmd_inq_loop.Parameters.Set("TICKET_NO", TICKET_NO);
		Log::Info("", "", "sqlstr = {0}", sqlstr);
		Log::Info("", "", "TICKET_NO = {0}", TICKET_NO);
		cmd_inq_loop.ExecuteReader();
		while (cmd_inq_loop.Read())
		{
			tsmpe11["BILL_OF_LADING_NO"] = cmd_inq_loop.GetString(1);
			tsmpe11["ORDER_NO"] = cmd_inq_loop.GetString(2);
			tsmpe11["STACKING_WT"] = cmd_inq_loop.GetDecimal(3);
			tsmpe11["STACKING_NUM"] = cmd_inq_loop.GetInt32(4);
			tsmpe11["STACKING_GROSS_WT"] = cmd_inq_loop.GetDecimal(5);
			tsmpe11["STACKING_TUBE"] = cmd_inq_loop.GetInt32(6);

			Log::Info("", "", "BILL_OF_LADING_NO = {0}", tsmpe11["BILL_OF_LADING_NO"].ToString());
			Log::Info("", "", "ORDER_NO = {0}", tsmpe11["ORDER_NO"].ToString());
			Log::Info("", "", "STACKING_WT = {0}", tsmpe11["STACKING_WT"].ToDecimal());



			// 检查发货材料重量是否超计划量
			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" )
			{
				CDecimal plan_wt = 0;
				sqlstr = "SELECT PLAN_WT_D FROM TSMPE10 WHERE BILL_OF_LADING_NO = '" + tsmpe11["BILL_OF_LADING_NO"].ToString() + "' "
					" AND ORDER_NO = '" + tsmpe11["ORDER_NO"].ToString() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					plan_wt = cmd_inq.GetDecimal(1);
					if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "1")	// 汽运
					{
						if (tsmpe11["STACKING_WT"].ToDecimal() > plan_wt)
						{
							CFormattable arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString(), tsmpe11["ORDER_NO"].ToString(),plan_wt, tsmpe11["STACKING_WT"].ToDecimal() };
							CMessageFormat::Format(s.msg, "检查是否超量时,实际重量【{2}】超计划量【{3}】，计划号【{0}】合同号【{1}】", arguments, 4);
							throw	CApplicationException(-1, s.msg, s.svc_name);
						}
					}

				}
				else
				{
					CFormattable arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString(), tsmpe11["ORDER_NO"].ToString() };
					CMessageFormat::Format(s.msg, "检查是否超量时，没有读到计划量，计划号【{0}】合同号【{1}】", arguments, 2);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
				cmd_inq.Close();
			}


			// 按合同号读取合同信息
			sqlstr = " select * from tom01 where order_no = @order_no ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("order_no", tsmpe11["ORDER_NO"].ToString());
			Log::Info("", "", "sqlstr = {0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				cmd_inq.Fetch(tom01);
			}
			cmd_inq.Close();


			////// 按合同号读取合同信息
			////sqlstr = " select * from tsmpe00 where order_no = @order_no ORDER BY REC_CREATE_TIME DESC ";
			////cmd_inq.SetCommandText(sqlstr);
			////cmd_inq.Parameters.Set("order_no", tsmpe11["ORDER_NO"].ToString());
			////Log::Info("", "", "sqlstr = {0}", sqlstr);
			////cmd_inq.ExecuteReader();
			////if (cmd_inq.Read())
			////{
			////	cmd_inq.Fetch(tsmpe00);
			////}
			////cmd_inq.Close();



			//调用函数码单号生成
			ds_mdno_rec.Tables[0].Rows[0]["stock_no"] = stock_no;
			doFlag = f_sm00_md_no(&ds_mdno_rec, &ds_mdno_ret, conn);
			if (doFlag < 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			stacking_no = ds_mdno_ret.Tables[0].Rows[0]["stacking_no"];
			EDLog(1, 1, "生成码单号 stacking_no = [%s]", (const char*)stacking_no);

			stacking_no_r = stacking_no_r + stacking_no + "  ";

			// 读取材料记录写码单材料表
			sqlstr = "select * from tsmpe02 where BILL_OF_LADING_NO = @BILL_OF_LADING_NO AND ORDER_NO = @ORDER_NO AND TICKET_NO = @TICKET_NO ";
			cmd_inq_loop2.SetCommandText(sqlstr);
			cmd_inq_loop2.Parameters.Set("BILL_OF_LADING_NO", tsmpe11["BILL_OF_LADING_NO"].ToString());
			cmd_inq_loop2.Parameters.Set("ORDER_NO", tsmpe11["ORDER_NO"].ToString());
			cmd_inq_loop2.Parameters.Set("TICKET_NO", TICKET_NO);
			Log::Info("", "", "sqlstr = {0}", sqlstr);
			cmd_inq_loop2.ExecuteReader();
			while (cmd_inq_loop2.Read())
			{
				cmd_inq_loop2.Fetch(tsmpe02);
				tsmpe12.CopyFrom(tsmpe02);
				tsmpe12["REC_CREATE_TIME"] = datetime;
				//tsmpe12["REC_CREATOR"] = s.userid;
				tsmpe12["REC_REVISOR"] = s.userid;
				tsmpe12["REC_REVISE_TIME"] = datetime;
				tsmpe12["STACKING_NO"] = stacking_no;

				////// 读取物料表上的出入库标记，不在库时，不调用仓库函数	2021-11-2
				////CString table_name = "TMM" + tsmpe12["MAT_KIND"].ToString() + "01";
				////CString in_flag = "";
				////sqlstr = "select in_flag from " + table_name + " where mat_no = '" + tsmpe12["MAT_NO"].ToString() + "' ";
				////cmd_inq.SetCommandText(sqlstr);
				////Log::Debug("", "", "sqlstr={0}", sqlstr);
				////cmd_inq.ExecuteReader();
				////if (cmd_inq.Read())
				////{
				////	in_flag = cmd_inq.GetString(1);
				////}
				////cmd_inq.Close();
				////Log::Debug("", "", "in_flag={0}", in_flag);
				////if (in_flag == "1")
				////{
				////	bcls_stock_out.Tables[wm_block].Rows.Add();
				////	int ii = bcls_stock_out.Tables[wm_block].Rows.get_Count() - 1;
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_NO"] = tsmpe12["MAT_NO"];
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_OPER_ORDER"] = "30";	// 30--倒跺
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_NO"] = tsmpe10["STOCK_NO"];
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["TO_STOCK_NO"] = tsmpe10["STOCK_NO"];
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["TO_STOCK_PLACE_NO"] = tsmpe10["STOCK_NO"].ToString() + "XXXXXX";
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["ROWNO"] = " ";
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["COLUMN_NO"] = " ";
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["TO_LAYERNO"] = 0;
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_PLACE_POSITION"] = " ";
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["VEHICLE_NO"] = tsmpe12["VEHICLE_NO"];  //车号 如果有就传
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_LINE_TYPE"] = tsmpe12["MAT_KIND"]; //取物料主档
				////	bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_KIND"] = tsmpe12["MAT_KIND"]; //取物料主档
				////}

				if (tsmpe02["VEHICLE_NO"].ToString().Trim() == "")
				{
					CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() };
					CMessageFormat::Format(s.msg, "此材料{0}的车牌号不能为空", arguments, 1);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}


				/*	调用函数新增履历记录												*/
				bcls_rec->Tables[record_name].Rows[0]["mat_no"] = tsmpe02["MAT_NO"];
				bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "C";	// 发货出库
				bcls_rec->Tables[record_name].Rows[0]["userid"] = s.userid;


				ret = 0;
				ret = f_sm00_record(bcls_rec, bcls_ret, conn);
				if (ret < 0)
				{
					Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}




				tsmpe12.TrimOrBlank();
				sqlstr = "insert into tsmpe12 where STACKING_NO = '" + tsmpe12["STACKING_NO"].ToString() + "' and mat_no = '" + tsmpe12["MAT_NO"].ToString() + "' ";
				tsmpe12.Insert();

			}
			cmd_inq_loop2.Close();


			tsmpe11["REC_CREATE_TIME"] = datetime;
			tsmpe11["REC_CREATOR"] = c_userid;
			tsmpe11["REC_REVISE_TIME"] = " ";
			tsmpe11["REC_REVISOR"] = " ";
			tsmpe11["REC_ERASE_TIME"] = " ";
			tsmpe11["REC_ERASOR"] = " ";
			tsmpe11["ARCHIVE_FLAG"] = " ";
			tsmpe11["COMPANY_CODE"] = tsmpe10["COMPANY_CODE"];
			tsmpe11["STACKING_NO"] = stacking_no;
			tsmpe11["FACTORY_DIV"] = tsmpe02["FACTORY_DIV"];
			tsmpe11["MAT_KIND"] = tsmpe02["MAT_KIND"];
			tsmpe11["STOCK_NO"] = tsmpe10["STOCK_NO"];
			//tsmpe11["BILL_OF_LADING_NO"] = tsmpe02["BILL_OF_LADING_NO"];
			tsmpe11["CONFM_PLAN_NO"] = " ";
			tsmpe11["READY_BILL_NO"] = " ";
			//tsmpe11["STACKING_WT"] = stacking_wt;
			//tsmpe11["STACKING_GROSS_WT"] = stacking_wt;
			//tsmpe11["STACKING_NUM"] = stacking_num;
			//tsmpe11["STACKING_TUBE"] = stacking_tube;
			//tsmpe11.VEHICLE_ID = tsmpe12["VEHICLE_ID"];
			tsmpe11["VEHICLE_NO"] = tsmpe12["VEHICLE_NO"];
			//tsmpe11["ORDER_NO"] = tsmpe00["ORDER_NO"];
			//tsmpe11["EXPORT_FLAG"] = tom01["EXPORT_FLAG"];
			//tsmpe11["SG_SIGN"] = tom01["SG_SIGN"];
			tsmpe11["SG_SIGN"] = tsmpe02["SG_SIGN"];
			//tsmpe11["SG_STD"] = tsmpe02.SG_STD;
			//tsmpe11["PROD_CODE"] = tom01["PROD_CODE"];
			tsmpe11["PROD_CODE"] = tsmpe02["PROD_CODE"];
			//tsmpe11["PROD_CNAME"] = tom01["PROD_CNAME"];
			tsmpe11["PROD_CNAME"] = tsmpe02["PROD_CNAME"];
			tsmpe11["PROD_ENAME"] = tsmpe02["PROD_ENAME"];
			//tsmpe11["ORDER_THICK"] = tsmpe00["ORDER_THICK"];
			//tsmpe11["ORDER_WIDTH"] = tsmpe00["ORDER_WIDTH"];
			//tsmpe11["ORDER_LEN"] = tsmpe00["ORDER_LEN"];
			//tsmpe11["ORDER_LEN_MIN"] = tsmpe00["ORDER_MIN_LEN"];
			//tsmpe11["ORDER_LEN_MAX"] = tsmpe00["ORDER_MAX_LEN"];
			tsmpe11["ORDER_THICK"] = tom01["ORDER_THICK"];
			tsmpe11["ORDER_WIDTH"] = tom01["ORDER_WIDTH"];
			tsmpe11["ORDER_LEN"] = tom01["ORDER_LEN"];
			tsmpe11["ORDER_LEN_MIN"] = tom01["ORDER_MIN_LEN"];
			tsmpe11["ORDER_LEN_MAX"] = tom01["ORDER_MAX_LEN"];
			tsmpe11["DELIVY_TIME"] = datetime;
			tsmpe11["DELIVY_GROUP"] = tsmpe02_in1["DELIVY_GROUP"];
			tsmpe11["DELIVY_SHIFT"] = tsmpe02_in1["DELIVY_SHIFT"];
			tsmpe11["DELIVY_MAKER"] = tsmpe02_in1["DELIVY_MAKER"];
			tsmpe11["TRNP_MODE_CODE"] = tsmpe10["TRNP_MODE_CODE"];
			tsmpe11["CONSIGNE_NAME"] = tsmpe10["CONSIGNE_NAME"];
			//tsmpe11["ORDER_CUST_CNAME"] = tsmpe00["ORDER_CUST_CNAME"];
			tsmpe11["ORDER_CUST_CNAME"] = tom01["ORDER_CUST_CNAME"];

			tsmpe11["BALANCE_USER_NAME"] = tsmpe10["BALANCE_USER_NAME"];	//结算用户
			tsmpe11["CONVEY_UNIT_NAME"] = tsmpe10["CONVEY_UNIT_NAME"];
			tsmpe11["DELIVY_PLACE_NAME"] = tsmpe10["DELIVY_PLACE_NAME"];
			//tsmpe11["PRIVATE_ROUTE_CODE"] = tsmpe10["PRIVATE_ROUTE_CODE"];
			tsmpe11["PRIVATE_ROUTE_NAME"] = tsmpe10["PRIVATE_ROUTE_NAME"];
			tsmpe11["STACKING_PRINTS"] = 0;
			//tsmpe11["DELIVY_REMARK"] = delivy_remark;
			tsmpe11["STACKING_STATUS"] = "1";
			tsmpe11["ORDER_NO"] = tsmpe02["ORDER_NO"];
			tsmpe11["WT_MODE"] = tsmpe02["WT_MODE"];	// 计重方式
			tsmpe11["LOADING_NO"] = TICKET_NO;	// 装车单号
			//tsmpe11["DELIVY_PLAN_NO"] = tsmpe10["DELIVY_PLAN_NO"];
			tsmpe11["DELIVY_PLAN_TYPE"] = tsmpe10["DELIVY_PLAN_TYPE"];
			tsmpe11["TICKET_NO"] = TICKET_NO;
			tsmpe11["LOADING_SOLUTION_CODE"] = tsmpe02["LOADING_SOLUTION_CODE"];

			tsmpe11.TrimOrBlank();
			sqlstr = "insert into tsmpe11 where STACKING_NO = '" + tsmpe12["STACKING_NO"].ToString() + "' ";
			tsmpe11.Insert();
		}
		cmd_inq_loop.Close();



		// 铁运检查装车重量是否超载（三明）
		Log::Debug("", "", "TRNP_MODE_CODE=[{0}]", tsmpe10["TRNP_MODE_CODE"].ToString());
		Log::Debug("", "", "VEHICLE_NO=[{0}]", tsmpe02_in1["VEHICLE_NO"].ToString());
		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "2")	// 铁运
		{
			////// 读取车皮类型
			////CString VEHICLE_TYPE = "";
			////sqlstr = "select VEHICLE_TYPE from tsm00b4 where VEHICLE_NO = '" + tsmpe02_in1["VEHICLE_NO"].ToString() + "' ";
			////cmd_inq.SetCommandText(sqlstr);
			////Log::Debug("", "", "sqlstr={0}", sqlstr);
			////cmd_inq.ExecuteReader();
			////if (cmd_inq.Read())
			////{
			////	VEHICLE_TYPE = cmd_inq.GetString(1);
			////}
			////else
			////{
			////	CFormattable arguments[] = {  tsmpe02_in1["VEHICLE_NO"].ToString() };
			////	CMessageFormat::Format(s.msg, "检查是否超量时，没有读到车号【{0}】的车皮类型", arguments, 1);
			////	throw	CApplicationException(-1, s.msg, s.svc_name);
			////}
			////cmd_inq.Close();

			CDecimal plan_wt = 0, stacking_wt = 0;
			sqlstr = "SELECT SUM(STACKING_WT) FROM TSMPE11 WHERE TICKET_NO = '" + tsmpe11["TICKET_NO"].ToString() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				stacking_wt = cmd_inq.GetDecimal(1);
				if (VEHICLE_TYPE == "C70")	// 70吨车，否则算60吨
				{
					plan_wt = 70;
				}
				else
				{
					plan_wt = 60;
				}

				if (stacking_wt > plan_wt)
				{
					CFormattable arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString(), stacking_wt, VEHICLE_TYPE };
					CMessageFormat::Format(s.msg, "铁运装车重量【{0}】车型【{1}】超标准，计划号【{2}】", arguments, 3);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
			}
			else
			{
				CFormattable arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString(), tsmpe11["TICKET_NO"].ToString() };
				CMessageFormat::Format(s.msg, "检查是否超量时，没有读到计划量，计划号【{0}】装车单号【{1}】", arguments, 2);
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}
			cmd_inq.Close();
		}



		////// 调用仓库函数
		////if (bcls_stock_out.Tables[wm_block].Rows.get_Count() >0)
		////{
		////	doFlag = f_wm_stock(&bcls_stock_out, bcls_ret, conn);
		////	if (doFlag != 0)
		////	{
		////		throw CApplicationException(-1, s.msg, log.Location);
		////	}

		////}


		////// 更新作业单表状态 为完成
		////sqlstr = "update tsmpe10a set status = '5' where work_id in (select distinct work_id from tsmpe02 where TICKET_NO = '" + TICKET_NO + "' ) ";
		////cmd_upd.SetCommandText(sqlstr);
		////cmd_upd.Parameters.Set("TICKET_NO", TICKET_NO);
		////Log::Info("", "", "sqlstr = {0}", sqlstr);
		////cmd_upd.ExecuteNonQuery();


		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "2")
		{
			STATUS = "3";

			sqlstr = "update tsm00b4 set status = @STATUS,TICKET_NO = @TICKET_NO,TERMINAL_NAME = @TERMINAL_NAME "
				" WHERE VEHICLE_NO = @vehicle_no AND STATUS = '2' ";

			cmd_upd.SetCommandText(sqlstr);
			cmd_upd.Parameters.Set("vehicle_no", tsmpe02_in1["VEHICLE_NO"].ToString().Trim());
			cmd_upd.Parameters.Set("TICKET_NO", TICKET_NO);
			cmd_upd.Parameters.Set("TERMINAL_NAME", tsmpe10["DELIVY_PLACE_NAME"].ToString());
			cmd_upd.Parameters.Set("STATUS", STATUS);
			Log::Info("", "", "sqlstr = {0}", sqlstr);
			update_rows = cmd_upd.ExecuteNonQuery();
			if (update_rows == 0)
			{
				CFormattable arguments[] = { tsmpe02_in1["VEHICLE_NO"].ToString() };
				CMessageFormat::Format(s.msg, "无此车皮{0}信息或此车皮不为可用状态", arguments, 1);
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}
		}
		//liguangyuan 20230907 add 汽运的更新车辆的装车单已经车辆状态
		else if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "1")//汽运
		{
			STATUS = "3";

			sqlstr = "update tsm00b4 set status = @STATUS,TICKET_NO = @TICKET_NO,TERMINAL_NAME = @TERMINAL_NAME "
				" WHERE VEHICLE_NO = @vehicle_no AND STATUS = '2' and BILL_OF_LADING_NO=@BILL_OF_LADING_NO ";

			cmd_upd.SetCommandText(sqlstr);
			cmd_upd.Parameters.Set("vehicle_no", tsmpe02_in1["VEHICLE_NO"].ToString().Trim());
			cmd_upd.Parameters.Set("BILL_OF_LADING_NO", tsmpe11["BILL_OF_LADING_NO"].ToString());
			cmd_upd.Parameters.Set("TICKET_NO", TICKET_NO);
			cmd_upd.Parameters.Set("TERMINAL_NAME", tsmpe10["DELIVY_PLACE_NAME"].ToString());
			cmd_upd.Parameters.Set("STATUS", STATUS);
			Log::Info("", "", "sqlstr = {0}", sqlstr);
			update_rows = cmd_upd.ExecuteNonQuery();
		}


		// 调用装车重量检查函数
		EIClass bcls_rec_check;
		bcls_rec_check.Tables[0].Columns.Add(DT_STRING, "VEHICLE_NO");
		bcls_rec_check.Tables[0].Columns.Add(DT_STRING, "BILL_OF_LADING_NO");

		bcls_rec_check.Tables[0].Rows.Add();
		int ii = bcls_rec_check.Tables[0].Rows.get_Count() - 1;
		bcls_rec_check.Tables[0].Rows[ii]["VEHICLE_NO"] = tsmpe02_in1["VEHICLE_NO"].ToString();
		//bcls_rec_check.Tables[0].Rows[ii]["BILL_OF_LADING_NO"] = tsmpe02["BILL_OF_LADING_NO"].ToString();

		doFlag = f_sm00_load_wt_check(&bcls_rec_check, bcls_ret, conn);
		if (doFlag != 0)
		{
			throw CApplicationException(doFlag, s.msg, log.Location);
		}


		/*++++++++++++++++++++++++++++++++++*/
		/* 铁运调用销售服务检查货款			*/
		/************************************/
		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "2")
		{
			CDecimal load_wt = 0, load_num = 0, stacking_wt = 0, stacking_num = 0;

			EIClass bcls_rec_in, bcls_ret_out;
			bcls_rec_in.Tables[0].Columns.Add(DT_STRING, "OP_FLAG");	// 操作标记 0--装车确认。1--装车取消
			bcls_rec_in.Tables[0].Columns.Add(DT_STRING, "load_no");	// 装车单号
			bcls_rec_in.Tables[0].Columns.Add(DT_DECIMAL, "load_wt");	// 装车重量
			bcls_rec_in.Tables[0].Columns.Add(DT_DECIMAL, "load_num");	// 装车件数
			bcls_rec_in.Tables[0].Columns.Add(DT_STRING, "order_no");	// 合同号
			bcls_rec_in.Tables[0].Columns.Add(DT_DECIMAL, "stacking_wt");	// 合同装车重量
			bcls_rec_in.Tables[0].Columns.Add(DT_DECIMAL, "stacking_num");	// 合同装车件数
			bcls_rec_in.Tables[0].Columns.Add(DT_STRING, "remark");	// 备注

			sqlstr = "select ORDER_NO,sum(stacking_wt),sum(stacking_num) from tsmpe11 where TICKET_NO = '" + TICKET_NO + "' GROUP BY ORDER_NO";
			cmd_inq_loop.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq_loop.ExecuteReader();
			while (cmd_inq_loop.Read())
			{
				tsmpe11["ORDER_NO"] = cmd_inq_loop.GetString(1);
				stacking_wt = cmd_inq_loop.GetDecimal(2);
				stacking_num = cmd_inq_loop.GetDecimal(3);

				bcls_rec_in.Tables[0].Rows.Add();
				int ii = bcls_rec_in.Tables[0].Rows.get_Count() - 1;
				bcls_rec_in.Tables[0].Rows[ii]["order_no"] = tsmpe11["ORDER_NO"].ToString();
				bcls_rec_in.Tables[0].Rows[ii]["stacking_wt"] = stacking_wt;
				bcls_rec_in.Tables[0].Rows[ii]["stacking_num"] = stacking_num;
				bcls_rec_in.Tables[0].Rows[ii]["remark"] = " ";

				load_wt = load_wt + stacking_wt;
				load_num = load_num + stacking_num;
			}
			cmd_inq_loop.Close();

			bcls_rec_in.Tables[0].Rows[0]["OP_FLAG"] = "0";
			bcls_rec_in.Tables[0].Rows[0]["load_no"] = TICKET_NO;
			bcls_rec_in.Tables[0].Rows[0]["load_wt"] = load_wt;
			bcls_rec_in.Tables[0].Rows[0]["load_num"] = load_num;

			//// 在EX04小代码上配置调用程序及IP地址
			//iPlat4C::CRestClient restClient(conn, "S_J1_N1_01");
			//restClient.AddHttpHeader("Accept", "*");
			//restClient.AddHttpHeader("Content-Type", "application/json;charset=UTF-8");
			//restClient.Call2("S_J1_N1_01", &bcls_rec_in, &bcls_ret_out); //2.0版本使用Call2方法
			//ei_sys ei_outsys;
			//bcls_ret_out.GetSYS(&ei_outsys);
			//if (ei_outsys.flag < 0)
			//{
			//	Log::Trace("", "", "--ei_outsys.msg: [{0}]", ei_outsys.msg);
			//	throw CApplicationException(ei_outsys.msg);
			//}
			//if (bcls_ret_out.Tables[0].Rows[0]["OP_FLAG"].ToString() == "1")
			//{
			//	sprintf(s.msg, "货款检查没有通过!");
			//	throw	CApplicationException(-1, s.msg, s.svc_name);
			//}
		}

		/*++++++++++++++++++++++++++++++++++*/
		/* 铁运检查是否有承运标记			*/
		/************************************/
		CString CARRY_MARK = "1";	//1--承运
		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "2")
		{
			sqlstr = "select * from tsmpe15 where BILL_OF_LADING_NO = '" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (!cmd_inq.Read())
			{
				CARRY_MARK = "0";
			}
			cmd_inq.Close();

		}


		/********************************/
		/* 炼钢发货发送计量委托			*/
		/********************************/
		// 是炼钢板坯在厂内库实重交货的材料需要发送计量委托	
		CString MAT_KIND = "", WT_MODE = "", PONDER_NO = "";
		CString	STOCK_TYPE_CODE = "";	// 厂内外区分  0--厂内库，1--厂外库
		MAT_KIND = tsmpe11["MAT_KIND"].ToString();
		WT_MODE = tsmpe11["WT_MODE"].ToString();
		PONDER_NO = tsmpe11["PONDER_NO"].ToString();

		Log::Info("", "", "MAT_KIND={0},WT_MODE={1},PONDER_NO={2}", MAT_KIND, WT_MODE, PONDER_NO);

//liguangyuan 20230919注释
#ifdef SM_JLWT_PD
		if (MAT_KIND == "SM")	// 是炼钢板坯
		{
			EIClass	bcls_rec_jlwt;
			bcls_rec_jlwt.Tables[0].set_TableName("JLWT");
			bcls_rec_jlwt.Tables[0].Columns.Add(DT_STRING, "TICKET_NO");
			bcls_rec_jlwt.Tables[0].Columns.Add(DT_STRING, "OPER_FLAG");

			if (WT_MODE == "0")	// 实重交货
			{
				sqlstr = " SELECT  STOCK_TYPE_CODE FROM TSI0021 WHERE STOCK_NO = '" + stock_no + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Info("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					STOCK_TYPE_CODE = cmd_inq.GetString(1);
				}
				cmd_inq.Close();
				if (STOCK_TYPE_CODE == "0")	// 是厂内库
				{
					if (PONDER_NO.Trim() == "")	// 磅单号为空，表示还没有发送计划委托
					{
						// 发送计量委托
						bcls_rec_jlwt.Tables[0].Rows.Add();
						int ii = bcls_rec_jlwt.Tables[0].Rows.get_Count() - 1;
						bcls_rec_jlwt.Tables[0].Rows[ii]["TICKET_NO"] = TICKET_NO;
						bcls_rec_jlwt.Tables[0].Rows[ii]["OPER_FLAG"] = "I";

						//ret = f_sm00_xxjlwt_snd(&bcls_rec_jlwt, bcls_ret, conn);
						if (ret < 0)
						{
							throw	CApplicationException(-1, s.msg, log.Location);
						}
						strcpy(s.msg, "炼钢实重发货，已发送计量委托！");
						return 0;
					}
					else
					{
						CFormattable arguments[] = { TICKET_NO };
						CMessageFormat::Format(s.msg, "装车单{0}已经发送了计量委托！", arguments, 1);
						throw	CApplicationException(-1, s.msg, s.svc_name);
					}
				}
			}
		}
#endif
		// 2022-3-25




		/********************************************************************************
		*****	调用码单确认函数	*****
		********************************************************************************/
		if (CARRY_MARK == "1")
		{
			if (!bcls_rec->Tables.Contains("MD_OK"))
			{
				bcls_rec->Tables.Add("MD_OK");
			}

			bcls_rec->Tables["MD_OK"].Rows.Clear();
			bcls_rec->Tables["MD_OK"].Rows.Add();
			bcls_rec->Tables["MD_OK"].Columns.Add(DT_STRING, "TICKET_NO");
			bcls_rec->Tables["MD_OK"].Rows[0]["TICKET_NO"] = TICKET_NO;
			ret = 0;
			ret = f_sm00_md_ok(bcls_rec, bcls_ret, conn);
			if (ret != 0)
			{
				//EDLog	(1,1,"调用md_ok出错");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}

		bcls_ret->Tables[0].Columns.Add(DT_STRING, "TICKET_NO");
		bcls_ret->Tables[0].Rows.Add();
		bcls_ret->Tables[0].Rows[0]["TICKET_NO"] = TICKET_NO;

		sprintf(s.msg, stacking_no_r);
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	cmd_inq.Close();
	if (doFlag < 0)
	{
		CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		//CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0} :{1}", arguments, 2);
	}

	return doFlag;

}
