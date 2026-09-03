/************************************************
*	程序名称：产成品发货处理——码单生成		*
*	编制日期：2021-1-19   	                    *
*	编 制 人：13801					            *
*************************************************/
#include "stdafx.h"




//程序用头文件




//#include "tom01.h"


/* ***** 外部函数申明 ***** */
int	f_sm00_md_no(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);		/* 码单号生成 */
int	f_sm00_md_ok(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);		//码单确认
int f_sm00_record(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 写履历记录 */
char * f_sm00_lsh(int	lsh);										/* 产生出厂码单号流水号 */
int f_wm_stock(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 仓库出库函数
int f_xxj701_snd(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 发送计量委托电文

// service入口
BM2F_ENTERACE2(sm0008_pro,f_sm00_md_create)
/* -EP_SYSTEM_HEAD_END */
//int f_sm0008_pro(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
//{
//	CTracer log(__FUNCTION__);
//	/*程序用变量*/
//	int blkNum = 0;
//	int	ret = 0;
//	int	doFlag = 0;
//	int	fetchRowCount = 0;
//	int	fetchRowCount1 = 0;
//	int	blkSeq = 0;
//	int	i = 0;
//	//int	blkname1 = 1;
//	int	v_flag = 0;
//	//CString	blk_name = "md_ok";			/* 定义传入的块名 */
//	CString	str = "";
//	//CString	blkName = "smbw07_pro";
//	CString	record_name = "sm00_record";
//	CString	wm_block = "WM_STOCK";
//	CString	lsh;															/* 流水号细项 */
//
//	int i_count = 0;
//	CString c_factory_div = " ";
//	CString c_mat_no = " ";
//	CString c_proc_type = " ";
//	CString c_userid = " ";
//	CString c_stacking_no = " ";	// 装车清单号，码单号前9位
//	CString	stacking_no_r = "发货成功，码单号：";
//	CString	delivy_remark = " ";
//	CString	stacking_no = "";			/* 码单号 */
//	CString	c_bill_of_lading_no = "";
//	CString	c_vehicle_no = "";
//	CString	v_crane_cmd_yn = "";			/* 吊车命令标记 */
//	CString v_ponder_mark = "";	// 称重标记
//	CString	v_out_stock_code = "";
//	CString c_delivy_shift = " ", c_delivy_group = " ";
//	char  c_datetime[15];
//
//	CString	datetime = "";
//	CString	date = "";
//	CString	time = "";
//	CString	v_userid = "";
//	CDecimal v_wt = 1;
//	CString DELIVY_MAKER = " ";	// 发货人
//
//	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
//	date = datetime.Substring(0, 8);
//	time = datetime.Substring(8, 6);
//
//	CModel tsmpe00("TSMPE00");
//	CModel tsmpe02("TSMPE02");
//	CModel tsmpe02_in("TSMPE02");
//	CModel tsmpe02_in1("TSMPE02");
//	CModel tsmpe10("TSMPE10");
//	CModel tsmpe11("TSMPE11");
//	CModel tsmpe12("TSMPE12");
//	//CTOM01	tom01(conn);
//	CModel ted21("TED21");
//
//	CDbCommand cmd_inq(conn);
//	CDbCommand cmd_inq_loop(conn);
//	CDbCommand cmd_inq_loop2(conn);
//	CDbCommand cmd_upd(conn);
//	CString sqlstr;
//
//	try
//	{
//
//		/*获得传入参数*/
//		c_userid = s.userid;
//
//		EIClass ds_mat_no;
//		EIClass ds_mdno_rec, ds_mdno_ret;	//码单号生成块
//		ds_mdno_rec.Tables[0].Columns.Add(DT_STRING, "stock_no");
//		ds_mdno_rec.Tables[0].Rows.Add();
//
//		/* 仓库函数定义块 */
//		EIClass bcls_stock_out;
//		bcls_stock_out.Tables[0].set_TableName(wm_block);
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_NO");	// 材料号
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_OPER_ORDER");	// 库业务类型
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_NO");
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "TO_STOCK_PLACE_NO");	//目标库位号
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "ROWNO");
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "COLUMN_NO");
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "TO_LAYERNO");	// 目标层号
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_PLACE_POSITION");
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "VEHICLE_NO");
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_LINE_TYPE");	//产线类型
//		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_KIND");	// 物料类型
//		bcls_stock_out.Tables[0].Rows.Clear();
//
//		/********************************
//		*	读取传入的参数				*
//		********************************/
//		int rows = bcls_rec->Tables[0].Rows.get_Count();
//		if (rows == 0)
//		{
//			sprintf(s.msg, "没有传入装车的材料记录");
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//
//
//
//		// 读取传入的多记录数据
//		for (i = 0; i < rows; i++)
//		{
//			tsmpe02_in.MergeFrom(bcls_rec->Tables[0].Rows[i]);
//			if (tsmpe02_in["MAT_NO"].ToString().Trim() == "")
//			{
//				// 按作业单读取材料
//				if (tsmpe02_in["WORK_ID"].ToString().Trim() == "")
//				{
//					sprintf(s.msg, "传入的作业单不能为空.");
//					throw	CApplicationException(-1, s.msg, s.svc_name);
//				}
//
//				sqlstr = "select * from tsmpe02 t where t.work_id = @work_id ";
//				cmd_inq_loop.SetCommandText(sqlstr);
//				cmd_inq_loop.Parameters.Set("work_id", tsmpe02_in["WORK_ID"].ToString().Trim());
//				Log::Info("", "", "sqlstr = {0}", sqlstr);
//				cmd_inq_loop.ExecuteReader();
//				while (cmd_inq_loop.Read())
//				{
//					cmd_inq_loop.Fetch(tsmpe02);
//					tsmpe02.MergeTo(ds_mat_no.Tables[0], false);
//				}
//				cmd_inq_loop.Close();
//				continue;
//			}
//			else
//			{
//				tsmpe02_in.MergeTo(ds_mat_no.Tables[0], false);
//				tsmpe02["MAT_NO"] = tsmpe02_in["MAT_NO"];
//				tsmpe02["STOCK_NO"] = tsmpe02_in["STOCK_NO"];
//			}
//		}
//
//		CString LOADING_SOLUTION_CODE = "";	// 装载方案号
//		CString	STEEL_BRACKET_NO = "", STEEL_BRACKET_NO2 = "", STEEL_BRACKET_NO3 = "", STEEL_BRACKET_NO4 = "";	//钢架号
//		CString	DELIVY_SHIFT = "", DELIVY_GROUP = "", DELIVY_MAKER = "";
//		CString	VEHICLE_NO = "";
//		// 数据检查
//		tsmpe02_in1.MergeFrom(bcls_rec->Tables[1].Rows[0]);
//		if (tsmpe02_in1["DELIVY_SHIFT"].ToString().Trim() == "")
//		{
//			sprintf(s.msg, "班次不能为空");
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//		if (tsmpe02_in1["DELIVY_GROUP"].ToString().Trim() == "")
//		{
//			sprintf(s.msg, "班组不能为空");
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//		if (tsmpe02_in1["DELIVY_MAKER"].ToString().Trim() == "")
//		{
//			sprintf(s.msg, "发货工不能为空");
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//		if (tsmpe02_in1["VEHICLE_NO"].ToString().Trim() == "")
//		{
//			sprintf(s.msg, "车号不能为空");
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//
//		// 判是冷轧铁运时检查钢架号
//
//		// 按材料号读取提单号
//		sqlstr = "select BILL_OF_LADING_NO from tsmpe02 where mat_no = '" + tsmpe02["MAT_NO"].ToString().Trim() + "' ";
//		cmd_inq.SetCommandText(sqlstr);
//		Log::Info("", "", "sqlstr = {0}", sqlstr);
//		cmd_inq.ExecuteReader();
//		if (cmd_inq.Read())
//		{
//			tsmpe10["BILL_OF_LADING_NO"] = cmd_inq.GetString(1);
//		}
//		cmd_inq.Close();
//		Log::Info("", "", "BILL_OF_LADING_NO = {0}", tsmpe10["BILL_OF_LADING_NO"].ToString());
//
//		// 按提单号读取运输方式，物料种类
//		sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = @BILL_OF_LADING_NO ";
//		cmd_inq.SetCommandText(sqlstr);
//		cmd_inq.Parameters.Set("BILL_OF_LADING_NO", tsmpe10["BILL_OF_LADING_NO"].ToString());
//		Log::Info("", "", "sqlstr = {0}", sqlstr);
//		cmd_inq.ExecuteReader();
//		if (cmd_inq.Read())
//		{
//			cmd_inq.Fetch(tsmpe10);
//		}
//		cmd_inq.Close();
//
//		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(0, 2) == "22")	// 铁运
//		{
//			if (tsmpe10["MAT_KIND"].ToString() == "CR")	// 冷轧
//			{
//				if (tsmpe02_in1.STEEL_BRACKET_NO.Trim() == "")
//				{
//					sprintf(s.msg, "第一钢架号不能为空");
//					throw	CApplicationException(-1, s.msg, s.svc_name);
//				}
//
//			}
//		}
//
//
//		// 生成装车单号
//		CString stock_no = tsmpe02["STOCK_NO"];
//		int count = 0;
//		CString	TICKET_NO = "";	// 装车单
//
//		if (stock_no.Trim() == "")
//		{
//			sprintf(s.msg, "传人的仓库代码不能为空");
//			throw CApplicationException(-1, s.msg, log.Location);
//		}
//
//		//到流水号表按关键字读取记录
//		ted21["SEQ_NAME"] = "ZCD_" + stock_no;
//		count = ted21.QueryCount("SEQ_NAME");
//		if (count == 0)
//		{
//			//新增记录，按年复位
//			ted21["SEQ_DESC"] = "仓库" + stock_no + "装车单流水号";
//			ted21["SEQ_BEGIN"] = 1;
//			ted21["SEQ_NOW"] = 0;
//			ted21["SEQ_END"] = 99999;
//			ted21["SEQ_PRE"] = stock_no;	// 流水号前缀
//			ted21["SEQ_LEN"] = 5;
//			ted21["SEQ_RECYCLE_FLAG"] = "1";	// 按年复位
//			ted21["REC_CREATE_TIME"] = datetime;
//			ted21["REC_CREATOR"] = s.userid;
//			ted21.TrimOrBlank();
//			if (ted21.Insert() == false)
//			{
//				sprintf(s.msg, "新增流水号记录失败");
//				throw CApplicationException(-1, s.msg, log.Location);
//			}
//		}
//		TICKET_NO = stock_no + datetime.Substring(2, 2) + EPGetNextSeq(ted21["SEQ_NAME"].ToString(), conn);
//
//		Log::Trace("", __FUNCTION__, "生成的装车单[{0}]", TICKET_NO);
//
//
//		// 循环读取压入的材料，更新材料上的装车单号、车号、班次、班组、操作工，钢架号，装载方案号
//		rows = ds_mat_no.Tables[0].Rows.get_Count();
//		if (rows == 0)
//		{
//			sprintf(s.msg, "没有读取到材料记录");
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//
//		CString mat_no = "";
//		for (i = 0; i < rows; i++)
//		{
//			tsmpe02["MAT_NO"] = ds_mat_no.Tables[0].Rows[i]["MAT_NO"];
//			if (i == 0)
//			{
//				mat_no = "'" + tsmpe02["MAT_NO"].ToString().Trim() + "'";
//			}
//			else
//			{
//				mat_no = mat_no + ",'" + tsmpe02["MAT_NO"].ToString().Trim() + "'";
//			}
//		}
//
//		// 按车号读取车皮ID
//		CString	VEHICLE_ID = " ",VEHICLE_TYPE = " ";
//		sqlstr = "select vehicle_id ,VEHICLE_TYPE from tsm00b4 where vehicle_no = @vehicle_no and status = '2' ";
//		cmd_inq.SetCommandText(sqlstr);
//		cmd_inq.Parameters.Set("vehicle_no", tsmpe02_in1["VEHICLE_NO"].ToString());
//		Log::Info("", "", "sqlstr = {0}", sqlstr);
//		cmd_inq.ExecuteReader();
//		if (cmd_inq.Read())
//		{
//			VEHICLE_ID = cmd_inq.GetString(1);
//			VEHICLE_TYPE = cmd_inq.GetString(2);
//		}
//		cmd_inq.Close();
//
//
//		sqlstr = "UPDATE TSMPE02 SET TICKET_NO = @TICKET_NO,VEHICLE_NO = @VEHICLE_NO ,DELIVY_SHIFT=@DELIVY_SHIFT,DELIVY_GROUP=@DELIVY_GROUP,DELIVY_MAKER=@DELIVY_MAKER"
//			",STEEL_BRACKET_NO = @STEEL_BRACKET_NO,STEEL_BRACKET_NO2=@STEEL_BRACKET_NO2,STEEL_BRACKET_NO3=@STEEL_BRACKET_NO3,STEEL_BRACKET_NO4=@STEEL_BRACKET_NO4 "
//			",VEHICLE_ID = @VEHICLE_ID ,TRNP_MODE_CODE = @TRNP_MODE_CODE "
//			",DELIVY_TIME = @DELIVY_TIME "
//			",STATUS = '0' "
//			" WHERE MAT_NO IN (" + mat_no + ") ";
//		cmd_upd.SetCommandText(sqlstr);
//		cmd_upd.Parameters.Set("TICKET_NO", TICKET_NO);
//		cmd_upd.Parameters.Set("VEHICLE_NO", tsmpe02_in1["VEHICLE_NO"].ToString());
//		cmd_upd.Parameters.Set("VEHICLE_ID", VEHICLE_ID.TrimOrBlank());
//		cmd_upd.Parameters.Set("DELIVY_SHIFT", tsmpe02_in1["DELIVY_SHIFT"].ToString());
//		cmd_upd.Parameters.Set("DELIVY_GROUP", tsmpe02_in1["DELIVY_GROUP"].ToString());
//		cmd_upd.Parameters.Set("DELIVY_MAKER", tsmpe02_in1["DELIVY_MAKER"].ToString());
//		cmd_upd.Parameters.Set("STEEL_BRACKET_NO", tsmpe02_in1.STEEL_BRACKET_NO.TrimOrBlank());
//		cmd_upd.Parameters.Set("STEEL_BRACKET_NO2", tsmpe02_in1.STEEL_BRACKET_NO2.TrimOrBlank());
//		cmd_upd.Parameters.Set("STEEL_BRACKET_NO3", tsmpe02_in1.STEEL_BRACKET_NO3.TrimOrBlank());
//		cmd_upd.Parameters.Set("STEEL_BRACKET_NO4", tsmpe02_in1.STEEL_BRACKET_NO4.TrimOrBlank());
//		cmd_upd.Parameters.Set("TRNP_MODE_CODE", tsmpe10["TRNP_MODE_CODE"].ToString().TrimOrBlank());
//		cmd_upd.Parameters.Set("DELIVY_TIME", datetime);
//		Log::Info("", "", "sqlstr = {0}", sqlstr);
//		int update_rows;
//		update_rows = cmd_upd.ExecuteNonQuery();
//		if (update_rows != rows)
//		{
//			CFormattable arguments[] = { update_rows, rows };
//			CMessageFormat::Format(s.msg, "更新材料数 {0} 和实际的材料数 {1} 不符", arguments, 2);
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//
//
//
//		sqlstr = " select BILL_OF_LADING_NO,ORDER_NO,sum(mat_wt),count(1),sum(MAT_GROSS_WT),sum(MAT_TUBE) from tsmpe02 "
//			" where TICKET_NO = @TICKET_NO group by BILL_OF_LADING_NO,ORDER_NO ";
//		cmd_inq_loop.SetCommandText(sqlstr);
//		cmd_inq_loop.Parameters.Set("TICKET_NO", TICKET_NO);
//		Log::Info("", "", "sqlstr = {0}", sqlstr);
//		cmd_inq_loop.ExecuteReader();
//		while (cmd_inq_loop.Read())
//		{
//			tsmpe11["BILL_OF_LADING_NO"] = cmd_inq_loop.GetString(1);
//			tsmpe11["ORDER_NO"] = cmd_inq_loop.GetString(2);
//			tsmpe11["STACKING_WT"] = cmd_inq_loop.GetDecimal(3);
//			tsmpe11["STACKING_NUM"] = cmd_inq_loop.GetInt32(4);
//			tsmpe11["STACKING_GROSS_WT"] = cmd_inq_loop.GetDecimal(5);
//			tsmpe11["STACKING_TUBE"] = cmd_inq_loop.GetInt32(6);
//
//
//			// 按库区代码读取仓库的行车标记
//			sqlstr = "select crane_mark,ponder_mark from tsi0021 where stock_no = @STOCK_NO ";
//			cmd_inq.SetCommandText(sqlstr);
//			cmd_inq.Parameters.Set("STOCK_NO", stock_no);
//			Log::Info("", "", "sqlstr = {0}", sqlstr);
//			cmd_inq.ExecuteReader();
//			if (cmd_inq.Read())
//			{
//				v_crane_cmd_yn = cmd_inq.GetString(1);
//				v_ponder_mark = cmd_inq.GetString(2);
//			}
//			else
//			{
//				sprintf(s.msg, "读取库区代码表TSI0021上行车标记出错，库区号=%s", (const char *)tsmpe02["STOCK_NO"].ToString());
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//			cmd_inq.Close();
//
//
//
//			//// 按合同号读取合同信息
//			//sqlstr = " select * from tom01 where order_no = @order_no ";
//			//cmd_inq.SetCommandText(sqlstr);
//			//cmd_inq.Parameters.Set("order_no", tsmpe11["ORDER_NO"].ToString());
//			//Log::Info("", "", "sqlstr = {0}", sqlstr);
//			//cmd_inq.ExecuteReader();
//			//if (cmd_inq.Read())
//			//{
//			//	cmd_inq.Fetch(tom01);
//			//}
//			//cmd_inq.Close();
//
//
//			// 按合同号读取合同信息
//			sqlstr = " select * from tsmpe00 where order_no = @order_no ORDER BY REC_CREATE_TIME DESC ";
//			cmd_inq.SetCommandText(sqlstr);
//			cmd_inq.Parameters.Set("order_no", tsmpe11["ORDER_NO"].ToString());
//			Log::Info("", "", "sqlstr = {0}", sqlstr);
//			cmd_inq.ExecuteReader();
//			if (cmd_inq.Read())
//			{
//				cmd_inq.Fetch(tsmpe00);
//			}
//			cmd_inq.Close();
//
//
//
//			//调用函数码单号生成
//			ds_mdno_rec.Tables[0].Rows[0]["stock_no"] = stock_no;
//			doFlag = f_sm00_md_no(&ds_mdno_rec, &ds_mdno_ret, conn);
//			if (doFlag < 0)
//			{
//				throw CApplicationException(-1, s.msg, s.svc_name);
//			}
//			stacking_no = ds_mdno_ret.Tables[0].Rows[0]["stacking_no"];
//			EDLog(1, 1, "生成码单号 stacking_no = [%s]", (const char*)stacking_no);
//
//
//
//			// 读取材料记录写码单材料表
//			sqlstr = "select * from tsmpe02 where BILL_OF_LADING_NO = @BILL_OF_LADING_NO AND ORDER_NO = @ORDER_NO AND TICKET_NO = @TICKET_NO ";
//			cmd_inq_loop2.SetCommandText(sqlstr);
//			cmd_inq_loop2.Parameters.Set("BILL_OF_LADING_NO", tsmpe11["BILL_OF_LADING_NO"].ToString());
//			cmd_inq_loop2.Parameters.Set("ORDER_NO", tsmpe11["ORDER_NO"].ToString());
//			cmd_inq_loop2.Parameters.Set("TICKET_NO", TICKET_NO);
//			Log::Info("", "", "sqlstr = {0}", sqlstr);
//			cmd_inq_loop2.ExecuteReader();
//			while (cmd_inq_loop2.Read())
//			{
//				cmd_inq_loop2.Fetch(tsmpe02);
//				tsmpe12.CopyFrom(tsmpe02);
//				tsmpe12["REC_CREATE_TIME"] = datetime;
//				//tsmpe12["REC_CREATOR"] = s.userid;
//				tsmpe12["STACKING_NO"] = stacking_no;
//
//
//				if (v_crane_cmd_yn == "1")	// 有吊车系统
//				{
//					if (tsmpe02["OUT_MARK"].ToString() != "2")
//					{
//						CFormattable arguments[] = { tsmpe02["STOCK_NO"].ToString(), tsmpe02["MAT_NO"].ToString() };
//						CMessageFormat::Format(s.msg, "此库区{0}是有行车系统，请先在仓库做材料出库，材料号{1}", arguments, 2);
//						throw	CApplicationException(-1, s.msg, s.svc_name);
//					}
//				}
//				else
//				{
//					bcls_stock_out.Tables[wm_block].Rows.Add();
//					int ii = bcls_stock_out.Tables[wm_block].Rows.get_Count() - 1;
//					bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_NO"] = tsmpe12["MAT_NO"];
//					bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_OPER_ORDER"] = "2E";
//					bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_NO"] = " ";
//					bcls_stock_out.Tables[wm_block].Rows[ii]["TO_STOCK_PLACE_NO"] = " ";
//					bcls_stock_out.Tables[wm_block].Rows[ii]["ROWNO"] = " ";
//					bcls_stock_out.Tables[wm_block].Rows[ii]["COLUMN_NO"] = " ";
//					bcls_stock_out.Tables[wm_block].Rows[ii]["TO_LAYERNO"] = 0;
//					bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_PLACE_POSITION"] = " ";
//					bcls_stock_out.Tables[wm_block].Rows[ii]["VEHICLE_NO"] = tsmpe12["VEHICLE_NO"];  //车号 如果有就传
//					bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_LINE_TYPE"] = tsmpe12["MAT_KIND"]; //取物料主档
//					bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_KIND"] = tsmpe12["MAT_KIND"]; //取物料主档
//
//				}
//
//				if (tsmpe02["VEHICLE_NO"].ToString().Trim() == "")
//				{
//					CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() };
//					CMessageFormat::Format(s.msg, "此材料{0}的车牌号不能为空", arguments, 1);
//					throw	CApplicationException(-1, s.msg, s.svc_name);
//				}
//
//
//				tsmpe12.TrimOrBlank();
//				sqlstr = "insert into tsmpe12 where STACKING_NO = '" + tsmpe12["STACKING_NO"].ToString() + "' and mat_no = '" + tsmpe12["MAT_NO"].ToString() + "' ";
//				tsmpe12.Insert();
//
//			}
//			cmd_inq_loop2.Close();
//
//
//			tsmpe11["REC_CREATE_TIME"] = datetime;
//			tsmpe11["REC_CREATOR"] = c_userid;
//			tsmpe11["REC_REVISE_TIME"] = " ";
//			tsmpe11["REC_REVISOR"] = " ";
//			tsmpe11["REC_ERASE_TIME"] = " ";
//			tsmpe11["REC_ERASOR"] = " ";
//			tsmpe11["ARCHIVE_FLAG"] = " ";
//			tsmpe11["COMPANY_CODE"] = tsmpe10["COMPANY_CODE"];
//			tsmpe11["STACKING_NO"] = stacking_no;
//			tsmpe11["FACTORY_DIV"] = tsmpe02["FACTORY_DIV"];
//			tsmpe11["MAT_KIND"] = tsmpe02["MAT_KIND"];
//			tsmpe11["STOCK_NO"] = tsmpe10["STOCK_NO"];
//			//tsmpe11["BILL_OF_LADING_NO"] = tsmpe02["BILL_OF_LADING_NO"];
//			tsmpe11["CONFM_PLAN_NO"] = " ";
//			tsmpe11["READY_BILL_NO"] = " ";
//			//tsmpe11["STACKING_WT"] = stacking_wt;
//			//tsmpe11["STACKING_GROSS_WT"] = stacking_wt;
//			//tsmpe11["STACKING_NUM"] = stacking_num;
//			//tsmpe11["STACKING_TUBE"] = stacking_tube;
//			tsmpe11["VEHICLE_NO"] = tsmpe12["VEHICLE_NO"];
//			//tsmpe11["ORDER_NO"] = tsmpe00["ORDER_NO"];
//			//tsmpe11["EXPORT_FLAG"] = tom01.EXPORT_FLAG;
//			//tsmpe11["SG_SIGN"] = tom01.SG_SIGN;
//			tsmpe11["SG_SIGN"] = tsmpe02["SG_SIGN"];
//			//tsmpe11["SG_STD"] = tsmpe02.SG_STD;
//			//tsmpe11["PROD_CODE"] = tom01.PROD_CODE;
//			tsmpe11["PROD_CODE"] = tsmpe02["PROD_CODE"];
//			//tsmpe11["PROD_CNAME"] = tom01.PROD_CNAME;
//			tsmpe11["PROD_CNAME"] = tsmpe02["PROD_CNAME"];
//			tsmpe11["PROD_ENAME"] = tsmpe02["PROD_ENAME"];
//			tsmpe11["ORDER_THICK"] = tsmpe00["ORDER_THICK"];
//			tsmpe11["ORDER_WIDTH"] = tsmpe00["ORDER_WIDTH"];
//			tsmpe11["ORDER_LEN"] = tsmpe00["ORDER_LEN"];
//			tsmpe11["ORDER_LEN_MIN"] = tsmpe00["ORDER_MIN_LEN"];
//			tsmpe11["ORDER_LEN_MAX"] = tsmpe00["ORDER_MAX_LEN"];
//			tsmpe11["DELIVY_TIME"] = datetime;
//			tsmpe11["DELIVY_GROUP"] = tsmpe02_in1["DELIVY_GROUP"];
//			tsmpe11["DELIVY_SHIFT"] = tsmpe02_in1["DELIVY_SHIFT"];
//			tsmpe11["DELIVY_MAKER"] = tsmpe02_in1["DELIVY_MAKER"];
//			tsmpe11["TRNP_MODE_CODE"] = tsmpe10["TRNP_MODE_CODE"];
//			tsmpe11["CONSIGNE_NAME"] = tsmpe10["CONSIGNE_NAME"];
//			tsmpe11["ORDER_CUST_CNAME"] = tsmpe00["ORDER_CUST_CNAME"];
//
//			tsmpe11["BALANCE_USER_NAME"] = tsmpe10["BALANCE_USER_NAME"];	//结算用户
//			tsmpe11["CONVEY_UNIT_NAME"] = tsmpe10["CONVEY_UNIT_NAME"];
//			tsmpe11["DELIVY_PLACE_NAME"] = tsmpe10["DELIVY_PLACE_NAME"];
//			//tsmpe11["PRIVATE_ROUTE_CODE"] = tsmpe10["PRIVATE_ROUTE_CODE"];
//			tsmpe11["PRIVATE_ROUTE_NAME"] = tsmpe10["PRIVATE_ROUTE_NAME"];
//			tsmpe11["STACKING_PRINTS"] = 0;
//			//tsmpe11["DELIVY_REMARK"] = delivy_remark;
//			tsmpe11["STACKING_STATUS"] = "1";
//			tsmpe11["ORDER_NO"] = tsmpe02["ORDER_NO"];
//			tsmpe11["WT_MODE"] = tsmpe02["WT_MODE"];	// 计重方式
//			tsmpe11["LOADING_NO"] = c_stacking_no;	// 装车单号
//			//tsmpe11.DELIVY_PLAN_NO = tsmpe10["DELIVY_PLAN_NO"];
//			tsmpe11["DELIVY_PLAN_TYPE"] = tsmpe10["DELIVY_PLAN_TYPE"];
//			tsmpe11["TICKET_NO"] = TICKET_NO;
//			tsmpe11.STEEL_BRACKET_NO = tsmpe02.STEEL_BRACKET_NO;
//			tsmpe11.STEEL_BRACKET_NO2 = tsmpe02.STEEL_BRACKET_NO2;
//			tsmpe11.STEEL_BRACKET_NO3 = tsmpe02.STEEL_BRACKET_NO3;
//			tsmpe11.STEEL_BRACKET_NO4 = tsmpe02.STEEL_BRACKET_NO4;
//			tsmpe11["LOADING_SOLUTION_CODE"] = tsmpe02["LOADING_SOLUTION_CODE"];
//			tsmpe11.STEEL_SERVICE_ID = tsmpe02.STEEL_SERVICE_ID;
//			tsmpe11.COVER_SERVICE_ID = tsmpe02.COVER_SERVICE_ID;
//			tsmpe11.MAT_SERVICE_ID = tsmpe02.MAT_SERVICE_ID;
//			tsmpe11.VEHICLE_TYPE = VEHICLE_TYPE;
//
//			tsmpe11.TrimOrBlank();
//			sqlstr = "insert into tsmpe11 where STACKING_NO = '" + tsmpe12["STACKING_NO"].ToString() + "' ";
//			tsmpe11.Insert();
//		}
//		cmd_inq_loop.Close();
//
//
//		// 调用仓库函数
//		if (bcls_stock_out.Tables[wm_block].Rows.get_Count() >0)
//		{
////			doFlag = f_wm_stock(&bcls_stock_out, bcls_ret, conn);
//			if (doFlag != 0)
//			{
//				throw CApplicationException(-1, s.msg, log.Location);
//			}
//
//		}
//
//
//		// 更新作业单表状态 为完成
//		sqlstr = "update tsmpe10a set status = '5' where work_id in (select distinct work_id from tsmpe02 where TICKET_NO = '" + TICKET_NO + "' ) ";
//		cmd_upd.SetCommandText(sqlstr);
//		cmd_upd.Parameters.Set("TICKET_NO", TICKET_NO);
//		Log::Info("", "", "sqlstr = {0}", sqlstr);
//		cmd_upd.ExecuteNonQuery();
//
//
//		// 更新车皮为已使用
//		sqlstr = "update tsm00b4 set status = '3',TICKET_NO = @TICKET_NO,TERMINAL_NAME = @TERMINAL_NAME "
//			"WHERE VEHICLE_NO = @vehicle_no";
//		cmd_upd.SetCommandText(sqlstr);
//		cmd_upd.Parameters.Set("vehicle_no", tsmpe02_in1["VEHICLE_NO"].ToString());
//		cmd_upd.Parameters.Set("TICKET_NO", TICKET_NO);
//		cmd_upd.Parameters.Set("TERMINAL_NAME", tsmpe10["DELIVY_PLACE_NAME"].ToString());
//		Log::Info("", "", "sqlstr = {0}", sqlstr);
//		update_rows = cmd_upd.ExecuteNonQuery();
//
//
//
//		if (!bcls_rec->Tables.Contains("MD_OK"))
//		{
//			bcls_rec->Tables.Add("MD_OK");
//		}
//
//
//		// 判运输方式是汽运还是铁运
//		if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) != "2")	// 铁运
//		{
//			// 判是否自动码单确认
//			if (v_ponder_mark != "1")	// 0--不称重，1--称重
//			{
//				bcls_rec->Tables["MD_OK"].Rows.Clear();
//				bcls_rec->Tables["MD_OK"].Rows.Add();
//				bcls_rec->Tables["MD_OK"].Columns.Add(DT_STRING, "TICKET_NO");
//				bcls_rec->Tables["MD_OK"].Rows[0]["TICKET_NO"] = TICKET_NO;
//				/********************************************************************************
//				*****	调用码单确认函数	*****
//				********************************************************************************/
//				//EDLog(1,1,"★★★★★调用函数 f_smbw_md_ok 开始★★★★★出厂★★★");
//				//EDLog	(1,1,"装车单号=[%s]"	, (const char*)c_stacking_no );
//				bcls_rec->SetSYS(s);
//				ret = 0;
//				ret = f_sm00_md_ok(bcls_rec, bcls_ret, conn);
//				if (ret != 0)
//				{
//					//EDLog	(1,1,"调用md_ok出错");
//					throw CApplicationException(-1, s.msg, s.svc_name);
//				}
//			}
//			else
//			{
//				// 发送计量委托电文
//				if (!bcls_rec->Tables.Contains("JLWT"))
//				{
//					bcls_rec->Tables.Add("JLWT");
//				}
//
//				bcls_rec->Tables["JLWT"].Rows.Clear();
//				bcls_rec->Tables["JLWT"].Rows.Add();
//				bcls_rec->Tables["JLWT"].Columns.Add(DT_STRING, "TICKET_NO");
//				bcls_rec->Tables["JLWT"].Columns.Add(DT_STRING, "TC_NO");
//				bcls_rec->Tables["JLWT"].Rows[0]["TICKET_NO"] = TICKET_NO;
//				bcls_rec->Tables["JLWT"].Rows[0]["TC_NO"] = "PDJ701";
//				/********************************************************************************
//				*****	发送计量委托函数	*****
//				********************************************************************************/
//				bcls_rec->SetSYS(s);
//				ret = 0;
//				//ret = f_xxj701_snd(bcls_rec, bcls_ret, conn);
//				if (ret != 0)
//				{
//					throw CApplicationException(-1, s.msg, s.svc_name);
//				}
//			}
//		}
//
//
//
//		bcls_ret->Tables[0].Columns.Add(DT_STRING, "TICKET_NO");
//		bcls_ret->Tables[0].Rows.Add();
//		bcls_ret->Tables[0].Rows[0]["TICKET_NO"] = TICKET_NO;
//
//		sprintf(s.msg, stacking_no_r);
//	}
//	catch (CDbException& ex)  //捕获数据库操作异常
//	{
//		CFormattable arguments[] = { ex.GetCode() };
//		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
//		CString str = sqlstr + "\r\n" + ex.GetMsg();
//		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
//		__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
//		s.flag = -1;
//		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
//	}
//	catch (CApplicationException& ex)  //捕获应用错误
//	{
//		s.flag = ex.GetCode();
//		doFlag = -1;
//	}
//	catch (CException& ex)
//	{
//		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
//		s.flag = ex.GetCode();
//		doFlag = -1;
//	}
//	cmd_inq.Close();
//	if (doFlag < 0)
//	{
//		CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
//		//CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
//		CMessageFormat::Format(s.msg, "{0} :{1}", arguments, 2);
//	}
//
//	return doFlag;
//
//}
