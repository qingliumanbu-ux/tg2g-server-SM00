/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      admin
Version:     1.0
Date:        2021-4-16 09:51:36
Description: 材料红冲入库
传入参数：	FLAG		标记		1--码单红冲、2--转库入库、3--转库出库、4--拒收入库
			MAT_NO		材料号
			STOCK_NO	入库库区
2020-8-22	013801	增加转库入库时，不是在PES准发的材料也可以转入，新增准发记录
2022-1-2	013801	转库入库时，库区号相同时跳过
**************************************************/

#include "stdafx.h"



BM2_FUNCTION_IMPORT
int f_sm00_record(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 写履历记录 */

BM2_FUNCTION_EXPORT


int f_sm00_stock_in(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);
	int doFlag = 0;
	CString sqlstr = " ";
	CString	blkname = "SM00_IN";	// 传入参数块名
	int fetchRowCount;	// 传入记录条数
	CString	mat_no;
	CString	mark;	// 1--出库，0--出库取消
	CString stock_no_out ="", stock_no_in="";
	CString	stock_type_code;	// 库类型，1--厂外库，0--厂内库
	CString sys_code_in = "", sys_code_out ="";
	CString	record_name = "sm00_record";
	CString	OLD_ORDER_NO = "";
	CString mat_kind="";
	CString c_shift = " ", c_group = " ";
	int	ret = 0;

	CModel tsmpe02("TSMPE02");
	CModel tsmpe12("TSMPE12");
	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_upd(conn);

	try
	{
		CString datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}


		fetchRowCount = bcls_rec->Tables[blkname].Rows.get_Count();
		if (fetchRowCount == 0)
		{
			sprintf(s.msg, "没有传入参数");
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		// 循环处理开始
		for (size_t i = 0; i < fetchRowCount; i++)
		{
			// 读取传入参数
			mat_no = bcls_rec->Tables[blkname].Rows[i]["MAT_NO"];
			//mat_kind = bcls_rec->Tables[blkname].Rows[i]["MAT_KIND"];
			//stock_no_out = bcls_rec->Tables[blkname].Rows[i]["STOCK_NO_OUT"];
			stock_no_in = bcls_rec->Tables[blkname].Rows[i]["STOCK_NO"];
			mark = bcls_rec->Tables[blkname].Rows[i]["FLAG"];


			// 打印传入的参数
			Log::Info("", "", "mat_no = {0} ", mat_no);
			//Log::Info("", "", "mat_kind = {0} ", mat_kind);
			//Log::Info("", "", "stock_no_out = {0} ", stock_no_out);
			Log::Info("", "", "stock_no_in = {0} ", stock_no_in);
			Log::Info("", "", "mark = {0} ", mark);


			// 数据检查
			if (mat_no.Trim() == "")
			{
				sprintf(s.msg, "传入的材料号不能为空");
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}
			//if (mat_kind.Trim() == "")
			//{
			//	sprintf(s.msg, "传入的物料种类不能为空");
			//	throw	CApplicationException(-1, s.msg, s.svc_name);
			//}
			if (mark != "1" && mark != "2" && mark != "3" && mark != "4")
			{
				sprintf(s.msg, "传入的操作类型不正确");
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			if (mark == "1")	// 码单红冲入库
			{
				stock_no_in = bcls_rec->Tables[blkname].Rows[i]["STOCK_NO"];
				Log::Info("", "", "stock_no_in = {0} ", stock_no_in);



				/* 根据库区读取分区 */
				CString SYS_CODE = "";
				sqlstr = "select TC_MARK from TSI0021 where stock_no = '" + stock_no_in.Trim() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					SYS_CODE = cmd_inq.GetString(1);
				}
				else
				{
					sprintf(s.msg, "无此[%s]库区代码！", (const char *)stock_no_in);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
				cmd_inq.Close();



				sqlstr = "select * from tsmpe02 WHERE MAT_NO = '" + mat_no.Trim() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					sprintf(s.msg, "材料%s还没有发货不能红冲", (const char *)mat_no);
					return 0;
				}
				cmd_inq.Close();


				sqlstr = " SELECT * FROM TSMPE12 WHERE MAT_NO = '" + mat_no.Trim() + "' "
					" order by REC_CREATE_TIME desc ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					cmd_inq.Fetch(tsmpe12);
					tsmpe02.CopyFrom(tsmpe12);
				}
				else
				{
					sprintf(s.msg, "此材料%s在本PES系统没有发货信息", (const char *)mat_no);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
				cmd_inq.Close();

				tsmpe02["REC_CREATOR"] = "00" + SYS_CODE + "02";

				tsmpe02["STOCK_NO"] = stock_no_in;
				tsmpe02["STACKING_NO"] = " ";
				tsmpe02["REC_CREATE_TIME"] = datetime;
				tsmpe02["BILL_OF_LADING_NO"] = " ";
				tsmpe02["VEHICLE_NO"] = " ";
				tsmpe02["VEHICLE_ID"] = " ";
				tsmpe02["CONFM_STATUS"] = "4";	// 准发确认
				tsmpe02["WORK_ID"] = " ";
				tsmpe02["FORCE_PASS_FLAG"] = "1";
				//tsmpe02.TC_FLAG = " ";
				//tsmpe02.STATUS = " ";
				tsmpe02["TICKET_NO"] = " ";
				tsmpe02.TrimOrBlank();
				tsmpe02.Insert();
				//tsmpe12.Delete();

			}   //

			if (mark == "2")	// 转库入库
			{
				stock_no_in = bcls_rec->Tables[blkname].Rows[i]["STOCK_NO"];
				mat_no = bcls_rec->Tables[blkname].Rows[i]["MAT_NO"];
				Log::Info("", "", "stock_no_in = {0} ", stock_no_in);
				Log::Info("", "", "mat_no = {0} ", mat_no);



				/* 根据库区读取分区 */
				CString SYS_CODE = "";
				sqlstr = "select TC_MARK from TSI0021 where stock_no = '" + stock_no_in.Trim() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					SYS_CODE = cmd_inq.GetString(1);
				}
				else
				{
					sprintf(s.msg, "无此[%s]库区代码！", (const char *)stock_no_in);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
				cmd_inq.Close();



				sqlstr = " SELECT * FROM TSMPE02 WHERE MAT_NO = '" + mat_no.Trim() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					cmd_inq.Fetch(tsmpe02);


					// 2022-1-2 down
					if (tsmpe02["STOCK_NO"].ToString() == stock_no_in)
					{
						continue;
					}
					// 2022-1-2 up


					if (tsmpe02["CONFM_STATUS"].ToString() != "9" && tsmpe02["CONFM_STATUS"].ToString() > "4")
					{
						sprintf(s.msg, "材料%s计划中，不能入库", (const char *)mat_no);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe02["CONFM_STATUS"].ToString() == "9")
					{
						tsmpe02["CONFM_STATUS"] = "4";
					}
					tsmpe02["STOCK_NO"] = stock_no_in;
					tsmpe02["BILL_OF_LADING_NO"] = " ";
					tsmpe02["VEHICLE_NO"] = " ";
					tsmpe02["WORK_ID"] = " ";
					tsmpe02["STACKING_NO"] = " ";
					tsmpe02["TICKET_NO"] = " ";
					//tsmpe02.TC_FLAG = " ";
					//tsmpe02.STATUS = " ";
					sqlstr = "update tsmpe02 set STOCK_NO ";
					if (tsmpe02.Update("STOCK_NO,CONFM_STATUS,BILL_OF_LADING_NO,VEHICLE_NO,WORK_ID,STACKING_NO,TICKET_NO", "MAT_NO") == 0)
					{
						sprintf(s.msg, "更新发货材料表上的库区异常！");
						throw	CApplicationException(-1, s.msg, s.svc_name);
					}
				}
				else
				{
					cmd_inq.Close();

					sqlstr = " SELECT * FROM TSMPE12 WHERE MAT_NO = '" + mat_no.Trim() + "' ";
					cmd_inq.SetCommandText(sqlstr);
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					cmd_inq.ExecuteReader();
					if (cmd_inq.Read())
					{
						cmd_inq.Fetch(tsmpe12);
						tsmpe02.CopyFrom(tsmpe12);

						tsmpe02["REC_CREATOR"] = "00" + SYS_CODE + "02";
						tsmpe02["STOCK_NO"] = stock_no_in;
						tsmpe02["CONFM_STATUS"] = "4";
						tsmpe02["BILL_OF_LADING_NO"] = " ";
						tsmpe02["VEHICLE_NO"] = " ";
						tsmpe02["WORK_ID"] = " ";
						tsmpe02["STACKING_NO"] = " ";
						tsmpe02["TICKET_NO"] = " ";
						//tsmpe02.TC_FLAG = " ";
						//tsmpe02.STATUS = " ";
						sqlstr = "insert tsmpe02 ";
						tsmpe02.TrimOrBlank();
						tsmpe02.Insert();
					}
					else
					{
						cmd_inq.Close();
						Log::Debug("", "", "此材料{0}在本PES系统没有发货信息,需要新增", mat_no);

						mat_kind = bcls_rec->Tables[blkname].Rows[i]["MAT_KIND"];
						Log::Info("", "", "mat_kind = {0} ", mat_kind);
						if (mat_kind.Trim() == "")
						{
							sprintf(s.msg, "传入的物料种类不能为空");
							throw	CApplicationException(-1, s.msg, s.svc_name);
						}

						CString table_name = "TMM" + mat_kind + "01";

						sqlstr = "select 'TMM' || @mat_kind || '01',factory_div from tsi0021 where stock_no = @stock_no";
						cmd_inq.SetCommandText(sqlstr);
						cmd_inq.Parameters.Set("stock_no", stock_no_in);
						cmd_inq.Parameters.Set("mat_kind", mat_kind);
						cmd_inq.ExecuteReader();
						if (cmd_inq.Read())
						{
							table_name = cmd_inq.GetString(1);
							tsmpe02["FACTORY_DIV"] = cmd_inq.GetString(2);
						}
						else
						{
							sprintf(s.msg, "无入库的库区代码[%s]", (const char *)stock_no_in);
							throw	CApplicationException(-1, s.msg, s.svc_name);
						}
						cmd_inq.Close();

						// 新增发货材料档记录
						sqlstr = "select * from " + table_name + " where mat_no = '" + mat_no.Trim() + "' ";
						cmd_inq.SetCommandText(sqlstr);
						Log::Debug("", "", "sqlstr={0}", sqlstr);
						cmd_inq.ExecuteReader();
						if (cmd_inq.Read())
						{
							cmd_inq.Fetch(tsmpe02);
						}
						else
						{
							sprintf(s.msg, "物料档无此材料[%s]", (const char *)mat_no);
							throw	CApplicationException(-1, s.msg, s.svc_name);
						}
						cmd_inq.Close();

						tsmpe02["REC_CREATOR"] = "00" + SYS_CODE + "02";
						tsmpe02["REC_CREATE_TIME"] = datetime;
						tsmpe02["REC_REVISE_TIME"] = " ";
						tsmpe02["REC_REVISOR"] = s.userid;
						tsmpe02["ARCHIVE_FLAG"] = "0";
						tsmpe02["MAT_NO"] = mat_no;
						tsmpe02["CONFM_STATUS"] = "4";
						tsmpe02["OLD_ORDER_NO"] = tsmpe02["ORDER_NO"];
						//tsmpe02["FACTORY_DIV"] = ct_factory_div;
						//tsmpe02["MAT_KIND"] = c_mat_kind;
						tsmpe02["STOCK_NO"] = stock_no_in;
						//tsmpe02["CONFM_PLAN_NO"] = c_confm_plan_no;
						//tsmpe02["READY_BILL_NO"] = c_ready_bill_no;
						tsmpe02["BILL_OF_LADING_NO"] = " ";
						tsmpe02["STACKING_NO"] = " ";
						tsmpe02["LEAVE_FACTORY_CARD"] = " ";
						tsmpe02["CONFM_TIME"] = datetime;
						tsmpe02["BILL_CONFM_DATE"] = " ";

						tsmpe02["CONFM_SHIFT"] = " ";
						tsmpe02["CONFM_GROUP"] = " ";
						// 调用函数生成班次、班组
						f_epep_get_shift_group("DEFAULT", datetime, c_shift, c_group , conn);
						tsmpe02["CONFM_SHIFT"] = c_shift;
						tsmpe02["CONFM_GROUP"] = c_group;


						tsmpe02["CONFM_MAKER"] = s.userid;
						tsmpe02["DELIVY_TIME"] = " ";
						tsmpe02["OUT_FACT_DATE"] = " ";
						tsmpe02["DELIVY_SHIFT"] = " ";
						tsmpe02["DELIVY_GROUP"] = " ";
						tsmpe02["DELIVY_MAKER"] = " ";
						tsmpe02["VEHICLE_NO"] = " ";

						//tsmpe02["MAT_WT"] = d_mat_wt;
						//tsmpe02["WT_MODE"] = tsmpe00.WT_METHOD_CODE;
						//tsmpe02["ORDER_NO"] = c_order_no;
						//tsmpe02["PROD_CODE"] = tsmpe00.PROD_CODE;
						//tsmpe02["PROD_CNAME"] = tsmpe00.PROD_CNAME;
						//tsmpe02["PROD_ENAME"] = tsmpe00.PROD_ENAME;

						// 根据品名代码读取产品名称
						sqlstr = " select code_desc_1_content , code_desc_2_content "
							" FROM	TEP0002 "
							" WHERE code_class = 'QM02' "
							" AND	CODE = @c_prod_code ";
						cmd_inq.SetCommandText(sqlstr);
						cmd_inq.Parameters.Clear();
						cmd_inq.Parameters.Set("c_prod_code", tsmpe02["PROD_CODE"].ToString());
						cmd_inq.ExecuteReader();
						if (cmd_inq.Read())
						{
							tsmpe02["PROD_CNAME"] = cmd_inq.GetString(1);
							tsmpe02["PROD_ENAME"] = cmd_inq.GetString(2);
						}
						cmd_inq.Close();



						//tsmpe02["SG_SIGN"] = c_sg_sign;
						tsmpe02["RED_FLAG"] = "0";
						tsmpe02["RED_CAUSE_CODE"] = " ";
						tsmpe02["RED_CAUSE_DESC"] = " ";
						tsmpe02["PRINT_NUM"] = 0;
						tsmpe02["CONFM_SCAN_MARK"] = " ";
						tsmpe02["DELIVY_SCAN_MARK"] = " ";
						//tsmpe02["MATCH_ERROR_SEQ"] =    ;

						tsmpe02["FORCE_PASS_FLAG"] = "1";
						tsmpe02["EXE_TIME_PLAN"] = " ";
						tsmpe02["DELIVY_QTY_FLAG"] = "0";	// 按量发货（捡配）标记 2013-08-14增加

						//tsmpe02["PONO"] = ct_pono;
						//tsmpe02["COMPLEX_DECIDE_CODE"] = ct_complex_decide_code;
						//tsmpe02["MAT_THICK"] = dt_mat_thick;
						//tsmpe02["MAT_WIDTH"] = dt_mat_width;
						//tsmpe02["MAT_LEN"] = dt_mat_len;
						//tsmpe02["MAT_THEORY_WT"] = dt_mat_theory_wt;
						//tsmpe02["MAT_GROSS_WT"] = dt_pack_mat_wt + tsmpe02["MAT_WT"].ToDecimal();		// 材料毛重
						//tsmpe02["MAT_ACT_WT"] = dt_mat_act_wt;

						tsmpe02["CUST_MAT_SPECS"] = CString::Format("%g", tsmpe02["MAT_THICK"].ToDecimal().ToDouble());
						if (tsmpe02["MAT_WIDTH"].ToDecimal() != 0)
						{
							tsmpe02["CUST_MAT_SPECS"] = tsmpe02["CUST_MAT_SPECS"].ToString() + "*" + CString::Format("%g", tsmpe02["MAT_WIDTH"].ToDecimal().ToDouble());
						}
						if (tsmpe02["MAT_LEN"].ToDecimal() != 0)
						{
							tsmpe02["CUST_MAT_SPECS"] = tsmpe02["CUST_MAT_SPECS"].ToString() + "*" + CString::Format("%d", tsmpe02["MAT_LEN"].ToDecimal().ToInt64());
						}

						Log::Trace("", __FUNCTION__, "CUST_MAT_SPECS=[{0}]", tsmpe02["CUST_MAT_SPECS"].ToString());

						//tsmpe02.TC_FLAG = " ";
						//tsmpe02.STATUS = " ";
						tsmpe02["TICKET_NO"] = " ";
						tsmpe02["WORK_ID"] = " ";


						sqlstr = "INSERT TSMPE02  MAT_NO = '" + tsmpe02["MAT_NO"].ToString() + "' ";
						tsmpe02.TrimOrBlank();

						if (!tsmpe02.Insert())
						{
							CFormattable arguments[] = { mat_no }; // 定义参数列表的数组
							CMessageFormat::Format(s.msg, "新增准发材料记录失败，材料号[{0}]", arguments, 1);
							throw CApplicationException(-1, s.msg, s.svc_name);
						}


					}
					cmd_inq.Close();
				}
			}

			//if (mark == "3")	// 转库出库
			//{
			//	stock_no_out = bcls_rec->Tables[blkname].Rows[i]["STOCK_NO_OUT"];
			//	Log::Info("", "", "stock_no_out = {0} ", stock_no_out);
			//	stock_no_in = bcls_rec->Tables[blkname].Rows[i]["STOCK_NO_IN"];
			//	Log::Info("", "", "stock_no_in = {0} ", stock_no_in);


			//	// 根据库区代码读取库类型，是厂外库时材料归档
			//	sqlstr = " SELECT stock_type_code,AREA_CODE FROM TSI0021 WHERE STOCK_NO = '" + stock_no_in.Trim() + "' ";
			//	cmd_inq.SetCommandText(sqlstr);
			//	Log::Debug("", "", "sqlstr={0}", sqlstr);
			//	cmd_inq.ExecuteReader();
			//	if (cmd_inq.Read())
			//	{
			//		stock_type_code = cmd_inq.GetString(1);
			//		sys_code_in = cmd_inq.GetString(2);
			//	}
			//	else
			//	{
			//		stock_type_code = "1";
			//	}
			//	cmd_inq.Close();


			//	sqlstr = " SELECT stock_type_code,AREA_CODE FROM TSI0021 WHERE STOCK_NO = '" + stock_no_out.Trim() + "' ";
			//	cmd_inq.SetCommandText(sqlstr);
			//	Log::Debug("", "", "sqlstr={0}", sqlstr);
			//	cmd_inq.ExecuteReader();
			//	if (cmd_inq.Read())
			//	{
			//		//stock_type_code = cmd_inq.GetString(1);
			//		sys_code_out = cmd_inq.GetString(2);
			//	}
			//	cmd_inq.Close();

			//	//是厂外库或主机代码不同时，归档
			//	if ( sys_code_in != sys_code_out)
			//	{
			//		tsmpe12.CopyFrom(tsmpe02);
			//		tsmpe12["STACKING_NO"] = datetime.SubstringNE(2, 10);
			//		tsmpe12["REC_CREATE_TIME"] = datetime;
			//		//tsmpe12["REC_REVISOR"] = s.userid;
			//		tsmpe12.TrimOrBlank();

			//		sqlstr = " INSERT INTO TSMPE12 ";
			//		tsmpe12.Insert();

			//		sqlstr = "DELETE FROM TSMPE02 WHERE MAT_NO = @mat_no ";
			//		cmd_inq.SetCommandText(sqlstr);
			//		cmd_inq.Parameters.Set("mat_no", mat_no);
			//		cmd_inq.ExecuteNonQuery();
			//	}
			//	else
			//	{
			//		// 更新材料库区代码为入库库区
			//		tsmpe02["STOCK_NO"] = stock_no_in;
			//		tsmpe02["MAT_NO"] = mat_no;
			//		tsmpe02.Update("STOCK_NO", "MAT_NO");
			//	}
			//}

			//if (mark == "4")	// 拒收入库
			//{
			//	stock_no_in = bcls_rec->Tables[blkname].Rows[i]["STOCK_NO"];
			//	mat_no = bcls_rec->Tables[blkname].Rows[i]["MAT_NO"];
			//	Log::Info("", "", "stock_no_in = {0} ", stock_no_in);
			//	Log::Info("", "", "mat_no = {0} ", mat_no);



			//	/* 根据库区读取分区 */
			//	CString SYS_CODE = "";
			//	sqlstr = "select sys_code from twm01 where stock_no = '" + stock_no_in.Trim() + "' ";
			//	cmd_inq.SetCommandText(sqlstr);
			//	cmd_inq.ExecuteReader();
			//	if (cmd_inq.Read())
			//	{
			//		SYS_CODE = cmd_inq.GetString(1);
			//	}
			//	else
			//	{
			//		sprintf(s.msg, "无此[%s]库区代码！",(const char *)stock_no_in);
			//		throw	CApplicationException(-1, s.msg, s.svc_name);
			//	}
			//	cmd_inq.Close();




			//	sqlstr = " SELECT * FROM TSMPE02 WHERE MAT_NO = '" + mat_no.Trim() + "' ";
			//	cmd_inq.SetCommandText(sqlstr);
			//	Log::Debug("", "", "sqlstr={0}", sqlstr);
			//	cmd_inq.ExecuteReader();
			//	if (cmd_inq.Read())
			//	{
			//		cmd_inq.Fetch(tsmpe02);

			//		tsmpe02["STOCK_NO"] = stock_no_in;
			//		//tsmpe02["CONFM_STATUS"] = "4";		// 2021-12-15
			//		//tsmpe02["BILL_OF_LADING_NO"] = " ";	// 2021-12-15
			//		tsmpe02["VEHICLE_NO"] = " ";
			//		tsmpe02["WORK_ID"] = " ";
			//		tsmpe02["STACKING_NO"] = " ";
			//		tsmpe02["TICKET_NO"] = " ";
			//		tsmpe02.TC_FLAG = " ";
			//		tsmpe02.STATUS = " ";
			//		sqlstr = "update tsmpe02 set STOCK_NO ";
			//		if (tsmpe02.Update("STOCK_NO,CONFM_STATUS,BILL_OF_LADING_NO,VEHICLE_NO,WORK_ID,STACKING_NO,TICKET_NO,TC_FLAG,STATUS", "MAT_NO") == 0)
			//		{
			//			sprintf(s.msg, "更新发货材料表上的库区异常！");
			//			throw	CApplicationException(-1, s.msg, s.svc_name);
			//		}
			//	}
			//	else
			//	{
			//		cmd_inq.Close();

			//		// 材料从码单材料表上读取，写到发货材料表上
			//		sqlstr = " select * from tsmpe12 WHERE MAT_NO = '" + mat_no.Trim() + "' order by REC_CREATE_TIME desc ";
			//		cmd_inq.SetCommandText(sqlstr);
			//		Log::Debug("", "", "sqlstr={0}", sqlstr);
			//		cmd_inq.ExecuteReader();
			//		if (cmd_inq.Read())
			//		{
			//			cmd_inq.Fetch(tsmpe12);
			//			tsmpe02.CopyFrom(tsmpe12);

			//			tsmpe02["REC_CREATE_TIME"] = datetime;
			//			tsmpe02["REC_CREATOR"] = "M1" + SYS_CODE + "02";

			//			tsmpe02["CONFM_STATUS"] = "4";
			//			tsmpe02["STOCK_NO"] = stock_no_in;
			//			tsmpe02["BILL_OF_LADING_NO"] = " ";
			//			tsmpe02["STACKING_NO"] = " ";
			//			tsmpe02["LEAVE_FACTORY_CARD"] = " ";

			//			tsmpe02["DELIVY_TIME"] = " ";
			//			tsmpe02["OUT_FACT_DATE"] = " ";
			//			tsmpe02["DELIVY_SHIFT"] = " ";
			//			tsmpe02["DELIVY_GROUP"] = " ";
			//			tsmpe02["DELIVY_MAKER"] = " ";
			//			tsmpe02["VEHICLE_NO"] = " ";
			//			tsmpe02["WORK_ID"] = " ";
			//			tsmpe02["TICKET_NO"] = " ";
			//			tsmpe02["FORCE_PASS_FLAG"] = "1";
			//			tsmpe02.TC_FLAG = " ";
			//			tsmpe02.STATUS = " ";

			//			sqlstr = "insert into tsmpe02 ";
			//			tsmpe02.Insert();

			//			//sqlstr = " delete from tsmpe12 ";
			//			//tsmpe12.Delete("MAT_NO,STACKING_NO");
			//		}
			//		else
			//		{
			//			sprintf(s.msg, "拒收失败：没有读取到材料%s 的出库记录", (const char *)mat_no);
			//			throw	CApplicationException(-1, s.msg, s.svc_name);
			//		}
			//		cmd_inq.Close();
			//	}
			//}

			bcls_rec->Tables[record_name].Rows[0]["mat_no"] = tsmpe02["MAT_NO"];
			bcls_rec->Tables[record_name].Rows[0]["userid"] = s.userid;
			if (mark == "1")
			{
				bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "6";	// 码单红冲
			}
			if (mark == "2")
			{
				bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "9";	// 转库入库
			}
			if (mark == "3")
			{
				bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "G";	// 转库出库
			}
			if (mark == "4")
			{
				bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "B";	// 拒收入库
			}

			ret = 0;
			ret = f_sm00_record(bcls_rec, bcls_ret, conn);
			if (ret < 0)
			{
				Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

		}
		sprintf(s.msg, "处理成功");
	}
	catch (CDbException& ex)
	{
		CFormattable arguments[] = { ex.GetCode(), ex.GetMsg() };
		CMessageFormat::Format(s.msg, "Database Error,sqlcode=[{0}],sqlmsg=[{1}]", arguments, 2);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;
	}
	catch (CApplicationException& ex)
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		strcpy(s.msg, ex.GetMsg());
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	if (doFlag < 0)
	{
		//CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, " {0} ：{1}", arguments, 2);
	}

	return doFlag;
}


