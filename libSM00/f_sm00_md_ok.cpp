/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      013801
Version:     1.0
Date:        2023-1-9 14:00:33 
Description: 码单确认
2021-11-14	13801	物料表上出入库标记不为1时不调用仓库函数
2021-11-24	13801	应金权要求增加条件 && wm_flag != "1"
2021-12-28	13801	当材料上准发计划号前后相同时，调用一次f_sm00_count 函数
2022-4-22	13801	转库出库调用仓库出库传 2G 事件
2022-6-14	13801	本分区转库出入库写履历事件G
2022-10-20	13801	TSMPE11表的出厂日期用码单确认的日期,先上冷轧
2022-10-24	13801	TSMPE12表的出厂日期用码单确认的日期,先上冷轧
**************************************************/


//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件
#include "epex.h"


BM2_FUNCTION_IMPORT
int f_sm00_record(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 写履历记录 */
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection * conn);				/* 计算准发计划、准发单据状态 */
int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int f_sm00_mm99(vector <CString> pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */

int f_xxsm01_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 码单实绩电文 */
int f_xxsm04_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 计划结案电文 */
int f_xx00s4_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 给L4发送发货实绩电文 */


#ifdef _LINE_SM
int f_wmsmsm_stock_out(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 仓库出库函数
#endif
#ifdef _LINE_BW
int f_wmbwbw_stock_out(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 仓库出库函数
#endif
#ifdef _LINE_HP
int f_wmhphp_stock_out(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 仓库出库函数
#endif
#ifdef _LINE_CR
int f_wmcrcr_stock_out(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 仓库出库函数
#endif
#ifdef _LINE_HR
int f_wmhrhr_stock_out(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 仓库出库函数
#endif

//int f_wm_stock(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection *conn);	// 仓库出库函数
//int f_wm00_queue(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);		// 仓库出入库队列
//int f_wm_send(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);	// 电文发送


BM2_FUNCTION_EXPORT
int f_sm00_md_ok(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int		fetchRowCount = 0;
	int		doFlag = 0;														/* 返回处理标记 */
	int		ret = 0;
	int		i = 0;
	int		v_count = 0;
	CDecimal	v_wt = 0;
	CString	str;
	CString	sql;
	CString	blkname = "md_ok";
	CString	record_name = "sm00_record";

	CString	datetime;
	CString	date;
	CString	time;
	CString	stacking_no;						/* 码单号 */
	CString	v_userid = s.userid;
	CString	c_inv_flag = "0";					/* 库存标记 */
	CString v_out_stock_code = "";
	CString	ticket_no;	//装车单
	CString wm_flag = "";	// 抛物料事件标记
	CString CONFM_PLAN_NO = "";
	// 创建电文处理对象
	EPEX epex(&s, conn);

	/* 实体类定义 */
	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");;
	date = datetime.Substring(0, 8);
	time = datetime.Substring(8, 6);

	CModel tsmpe00("TSMPE00");
	CModel tsmpe02("TSMPE02");
	CModel tsmpe10("TSMPE10");
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");
	CDbCommand cmd_inq1(conn);
	CDbCommand cmd_inq2(conn);
	CDbCommand cmd(conn);
	CString sqlstr;
	CString sqlstr1;
	CString sqlstr2;
	CDbCommand cmd_inq(conn);
	vector <CString> mat_no;

	try
	{
		// 码单电文发送函数块
		if (!bcls_rec->Tables.Contains("0"))
		{
			bcls_rec->Tables.Add("0");
			bcls_rec->Tables["0"].Columns.Add(DT_STRING, "stacking_no");
			bcls_rec->Tables["0"].Columns.Add(DT_STRING, "tc_no");
			bcls_rec->Tables["0"].Columns.Add(DT_STRING, "oper_flag");
		}



		//入库队列
		CString blk_name_wm = "WM00QUE";
		EIClass bcls_stock_que;
		bcls_stock_que.Tables.Add(blk_name_wm);
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "MAT_NO");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "TO_STOCK_NO");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "FROM_STOCK_NO");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "UNIT_CODE");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "OPER_FLAG");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "STOCK_OPER_ORDER");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "VEHICLE_NO");
		bcls_stock_que.Tables[blk_name_wm].Rows.Clear();



		/* 仓库函数定义块 */
		CString	wm_block = "WM_STOCK";
		EIClass bcls_stock_out;
		bcls_stock_out.Tables[0].set_TableName(wm_block);
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_NO");	// 材料号
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_OPER_ORDER");	// 库业务类型
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_NO");
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_PLACE_NO");	//库位号
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "LAYERNO");	// 目标层号
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "STOCK_PLACE_POSITION");	// 库位内位置
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "CRANE_NO");	// 吊车号
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "VEHICLE_NO");	// 车牌号
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "TO_STOCK_NO");
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "COLUMN_NO");
		//bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_LINE_TYPE");	//产线类型
		bcls_stock_out.Tables[wm_block].Columns.Add(DT_STRING, "MAT_KIND");	// 物料类型
		bcls_stock_out.Tables[0].Rows.Clear();

		/* 发货记录履历块 */
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}

		int	rows = bcls_rec->Tables[blkname].Rows.get_Count();
		if (rows == 0)
		{
			sprintf(s.msg, "没有传入数据");
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}



		for (i = 0; i < rows; i++)
		{
			ticket_no = bcls_rec->Tables[blkname].Rows[i]["TICKET_NO"];		//装车单
			Log::Info("", __FUNCTION__, "[{0}/{1}],装车单号=[{2}]", i + 1, rows, ticket_no);
			tsmpe11["TICKET_NO"] = ticket_no;			/* 装车单号 */

			//定义循环查询
			sqlstr = "SELECT *	FROM	tsmpe11 WHERE	TICKET_NO	=	@ticket_no";	// 2020-2-27
			//开始循环查询
			bcls_stock_out.Tables[wm_block].Rows.Clear();
			mat_no.clear();

			cmd_inq1.SetCommandText(sqlstr);
			cmd_inq1.Parameters.Set("ticket_no", ticket_no);
			cmd_inq1.ExecuteReader();
			fetchRowCount = 0;
			while (cmd_inq1.Read())
			{
				cmd_inq1.Fetch(tsmpe11);
				fetchRowCount++;
				Log::Trace("", __FUNCTION__, "读取第[{0}]个码单,码单号=[{1}]", fetchRowCount, tsmpe11["STACKING_NO"].ToString());
				if (tsmpe11["STACKING_STATUS"].ToString() != "1")
				{
					sprintf(s.msg, "读取的码单状态不在未确认状态，码单号=[%s]", (const char*)tsmpe11["STACKING_NO"].ToString());
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}

				tsmpe11["STACKING_STATUS"] = "0";
				tsmpe11["DELIVY_TIME"] = datetime;	// 2022-10-20
				v_count = tsmpe11.Update("STACKING_STATUS,DELIVY_TIME ", "STACKING_NO");
				Log::Debug("", __FUNCTION__, "v_count=[{0}] , BILL_OF_LADING_NO=[{1}], ORDER_NO=[{2}]", v_count, tsmpe11["BILL_OF_LADING_NO"].ToString(), tsmpe11["ORDER_NO"].ToString());

				// 2022-10-24 down
				tsmpe12["DELIVY_TIME"] = datetime;
				tsmpe12["STACKING_NO"] = tsmpe11["STACKING_NO"];
				v_count = tsmpe12.Update("DELIVY_TIME ", "STACKING_NO");
				// 2022-10-24 up

				sqlstr = "SELECT	*   FROM	tsmpe10   WHERE	bill_of_lading_no	=	@BILL_OF_LADING_NO ";
				cmd.SetCommandText(sqlstr);
				cmd.Parameters.Clear();
				cmd.Parameters.Set("BILL_OF_LADING_NO", tsmpe11["BILL_OF_LADING_NO"].ToString());
				Log::Info("", "", "sqlstr=[{0}]", sqlstr);
				cmd.ExecuteReader();
				if (cmd.Read())
				{
					cmd.Fetch(tsmpe10);
				}
				cmd.Close();


				// 判材料是否需要归档，当是转库计划并且转到外库就归档
				wm_flag = "1";	// 是否需要归档，1--归档，0--不归
				////CString area_code = "", area_code_1 = "";
				////if (tsmpe10["STOCK_NO_TO"].ToString().Trim() != "")
				////{
				////	sqlstr = "select area_code from twm01 where stock_no = @stock_no ";
				////	cmd_inq.SetCommandText(sqlstr);
				////	cmd_inq.Parameters.Set("stock_no", tsmpe10["STOCK_NO_TO"].ToString());
				////	Log::Debug("", "", "sqlstr=[{0}]", sqlstr);
				////	cmd_inq.ExecuteReader();
				////	if (cmd_inq.Read())
				////	{
				////		area_code_1 = cmd_inq.GetString(1);

				////		sqlstr = "select area_code from twm01 where stock_no = @stock_no ";
				////		cmd_inq2.SetCommandText(sqlstr);
				////		cmd_inq2.Parameters.Set("stock_no", tsmpe10["STOCK_NO"].ToString());
				////		Log::Debug("", "", "sqlstr=[{0}]", sqlstr);
				////		cmd_inq2.ExecuteReader();
				////		if (cmd_inq2.Read())
				////		{
				////			area_code = cmd_inq2.GetString(1);
				////		}
				////		cmd_inq2.Close();

				////		if (area_code != area_code_1)
				////		{
				////			wm_flag = "2";	// PES归档，MMS不归档  2022-4-22
				////		}
				////		else
				////		{
				////			wm_flag = "0";	// PES不归档
				////		}
				////	}
				////	else
				////	{
				////		wm_flag = "2";	// 发货归档
				////	}
				////	cmd_inq.Close();
				////}
				////else
				////{
				////	if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "3")
				////	{
				////		wm_flag = "3";
				////	}
				////}
				Log::Debug("", "", "wm_flag={0}", wm_flag);

				CString c_order_no = " ";
				if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" )
				{
					sqlstr = "SELECT *   FROM	tsmpe10  WHERE	bill_of_lading_no	=	@BILL_OF_LADING_NO  AND	order_no=	@ORDER_NO ";
					cmd.SetCommandText(sqlstr);
					cmd.Parameters.Clear();
					cmd.Parameters.Set("BILL_OF_LADING_NO", tsmpe11["BILL_OF_LADING_NO"].ToString());
					cmd.Parameters.Set("ORDER_NO", tsmpe11["ORDER_NO"].ToString());
					cmd.ExecuteReader();
					if (cmd.Read())
					{
						cmd.Fetch(tsmpe10);
					}
					cmd.Close();
					c_order_no = tsmpe11["ORDER_NO"];
				}
				Log::Trace("", __FUNCTION__, "查询tsmbw10表完成");



				// 更新发货计划上的出厂量
				tsmpe10["DELIVY_WT"] = tsmpe10["DELIVY_WT"].ToDecimal() + tsmpe11["STACKING_WT"].ToDecimal();
				tsmpe10["DELIVY_NUM"] = tsmpe10["DELIVY_NUM"].ToDecimal() + tsmpe11["STACKING_NUM"].ToDecimal();

				//如果是四级下发的计划，计算状态
				if (tsmpe10["REMARK1"].ToString() == "L4")
				{
					tsmpe10["DELIVY_PLAN_STATUS"] = "4";
					if (tsmpe10["PLAN_WT"].ToDecimal() < tsmpe10["DELIVY_WT"].ToDecimal() + 0.000001 || tsmpe10["PLAN_NUM"].ToDecimal() == tsmpe10["DELIVY_NUM"].ToDecimal())
					{
						tsmpe10["DELIVY_PLAN_STATUS"] = "5";
					}
				}
				else
				{
					tsmpe10["DELIVY_PLAN_STATUS"] = "5";
				}

				sqlstr = "update tsmpe10 set DELIVY_WT = DELIVY_WT + @DELIVY_WT , DELIVY_NUM = DELIVY_NUM + @DELIVY_NUM "
					" , DELIVY_PLAN_STATUS = @DELIVY_PLAN_STATUS "
					" WHERE BILL_OF_LADING_NO = @BILL_OF_LADING_NO ";
				if (c_order_no.Trim() != "")
				{
					sqlstr += " AND ORDER_NO IN (' ', '" + tsmpe11["ORDER_NO"].ToString().Trim() + "') ";
				}
				cmd.SetCommandText(sqlstr);
				cmd.Parameters.Set("BILL_OF_LADING_NO", tsmpe11["BILL_OF_LADING_NO"].ToString());
				cmd.Parameters.Set("DELIVY_NUM", tsmpe11["STACKING_NUM"].ToDecimal());
				cmd.Parameters.Set("DELIVY_WT", tsmpe11["STACKING_WT"].ToDecimal());
				cmd.Parameters.Set("DELIVY_PLAN_STATUS", tsmpe10["DELIVY_PLAN_STATUS"].ToString());
				Log::Debug("", "", "sqlstr = {0}", sqlstr);
				int k = cmd.ExecuteNonQuery();
				if (k != 1)
				{
					CFormattable	arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString(), tsmpe11["ORDER_NO"].ToString(), k };
					CMessageFormat::Format(s.msg, "更新发货计划表出错:计划号{0}，合同号{1},记录数{2}", arguments, 3);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}



				//开始循环查询
				sqlstr = "SELECT *	FROM	tsmpe12 WHERE	stacking_no	=	@STACKING_NO ";
				cmd_inq2.SetCommandText(sqlstr);
				cmd_inq2.Parameters.Set("STACKING_NO", tsmpe11["STACKING_NO"].ToString());
				cmd_inq2.ExecuteReader();
				fetchRowCount = 0;
				Log::Trace("", __FUNCTION__, "开始fetch tsmpe12表");
				while (cmd_inq2.Read())
				{
					cmd_inq2.Fetch(tsmpe12);
					////if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" || tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2" )
					////{
					////	sqlstr = "UPDATE	tsmpe10	SET		delivy_plan_status	= @DELIVY_PLAN_STATUS "
					////		" ,	rec_revise_time	= @datetime	,	rec_revisor	= @v_userid "
					////		" ,	delivy_num 	= delivy_num + 1 "
					////		" ,	delivy_wt 	= delivy_wt	 + @MAT_WT "
					////		" WHERE 	bill_of_lading_no 	= @BILL_OF_LADING_NO ";
							////if (c_order_no.Trim() != "")
							////{
							////	sqlstr += " AND		order_no			= @c_order_no ";
							////}

					////	tsmpe10["DELIVY_WT"] = tsmpe10["DELIVY_WT"].ToDecimal() + tsmpe12["MAT_WT"].ToDecimal();
					////	tsmpe10["DELIVY_PLAN_STATUS"] = "4";
					////	if (tsmpe10["PLAN_WT"].ToDecimal() > (tsmpe10["DELIVY_WT"].ToDecimal() + 0.1))
					////	{
					////		tsmpe10["DELIVY_PLAN_STATUS"] = "4";
					////	}
					////	else
					////	{
					////		tsmpe10["DELIVY_PLAN_STATUS"] = "5";
					////	}

					////	cmd.SetCommandText(sqlstr);
					////	cmd.Parameters.Clear();
					////	cmd.Parameters.Set("DELIVY_PLAN_STATUS", tsmpe10["DELIVY_PLAN_STATUS"].ToString());
					////	cmd.Parameters.Set("BILL_OF_LADING_NO", tsmpe12["BILL_OF_LADING_NO"].ToString());
					////	cmd.Parameters.Set("c_order_no", tsmpe12["ORDER_NO"].ToString());
					////	cmd.Parameters.Set("MAT_WT", tsmpe12["MAT_WT"].ToDecimal());
					////	cmd.Parameters.Set("datetime", datetime);
					////	cmd.Parameters.Set("v_userid", v_userid);
					////	cmd.ExecuteNonQuery();
					////	Log::Trace("", __FUNCTION__, "aaaaaaa");
					////	Log::Trace("", __FUNCTION__, "tsmpe10["DELIVY_PLAN_STATUS"] =[{0}]", tsmpe10["DELIVY_PLAN_STATUS"].ToString());
					////	CDbCommand cmd_tmp(conn);
					////	sqlstr = " SELECT	COUNT(*) FROM TSMPE10 WHERE bill_of_lading_no = @tsmpe12.BILL_OF_LADING_NO AND order_no like @tsmpe12.ORDER_NO||'%' AND plan_wt < delivy_wt ";
					////	cmd_tmp.SetCommandText(sqlstr);
					////	cmd_tmp.Parameters.Set("tsmpe12.BILL_OF_LADING_NO", tsmpe12["BILL_OF_LADING_NO"].ToString());
					////	cmd_tmp.Parameters.Set("tsmpe12.ORDER_NO", tsmpe12["ORDER_NO"].ToString());
					////	cmd_tmp.ExecuteReader();
					////	Log::Trace("", __FUNCTION__, "bbbbbbbbbb");
					////	v_count = 0;
					////	if (cmd_tmp.Read())
					////	{
					////		cmd_tmp.Get(1, v_count);
					////	}
					////	cmd_tmp.Close();
					////	if (v_count > 0)
					////	{
					////		sprintf(s.msg, "发货量大于计划量，合同号=[%s]", (const char*)tsmpe12["ORDER_NO"].ToString());
					////		throw	CApplicationException(-1, s.msg, s.svc_name);
					////	}
					////	Log::Trace("", __FUNCTION__, "update tsmbw10表完成1");
					////}
					////else
					////{
					////	sqlstr = "UPDATE	tsmpe10 "
					////		" SET		delivy_plan_status	= @tsmpe10.DELIVY_PLAN_STATUS "
					////		" ,	rec_revise_time		= @datetime "
					////		" ,	rec_revisor			= @v_userid "
					////		" ,	delivy_num 				= delivy_num + 1 "
					////		" ,	delivy_wt 			= delivy_wt	 + @tsmpe12.MAT_WT "
					////		" WHERE 	bill_of_lading_no 	= @tsmpe12.BILL_OF_LADING_NO"
					////		" AND       ORDER_NO = @tsmpe12.ORDER_NO ";

					////	tsmpe10["DELIVY_WT"] = tsmpe10["DELIVY_WT"].ToDecimal() + tsmpe12["MAT_WT"].ToDecimal();
					////	tsmpe10["DELIVY_NUM"] = tsmpe10["DELIVY_WT"].ToDecimal() + 1;
					////	if (tsmpe10["PLAN_WT"].ToDecimal() < tsmpe10["DELIVY_WT"].ToDecimal() + 0.000001 || tsmpe10["PLAN_NUM"].ToDecimal() == tsmpe10["DELIVY_NUM"].ToDecimal())
					////	{
					////		tsmpe10["DELIVY_PLAN_STATUS"] = "5";
					////	}
					////	else
					////	{
					////		tsmpe10["DELIVY_PLAN_STATUS"] = "4";
					////	}

					////	cmd.SetCommandText(sqlstr);
					////	cmd.Parameters.Clear();
					////	cmd.Parameters.Set("tsmpe10.DELIVY_PLAN_STATUS", tsmpe10["DELIVY_PLAN_STATUS"].ToString());
					////	cmd.Parameters.Set("tsmpe12.BILL_OF_LADING_NO", tsmpe12["BILL_OF_LADING_NO"].ToString());
					////	cmd.Parameters.Set("tsmpe12.ORDER_NO", tsmpe12["ORDER_NO"].ToString());
					////	cmd.Parameters.Set("tsmpe10.DELIVY_WT", tsmpe10["DELIVY_WT"].ToDecimal());
					////	cmd.Parameters.Set("tsmpe12.MAT_WT", tsmpe12["MAT_WT"].ToDecimal());
					////	cmd.Parameters.Set("datetime", datetime);
					////	cmd.Parameters.Set("v_userid", v_userid);
					////	v_count = cmd.ExecuteNonQuery();
					////	Log::Trace("", __FUNCTION__, "update tsmpe10表完成[{0}],BILL_OF_LADING_NO = [{1}],DELIVY_WT = [{2}], MAT_WT = [{3}]", v_count, tsmpe12["BILL_OF_LADING_NO"].ToString(), tsmpe10["DELIVY_WT"].ToDecimal(), tsmpe12["MAT_WT"].ToDecimal());
					////	Log::Trace("", __FUNCTION__, "tsmpe10["DELIVY_PLAN_STATUS"] =[{0}]", tsmpe10["DELIVY_PLAN_STATUS"].ToString());
					////}

					sqlstr = "UPDATE	tsmpe00		SET		confm_status = '8' "
						" WHERE	ready_bill_no =  @READY_BILL_NO"
						" AND		confm_status = '6'";

					cmd.SetCommandText(sqlstr);
					cmd.Parameters.Clear();
					cmd.Parameters.Set("READY_BILL_NO", tsmpe12["READY_BILL_NO"].ToString());
					cmd.ExecuteNonQuery();

					sqlstr = "UPDATE	tsmpe01		SET		confm_status = '8' "
						" WHERE	confm_plan_no =  @CONFM_PLAN_NO "
						" AND		confm_status = '6'";

					cmd.SetCommandText(sqlstr);
					cmd.Parameters.Clear();
					cmd.Parameters.Set("CONFM_PLAN_NO", tsmpe12["CONFM_PLAN_NO"].ToString());
					cmd.ExecuteNonQuery();

					sqlstr = " UPDATE	tsmpe02 SET		confm_status	=	'9' "
						" ,	rec_revisor		=	@v_userid	,	rec_revise_time	=	@datetime "
						" ,	delivy_time		=	@datetime	,	out_fact_date	=	@date "
						" ,	stacking_no		=	@STACKING_NO "
						" ,	vehicle_no		=	@VEHICLE_NO "
						" ,	delivy_maker	=	@v_userid "
						" WHERE	mat_no		=	@MAT_NO	";

					Log::Trace("", __FUNCTION__, "STACKING_NO=[{0}]", tsmpe12["STACKING_NO"].ToString());
					Log::Trace("", __FUNCTION__, "VEHICLE_NO=[{0}]", tsmpe12["VEHICLE_NO"].ToString());
					Log::Trace("", __FUNCTION__, "MAT_NO=[{0}]", tsmpe12["MAT_NO"].ToString());
					Log::Trace("", __FUNCTION__, "test****");
					cmd.SetCommandText(sqlstr);
					cmd.Parameters.Clear();
					cmd.Parameters.Set("v_userid", v_userid);
					cmd.Parameters.Set("datetime", datetime);
					cmd.Parameters.Set("date", date);
					cmd.Parameters.Set("STACKING_NO", tsmpe12["STACKING_NO"].ToString());
					Log::Trace("", __FUNCTION__, "STACKING_NO=[{0}]", tsmpe12["STACKING_NO"].ToString());
					cmd.Parameters.Set("VEHICLE_NO", tsmpe12["VEHICLE_NO"].ToString());
					Log::Trace("", __FUNCTION__, "VEHICLE_NO=[{0}]", tsmpe12["VEHICLE_NO"].ToString());
					cmd.Parameters.Set("MAT_NO", tsmpe12["MAT_NO"].ToString());
					Log::Trace("", __FUNCTION__, "MAT_NO=[{0}]", tsmpe12["MAT_NO"].ToString());
					Log::Info("", "", "sqlstr=[{0}]", sqlstr);

					cmd.ExecuteNonQuery();
					Log::Trace("", __FUNCTION__, "update tsmpe02表完成");

					sqlstr = "SELECT	* "
						"  FROM	tsmpe02 "
						"  WHERE	mat_no	=	@MAT_NO "
						;

					cmd.SetCommandText(sqlstr);
					cmd.Parameters.Clear();
					cmd.Parameters.Set("MAT_NO", tsmpe12["MAT_NO"].ToString());
					cmd.ExecuteReader();
					Log::Trace("", __FUNCTION__, "查询 tsmpe02表完成");

					if (cmd.Read())
					{
						cmd.Fetch(tsmpe02);
					}
					cmd.Close();




					if (wm_flag == "1" || wm_flag == "2")
					{
						Log::Trace("", __FUNCTION__, "开始删除tsmpe02表，材料号[{0}]", tsmpe02["MAT_NO"].ToString());
						sqlstr = "DELETE tsmpe02 "
							"  WHERE	mat_no	=	@MAT_NO "
							;

						cmd.SetCommandText(sqlstr);
						cmd.Parameters.Clear();
						cmd.Parameters.Set("MAT_NO", tsmpe12["MAT_NO"].ToString());
						cmd.ExecuteNonQuery();
						Log::Trace("", __FUNCTION__, "删除tsmpe02表成功，材料号[{0}]", tsmpe12["MAT_NO"].ToString());
					}




					/*	调用函数新增履历记录	*/
					bcls_rec->Tables[record_name].Rows[0]["mat_no"] = tsmpe02["MAT_NO"];
					bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "5";	// 发货
					bcls_rec->Tables[record_name].Rows[0]["userid"] = v_userid;
					//if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "1")
					//{
					//	bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "G";	// 转库出库
					//}

					ret = 0;
					ret = f_sm00_record(bcls_rec, bcls_ret, conn);
					if (ret < 0)
					{
						Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					/////* 转库计划，写入库队列*/
					////if ((wm_flag == "2" || wm_flag == "0") && area_code_1.Trim() != "")
					////{
					////	/*增加写入库队列 */
					////	bcls_stock_que.Tables[blk_name_wm].Rows.Add();
					////	int ii = bcls_stock_que.Tables[blk_name_wm].Rows.get_Count() - 1;
					////	bcls_stock_que.Tables[blk_name_wm].Rows[ii]["MAT_NO"] = tsmpe02["MAT_NO"];
					////	bcls_stock_que.Tables[blk_name_wm].Rows[ii]["TO_STOCK_NO"] = tsmpe10["STOCK_NO_TO"];
					////	bcls_stock_que.Tables[blk_name_wm].Rows[ii]["FROM_STOCK_NO"] = tsmpe10["STOCK_NO"];
					////	bcls_stock_que.Tables[blk_name_wm].Rows[ii]["UNIT_CODE"] = " ";
					////	bcls_stock_que.Tables[blk_name_wm].Rows[ii]["OPER_FLAG"] = "I";
					////	bcls_stock_que.Tables[blk_name_wm].Rows[ii]["STOCK_OPER_ORDER"] = "1G";
					////	bcls_stock_que.Tables[blk_name_wm].Rows[ii]["VEHICLE_NO"] = tsmpe11["VEHICLE_NO"];
					////}



					// 读取物料表上的出入库标记，不在库时，不调用仓库函数	2021-11-14
					CString table_name = "TMM" + tsmpe12["MAT_KIND"].ToString() + "01";
					CString in_flag = "";
					sqlstr = "select in_flag from " + table_name + " where mat_no = '" + tsmpe12["MAT_NO"].ToString() + "' ";
					cmd_inq.SetCommandText(sqlstr);
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					cmd_inq.ExecuteReader();
					if (cmd_inq.Read())
					{
						in_flag = cmd_inq.GetString(1);
					}
					cmd_inq.Close();
					Log::Debug("", "", "in_flag={0}", in_flag);
					if (in_flag == "1")
					{
						//调用仓库出库主函数
						//bcls_stock_out.Tables[wm_block].Rows.Clear();
						bcls_stock_out.Tables[wm_block].Rows.Add();
						int ii = bcls_stock_out.Tables[wm_block].Rows.get_Count() - 1;
						bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_NO"] = tsmpe12["MAT_NO"];
						bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_OPER_ORDER"] = "2E";
						bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_NO"] = tsmpe12["STOCK_NO"];
						bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_PLACE_NO"] = " ";
						bcls_stock_out.Tables[wm_block].Rows[ii]["LAYERNO"] = 0;
						bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_PLACE_POSITION"] = " ";
						bcls_stock_out.Tables[wm_block].Rows[ii]["CRANE_NO"] = " ";	// 吊车号
						bcls_stock_out.Tables[wm_block].Rows[ii]["VEHICLE_NO"] = tsmpe12["VEHICLE_NO"];  //车号 如果有就传
						////bcls_stock_out.Tables[wm_block].Rows[ii]["COLUMN_NO"] = " ";
						////bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_LINE_TYPE"] = tsmpe12["MAT_KIND"]; //取物料主档
						////bcls_stock_out.Tables[wm_block].Rows[ii]["MAT_KIND"] = tsmpe12["MAT_KIND"]; //取物料主档
						////bcls_stock_out.Tables[wm_block].Rows[ii]["TO_STOCK_NO"] = tsmpe10["STOCK_NO_TO"];
						if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "1" )	
						{
							bcls_stock_out.Tables[wm_block].Rows[ii]["STOCK_OPER_ORDER"] = "2G";
							bcls_stock_out.Tables[wm_block].Rows[ii]["TO_STOCK_NO"] = tsmpe10["STOCK_NO_TO"];
						}

					}

					//调用计划 单据材料计算函数
					if (CONFM_PLAN_NO != tsmpe12["CONFM_PLAN_NO"].ToString() && CONFM_PLAN_NO.Trim() != "")	// 2021-12-28
					{
						ret = f_sm00_count(CONFM_PLAN_NO, v_userid, conn);
						if (ret != 0)
						{
							throw CApplicationException(-1, s.msg, s.svc_name);
						}
					}
					CONFM_PLAN_NO = tsmpe12["CONFM_PLAN_NO"];	// 2021-12-28
					//if (tsmpe10["STOCK_NO_TO"].ToString().Trim() == "" )
					//{
					//	mat_no.push_back(tsmpe02["MAT_NO"].ToString());
					//}
					mat_no.push_back(tsmpe02["MAT_NO"].ToString());

				}
				cmd_inq2.Close();


				//发送码单电文
				bcls_rec->Tables["0"].Rows.Clear();
				bcls_rec->Tables["0"].Rows.Add();
				int jj = bcls_rec->Tables["0"].Rows.get_Count() - 1;
				bcls_rec->Tables["0"].Rows[jj]["stacking_no"] = tsmpe11["STACKING_NO"];

				CString	tc_no;
				Log::Debug("", "", "mat_kind=[{0}]", tsmpe12["MAT_KIND"].ToString());
				if (tsmpe12["MAT_KIND"].ToString() == "SM")	tc_no = "2000S4";
				if (tsmpe12["MAT_KIND"].ToString() == "BW")	tc_no = "7000S4";
				if (tsmpe12["MAT_KIND"].ToString() == "CR")	tc_no = "4000S4";
				if (tsmpe12["MAT_KIND"].ToString() == "HR")	tc_no = "3000S4";
				if (tsmpe12["MAT_KIND"].ToString() == "HP")	tc_no = "5000S4";


				// 根据发货计划电文来确定码单电文号
				tc_no = tsmpe10["REC_CREATOR"].ToString().SubstringNE(2, 2) + tsmpe10["REC_CREATOR"].ToString().SubstringNE(0, 2) + "01";


				bcls_rec->Tables["0"].Rows[jj]["tc_no"] = tc_no;
				bcls_rec->Tables["0"].Rows[jj]["oper_flag"] = "I";
				//liguangyuan 20230908 add 判定计划是L4接收的还是销售物流接收的
				if (tsmpe10["REMARK1"].ToString() == "L4")
					ret = f_xx00s4_snd(bcls_rec, bcls_ret, conn);
				else
					ret = f_xxsm01_snd(bcls_rec, bcls_ret, conn);
				if (ret != 0)
				{
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				fetchRowCount++;



				/////* 更新发货计划状态  */
				////CString table_name = "";
				////if (tsmpe10["MAT_KIND"].ToString() == "BW")	table_name = "TMMBW01";
				////if (tsmpe10["MAT_KIND"].ToString() == "SM")	table_name = "TMMSM01";
				////if (tsmpe10["MAT_KIND"].ToString() == "CR")	table_name = "TMMCR01";
				////if (tsmpe10["MAT_KIND"].ToString() == "HR")	table_name = "TMMHR01";
				////if (tsmpe10["MAT_KIND"].ToString() == "HP")	table_name = "TMMHP01";
				////if (tsmpe10["MAT_KIND"].ToString() == "BS")	table_name = "TMMBS01";
				////if (tsmpe10["MAT_KIND"].ToString() == "BT")	table_name = "TMMBT01";
				////if (tsmpe10["MAT_KIND"].ToString() == "SN")	table_name = "TMMSN01";
				////if (table_name == "")
				////{
				////	sprintf(s.msg, "物料种类不能为空 %s", (const char *)tsmpe10["MAT_KIND"].ToString());
				////	throw	CApplicationException(-1, s.msg, s.svc_name);
				////}

				////v_wt = 0.05;
				////if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" || tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2")
				////{
				////	if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2")
				////	{
				////		Log::Debug("", __FUNCTION__, "AAAAAAAAAAAA");
				////		sqlstr = "select nvl( min(a.PLAN_WT - a.DELIVY_WT - b.mat_wt ),0) "
				////			" from    tsmpe10 a , tsmpe02 b "
				////			" where   a.DELIVY_PLAN_STATUS < '5' "
				////			" and     a.PLAN_WT - a.DELIVY_WT >= 0 "
				////			" and     a.BILL_OF_LADING_NO = @c_bill_of_lading_no "
				////			" and     a.ORDER_NO = @tsmpe02.ORDER_NO "
				////			" and	  a.order_no = b.order_no ";
				////	}
				////	else
				////	{
				////		Log::Debug("", __FUNCTION__, "bbbbbbbbbbb");
				////		sqlstr = "select nvl( min(a.PLAN_WT - a.DELIVY_WT - b.mat_wt ),0) "
				////			" from    tsmpe10 a , tsmpe02 b  ," + table_name + " d "
				////			" where   a.DELIVY_PLAN_STATUS < '5' "
				////			" and     a.PLAN_WT - a.DELIVY_WT >= 0 "
				////			" and     a.SG_SIGN = b.SG_SIGN "
				////			" and     b.MAT_THICK = a.ORDER_THICK "
				////			" and     b.MAT_WIDTH = a.ORDER_WIDTH "
				////			" and     b.MAT_LEN between a.ORDER_MIN_LEN and a.ORDER_MAX_LEN "
				////			" and     a.BILL_OF_LADING_NO = @c_bill_of_lading_no "
				////			" and     a.ORDER_NO = @tsmpe02.ORDER_NO "
				////			" and     b.mat_no = d.mat_no "
				////			" and     d.psc = a.psc ";
				////	}

				////	cmd.SetCommandText(sqlstr);
				////	cmd.Parameters.Clear();
				////	cmd.Parameters.Set("tsmpe02.ORDER_NO", tsmpe11["ORDER_NO"].ToString());
				////	cmd.Parameters.Set("c_bill_of_lading_no", tsmpe11["BILL_OF_LADING_NO"].ToString());
				////	Log::Debug("", "", "sqlstr = {0}", sqlstr);
				////	cmd.ExecuteReader();
				////	if (cmd.Read())
				////	{
				////		v_wt = cmd.GetDecimal(1);
				////	}
				////	cmd.Close();
				////	if (v_wt <= 0)
				////	{
				////		Log::Debug("", __FUNCTION__, "重量=[{0}]", v_wt);
				////		sqlstr = " UPDATE TSMPE10 SET DELIVY_PLAN_STATUS = '5' "
				////			" WHERE BILL_OF_LADING_NO = @c_bill_of_lading_no "
				////			" AND ORDER_NO IN(' ', '" + tsmpe02["ORDER_NO"].ToString().Trim() + "') ";

				////		cmd.SetCommandText(sqlstr);
				////		cmd.Parameters.Clear();
				////		cmd.Parameters.Set("tsmpe02.ORDER_NO", tsmpe02["ORDER_NO"].ToString());
				////		cmd.Parameters.Set("c_bill_of_lading_no", tsmpe11["BILL_OF_LADING_NO"].ToString());
				////		Log::Debug("", "", "sqlstr = {0}", sqlstr);
				////		v_count = cmd.ExecuteNonQuery();
				////	}
				////}


			}
			cmd_inq1.Close();

			// 调用仓库函数 改到码单生成时调用
			Log::Debug("", "", "MAT_KIND=【{0}】", tsmpe12["MAT_KIND"].ToString());
#ifdef _LINE_BW
			if (tsmpe12["MAT_KIND"].ToString() == "BW")
			{
				Log::Debug("", "", "MAT_KIND=【{0}】", tsmpe12["MAT_KIND"].ToString());
				if (bcls_stock_out.Tables[wm_block].Rows.get_Count() > 0)
				{
					doFlag = f_wmbwbw_stock_out(&bcls_stock_out, bcls_ret, conn);
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
				if (bcls_stock_out.Tables[wm_block].Rows.get_Count() > 0)
				{
					doFlag = f_wmsmsm_stock_out(&bcls_stock_out, bcls_ret, conn);
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
				if (bcls_stock_out.Tables[wm_block].Rows.get_Count() > 0)
				{
					doFlag = f_wmhphp_stock_out(&bcls_stock_out, bcls_ret, conn);
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
				if (bcls_stock_out.Tables[wm_block].Rows.get_Count() > 0)
				{
					doFlag = f_wmcrcr_stock_out(&bcls_stock_out, bcls_ret, conn);
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
				if (bcls_stock_out.Tables[wm_block].Rows.get_Count() > 0)
				{
					doFlag = f_wmhrhr_stock_out(&bcls_stock_out, bcls_ret, conn);
					if (doFlag != 0)
					{
						throw CApplicationException(-1, s.msg, log.Location);
					}

				}
			}
#endif

			///* 暂时不调用 */
			//if (bcls_stock_que.Tables[blk_name_wm].Rows.get_Count() > 0)
			//{
			//	doFlag = f_wm00_queue(&bcls_stock_que, bcls_ret, conn);
			//	if (doFlag != 0)
			//	{
			//		throw CApplicationException(doFlag, s.msg, log.Location);
			//	}
			//}


			if (wm_flag == "0")
			{
				//ret = f_sm00_mm99(mat_no, 3, 6, s.msg, conn);
			}
			else if (wm_flag == "2")
			{
				ret = f_sm00_mm99(mat_no, 3, 3, s.msg, conn);
			}
			else if (wm_flag == "1")
			{
				ret = f_sm00_mm99(mat_no, 3, 3, s.msg, conn);
			}
			//else if (wm_flag == "3")
			//{
			//	ret = f_sm00_mm99(mat_no, 3, 8, s.msg, conn);
			//}

			if (ret != 0)
			{
				CFormattable	arguments[] = { s.msg };
				CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			//ret = f_xxe501_snd(bcls_rec, bcls_ret, conn);
			//if (ret != 0)
			//{
			//	throw CApplicationException(-1, s.msg, s.svc_name);
			//}


			//for (int i = 0; i < mat_no.size(); i++)
			//{
			//	tsmpe02["MAT_NO"] = mat_no[i];
			//	if (wm_flag == "0")
			//	{
			//		ret = f_sm00_mm99(tsmpe02["MAT_NO"].ToString(), 3, 6, s.msg, conn);
			//	}
			//	else if (wm_flag == "2")
			//	{
			//		//ret = f_sm00_mm99(tsmpe02["MAT_NO"].ToString(), 3, 7, s.msg, conn);
			//	}
			//	else if (wm_flag == "1")
			//	{
			//		ret = f_sm00_mm99(tsmpe02["MAT_NO"].ToString(), 3, 3, s.msg, conn);
			//	}
			//	else if (wm_flag == "3")
			//	{
			//		ret = f_sm00_mm99(tsmpe02["MAT_NO"].ToString(), 3, 8, s.msg, conn);
			//	}

			//	if (ret != 0)
			//	{
			//		CFormattable	arguments[] = { s.msg };
			//		CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
			//		throw CApplicationException(-1, s.msg, s.svc_name);
			//	}
			//}
			
			//liguangyuan 20230908 add 判定计划是L4接收的还是销售物流接收的
			if (tsmpe10["REMARK1"].ToString() == "L4")
			{

			}
			else
			{
				// 发送计划完成电文
				// 按装车单读取发货计划
				EIClass bcls_rec_fhja;
				bcls_rec_fhja.Tables[0].Columns.Add(DT_STRING, "BILL_OF_LADING_NO");
				bcls_rec_fhja.Tables[0].Columns.Add(DT_STRING, "ORDER_NO");
				bcls_rec_fhja.Tables[0].Columns.Add(DT_STRING, "OPER_FLAG");

				CString bill_of_lading_no = "";
				sqlstr = "select DISTINCT BILL_OF_LADING_NO from tsmpe11 where ticket_no = '" + ticket_no + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();

				while (cmd_inq.Read())
				{
					bill_of_lading_no = cmd_inq.GetString(1);
					// 按计划号到计划表上读取合同号，合约号
					CString order_no = "", contract_no = "", delivy_qty_flag="";
					sqlstr = "select DISTINCT ORDER_NO,CONTRACT_NO,DELIVY_QTY_FLAG from tsmpe10 where BILL_OF_LADING_NO = '" + bill_of_lading_no + "' ";
					cmd_inq1.SetCommandText(sqlstr);
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					cmd_inq1.ExecuteReader();

					while (cmd_inq1.Read())
					{
						order_no = cmd_inq1.GetString(1);
						contract_no = cmd_inq1.GetString(2);
						delivy_qty_flag = cmd_inq1.GetString(3);
						if (delivy_qty_flag == "2")	order_no = contract_no;

						bcls_rec_fhja.Tables[0].Rows.Clear();
						bcls_rec_fhja.Tables[0].Rows.Add();
						int ii = bcls_rec_fhja.Tables[0].Rows.get_Count() - 1;
						bcls_rec_fhja.Tables[0].Rows[ii]["BILL_OF_LADING_NO"] = bill_of_lading_no;
						bcls_rec_fhja.Tables[0].Rows[ii]["ORDER_NO"] = order_no;
						bcls_rec_fhja.Tables[0].Rows[ii]["OPER_FLAG"] = "I";

						ret = 0;
						ret = f_xxsm04_snd(&bcls_rec_fhja, bcls_ret, conn);
						if (ret < 0)
						{
							Log::Debug("", __FUNCTION__, "f_xxsm04_snd函数调用出错.");
							throw CApplicationException(-1, s.msg, s.svc_name);
						}

					}
					cmd_inq1.Close();
				}
				cmd_inq.Close();
			}

			//liguangyuan 20230907 add 汽运的发货确认时删除车辆信息
			if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "1")//汽运
			{
				sqlstr = " INSERT INTO HSM00B4 "
					" SELECT * FROM TSM00B4 "
					" WHERE BILL_OF_LADING_NO='" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' "
					"   AND VEHICLE_NO='" + tsmpe11["VEHICLE_NO"].ToString() + "'";
				cmd.SetCommandText(sqlstr);
				cmd.ExecuteNonQuery();

				sqlstr = " DELETE FROM TSM00B4 "
					" WHERE BILL_OF_LADING_NO='" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' "
					"   AND VEHICLE_NO='" + tsmpe11["VEHICLE_NO"].ToString() + "'";
				cmd.SetCommandText(sqlstr);
				cmd.ExecuteNonQuery();
			}

		}

		sprintf(s.msg, "共确认=[%d]个码单", fetchRowCount);
		doFlag = 0;
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, "数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。" /* _RES("GCRSS0000006")*//*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		Log::Error("", __FUNCTION__, "error=[{0}]", s.sysmsg);
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
	if (doFlag < 0)
	{
		//CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
	}

	return doFlag;
}
