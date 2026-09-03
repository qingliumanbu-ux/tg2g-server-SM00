/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: cm_0070sb_rcv
*  程序描述			: 接收L4下发出厂电文
*  备注说明			: 用于接收L4下发的出厂计划电文(0070SB)
*  修改历史			:
*  		2012-04-25 	BM2IDE			(ADD)程序建立
*			... ...
* **************************************************************************** */
/**********************************************************************
*	1.	读取计划电文头
*	2.	判提单号、计划类型、计划吊销标记是否为空，是报错
*	3.	判计划类型是2时，置按量发货标记为1，否则置0
*	4.	判计划吊销标记为2时，走吊销流程
*	4.1	循环读取传入的材料，总个数为计划头上的个数
*	4.2	判材料在发货材料表中是否存在，不存在报错，材料状态不为6时报错
*	4.3	调用物料跟踪函数，事件为提单撤销 SM05
*	4.4	更新发货材料表上的提单号为空，计划状态为‘4'
*	4.5	按材料上的准发计划号统计计划下的最大材料状态
*	4.6	更新准发单据表上的准发状态为刚才统计的材料状态
*	4.7	更新准发计划表上的计划状态、
*	4.9	更新发货计划表上的计划量，计划件数
*	4.10更新发货计划表上的计划状态为完成，条件是计划重量等于发货完成量
*	4.11删除发货计划，条件是计划量、计划件数为0
*	5.	判计划吊销标记为1时，走下发流程
***********************************************************************/
/***** C/C++ 的标准头文件部分 *****/
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"
#include "epex.h"








int f_sm00_record(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection* conn);
int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection* conn);	/* 抛物料跟踪打包函数 */

//// service入口
//BM2F_ENTERACE_TELE(cm_0070sb_rcv)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_00xxsb_rcv(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);
	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, i = 0, ret = 0;
	CString	record_name = "sm00_record";

	CString lpsz_user_id, c_datetime = s.datetime;
	CString	lpsz_out_div;
	EIClass sm_bcls_rec;

	CModel tsmpe02("TSMPE02");
	CModel tsmpe01("TSMPE01");
	CModel tsmpe00("TSMPE00");
	CModel tsmpe10("TSMPE10");

	/* ***** 电文变量定义 ***** */
	CString    c_operate_flag;
	CString    c_bill_of_lading_no;
	CString    c_plan_start_date;
	CString    c_plan_start_time;
	CString    c_mat_no;
	CDecimal   d_mat_wt = 0;
	CString		c_order_no;
	CString  table_name = "";

	/* ***** 程序变量 ***** */
	CString c_user = " ", c_mat_kind = " ", datetime = " ";
	CString    c_delivy_plan_status;

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr_1(""), sqlstr_2(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr4(""), sqlstr5(""), sqlstr6(""), sqlstr7(""), sqlstr8(""), sqlstr9("");
	CString sqlstr0("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);

	/* ***** 应用程序开始处理 ***** */
	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}


		/* ***** 获取电文号 ***** */
		c_user = s.username;

		Log::Trace("", __FUNCTION__, "电文号=[{0}]", c_user);
		/* ***** 解析电文 ***** */
		tsmpe10["DELIVY_QTY_FLAG"] = bcls_rec->Tables[0].Rows[0]["delivy_qty_flag"].ToString().TrimOrBlank();
		//tsmpe10["DELIVY_PLAN_TYPE"]       = bcls_rec->Tables[0].Rows[0]["plan_flag"].ToString().TrimOrBlank();
		tsmpe10["DELIVY_PLAN_TYPE"] = bcls_rec->Tables[0].Rows[0]["delivy_plan_type"].ToString().TrimOrBlank();
		c_operate_flag = bcls_rec->Tables[0].Rows[0]["operate_flag"].ToString().TrimOrBlank();
		c_bill_of_lading_no = bcls_rec->Tables[0].Rows[0]["bill_of_lading_no"].ToString().TrimOrBlank();
		tsmpe10["TRNP_MODE_CODE"] = bcls_rec->Tables[0].Rows[0]["trnp_mode_code"].ToString().TrimOrBlank();
		//tsmpe10["CONVEY_UNIT_NAME"]             = bcls_rec->Tables[0].Rows[0]["carry_company_name"].ToString().TrimOrBlank();
		tsmpe10["CONVEY_UNIT_NAME"] = bcls_rec->Tables[0].Rows[0]["convey_unit_name"].ToString().TrimOrBlank();
		//tsmpe10["LOADING_PLACE_NAME"]           = bcls_rec->Tables[0].Rows[0]["move_out_place"].ToString().TrimOrBlank();
		tsmpe10["LOADING_PLACE_NAME"] = bcls_rec->Tables[0].Rows[0]["loading_place_name"].ToString().TrimOrBlank();
		tsmpe10["DELIVY_PLACE_NAME"] = bcls_rec->Tables[0].Rows[0]["delivy_place_name"].ToString().TrimOrBlank();
		//tsmpe10["STOCK_NO"]                     = bcls_rec->Tables[0].Rows[0]["out_stock_code"].ToString().TrimOrBlank();
		tsmpe10["STOCK_NO"] = bcls_rec->Tables[0].Rows[0]["stock_no"].ToString().TrimOrBlank();
		//tsmpe10["PLAN_MAKER"]                   = bcls_rec->Tables[0].Rows[0]["plan_editor"].ToString().TrimOrBlank();
		tsmpe10["PLAN_MAKER"] = bcls_rec->Tables[0].Rows[0]["plan_maker"].ToString().TrimOrBlank();
		//tsmpe10["PLAN_MAKE_TIME"]               = bcls_rec->Tables[0].Rows[0]["rec_create_date"].ToString().TrimOrBlank();
		tsmpe10["PLAN_MAKE_TIME"] = bcls_rec->Tables[0].Rows[0]["plan_make_time"].ToString().TrimOrBlank();
		c_plan_start_date = bcls_rec->Tables[0].Rows[0]["plan_start_date"].ToString().TrimOrBlank();
		c_plan_start_time = bcls_rec->Tables[0].Rows[0]["plan_start_time"].ToString().TrimOrBlank();
		tsmpe10["PLAN_WT"] = bcls_rec->Tables[0].Rows[0]["plan_wt"].ToDecimal();
		tsmpe10["PLAN_NUM"] = ((CDecimal)(bcls_rec->Tables[0].Rows[0]["plan_num"])).ToInt32();
		c_order_no = bcls_rec->Tables[0].Rows[0]["order_no_erp"].ToString().TrimOrBlank();
		//	c_mat_no                       = bcls_rec->Tables[0].Rows[0]["mat_no"].ToString().TrimOrBlank();
		//	d_mat_wt                       = bcls_rec->Tables[0].Rows[0]["mat_wt"].ToDecimal();
		if (bcls_rec->Tables[0].Columns.Contains("wt_mode"))
		{
			tsmpe10["WT_MODE"] = bcls_rec->Tables[0].Rows[0]["wt_mode"];	// 计重方式
		}
		tsmpe10["PLAN_WT_D"] = tsmpe10["PLAN_WT"];
		tsmpe10["REMARK1"] = "L4";

		Log::Info("", __FUNCTION__, "tsmpe10[DELIVY_QTY_FLAG] =[{0}]", tsmpe10["DELIVY_QTY_FLAG"].ToString());
		Log::Info("", __FUNCTION__, "tsmpe10[DELIVY_PLAN_TYPE] =[{0}]", tsmpe10["DELIVY_PLAN_TYPE"].ToString());

		if (c_bill_of_lading_no.Compare(" ") == 0)
		{
			doFlag = -1;
			sprintf(s.msg, _RES("SM00S0000769")/*接收提单号为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (c_operate_flag.Compare(" ") == 0)
		{
			doFlag = -1;
			sprintf(s.msg, _RES("SM00S0000770")/*接收下发吊销标记为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (tsmpe10["DELIVY_PLAN_TYPE"].ToString().Compare(" ") == 0)
		{

			doFlag = -1;
			sprintf(s.msg, _RES("SM00S0000771")/*接收计划类型为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		Record  mat_red= Db::QueryFirst("SELECT MAT_KIND,FACTORY_DIV FROM TWM01 WHERE STOCK_NO ='" + tsmpe10["STOCK_NO"].ToString() + "'");
		tsmpe10["MAT_KIND"] = mat_red.GetCString("MAT_KIND");
		tsmpe10["FACTORY_DIV"] = mat_red.GetCString("FACTORY_DIV");
		table_name = "TMM" + tsmpe10["MAT_KIND"].ToString() + "01";

		Log::Info("", __FUNCTION__, "c_operate_flag=[{0}]", c_operate_flag);
		//*********发货计划接收*********
		if (c_operate_flag.Compare("1") == 0)
		{
			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0" || tsmpe10["DELIVY_QTY_FLAG"].ToString() == " ")	/* 按件 */
			{
				//*********发货材料进行循环处理*********
				for (i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
				{
					c_mat_no = bcls_rec->Tables[0].Rows[i]["mat_no"].ToString().TrimOrBlank();
					d_mat_wt = bcls_rec->Tables[0].Rows[i]["mat_wt"];
					Log::Info("", __FUNCTION__, "c_mat_no=[{0}]", c_mat_no);
					Log::Info("", __FUNCTION__, "d_mat_wt=[{0}]", d_mat_wt);

					if (0 == c_mat_no.Compare(" "))
					{
						break;
					}

					switch (conn->DatabaseKind)
					{
					case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
					case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
					case DB_KIND_MSSQL:	        // MS SQL Server数据库
					case DB_KIND_ORACLE:	        // Oracle 数据库
					default:
						sqlstr_1 = CString(
							" select count(1) from user_tables where table_name = @table_name "
						);

						sqlstr_2 = CString(
							" select 'SM' from "+ table_name +" where mat_no = @mat_no "
						);

						sqlstr1 = CString(
							" select count(1) from tsmpe02 where mat_no = @mat_no "
						);

						sqlstr2 = CString(
							" select * from tsmpe02 where mat_no = @mat_no "
						);

						sqlstr3 = CString(
							"  UPDATE	tsmpe02 "
							"              SET	   bill_of_lading_no = @bill_of_lading_no, "
							"                     confm_status      = '6', "
							"                     rec_revise_time   = @datetime, "
							"                     rec_revisor       = @c_user  "
							"                     WHERE	mat_no = @mat_no "
						);

						sqlstr4 = CString(
							" UPDATE	tsmpe00 a "
							"           SET confm_status	= (select nvl(max(confm_status),a.confm_status)  from tsmpe02 b where a.ready_bill_no = b.ready_bill_no ), "
							"      	     rec_revise_time        =  @datetime, "
							"      	     rec_revisor		    =  @c_user  "
							"               WHERE	ready_bill_no	= @ready_bill_no "
						);

						sqlstr5 = CString(
							" UPDATE	tsmpe01 a "
							"           SET confm_status	= (select nvl(max(confm_status),a.confm_status)  from tsmpe02 b where a.confm_plan_no = b.confm_plan_no ), "
							"      	     rec_revise_time        =  @datetime, "
							"      	     rec_revisor		    =  @c_user  "
							"               WHERE	confm_plan_no	= @confm_plan_no "
						);

						sqlstr6 = CString(
							" delete from tsmpe10 where bill_of_lading_no = @bill_of_lading_no  "
						);

						sqlstr7 = CString( /* 计算某计划下的计划量，发货量 */
							"  update  tsmpe10 a set (plan_num,plan_wt) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe02 b where a.bill_of_lading_no = b.bill_of_lading_no ), "
							"                        (delivy_num,delivy_wt ) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe12 b where a.bill_of_lading_no = b.bill_of_lading_no ) "
							"                    where bill_of_lading_no = @bill_of_lading_no "

						);

						break;
					}
					/**************** 判断材料号是否合法 **********************/
					sqlstr = sqlstr1;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					row_count = execute_sql.ExecuteScalar().ToInt32();

					if (0 == row_count)
					{
						doFlag = -1;
						sprintf(s.msg, _RES("SM00S0000781")/*接收出厂计划电文发现材料号在准发材料表中不存在.*/);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					sqlstr = sqlstr2;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					execute_sql.ExecuteReader();
					while (execute_sql.Read())
					{
						execute_sql.Fetch(tsmpe02);
					}
					execute_sql.Close();

					if (tsmpe02["CONFM_STATUS"].ToString().Compare("4") != 0)
					{
						doFlag = -1;
						sprintf(s.msg, _RES("SM00S0000782")/*材料状态不为4.*/);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe02["BILL_OF_LADING_NO"].ToString().TrimOrBlank().Compare(" ") != 0)
					{
						doFlag = -1;
						sprintf(s.msg, _RES("SM00S0000783")/*已经接受了出厂计划[{0}]，不能再次接受！*/);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					/* *****	修改准发材料表发货状态及提单号 *********************************************** */
					sqlstr = sqlstr3;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					execute_sql.ExecuteNonQuery();

					/* *****	修改准发单据的状态 *********************************************** */
					sqlstr = sqlstr4;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("ready_bill_no", tsmpe02["READY_BILL_NO"]);
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.ExecuteNonQuery();

					/* *****	修改准发计划的状态 *********************************************** */
					sqlstr = sqlstr5;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("confm_plan_no", tsmpe02["CONFM_PLAN_NO"]);
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.ExecuteNonQuery();

					/**************** 调用物料封装  函数*********************/
					if (f_sm00_mm99(c_mat_no, 3, 1, s.msg, conn) != 0)
					{
						doFlag = -1;
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					/**************** 调用物料封装  函数结束  **********************/

					bcls_rec->Tables[record_name].Rows[0]["mat_no"] = c_mat_no;
					bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "7";
					bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;

					ret = 0;
					ret = f_sm00_record(bcls_rec, bcls_ret, conn);
					if (ret < 0)
					{
						Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
				}

				/* 新增出厂计划记录 */
				tsmpe10["REC_CREATE_TIME"] = datetime;						/* 创建日期 */
				tsmpe10["REC_CREATOR"] = c_user;							/* 创建人 */
				tsmpe10["BILL_OF_LADING_NO"] = c_bill_of_lading_no;			/* 提单号 */
				//	tsmpe10.READY_BILL_NO       =   tsmpe02["READY_BILL_NO"];
				tsmpe10["MAT_KIND"] = tsmpe02["MAT_KIND"];	/* 物料区分 */
				//tsmpe10.FACTORY_DIV		    =	"BW"			;				/* 厂别区分 */
				tsmpe10["STOCK_NO"] = tsmpe10["STOCK_NO"];	/* 仓库代码 */
				tsmpe10["PLAN_MAKE_TIME"] = tsmpe10["PLAN_MAKE_TIME"];		/* 计划编程时刻 */
				tsmpe10["PLAN_MAKER"] = tsmpe10["PLAN_MAKER"];		/* 计划责任者 */
				tsmpe10["DELIVY_PLAN_STATUS"] = "3";			/* 计划状态 */
				tsmpe10["PRG_SEND_TIME"] = c_plan_start_date + c_plan_start_time;	/* 计划下达时间 */
				tsmpe10["VEHICLE_NO"] = " ";
				tsmpe10["DELIVY_PLACE_NAME"] = tsmpe10["DELIVY_PLACE_NAME"];	/* 交货地点名称 */
				tsmpe10["LOADING_PLACE_NAME"] = tsmpe10["LOADING_PLACE_NAME"];	/* 装货地点名称 */
				tsmpe10["CONSIGNE_NAME"] = " ";	/* 收货单位名称 */
				tsmpe10["CONVEY_UNIT_NAME"] = tsmpe10["CONVEY_UNIT_NAME"];	/* 承运单位名称 */
				tsmpe10["BALANCE_USER_NAME"] = " ";	/* 结算用户名称 */
				tsmpe10["TRNP_MODE_CODE"] = tsmpe10["TRNP_MODE_CODE"];	/* 运输方式代码 */
				tsmpe10["DELIVY_REMARK"] = " ";	/* 出厂备注 */
				tsmpe10["PLAN_NUM"] = tsmpe10["PLAN_NUM"];		/* 计划数量 */
				tsmpe10["PLAN_WT"] = tsmpe10["PLAN_WT"];			/* 计划重量 */
				tsmpe10["PLAN_TUBE"] = 0;						/* 计划根数 */
				tsmpe10["DELIVY_NUM"] = 0;						/* 发货数量 */
				tsmpe10["DELIVY_WT"] = 0;						/* 发货重量 */
				tsmpe10["DELIVY_TUBE"] = 0;						/* 发货根数 */
				tsmpe10["DELIVY_QTY_FLAG"] = tsmpe10["DELIVY_QTY_FLAG"];	/* 按量发货标记	*/
				/* ***** 删除原来的出厂计划 *********************************************** */
				/* 需要将表的primary key (bill_of_lading_no,ready_bill_no) 修改成primary key(bill_of_lading_no) */
				sqlstr = sqlstr6;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				execute_sql.ExecuteNonQuery();

				/* 插入计划表 */
				tsmpe10.TrimOrBlank();
				if (!tsmpe10.Insert())
				{
					doFlag = -1;
					sprintf(s.msg, "出厂计划号[%s] insert TABLE tsmpe10 失败.", (const char*)c_bill_of_lading_no);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				/* 计算该出厂计划相关的计划量 */
				sqlstr = sqlstr7;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				execute_sql.ExecuteNonQuery();


			}//if-按件

			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1")	/* 按量 */
			{

				for (i = 0; i < tsmpe10["PLAN_NUM"].ToDecimal(); i++)
				{
					/* *****11.	获得合同号 *********************************************** */
					tsmpe10["PSC"] = bcls_rec->Tables[0].Rows[i]["psc"].ToString().TrimOrBlank();		/* 产品规范码 */
					tsmpe10["ORDER_NO"] = bcls_rec->Tables[0].Rows[i]["order_no_erp"].ToString().TrimOrBlank();/* 合同号 */
					tsmpe10["SG_SIGN"] = bcls_rec->Tables[0].Rows[i]["sg_sign"].ToString().TrimOrBlank();	/* 产品规范码 */
					tsmpe10["ORDER_THICK"] = bcls_rec->Tables[0].Rows[i]["order_thick"];						/* 合同订货厚度	*/
					tsmpe10["ORDER_WIDTH"] = bcls_rec->Tables[0].Rows[i]["order_width"];						/* 合同订货宽度	*/
					tsmpe10["ORDER_MIN_LEN"] = bcls_rec->Tables[0].Rows[i]["order_min_len"];					/* 合同订货长度下限	*/
					tsmpe10["ORDER_MAX_LEN"] = bcls_rec->Tables[0].Rows[i]["order_max_len"];					/* 合同订货长度上线	*/

					if (tsmpe10["ORDER_NO"].ToString()[0] == ' ')
					{
						sprintf(s.msg, _RES("GCRSS0000026")/*合同号不能为空。*/);
						doFlag = -1;
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					if (tsmpe10["PSC"].ToString()[0] == ' ')
					{
						sprintf(s.msg, _RES("SM00S0001478")/*PSC不能为空*/);
						doFlag = -1;
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					if (tsmpe10["SG_SIGN"].ToString()[0] == ' ')
					{
						sprintf(s.msg, _RES("SM00S0001477")/*牌号不能为空*/);
						doFlag = -1;
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					Log::Info("", __FUNCTION__, "FIX_FLAG=[{0}]", tsmpe10["FIX_FLAG"].ToString());
					tsmpe10["FIX_FLAG"] = "0";
					if (tsmpe10["ORDER_MIN_LEN"].ToDecimal() != 0
						&& tsmpe10["ORDER_MAX_LEN"].ToDecimal() != 0)
					{
						if (tsmpe10["ORDER_MIN_LEN"].ToDecimal() == tsmpe10["ORDER_MAX_LEN"].ToDecimal())
						{
							tsmpe10["FIX_FLAG"] = "1";
						}
					}
					
					tsmpe10["CUST_MAT_SPECS"] = CString::Format("%g", tsmpe10["MAT_THICK"].ToDouble());
					if (tsmpe10["MAT_WIDTH"].ToDouble() != 0)
					{
						tsmpe10["CUST_MAT_SPECS"] = tsmpe10["CUST_MAT_SPECS"].ToString() + "*" + CString::Format("%g", tsmpe10["MAT_WIDTH"].ToDouble());
					}
					else
					{
						tsmpe10["CUST_MAT_SPECS"] = "φ" + tsmpe10["CUST_MAT_SPECS"].ToString();
					}
					if (tsmpe10["MAT_LEN"].ToDouble() != 0)
					{
						tsmpe10["CUST_MAT_SPECS"] = tsmpe10["CUST_MAT_SPECS"].ToString() + "*" + CString::Format("%g", tsmpe10["MAT_LEN"].ToDouble());
					}


					/* 新增出厂计划记录 */
					tsmpe10["REC_CREATE_TIME"] = datetime;						/* 创建日期 */
					tsmpe10["REC_CREATOR"] = c_user;							/* 创建人 */
					tsmpe10["BILL_OF_LADING_NO"] = c_bill_of_lading_no;			/* 提单号 */
					//tsmpe10.READY_BILL_NO       =   tsmpe02["READY_BILL_NO"];
					//tsmpe10["MAT_KIND"] = "BW";	/* 物料区分 */
					//tsmpe10.FACTORY_DIV		    =	"BW"			;				/* 厂别区分 */
					tsmpe10["STOCK_NO"] = tsmpe10["STOCK_NO"];	/* 仓库代码 */
					tsmpe10["PLAN_MAKE_TIME"] = tsmpe10["PLAN_MAKE_TIME"];		/* 计划编程时刻 */
					tsmpe10["PLAN_MAKER"] = tsmpe10["PLAN_MAKER"];		/* 计划责任者 */
					tsmpe10["DELIVY_PLAN_STATUS"] = "3";			/* 计划状态 */
					tsmpe10["PRG_SEND_TIME"] = c_plan_start_date + c_plan_start_time;	/* 计划下达时间 */
					tsmpe10["VEHICLE_NO"] = " ";
					tsmpe10["DELIVY_PLACE_NAME"] = tsmpe10["DELIVY_PLACE_NAME"];	/* 交货地点名称 */
					tsmpe10["LOADING_PLACE_NAME"] = tsmpe10["LOADING_PLACE_NAME"];	/* 装货地点名称 */
					tsmpe10["CONSIGNE_NAME"] = " ";	/* 收货单位名称 */
					tsmpe10["CONVEY_UNIT_NAME"] = tsmpe10["CONVEY_UNIT_NAME"];	/* 承运单位名称 */
					tsmpe10["BALANCE_USER_NAME"] = " ";	/* 结算用户名称 */
					tsmpe10["TRNP_MODE_CODE"] = tsmpe10["TRNP_MODE_CODE"];	/* 运输方式代码 */
					tsmpe10["DELIVY_REMARK"] = " ";	/* 出厂备注 */
					tsmpe10["PLAN_NUM"] = tsmpe10["PLAN_NUM"];		/* 计划数量 */
					tsmpe10["PLAN_WT"] = tsmpe10["PLAN_WT"];			/* 计划重量 */
					tsmpe10["PLAN_TUBE"] = 0;						/* 计划根数 */
					tsmpe10["DELIVY_NUM"] = 0;						/* 发货数量 */
					tsmpe10["DELIVY_WT"] = 0;						/* 发货重量 */
					tsmpe10["DELIVY_TUBE"] = 0;						/* 发货根数 */
					tsmpe10["DELIVY_QTY_FLAG"] = tsmpe10["DELIVY_QTY_FLAG"];	/* 按量发货标记	*/
					tsmpe10["PSC"] = tsmpe10["PSC"];	/* 产品规范码	*/
					tsmpe10["SG_SIGN"] = tsmpe10["SG_SIGN"];	/* 牌号			*/
					tsmpe10["ORDER_THICK"] = tsmpe10["ORDER_THICK"];	/* 订货厚度		*/
					tsmpe10["ORDER_WIDTH"] = tsmpe10["ORDER_WIDTH"];	/* 订货宽度		*/
					tsmpe10["ORDER_MIN_LEN"] = tsmpe10["ORDER_MIN_LEN"];	/* 订货最小长度 */
					tsmpe10["ORDER_MAX_LEN"] = tsmpe10["ORDER_MAX_LEN"];	/* 订货最大长度 */
					tsmpe10.TrimOrBlank();

					/* 插入计划表 */
					if (!tsmpe10.Insert())
					{
						doFlag = -1;
						sprintf(s.msg, "出厂计划号[%s] insert TABLE tsmpe10 失败.", (const char*)c_bill_of_lading_no);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
				}//for
			}//if-按量

		}//if-提单接收

		//*********出厂计划吊销*********
		if (c_operate_flag.Compare("0") == 0)
		{
			switch (conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr = CString(
					" select delivy_plan_status from tsmpe10 where bill_of_lading_no = @bill_of_lading_no "
				);
				break;
			}
			//判断棒线主表中是否存在
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
			execute_sql.ExecuteReader();
			while (execute_sql.Read())
			{
				c_delivy_plan_status = execute_sql.GetString(1);
			}
			execute_sql.Close();
			if (c_delivy_plan_status != "3")
			{
				{
					CFormattable arguments[] = { c_bill_of_lading_no , c_delivy_plan_status };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000908")/*发货计划号[{0}]的状态不是释放状态[{1}]*/, arguments, 2);//格式化字符串
				}
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0")	/* 按件 */
			{
				switch (conn->DatabaseKind)
				{
				case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
				case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
				case DB_KIND_MSSQL:	        // MS SQL Server数据库
				case DB_KIND_ORACLE:	        // Oracle 数据库
				default:
					sqlstr0 = CString(
						" select count(1) from "+ table_name +" where mat_no = @mat_no "
					);


					sqlstr1 = CString(
						" select count(1) from tsmpe02 where mat_no = @mat_no "
					);

					sqlstr2 = CString(
						" SELECT * FROM tsmpe02 WHERE mat_no = @mat_no "
					);

					sqlstr3 = CString(
						"  UPDATE	tsmpe02 "
						"              SET	   bill_of_lading_no = ' ', "
						"                     confm_status      = '4', "
						"                     rec_revise_time   = @datetime, "
						"                     rec_revisor       = @c_user  "
						"                     WHERE	mat_no = @mat_no "
					);

					sqlstr4 = CString(
						" UPDATE	tsmpe00 a "
						"           SET confm_status	= (select nvl(max(confm_status),a.confm_status)  from tsmpe02 b where a.ready_bill_no = b.ready_bill_no ), "
						"      	     rec_revise_time        =  @datetime, "
						"      	     rec_revisor		    =  @c_user  "
						"               WHERE	ready_bill_no	= @ready_bill_no "
					);

					sqlstr5 = CString(
						" UPDATE	tsmpe01 a "
						"           SET confm_status	= (select nvl(max(confm_status),a.confm_status)  from tsmpe02 b where a.confm_plan_no = b.confm_plan_no ), "
						"      	     rec_revise_time        =  @datetime, "
						"      	     rec_revisor		    =  @c_user  "
						"               WHERE	confm_plan_no	= @confm_plan_no "
					);

					sqlstr6 = CString(
						" delete from tsmpe10 where bill_of_lading_no = @bill_of_lading_no  "
					);

					sqlstr7 = CString( /* 计算某计划下的计划量，发货量 */
						"  update  tsmpe10 a set rec_revise_time        =  @datetime, "
						"     	                   rec_revisor		  =  @c_user, "
						"                        (plan_num,plan_wt) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe02 b where a.bill_of_lading_no = b.bill_of_lading_no ), "
						"                        (delivy_num,delivy_wt ) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe12 b where a.bill_of_lading_no = b.bill_of_lading_no ) "
						"                    where bill_of_lading_no = @bill_of_lading_no "

					);

					sqlstr8 = CString( /* 计划量 = 发货量，状态置为 ‘5’ */
						"  update  tsmpe10 a set DELIVY_PLAN_STATUS = '5' "
						"                    where bill_of_lading_no = @bill_of_lading_no "
						"                      and plan_num = delivy_num "

					);

					sqlstr9 = CString( /* 如果计划量为0，则删除该记录  */
						"  delete from  tsmpe10 a   "
						"                    where bill_of_lading_no = @bill_of_lading_no "
						"                      and plan_num = 0 "

					);

					break;
				}

				//*********发货材料进行循环处理*********
				for (i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
				{

					c_mat_no = bcls_rec->Tables[0].Rows[i]["mat_no"].ToString().TrimOrBlank();

					if (0 == c_mat_no.Compare(" "))
					{
						break;
					}
					//判断棒线主表中是否存在
					sqlstr = sqlstr0;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("mat_no", c_mat_no);

					row_count = execute_sql.ExecuteScalar().ToInt32();
					if (row_count <= 0)
					{
						doFlag = -1;
						Log::Trace("", __FUNCTION__, "材料号[{0}]在主档中不存在", c_mat_no);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					/**************** 判断材料号是否合法 **********************/
					sqlstr = sqlstr1;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					row_count = execute_sql.ExecuteScalar().ToInt32();

					if (0 == row_count)
					{
						doFlag = -1;
						sprintf(s.msg, _RES("SM00S0000781")/*接收出厂计划电文发现材料号在准发材料表中不存在.*/);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					sqlstr = sqlstr2;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					execute_sql.ExecuteReader();
					while (execute_sql.Read())
					{
						execute_sql.Fetch(tsmpe02);
					}
					execute_sql.Close();

					/* *****	修改准发材料表发货状态 *********************************************** */
					sqlstr = sqlstr3;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					execute_sql.ExecuteNonQuery();

					/* *****	修改准发单据的状态 *********************************************** */
					sqlstr = sqlstr4;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("ready_bill_no", tsmpe02["READY_BILL_NO"]);
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.ExecuteNonQuery();

					/* *****	修改准发计划的状态 *********************************************** */
					sqlstr = sqlstr5;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("confm_plan_no", tsmpe02["CONFM_PLAN_NO"]);
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.ExecuteNonQuery();

					/* 计算该出厂计划相关的计划量 */
					sqlstr = sqlstr7;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.ExecuteNonQuery();

					sqlstr = sqlstr8;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					execute_sql.ExecuteNonQuery();

					sqlstr = sqlstr9;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					execute_sql.ExecuteNonQuery();

					/**************** 调用物料封装  函数*********************/
					if (f_sm00_mm99(c_mat_no, 3, -1, s.msg, conn) != 0)
					{
						doFlag = -1;
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					/**************** 调用物料封装  函数结束  **********************/

					bcls_rec->Tables[record_name].Rows[0]["mat_no"] = c_mat_no;
					bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "8";
					bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;

					ret = 0;
					ret = f_sm00_record(bcls_rec, bcls_ret, conn);
					if (ret < 0)
					{
						Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
				}
			}//if-按件
			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1")	/* 按量 */
			{
				switch (conn->DatabaseKind)
				{
				case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
				case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
				case DB_KIND_MSSQL:	        // MS SQL Server数据库
				case DB_KIND_ORACLE:	        // Oracle 数据库
				default:
					sqlstr9 = CString( /* 如果计划量为0，则删除该记录  */
						"  delete from  tsmpe10 a   "
						"                    WHERE bill_of_lading_no = @bill_of_lading_no "
						"					   AND ORDER_NO = @order_no "
						"                      AND DELIVY_NUM = 0 "
						"                      AND DELIVY_WT = 0 "
					);

					break;
				}
				sqlstr = sqlstr9;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				execute_sql.Parameters.Set("order_no", c_order_no);
				execute_sql.ExecuteNonQuery();

			}//if-按量
		}//提单吊销	 
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{

		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚

	}
	catch (const CApplicationException& ex)
	{
		//	strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		Log::Error("", __FUNCTION__, "error=[{0}]", s.msg);
		doFlag = -1;
	}

	catch (const CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1;
	}

	return doFlag;
}
