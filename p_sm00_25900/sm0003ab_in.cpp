#include "stdafx.h"
#include "epex.h"






//外部函数声明
BM2_FUNCTION_IMPORT
int f_sm00_record(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);/* 写履历记录 */
BM2_FUNCTION_IMPORT
int f_mm0099(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
//int f_sm00_stock_in(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);//入库
//int f_xxsm05_snd(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
int	f_sm00_md_no(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);/* 码单号生成 */
#ifdef _LINE_SM
int f_wmsmsm_stock_in(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);	// 仓库入库函数
#endif
#ifdef _LINE_BW
int f_wmbwbw_stock_in(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);	// 仓库入库函数
#endif
#ifdef _LINE_HP
int f_wmhphp_stock_in(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);	// 仓库入库函数
#endif
#ifdef _LINE_CR
int f_wmcrcr_stock_in(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);	// 仓库入库函数
#endif
#ifdef _LINE_HR
int f_wmhrhr_stock_in(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);	// 仓库入库函数
#endif
// service入口
BM2F_ENTERACE(sm0003ab_in)

int f_sm0003ab_in(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{

	CTracer log(__FUNCTION__);

	/* 程序内部变量 */
	int   doFlag = 0;
	int   fetchRowCount = 0;
	int   outBlockRow = 0;
	int   ren = 0;
	int   rows;
	int   blkNum = 0;
	int	  ret = 0;

	CString bill_of_lading_no = "";
	CString bill_of_lading_detailno = "";
	CString delivy_plan_type = "";
	CString c_stacking_no = "";
	CString old_stacking_no = "";
	CString	c_mat_kind = "";
	CString	c_factory_div = "";

	CDecimal	stacking_wt = 0;                                      /* 净重 */
	CDecimal	stacking_gross_wt = 0;                                /* 毛重 */
	CDecimal    stacking_discrep_wt = 0;							  /* 磅差 */
	CDecimal	stacking_num = 0;                                     /* 捆数 */
	CDecimal	stacking_tube = 0;									  /* 支数 */

	CString	record_name = "sm00_record";
	/* 业务变量 */
	CDecimal v_count = 0;

	CString	v_userid = s.userid;
	CString	datetime = "";
	CString	date = "";
	CString	time = "";

	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	date = datetime.Substring(0, 8);
	time = datetime.Substring(8, 6);

	/* 实体类定义 */
	CModel tsm0012("TSM0012");
	CModel tsm0013("TSM0013");
	CModel tsmpe12("TSMPE12");
	CModel tsmpe12_upd("TSMPE12");
	CModel tsmpe02("TSMPE02");
	CModel tsmpe11("TSMPE11");
	CModel tsmpe10("TSMPE10");
	// 数据库SQL操作字符串
	CString sqlstr("");
	CDbCommand cmd(conn);

	CDbCommand cmd_tmmxx01(conn);
	CDataTable tmmxx01;

	/* 数据库操作类定义 */

	EIClass bcls_mm0099, bcls_mm0099_ret;;
	bcls_mm0099.Tables[0].set_TableName("MM0099");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "EVENT_ID");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "EVENT_LINE_TYPE");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "SYSTEM_ID");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "FUNC_ID");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "MAT_KIND");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "MAT_NO");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "STOCK_NO");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "IN_STOCK_TIME");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "STOCK_PLACE_NO");
	bcls_mm0099.Tables[0].Columns.Add(DT_DECIMAL, "LAYERNO");
	bcls_mm0099.Tables[0].Columns.Add(DT_STRING, "IN_FLAG");
	//人工修改重量
	EIClass bcls_mm03;
	bcls_mm03.Tables[0].set_TableName("MM0099");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "EVENT_ID");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "EVENT_LINE_TYPE");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "SYSTEM_ID");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "FUNC_ID");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_KIND");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_NO");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_THICK");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_WIDTH");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_LEN");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_ACT_THICK");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_ACT_WIDTH");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_ACT_LEN");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_ACT_WT");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_THEORY_WT");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MEASURE_WT_FLAG");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_NUM");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "FIX_FLAG");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "SHORT_FLAG");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_ACT_INNER_DIA");
	bcls_mm03.Tables[0].Columns.Add(DT_STRING, "MAT_WT");

	EIClass bcls_sm_snd;
	//bcls_sm_snd.Tables[0].set_TableName();
	bcls_sm_snd.Tables[0].Columns.Add(DT_STRING, "FLAG");
	bcls_sm_snd.Tables[0].Columns.Add(DT_STRING, "STACKING_NO");
	bcls_sm_snd.Tables[0].Columns.Add(DT_STRING, "BILL_OF_LADING_NO");
	bcls_sm_snd.Tables[0].Columns.Add(DT_STRING, "STOCK_NO");
	bcls_sm_snd.Tables[0].Columns.Add(DT_STRING, "MAT_NO");
	bcls_sm_snd.Tables[0].Columns.Add(DT_STRING, "OLD_STACKING_NO");
	bcls_sm_snd.Tables[0].Columns.Add(DT_STRING, "MAT_KIND");
	bcls_sm_snd.Tables[0].Columns.Add(DT_STRING, "FACTORY_DIV");

	CString wm_block = wm_block;
	//调用仓库入库主函数
	EIClass bcls_stock_in;
	bcls_stock_in.Tables[0].set_TableName(wm_block);
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "MAT_NO");		            //材料号
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_OPER_ORDER");       //库业务类型
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_OPER_ORDER_DIV");	//业务类型内区分
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_NO");				//库区号
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_PLACE_NO");	   //材料库位号
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "ROWNO");				   //行
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "COLUMN_NO");	           //列
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "LAYERNO");				//层
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_PLACE_POSITION");	//库位内位置
	bcls_stock_in.Tables[wm_block].Columns.Add(DT_DECIMAL, "MAT_NUM");	//入库支数


	if (bcls_rec->Tables.IndexOf(record_name) < 0)
	{
		bcls_rec->Tables.Add(record_name);
		bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
		bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
		bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
	}

	try
	{

		EDLog(1, 1, "函数调用准备，生成ds_mdno_rec");
		EIClass  ds_mdno_rec;
		ds_mdno_rec.Tables[0].Columns.Add(DT_STRING, "stock_no");
		EIClass  ds_mdno_ret;

		Log::Trace("", __FUNCTION__, "传入记录数 = [{0}]", bcls_rec->Tables[0].Rows.get_Count());

		bill_of_lading_no = bcls_rec->Tables[0].Rows[0]["BILL_OF_LADING_NO"].ToString().Trim();
		bill_of_lading_detailno = bcls_rec->Tables[0].Rows[0]["BILL_OF_LADING_DETAILNO"].ToString().Trim();
		if (bcls_rec->Tables[0].Columns.Contains("STACKING_NO"))
			old_stacking_no = bcls_rec->Tables[0].Rows[0]["STACKING_NO"].ToString().Trim();
		delivy_plan_type = bcls_rec->Tables[0].Rows[0]["DELIVY_PLAN_TYPE"].ToString().Trim();
		/////增加码单的标记
		//tsmpe11["STACKING_NO"] = old_stacking_no;
		//tsmpe11["COMPANY_CODE"] = "1";
		//tsmpe11.Update("COMPANY_CODE", "STACKING_NO");
		////效验看计划是否已经结束

		Log::Trace("", __FUNCTION__, "传入记录数 1 = [{0}]", bcls_rec->Tables[0].Rows.get_Count());

		tsm0012.Reset();
		tsm0012["BILL_OF_LADING_NO"] = bill_of_lading_no;
		tsm0012["STATUS_H"] = "1";
		tsm0012["BILL_OF_LADING_DETAILNO"] = bill_of_lading_detailno;
		Log::Trace("", __FUNCTION__, "传入记录数 1.5 = [{0}]", bcls_rec->Tables[0].Rows.get_Count());
		Log::Trace("", __FUNCTION__, "tsm0012[BILL_OF_LADING_NO] = [{0}]", tsm0012["BILL_OF_LADING_NO"].ToString());
		Log::Trace("", __FUNCTION__, "tsm0012.STATUS_H          = [{0}]", tsm0012["STATUS_H"].ToString());
		if (!tsm0012.Query("BILL_OF_LADING_NO,STATUS_H")) {
			sprintf(s.msg, "计划状态已经刷新,请重新刷新！");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		Log::Trace("", __FUNCTION__, "传入记录数 2 = [{0}]", bcls_rec->Tables[0].Rows.get_Count());

		tsm0013.Reset();
		tsm0013["BILL_OF_LADING_NO"] = bill_of_lading_no;
		if (tsm0013.QueryCount("BILL_OF_LADING_NO") != bcls_rec->Tables[0].Rows.get_Count()){
		sprintf(s.msg, "计划材料数与传入的勾选数不一致,请重新刷新！");
		throw CApplicationException(-1, s.msg, s.svc_name);
		}
		
		CString mdno_stock_no = bcls_rec->Tables[0].Rows[0]["STOCK_NO"].ToString().Trim();
		Log::Trace("", __FUNCTION__, "BILL_OF_LADING_NO = [{0}]", bill_of_lading_no);

		//调用函数装车清单号生成
		ds_mdno_rec.Tables[0].Rows.Add();
		//避免红冲码单与出厂码单重复2016.06.08
		ds_mdno_rec.Tables[0].Rows[0]["stock_no"] = "H" + mdno_stock_no;
		doFlag = f_sm00_md_no(&ds_mdno_rec, &ds_mdno_ret, conn);
		if (doFlag < 0)
		{
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		c_stacking_no = ds_mdno_ret.Tables[0].Rows[0]["stacking_no"];
		EDLog(1, 1, "生成装车单号 c_stacking_no = [%s]", (const char*)c_stacking_no);

		//获得传入的数据行数
		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			//进行效验,看计划是否已经结案
			tsm0013.Reset();
			tsm0013["BILL_OF_LADING_NO"] = bcls_rec->Tables[0].Rows[i]["BILL_OF_LADING_NO"];
			tsm0013["MAT_NO"] = bcls_rec->Tables[0].Rows[i]["MAT_NO"];
			if (!tsm0013.Query("BILL_OF_LADING_NO,MAT_NO")) {
				sprintf(s.msg, "计划状态已经刷新,请重新刷新！");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			CString mat_no = bcls_rec->Tables[0].Rows[i]["MAT_NO"].ToString().Trim();
			CString stock_oper_order = bcls_rec->Tables[0].Rows[i]["STOCK_OPER_ORDER"].ToString().Trim();
			CString stacking_no = bcls_rec->Tables[0].Rows[i]["STACKING_NO"].ToString().Trim();
			CDecimal mat_wt = bcls_rec->Tables[0].Rows[i]["MAT_WT"].ToDecimal();//退库重量
			CString stock_no = bcls_rec->Tables[0].Rows[i]["STOCK_NO"].ToString().Trim();
			CDecimal layerno = bcls_rec->Tables[0].Rows[i]["LAYERNO"].ToDecimal();//层号
			CString stock_place_no = bcls_rec->Tables[0].Rows[i]["STOCK_PLACE_NO"].ToString().Trim();
			CString prod_shift_no = bcls_rec->Tables[0].Rows[i]["PROD_SHIFT_NO"].ToString().Trim();
			CString prod_shift_group = bcls_rec->Tables[0].Rows[i]["PROD_SHIFT_GROUP"].ToString().Trim();

			CDecimal mat_num = 0;//退库支数
			CDecimal mat_act_wt = 0;
			CDecimal mat_theory_wt = 0;

			Log::Trace("", __FUNCTION__, "MAT_WT = [{0}]", mat_wt);
			if (!bcls_rec->Tables[0].Columns.Contains("STACKING_NO_NEW"))
				bcls_rec->Tables[0].Columns.Add(DT_STRING, "STACKING_NO_NEW");

			bcls_rec->Tables[0].Rows[i]["STACKING_NO_NEW"] = c_stacking_no;

			//校验库操作指示
			if (stock_oper_order != "1N" && stock_oper_order != "1Z")
			{
				sprintf(s.msg, "库操作指示错误，重新登录系统，如若不行请联系管理员！");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			Log::Trace("", __FUNCTION__, "mat_no = [{0}]", mat_no);
			Log::Trace("", __FUNCTION__, "stock_no = [{0}]", stock_no);

			//更新发货计划明细信息
			sqlstr = " SELECT MAT_KIND,FACTORY_DIV FROM TWM01 "
				" WHERE	STOCK_NO= @stock_no ";
			cmd.SetCommandText(sqlstr);
			cmd.Parameters.Clear();
			cmd.Parameters.Set("stock_no", stock_no);
			cmd.ExecuteReader();
			if (cmd.Read())
			{
				c_mat_kind = cmd.GetString(1);
				c_factory_div = cmd.GetString(2);
			}
			cmd.Close();
			Log::Trace("", __FUNCTION__, "c_mat_kind = [{0}]", c_mat_kind);
			Log::Trace("", __FUNCTION__, "c_factory_div = [{0}]", c_factory_div);

			if (c_mat_kind != "BW" && c_mat_kind == "HR" && c_mat_kind == "SM")
			{
				sprintf(s.msg, "物料种类不对[%s]", (const char*)c_mat_kind);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			tmmxx01.Clear();
			sqlstr = "SELECT * FROM HMM" + c_mat_kind + "01 WHERE MAT_NO=@MAT_NO AND MAT_STATUS = '36' ";
			cmd_tmmxx01.Parameters.Set("MAT_NO", mat_no);
			cmd_tmmxx01.SetCommandText(sqlstr);
			cmd_tmmxx01.ExecuteQuery(tmmxx01);

			//从准发历史档拉回
			//校验历史档是否存在该材料
			if (tmmxx01.Rows.get_Count() > 0)
			{
				//调用物料跟踪--材料从历史档拉回
				CDataRow& sm02_dr = bcls_mm0099.Tables["MM0099"].Rows.Add();
				sm02_dr["EVENT_ID"] = "SM02";
				sm02_dr["EVENT_LINE_TYPE"] = "00";
				sm02_dr["SYSTEM_ID"] = "SM" + c_mat_kind;
				sm02_dr["FUNC_ID"] = s.svc_name;
				sm02_dr["MAT_KIND"] = c_mat_kind;
				sm02_dr["MAT_NO"] = mat_no;
				sm02_dr["STOCK_NO"] = stock_no;
				sm02_dr["LAYERNO"] = layerno;
				sm02_dr["STOCK_PLACE_NO"] = " ";
				sm02_dr["IN_STOCK_TIME"] = s.datetime;
				sm02_dr["IN_FLAG"] = "0";
			}
			else
			{
				//Log::Trace("", __FUNCTION__, "调用KG7FM 开始");
				//EIClass bcls_ret_7f, bcls_rec_7f;
				//bcls_rec_7f.Tables[0].Columns.Add(DT_STRING, "SQL");
				//bcls_rec_7f.Tables[0].Rows.Add();
				//bcls_rec_7f.Tables[0].Rows[0]["SQL"] = "SELECT * FROM HMM" + c_mat_kind + "01 WHERE MAT_NO='" + mat_no + "' AND MAT_STATUS = '36' ";
				//f_epex_call_cgi_svc(conn, "KG7FM", "mm00_01inq", &bcls_rec_7f, &bcls_ret_7f, 30);
				//Log::Trace("", __FUNCTION__, "调用KG7FM结束");
				//struct ei_sys s_tmp7f;
				//bcls_ret_7f.GetSYS(&s_tmp7f);
				//if (s_tmp7f.flag < 0)
				//{
				//	Log::Trace("", __FUNCTION__, "调用KG7FM失败. s.flag=[{0}] s.msg =[{1}] s.sysmsg=[{2}]", s_tmp7f.flag, s_tmp7f.msg, s_tmp7f.sysmsg);
				//	throw CApplicationException(-1, s_tmp7f.msg, "调KG7FM作业层后台失败");
				//}
				//tmmxx01.Copy(bcls_ret_7f.Tables[0]);
				//if (tmmxx01.Rows.get_Count() > 0)
				//{
				//	Log::Trace("", __FUNCTION__, "调用KG7FM 后1");
				//	bcls_mm0099_ret.Tables[0].Copy(tmmxx01);
				//	bcls_mm0099_ret.Tables[0].Columns.Add(DT_STRING, "EVENT_ID");
				//	bcls_mm0099_ret.Tables[0].Columns.Add(DT_STRING, "EVENT_LINE_TYPE");
				//	bcls_mm0099_ret.Tables[0].Columns.Add(DT_STRING, "SYSTEM_ID");
				//	bcls_mm0099_ret.Tables[0].Columns.Add(DT_STRING, "FUNC_ID");
				//	bcls_mm0099_ret.Tables[0].set_TableName("MM0099");
				//	bcls_mm0099_ret.Tables[0].Rows[0]["EVENT_ID"] = "SMA2";
				//	bcls_mm0099_ret.Tables[0].Rows[0]["EVENT_LINE_TYPE"] = "00";
				//	bcls_mm0099_ret.Tables[0].Rows[0]["SYSTEM_ID"] = "SM" + c_mat_kind;
				//	bcls_mm0099_ret.Tables[0].Rows[0]["FUNC_ID"] = s.svc_name;
				//	bcls_mm0099_ret.Tables[0].Rows[0]["MAT_KIND"] = c_mat_kind;
				//	bcls_mm0099_ret.Tables[0].Rows[0]["MAT_NO"] = mat_no;
				//	bcls_mm0099_ret.Tables[0].Rows[0]["LAYERNO"] = " ";
				//	bcls_mm0099_ret.Tables[0].Rows[0]["STOCK_PLACE_NO"] = " ";
				//	bcls_mm0099_ret.Tables[0].Rows[0]["IN_STOCK_TIME"] = " ";
				//	bcls_mm0099_ret.Tables[0].Rows[0]["IN_FLAG"] = "0";

				//	doFlag = f_mm0099(&bcls_mm0099_ret, bcls_ret, conn);
				//	if (doFlag != 0)
				//	{
				//		throw CApplicationException(-1, s.msg, log.Location);
				//	}
				//	bcls_mm0099_ret.Tables[0].Clear();
				//}
				//else
				//{
					sprintf(s.msg, "未找到材料[" + mat_no + "]信息，无法执行红冲操作！");
					throw CApplicationException(-1, s.msg, s.svc_name);
				//}
			}
			Log::Trace("", __FUNCTION__, "MAT_WT = [{0}]", mat_wt);
			Log::Trace("", __FUNCTION__, "MAT_WT = [{0}]", tsm0013["MAT_WT"].ToDecimal());
			bool upd_flag = true;

			//BACK_C1 是否整件0散件，1整件
			if (tsm0013["BACK_C1"].ToString() == "0")
			{
				mat_act_wt = mat_wt;
				mat_theory_wt = mat_wt;
				mat_num = tmmxx01.Rows[0]["MAT_NUM"].ToDecimal();

				if (tsm0012["WT_MODE"].ToString() == "0" &&
					mat_wt > tmmxx01.Rows[0]["MAT_ACT_WT"].ToDecimal())
				{
					sprintf(s.msg, "材料号[" + tsm0013["MAT_NO"].ToString() + "]不能大于原重量,请重新修改重量！");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				else if (tsm0012["WT_MODE"].ToString() == "1" &&
					mat_wt > tmmxx01.Rows[0]["MAT_THEORY_WT"].ToDecimal())
				{
					sprintf(s.msg, "材料号[" + tsm0013["MAT_NO"].ToString() + "]不能大于原重量,请重新修改重量！");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				//更新退库计划明细材料重量
				tsm0013["MAT_WT"] = mat_wt;
				tsm0013.Update("MAT_WT", "BILL_OF_LADING_NO,MAT_NO");
			}
			else
			{
				upd_flag = false;
				mat_wt = tsm0013["MAT_WT"];
				mat_act_wt = tmmxx01.Rows[0]["MAT_ACT_WT"].ToDecimal();
				mat_theory_wt = tmmxx01.Rows[0]["MAT_THEORY_WT"].ToDecimal();
				mat_num = tmmxx01.Rows[0]["MAT_NUM"].ToDecimal();
			}

			//插入tsmpe12(发货码单)表
			tsmpe12.Reset();
			tsmpe12["STACKING_NO"] = stacking_no;
			tsmpe12["MAT_NO"] = mat_no;
			if (tsmpe12.Query("MAT_NO,STACKING_NO"))
			{
				Log::Trace("", __FUNCTION__, "1.1");
				if (delivy_plan_type == "4")//异议退库
				{
					Log::Info("", __FUNCTION__, "2-delivy_plan_type=[{0}]", delivy_plan_type);
					//更新原码单对应的红冲类型
					tsmpe12_upd["RED_FLAG"] = "2";
					tsmpe12["RED_FLAG"] = "2";
				}
				else
				{
					Log::Info("", __FUNCTION__, "1-delivy_plan_type=[{0}]", delivy_plan_type);
					//更新原码单对应的红冲类型
					tsmpe12_upd["RED_FLAG"] = "1";
					tsmpe12["RED_FLAG"] = "1";
				}

				tsmpe12_upd["STACKING_NO"] = tsmpe12["STACKING_NO"];
				tsmpe12_upd["MAT_NO"] = mat_no;
				tsmpe12_upd.Update("RED_FLAG", "STACKING_NO,MAT_NO");

				//生成退库履历
				tsmpe12["REC_CREATOR"] = s.userid;
				tsmpe12["REC_CREATE_TIME"] = datetime;
				tsmpe12["REC_REVISOR"] = " ";
				tsmpe12["REC_REVISE_TIME"] = " ";
				tsmpe12["DELIVY_TIME"] = datetime;
				tsmpe12["OUT_FACT_DATE"] = date;
				tsmpe12["DELIVY_SHIFT"] = prod_shift_no;
				tsmpe12["DELIVY_GROUP"] = prod_shift_group;
				tsmpe12["DELIVY_MAKER"] = s.username;
				tsmpe12["MAT_WT"] = -mat_wt;
				tsmpe12["MAT_ACT_WT"] = -mat_act_wt;
				tsmpe12["MAT_THEORY_WT"] = -mat_theory_wt;
				tsmpe12["MAT_DISCREP_WT"] = 0;
				tsmpe12["MAT_NUM"] = -mat_num;
				stacking_wt = stacking_wt + tsmpe12["MAT_WT"].ToDecimal();                                      /* 净重 */
				stacking_gross_wt = stacking_gross_wt + tsmpe12["MAT_ACT_WT"].ToDecimal();                      /* 毛重 */
				stacking_discrep_wt = stacking_discrep_wt + tsmpe12["MAT_DISCREP_WT"].ToDecimal();				 /* 磅差 */
				stacking_num = stacking_num + (-1);												/* 捆数 */
				stacking_tube = stacking_tube + tsmpe12["MAT_NUM"].ToDecimal();							    /* 支数 */
				tsmpe12["STACKING_NO"] = c_stacking_no;
				tsmpe12["BILL_OF_LADING_NO"] = tsm0012["BILL_OF_LADING_NO"];
				tsmpe12["ORDER_NO"] = tsm0012["RMA_NO"];
				tsmpe12["VEHICLE_NO"] = tsm0012["VEHICLE_NO"];
				tsmpe12["STOCK_NO"] = stock_no;
				tsmpe12["STOCK_PLACE_NO"] = stock_place_no;
				tsmpe12["FACTORY_DIV"] = c_factory_div;
				//tsmpe12.DELIVY_PLAN_TYPE = delivy_plan_type;
				tsmpe12.TrimOrBlank();
				tsmpe12.Insert();

				//更新发货计划明细信息
				sqlstr = " UPDATE tsmpe10 SET RED_QTY = RED_QTY + 1,RED_WT = RED_WT + @MAT_WT "
					" WHERE	BILL_OF_LADING_NO = @BILL_OF_LADING_NO AND ORDER_NO =@ORDER_NO ";
				cmd.SetCommandText(sqlstr);
				Log::Trace("", __FUNCTION__, "sqlstr=[{0}]", sqlstr);
				cmd.Parameters.Set("BILL_OF_LADING_NO", tsmpe12["BILL_OF_LADING_NO"].ToString());
				cmd.Parameters.Set("ORDER_NO", tsmpe12["ORDER_NO"].ToString());
				cmd.Parameters.Set("MAT_WT", tsmpe12["MAT_WT"].ToDecimal().Abs());
				int upd_count = cmd.ExecuteNonQuery();
				if (upd_count < 0)
				{
				}
			}
			else
			{
				//生成退库履历
				tsmpe12.MergeFrom(tmmxx01.Rows[0]);
				tsmpe12["BATCH_NO"] = tmmxx01.Rows[0]["ROLL_PLAN_NO"];
				tsmpe12["PROD_CODE"] = tsm0012["PROD_CODE"];
				tsmpe12["PROD_CNAME"] = tsm0012["PROD_CNAME"];
				tsmpe12["REC_CREATOR"] = s.userid;
				tsmpe12["REC_CREATE_TIME"] = datetime;
				tsmpe12["REC_REVISOR"] = " ";
				tsmpe12["REC_REVISE_TIME"] = " ";
				tsmpe12["DELIVY_TIME"] = datetime;
				tsmpe12["OUT_FACT_DATE"] = date;
				tsmpe12["DELIVY_SHIFT"] = prod_shift_no;
				tsmpe12["DELIVY_GROUP"] = prod_shift_group;
				tsmpe12["DELIVY_MAKER"] = s.username;
				tsmpe12["MAT_WT"] = -mat_wt;
				tsmpe12["MAT_ACT_WT"] = -mat_act_wt;
				tsmpe12["MAT_THEORY_WT"] = -mat_theory_wt;
				tsmpe12["MAT_DISCREP_WT"] = 0;
				tsmpe12["MAT_NUM"] = -mat_num;
				stacking_wt = stacking_wt + tsmpe12["MAT_WT"].ToDecimal();                                      /* 净重 */
				stacking_gross_wt = stacking_gross_wt + tsmpe12["MAT_ACT_WT"].ToDecimal();                      /* 毛重 */
				stacking_discrep_wt = stacking_discrep_wt + tsmpe12["MAT_DISCREP_WT"].ToDecimal();				 /* 磅差 */
				stacking_num = stacking_num + (-1);												/* 捆数 */
				stacking_tube = stacking_tube + tsmpe12["MAT_NUM"].ToDecimal();							    /* 支数 */
				tsmpe12["STACKING_NO"] = c_stacking_no;
				tsmpe12["BILL_OF_LADING_NO"] = tsm0012["BILL_OF_LADING_NO"];
				tsmpe12["ORDER_NO"] = tsm0012["RMA_NO"];
				tsmpe12["VEHICLE_NO"] = tsm0012["VEHICLE_NO"];
				tsmpe12["STOCK_NO"] = stock_no;
				tsmpe12["STOCK_PLACE_NO"] = stock_place_no;
				tsmpe12["FACTORY_DIV"] = c_factory_div;
				tsmpe12["READY_BILL_NO"] = tmmxx01.Rows[0]["SPARE_ITEM_0"];
				//tsmpe12.DELIVY_PLAN_TYPE = delivy_plan_type;
				tsmpe12.TrimOrBlank();
				tsmpe12.Insert();
			}

			Log::Trace("", __FUNCTION__, " 材料[{0}]执行入库操作f_wmbwsm_stock_in", mat_no);
			CDataRow& in_dr = bcls_stock_in.Tables[wm_block].Rows.Add();
			in_dr["MAT_NO"] = mat_no;
			in_dr["STOCK_OPER_ORDER"] = stock_oper_order;    //热送接收
			in_dr["STOCK_OPER_ORDER_DIV"] = "1";//入库
			in_dr["STOCK_NO"] = stock_no;
			in_dr["STOCK_PLACE_NO"] = stock_place_no;
			in_dr["ROWNO"] = " ";
			in_dr["COLUMN_NO"] = " ";
			in_dr["LAYERNO"] = " ";
			in_dr["STOCK_PLACE_POSITION"] = " ";
			in_dr["MAT_NUM"] = 1;//入库支数

			//调用函数新增履历记录
			bcls_rec->Tables[record_name].Rows.Add();
			bcls_rec->Tables[record_name].Rows[i]["mat_no"] = tsmpe12["MAT_NO"];
			bcls_rec->Tables[record_name].Rows[i]["event_mark"] = "6";
			bcls_rec->Tables[record_name].Rows[i]["userid"] = v_userid;



			tsmpe02.MergeFrom(tmmxx01.Rows[0]);
			tsmpe02.MergeFrom(bcls_rec->Tables[0].Rows[i]);
			tsmpe02["MAT_WT"] = mat_wt;
			tsmpe02["MAT_ACT_WT"] = mat_act_wt;
			tsmpe02["MAT_THEORY_WT"] = mat_theory_wt;
			tsmpe02["REC_CREATE_TIME"] = datetime;
			tsmpe02["REC_CREATOR"] = s.userid;
			tsmpe02["REC_REVISE_TIME"] = datetime;
			tsmpe02["REC_REVISOR"] = s.userid;
			tsmpe02["BILL_OF_LADING_NO"] = "";	//提单号
			tsmpe02["CONFM_STATUS"] = "4";	//准发状态
			tsmpe02["ORDER_NO"] = tmmxx01.Rows[0]["ORDER_NO"];
			tsmpe02["DELIVY_TIME"] = "";	//出厂时刻
			tsmpe02["STACKING_NO"] = "";	//码单号
			tsmpe02["OUT_FACT_DATE"] = "";	//出厂日期
			tsmpe02["DELIVY_SHIFT"] = "";	//出厂班次
			tsmpe02["DELIVY_GROUP"] = "";	//出厂班组
			tsmpe02["DELIVY_MAKER"] = "";	//出厂责任者
			tsmpe02["VEHICLE_NO"] = "";	//车船号
			tsmpe02["OUT_MARK"] = "";	//出库标志
			tsmpe02["RED_FLAG"] = "0";	//红冲标记
			tsmpe02["PROD_CODE"] = tsmpe12["PROD_CODE"];
			tsmpe02["PROD_CNAME"] = tsmpe12["PROD_CNAME"];
			tsmpe02["READY_BILL_NO"] = tsmpe12["READY_BILL_NO"];
			tsmpe02["FACTORY_DIV"] = c_factory_div;
			tsmpe02["STOCK_NO"] = stock_no;
			tsmpe02["STOCK_PLACE_NO"] = stock_place_no;
			tsmpe02["BATCH_NO"] = tmmxx01.Rows[0]["ROLL_PLAN_NO"];
			tsmpe02.TrimOrBlank();
			sqlstr = " INSERT INTO tsmpe02 ";
			if (tsmpe02.Insert() == false)
			{
				sprintf(s.msg, "新增材料记录出错，材料号[%s]", (const char*)tsmpe02["MAT_NO"].ToString());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			//调用物料跟踪--异议退库计划：材料人工修改规格
			if (delivy_plan_type == "4")
			{
				if (upd_flag)
				{
					CDataRow& mm03_dr = bcls_mm03.Tables["MM0099"].Rows.Add();
					mm03_dr.Merge(tmmxx01.Rows[0]);
					mm03_dr["EVENT_ID"] = "MM0Z";
					mm03_dr["EVENT_LINE_TYPE"] = "00";
					mm03_dr["SYSTEM_ID"] = "MM" + c_mat_kind;
					mm03_dr["FUNC_ID"] = s.svc_name;
					mm03_dr["MAT_KIND"] = c_mat_kind;
					mm03_dr["MAT_NO"] = mat_no;
					mm03_dr["MAT_THICK"] = tmmxx01.Rows[0]["MAT_THICK"];
					mm03_dr["MAT_WIDTH"] = tmmxx01.Rows[0]["MAT_WIDTH"];
					mm03_dr["MAT_LEN"] = tmmxx01.Rows[0]["MAT_LEN"];
					mm03_dr["MAT_ACT_THICK"] = tmmxx01.Rows[0]["MAT_ACT_THICK"];
					mm03_dr["MAT_ACT_WIDTH"] = tmmxx01.Rows[0]["MAT_ACT_WIDTH"];
					mm03_dr["MAT_ACT_LEN"] = tmmxx01.Rows[0]["MAT_ACT_LEN"];
					mm03_dr["MEASURE_WT_FLAG"] = tmmxx01.Rows[0]["MEASURE_WT_FLAG"];
					if (c_mat_kind != "HR")
					{
						mm03_dr["FIX_FLAG"] = tmmxx01.Rows[0]["FIX_FLAG"];
					}
					mm03_dr["MAT_WT"] = mat_wt;
					mm03_dr["MAT_ACT_WT"] = mat_act_wt;
					mm03_dr["MAT_THEORY_WT"] = mat_theory_wt;
					mm03_dr["MAT_NUM"] = mat_num;
				}
			}

			//抛出厂
			CDataRow& smsnd_dr = bcls_sm_snd.Tables[0].Rows.Add();
			if (delivy_plan_type == "4")//异议退库计划
			{
				smsnd_dr["FLAG"] = "2";
			}
			else
			{
				smsnd_dr["FLAG"] = "1";
			}
			smsnd_dr["STACKING_NO"] = c_stacking_no;
			smsnd_dr["BILL_OF_LADING_NO"] = bill_of_lading_no;
			smsnd_dr["STOCK_NO"] = stock_no;
			smsnd_dr["MAT_NO"] = mat_no;
			smsnd_dr["OLD_STACKING_NO"] = stacking_no;
			smsnd_dr["FACTORY_DIV"] = c_factory_div;
			smsnd_dr["MAT_KIND"] = c_mat_kind;

		}
		tsmpe11.CopyFrom(tsm0012);
		tsmpe11["REC_CREATE_TIME"] = datetime;
		tsmpe11["REC_CREATOR"] = s.userid;
		tsmpe11["REC_REVISE_TIME"] = datetime;
		tsmpe11["REC_REVISOR"] = s.userid;
		tsmpe11["STACKING_TYPE"] = tsm0012["DELIVY_PLAN_TYPE"];
		tsmpe11["BILL_OF_LADING_NO"] = bill_of_lading_no;
		tsmpe11["STACKING_NO"] = c_stacking_no;
		tsmpe11["FACTORY_DIV"] = c_factory_div;
		tsmpe11["STOCK_NO"] = tsm0012["IN_STOCK_CODE"];
		tsmpe11["DELIVY_TIME"] = tsmpe12["DELIVY_TIME"];
		tsmpe11["DELIVY_MAKER"] = tsmpe12["DELIVY_MAKER"];
		tsmpe11["DELIVY_GROUP"] = tsmpe12["DELIVY_GROUP"];
		tsmpe11["DELIVY_SHIFT"] = tsmpe12["DELIVY_SHIFT"];
		tsmpe11["STACKING_WT"] = stacking_wt;
		tsmpe11["STACKING_GROSS_WT"] = stacking_gross_wt;
		tsmpe11["STACKING_DISCREP_WT"] = stacking_discrep_wt;
		tsmpe11["STACKING_NUM"] = stacking_num;
		tsmpe11["STACKING_TUBE"] = stacking_tube;
		tsmpe11["VEHICLE_NO"] = tsm0012["VEHICLE_NO"];
		tsmpe11["STACKING_PRINTS"] = 0;
		tsmpe11["DELIVY_REMARK"] = " ";
		tsmpe11["STACKING_STATUS"] = "1";
		tsmpe11["LOADING_NO"] = c_stacking_no;
		tsmpe11["PROD_CODE"] = tsm0012["PROD_CODE"];
		tsmpe11["PROD_CNAME"] = tsm0012["PROD_CNAME"];
		tsmpe11["CONFM_PLAN_NO"] = " ";
		tsmpe11["READY_BILL_NO"] = " ";
		tsmpe11.TrimOrBlank();
		tsmpe11.Insert();

		//记出厂履历
		if (bcls_rec->Tables[record_name].Rows.get_Count() > 0)
		{
			doFlag = f_sm00_record(bcls_rec, bcls_ret, conn);
			if (doFlag != 0)
			{
				Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}

		//材料从历史档拉回
		if (bcls_mm0099.Tables["MM0099"].Rows.get_Count() > 0)
		{
			doFlag = f_mm0099(&bcls_mm0099, bcls_ret, conn);
			if (doFlag != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}

		////修改入库标记为IN_FLAG=0
		//for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		//{
		//	tmmbw01.IN_FLAG = "0";
		//	tmmbw01.MAT_NO = bcls_rec->Tables[0].Rows[i]["MAT_NO"];
		//	tmmbw01.Update("IN_FLAG", "MAT_NO");
		//}

		//修改重量
		if (bcls_mm03.Tables["MM0099"].Rows.get_Count() > 0)
		{
			doFlag = f_mm0099(&bcls_mm03, bcls_ret, conn);
			if (doFlag != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#ifdef _LINE_BW
		if (tsmpe12["MAT_KIND"].ToString() == "BW")
		{
			Log::Debug("", "", "MAT_KIND=【{0}】", tsmpe12["MAT_KIND"].ToString());
			if (bcls_stock_in.Tables[wm_block].Rows.get_Count() > 0)
			{
				doFlag = f_wmbwbw_stock_in(&bcls_stock_in, bcls_ret, conn);
				if (doFlag != 0)
				{
					throw CApplicationException(-1, s.msg, log.Location);
				}

			}
		}
#endif

#ifdef _LINE_SM
		if (tsmpe12["MAT_KIND"].ToString() == "SM")
		{
			Log::Debug("", "", "MAT_KIND=【{0}】", tsmpe12["MAT_KIND"].ToString());
			if (bcls_stock_in.Tables[wm_block].Rows.get_Count() > 0)
			{
				doFlag = f_wmsmsm_stock_in(&bcls_stock_in, bcls_ret, conn);
				if (doFlag != 0)
				{
					throw CApplicationException(-1, s.msg, log.Location);
				}

			}
		}
#endif

#ifdef _LINE_HP
		if (tsmpe12["MAT_KIND"].ToString() == "HP")
		{
			Log::Debug("", "", "MAT_KIND=【{0}】", tsmpe12["MAT_KIND"].ToString());
			if (bcls_stock_in.Tables[wm_block].Rows.get_Count() > 0)
			{
				doFlag = f_wmhphp_stock_in(&bcls_stock_in, bcls_ret, conn);
				if (doFlag != 0)
				{
					throw CApplicationException(-1, s.msg, log.Location);
				}

			}
		}
#endif

#ifdef _LINE_CR
		if (tsmpe12["MAT_KIND"].ToString() == "CR")
		{
			Log::Debug("", "", "MAT_KIND=【{0}】", tsmpe12["MAT_KIND"].ToString());
			if (bcls_stock_in.Tables[wm_block].Rows.get_Count() > 0)
			{
				doFlag = f_wmcrcr_stock_in(&bcls_stock_in, bcls_ret, conn);
				if (doFlag != 0)
				{
					throw CApplicationException(-1, s.msg, log.Location);
				}

			}
		}
#endif

#ifdef _LINE_HR
		if (tsmpe12["MAT_KIND"].ToString() == "HR")
		{
			Log::Debug("", "", "MAT_KIND=【{0}】", tsmpe12["MAT_KIND"].ToString());
			if (bcls_stock_in.Tables[wm_block].Rows.get_Count() > 0)
			{
				doFlag = f_wmhrhr_stock_in(&bcls_stock_in, bcls_ret, conn);
				if (doFlag != 0)
				{
					throw CApplicationException(-1, s.msg, log.Location);
				}

			}
		}
#endif
		//调用入库函数
		if (bcls_stock_in.Tables[wm_block].Rows.get_Count() > 0)
		{
			//doFlag = f_sm00_stock_in(&bcls_stock_in, bcls_ret, conn);
			//if (doFlag < 0)
			//{
			//	throw CApplicationException(-1, s.msg, log.Location);
			//}
		}

		//发送红冲实绩电文
		if (bcls_sm_snd.Tables[0].Rows.get_Count() > 0)
		{
			//doFlag = f_xxsm05_snd(&bcls_dx7fs1, bcls_ret, conn);
			//if (doFlag != 0)
			//{
			//	throw CApplicationException(-1, s.msg, log.Location);
			//}
		}

		//更新退库计划主表状态【确认】
		tsm0012["STATUS_H"] = "2";
		tsm0012["BILL_OF_LADING_NO"] = bill_of_lading_no;
		tsm0012["BILL_OF_LADING_DETAILNO"] = bill_of_lading_detailno;
		tsm0012.Update("STATUS_H", "BILL_OF_LADING_NO,BILL_OF_LADING_DETAILNO");

		////向物流发送电文删除实绩
		///* ***** 创建电文处理对象 ***** */
		//EPEX epex(&s);
		//// 初始化电文格式
		//if (epex.Initialize("GFG801") < 0)   //电文号
		//{
		//	{
		//		CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
		//		CMessageFormat::Format(s.msg, _RES("SM00S0001464")/*初始化电文时失败！ 原因描述：[ {0}]*/, arguments, 1);//格式化字符串
		//	}
		//	//sprintf(s.msg,"初始化电文时失败! 原因描述: %s", epex.GetMsg());//转换前
		//	throw	CApplicationException(-1, s.msg, log.Location);
		//}
		////码单号
		//if (epex.SetValue("stacking_no", 0, old_stacking_no) < 0)
		//{
		//	sprintf(s.msg, "输入的码单号[%s]有误，请重新输入！", old_stacking_no);
		//	throw	CApplicationException(-1, s.msg, log.Location);
		//}
		//// 发送电文
		//if (epex.SendTele() < 0)
		//{
		//	{
		//		CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
		//		CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
		//	}
		//	//sprintf(s.msg,"发送电文时失败! 原因描述: %s", epex.GetMsg());//转换前
		//	throw	CApplicationException(-1, s.msg, log.Location);
		//}
		//// 释放
		//epex.Uninitialize(); 
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, "数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。" /* _RES("GCRSS0000006")*//*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		//EDLog(1,1, "[%s]", s.sysmsg);
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
	return doFlag;
}
