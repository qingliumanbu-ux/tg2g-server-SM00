/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      013801
Version:     1.0
Date:        2023-1-3 14:32:11
Description: 发货/转库计划电文接收
**************************************************/
/**********************************************************************
*	1.	读取计划电文头
*	2.	对传入的数据进行检查
*	3.	操作标记为0 ，计划下发
*	3.1	按件计划，循环读取计划下材料，更新准发材料表上的计划号
*	3.2	调用发货履历函数，计划接收
*	3.3	调用物料跟踪履历函数
*	3.4	写发货计划表 TSMPE10
*	3.5	按量计划，循环读取计划下的合同，新增发货计划表，计划状态为‘3’
*	3,6	按合约计划，检查是否厚板铁运计划，是新增发货计划表，计划状态为‘3’
*	4.	操作标记为 E ,X 时 置计划强制完成
*	4.1	按件计划，记履历，调用物料跟踪，材料上计划号删除，发货计划置结案
*	4.2	按量计划，计划置完成
*	5	操作标记为 A ，追加按量发货的计划合同
*	6	操作标记为 D ，删除按量发货的计划合同
*	7	操作标记为 U ，更新按量发货标记为‘2’的合约号，计划量
***********************************************************************/
/***** C/C++ 的标准头文件部分 *****/
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"
#include "epex.h"









int f_sm00_record(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection * conn);
int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int f_sm00_mm99(vector <CString> pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */


BM2_FUNCTION_EXPORT


int f_smxx01_rcv(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
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
	CModel tom01("TOM01");

	/* ***** 电文变量定义 ***** */
	CString    c_operate_flag;	// 【0-下发，X-作废，E-结案，A-合同项次追加，D-合同项次删除，U-修改(量,合约号)】
	CString    c_bill_of_lading_no;	//提单号
	CString    c_plan_start_date;
	CString    c_plan_start_time;
	CString    c_mat_no;
	CDecimal   d_mat_wt = 0;
	CString		c_order_no, c_order_no1;

	CDecimal	sum_mat_wt = 0;	// 合计材料重量
	int		sum_mat_num = 0;	// 合计材料件数

	/* ***** 程序变量 ***** */
	CString c_user = " ", c_mat_kind = " ", datetime = " ";
	CString    c_delivy_plan_status;
	CString	block_name_master = "BODY";
	CString	block_name_detail = "DETAIL";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);
	vector <CString> mat_no;

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
		c_operate_flag = bcls_rec->Tables[block_name_master].Rows[0]["OPER_FLAG"].ToString().TrimOrBlank();		// 操作标志 0--下发，D--材料删除，X--作废，E--结案
		tsmpe10["DELIVY_PLAN_TYPE"] = bcls_rec->Tables[block_name_master].Rows[0]["DELIVY_PLAN_TYPE"].ToString().TrimOrBlank();	// 计划类型 0--出厂，1--内部转库，2--转厂外库,3--领用计划（同0，销售要区分）
		tsmpe10["DELIVY_QTY_FLAG"] = bcls_rec->Tables[block_name_master].Rows[0]["DELIVY_FLAG"].ToString().TrimOrBlank();	// 按量发货标记 0--合同，1--按量(品规)，2--统货，3--材料，4--合约
		tsmpe10["BILL_OF_LADING_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["BILL_OF_LADING_NO"].ToString().TrimOrBlank();// 提单号
		tsmpe10["PLAN_WT"] = bcls_rec->Tables[block_name_master].Rows[0]["PLAN_WT"].ToDecimal();						// 计划重量
		tsmpe10["PLAN_NUM"] = ((CDecimal)(bcls_rec->Tables[block_name_master].Rows[0]["PLAN_NUM"])).ToInt32();	// 计划件数
		tsmpe10["DELIVY_PLACE_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["DELIVY_PLACE_CODE"].ToString();	// 交货地点代码
		tsmpe10["DELIVY_PLACE_NAME"] = bcls_rec->Tables[block_name_master].Rows[0]["DELIVY_PLACE_NAME"].ToString().TrimOrBlank();// 交货地点名称
		tsmpe10["CONSIGN_USER_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["CONSIGN_USER_CODE"].ToString();	// 收货用户代码
		tsmpe10["CONSIGN_DEPT_CNAME"] = bcls_rec->Tables[block_name_master].Rows[0]["CONSIGN_DEPT_CNAME"].ToString().TrimOrBlank();		// 收货单位
		tsmpe10["CONSIGNE_NAME"] = bcls_rec->Tables[block_name_master].Rows[0]["CONSIGNE_NAME"].ToString().TrimOrBlank();		// 收货地址
		tsmpe10["CONTACT_PERSON"] = bcls_rec->Tables[block_name_master].Rows[0]["CONTACT_PERSON"].ToString().TrimOrBlank();		// 联系人
		tsmpe10["CONTACT_PHONE"] = bcls_rec->Tables[block_name_master].Rows[0]["CONTACT_PHONE"].ToString().TrimOrBlank();// 客户联系电话
		tsmpe10["BILL_TITLE_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["BILL_TITLE_CODE"].ToString().TrimOrBlank();	// 发票抬头单位代码
		tsmpe10["LABEL_NUM"] = bcls_rec->Tables[block_name_master].Rows[0]["LABEL_NUM"].ToString().TrimOrBlank();	// 发票抬头单位名称
		tsmpe10["ORDER_CUST_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["ORDER_CUST_CODE"].ToString();	// 订货用户代码
		tsmpe10["ORDER_CUST_CNAME"] = bcls_rec->Tables[block_name_master].Rows[0]["ORDER_CUST_CNAME"].ToString();	/* 订货用户名称 */
		tsmpe10["BALANCE_USER_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["BALANCE_USER_CODE"].ToString();	// 结算用户代码
		tsmpe10["BALANCE_USER_NAME"] = bcls_rec->Tables[block_name_master].Rows[0]["BALANCE_USER_NAME"].ToString();	/* 结算用户名称 */
		tsmpe10["SHIP_LOT_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["SHIP_LOT_NO"].ToString();	/* 船批号 */
		tsmpe10["VEHICLE_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["VEHICLE_NO"].ToString().TrimOrBlank();	// 车船号
		tsmpe10["CARRY_COMPANY_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["CARRY_COMPANY_CODE"].ToString();	// 承运公司代码
		tsmpe10["CARRY_COMPANY_NAME"] = bcls_rec->Tables[block_name_master].Rows[0]["CARRY_COMPANY_NAME"].ToString().TrimOrBlank();	// 承运单位名称
		tsmpe10["PRIVATE_ROUTE_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["PRIVATE_ROUTE_CODE"].ToString();	// 专用线代码
		tsmpe10["PRIVATE_ROUTE_NAME"] = bcls_rec->Tables[block_name_master].Rows[0]["PRIVATE_FULL_NAME"].ToString();	// 专用线名称
		tsmpe10["EXPORT_FLAG"] = bcls_rec->Tables[block_name_master].Rows[0]["EXPORT_FLAG"].ToString();	// 内外销标记
		tsmpe10["MAT_KIND"] = bcls_rec->Tables[block_name_master].Rows[0]["MAT_KIND"].ToString();	// 物料种类
		tsmpe10["STOCK_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["OUT_STOCK_CODE"].ToString().TrimOrBlank();			// 库区代码
		tsmpe10["STOCK_NO_TO"] = bcls_rec->Tables[block_name_master].Rows[0]["IN_STOCK_CODE"].ToString().TrimOrBlank();		// 入库库区
		tsmpe10["PLAN_START_TIME"] = bcls_rec->Tables[block_name_master].Rows[0]["PLAN_START_TIME"].ToString();	// 计划开始日期
		tsmpe10["PLAN_END_TIME"] = bcls_rec->Tables[block_name_master].Rows[0]["PLAN_END_TIME"].ToString();	// 计划结束日期
		tsmpe10["PRG_SEND_TIME"] = bcls_rec->Tables[block_name_master].Rows[0]["PRG_SEND_TIME"].ToString();	/* 计划下达时间 */
		tsmpe10["PLAN_MAKER"] = bcls_rec->Tables[block_name_master].Rows[0]["PLAN_MAKER"].ToString().TrimOrBlank();	// 计划编制人
		tsmpe10["TRNP_MODE_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["TRNP_MODE_CODE"].ToString().TrimOrBlank();	// 运输方式
		tsmpe10["DELIVY_REMARK"] = bcls_rec->Tables[block_name_master].Rows[0]["DELIVY_REMARK"].ToString();	// 计划备注
		tsmpe10["REMARK"] = bcls_rec->Tables[block_name_master].Rows[0]["REMARK"].ToString();	// 计划备注2
		tsmpe10["SHARE_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["SHARE_NO"].ToString();	// 拼车单号
		tsmpe10["DELIVY_LEVEL"] = bcls_rec->Tables[block_name_master].Rows[0]["LEVEL"].ToString();	// 优先级
		tsmpe10["DELIVY_ENTERPORT_CODE"] = bcls_rec->Tables[block_name_master].Rows[0]["DELIVY_ENTERPORT_CODE"].ToString();	// 中转港代码
		tsmpe10["DELIVY_ENTERPORT_NAME"] = bcls_rec->Tables[block_name_master].Rows[0]["DELIVY_ENTERPORT_NAME"].ToString();	// 中转港名称


		if (tsmpe10["SHARE_NO"].ToString().Trim() == "")
		{
			tsmpe10["SHARE_NO"] = tsmpe10["BILL_OF_LADING_NO"];
		}
		tsmpe10["PLAN_GROUP"] = tsmpe10["SHARE_NO"];	// 计划组合码
		// -------汽运司机信息-------
		//if (bcls_rec->Tables[block_name_master].Rows[0]["VEHICLE_NO1"].ToString().Trim() != "")
		//{
		//	tsmpe10["VEHICLE_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["VEHICLE_NO1"].ToString();	// 卡车号
		//}
		tsmpe10["VEHICLE_TYPE"] = bcls_rec->Tables[block_name_master].Rows[0]["VEHICLE_TYPE"].ToString();	// 车辆类型
		tsmpe10["VEHICLE_LOAD_WT"] = bcls_rec->Tables[block_name_master].Rows[0]["VEHICLE_LOAD_WT"].ToDecimal();	// 载重
		tsmpe10["DRIVER_NAME"] = bcls_rec->Tables[block_name_master].Rows[0]["DRIVER_NAME"].ToString();	// 司机姓名
		tsmpe10["ID_CARD"] = bcls_rec->Tables[block_name_master].Rows[0]["DRIVER_ID_NUM"].ToString();	// 身份证
		tsmpe10["DRIVER_TEL"] = bcls_rec->Tables[block_name_master].Rows[0]["DRIVER_TEL"].ToString();	// 联系电话
		tsmpe10["CONTRACT_NO"] = bcls_rec->Tables[block_name_master].Rows[0]["CONTRACT_NO"].ToString();	// 合约号

		// 循环
		//tsmpe10["ORDER_NO"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_NO"].ToString().TrimOrBlank();	// 合同号
		//tsmpe10.MAT_NO = bcls_rec->Tables[block_name_detail].Rows[i]["CUST_MAT_NO"].ToString().TrimOrBlank();	// 材料号
		//tsmpe10["PLAN_WT_D"] = bcls_rec->Tables[block_name_detail].Rows[i]["PLAN_WT_D"].ToDecimal();	// 合同计划量
		//tsmpe10["ORDER_THICK"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_THICK"].ToDecimal();	// 厚度直径
		//tsmpe10["ORDER_WIDTH"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_WIDTH"].ToDecimal();	// 宽度
		//tsmpe10["ORDER_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN"].ToDecimal();	// 长度
		//tsmpe10["ORDER_MIN_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN_MIN"].ToDecimal();	//最小长度
		//tsmpe10["ORDER_MAX_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN_MAX"].ToDecimal();	//最大长度
		//tsmpe10["CROSS_CODE"] = bcls_rec->Tables[block_name_detail].Rows[i]["CROSS_CODE"].ToString();	// 截面代码
		//tsmpe10["PSC"] = bcls_rec->Tables[block_name_detail].Rows[i]["PSC"].ToString().TrimOrBlank();	// 产品规范码
		//tsmpe10["MSC"] = bcls_rec->Tables[block_name_detail].Rows[i]["MSC"].ToString().TrimOrBlank();	// 冶金规范码
		//tsmpe10["SG_SIGN"] = bcls_rec->Tables[block_name_detail].Rows[i]["SG_SIGN"].ToString().TrimOrBlank();	// 牌号
		//tsmpe10["SG_STD"] = bcls_rec->Tables[block_name_detail].Rows[i]["SG_STD"].ToString().TrimOrBlank();	// 标准
		//tsmpe10["WT_MODE"] =bcls_rec->Tables[block_name_detail].Rows[i]["WT_METHOD"].ToString().TrimOrBlank();	// 计重方式
		//tsmpe10["FIX_FLAG"] = bcls_rec->Tables[block_name_detail].Rows[i]["FIX_FLAG"].ToString();	// 定尺标记
		//tsmpe10["RAIN_COAT_FLAG"] = bcls_rec->Tables[block_name_detail].Rows[i]["RAIN_COAT_FLAG"].ToString().TrimOrBlank();	// 加盖雨布标志(苫盖)

		

		/* 读取数据的合理性校验 */
		c_order_no = tsmpe10["ORDER_NO"];
		c_bill_of_lading_no = tsmpe10["BILL_OF_LADING_NO"];
		int plan_num_count = tsmpe10["PLAN_NUM"].ToDecimal().ToInt32();

		Log::Info("", __FUNCTION__, "DELIVY_QTY_FLAG=[{0}]", tsmpe10["DELIVY_QTY_FLAG"].ToString());
		Log::Info("", __FUNCTION__, "DELIVY_PLAN_TYPE=[{0}]", tsmpe10["DELIVY_PLAN_TYPE"].ToString());


		if (c_bill_of_lading_no.Trim() == "")
		{
			sprintf(s.msg, _RES("SM00S0000769")/*接收提单号为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		Log::Info("", __FUNCTION__, "c_operate_flag=[{0}]", c_operate_flag);

		if (c_operate_flag.Trim() != "0"	// 计划接收
			&& c_operate_flag.Trim() != "A"	// 合同项次追加
			&& c_operate_flag.Trim() != "D"	// 合同项次删除
			&& c_operate_flag.Trim() != "X"	// 作废 
			&& c_operate_flag.Trim() != "E"	// 结案
			&& c_operate_flag.Trim() != "U"	// 修改（量、合约号）
			)
		{
			doFlag = -1;
			sprintf(s.msg, "接收下发标记错【0-下发，X-作废，E-结案，A-合同项次追加，D-合同项次删除，U-修改(量,合约号)】");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}


		// 计划类型检查
		if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() != "0" 
			&& tsmpe10["DELIVY_PLAN_TYPE"].ToString() != "1"
			&& tsmpe10["DELIVY_PLAN_TYPE"].ToString() != "2"
			&& tsmpe10["DELIVY_PLAN_TYPE"].ToString() != "3")
		{
			sprintf(s.msg, "接收计划类型不为0或2,3");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		// 计划类型转换
		if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "0" || tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "3")
		{
			tsmpe10["DELIVY_PLAN_TYPE"] = "0";	// 出厂计划
		}
		else if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "2" || tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "1")
		{
			tsmpe10["DELIVY_PLAN_TYPE"] = "1";	// 转库计划
		}


		if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "1"  && tsmpe10["STOCK_NO_TO"].ToString().Trim() == "")
		{
			sprintf(s.msg, "转库计划必须有入库库区");
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}

		if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "0" && tsmpe10["STOCK_NO_TO"].ToString().Trim() != "")
		{
			sprintf(s.msg, "出厂计划不应该有入库库区");
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}

		if (tsmpe10["STOCK_NO_TO"].ToString().GetLength() > 3)
		{
			tsmpe10["STOCK_NO_TO"] = "XXX";
		}


		if (tsmpe10["DELIVY_QTY_FLAG"].ToString().Trim() != "0" 
			&& tsmpe10["DELIVY_QTY_FLAG"].ToString().Trim() != "1" 
//			&& tsmpe10["DELIVY_QTY_FLAG"].ToString().Trim() != "2"
			&& tsmpe10["DELIVY_QTY_FLAG"].ToString().Trim() != "3"
			&& tsmpe10["DELIVY_QTY_FLAG"].ToString().Trim() != "4")
		{
			sprintf(s.msg, "按量发货标记出错[%s]，不为0,1,2,3,4", (const char *)tsmpe10["DELIVY_QTY_FLAG"].ToString());
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		// 按量发货标记转换
		if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0")
		{
			tsmpe10["DELIVY_QTY_FLAG"] = "2";	// 合同
		}
		else if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1")
		{
			tsmpe10["DELIVY_QTY_FLAG"] = "1";	// 按钢种规格拣配
		}
		else if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "3")
		{
			tsmpe10["DELIVY_QTY_FLAG"] = "0";	// 按材料
		}
		else if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "4")
		{
			tsmpe10["DELIVY_QTY_FLAG"] = "3";	// 按合约
		}

		//物料种类转换
		CString c_mat_kind = tsmpe10["MAT_KIND"].ToString();
		if (c_mat_kind == "B" || c_mat_kind == "C" || c_mat_kind == "D" || c_mat_kind == "E") tsmpe10["MAT_KIND"] = "BW";
		if (c_mat_kind == "A") tsmpe10["MAT_KIND"] = "SM";
		if (c_mat_kind == "J") tsmpe10["MAT_KIND"] = "HP";

		// 物料种类检查
		if (tsmpe10["MAT_KIND"].ToString().Trim() != "SM" 
			&& tsmpe10["MAT_KIND"].ToString().Trim() != "BW" 
			&& tsmpe10["MAT_KIND"].ToString().Trim() != "HP"
			&& tsmpe10["MAT_KIND"].ToString().Trim() != "HR"
			&& tsmpe10["MAT_KIND"].ToString().Trim() != "CR")
		{
			sprintf(s.msg, "物料种类不正确[%s]", (const char *)tsmpe10["MAT_KIND"].ToString());
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		// 库区代码检查
		if (tsmpe10["STOCK_NO"].ToString().Trim() == "")
		{
			sprintf(s.msg, "库区代码不能为空[%s]", (const char *)tsmpe10["STOCK_NO"].ToString());
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		sqlstr = "select FACTORY_DIV from tsi0021 where STOCK_ADDR = '" + tsmpe10["STOCK_NO"].ToString() + "' ";
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			tsmpe10["FACTORY_DIV"] = cmd_inq.GetString(1);
		}
		else
		{
			sprintf(s.msg, "无此库区代码的配置[%s]", (const char *)tsmpe10["STOCK_NO"].ToString());
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		cmd_inq.Close();

		// 汽运时检查车号
		if ((tsmpe10["TRNP_MODE_CODE"].ToString() == "11" || tsmpe10["TRNP_MODE_CODE"].ToString() == "21" )
			&& c_operate_flag.Trim() == "0")
		{
			if (tsmpe10["VEHICLE_NO"].ToString().Trim() == "")
			{
				sprintf(s.msg, "汽运计划车号不能为空；");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}

		Log::Info("", __FUNCTION__, "c_operate_flag=[{0}]", c_operate_flag);
		CString mark_flag = "";
		CString c_ver_hor_flag = "";	// 立卧标记
		//*********发货计划接收*********
		if (c_operate_flag.Compare("0") == 0)	// 计划下发接收
		{
			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0")	/* 按件 */
			{
				//*********发货材料进行循环处理*********
				for (i = 0; i < plan_num_count; i++)
				{
					c_mat_no = bcls_rec->Tables[block_name_detail].Rows[i]["CUST_MAT_NO"].ToString().TrimOrBlank();
					c_order_no1 = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_NO"].ToString().TrimOrBlank();
					d_mat_wt = bcls_rec->Tables[block_name_detail].Rows[i]["PLAN_WT_D"].ToDecimal();	// 合同计划量
					//c_ver_hor_flag = bcls_rec->Tables[block_name_detail].Rows[i]["VER_HOR_FLAG"].ToString();	// 立卧标记
					Log::Info("", __FUNCTION__, "c_mat_no=[{0}],i=[{1}]", c_mat_no, i);
					Log::Info("", __FUNCTION__, "c_order_no=[{0}]],i=[{1}]", c_order_no, i);
					Log::Info("", __FUNCTION__, "d_mat_wt=[{0}]],i=[{1}]", d_mat_wt, i);

					if (0 == c_mat_no.Compare(" "))
					{
						//break;
					}


					/**************** 判断材料号是否合法 **********************/
					sqlstr = CString(" select * from tsmpe02 where mat_no = @mat_no ");
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					execute_sql.ExecuteReader();
					if (execute_sql.Read())
					{
						execute_sql.Fetch(tsmpe02);
					}
					else
					{
						CFormattable	arguments[] = { c_mat_no, tsmpe10["BILL_OF_LADING_NO"].ToString() };
						CMessageFormat::Format(s.msg, "接收出厂计划{1}电文发现材料号{0}在准发材料表中不存在", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					execute_sql.Close();

					if (tsmpe02["BILL_OF_LADING_NO"].ToString().TrimOrBlank().Compare(" ") != 0)
					{
						if (tsmpe02["BILL_OF_LADING_NO"].ToString() == tsmpe10["BILL_OF_LADING_NO"].ToString())
						{
							mark_flag = "1";
							continue;
						}
						CFormattable	arguments[] = { c_mat_no, tsmpe02["BILL_OF_LADING_NO"].ToString(), tsmpe10["BILL_OF_LADING_NO"].ToString() };
						CMessageFormat::Format(s.msg, "接收出厂计划{2}电文发现材料号{0}已经在发货计划{1}中", arguments, 3);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe02["CONFM_STATUS"].ToString().Compare("4") != 0)
					{
						sprintf(s.msg, "材料号[%s]材料状态[%s]不为准发确认", (const char *)tsmpe02["MAT_NO"].ToString(), (const char *)tsmpe02["CONFM_STATUS"].ToString());
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe02["ORDER_NO"].ToString() != c_order_no1 /*&& tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "0"*/)
					{
						CFormattable arguments[] = { tsmpe02["ORDER_NO"].ToString(), c_order_no1, tsmpe02["MAT_NO"].ToString() };// 定义参数列表的数组
						CMessageFormat::Format(s.msg, "计划上的合同号[{1}]和材料上的合同号[{0}]不一致,材料号=[{2}]", arguments, 3);



						// 更新材料表上的合同号(充当合同） 2021-9-6
						sqlstr = "select * from tom01 where order_no = '" + c_order_no1 + "' ";
						cmd_inq.SetCommandText(sqlstr);
						Log::Debug("", "", "sqlstr={0}", sqlstr);
						cmd_inq.ExecuteReader();
						if (cmd_inq.Read())
						{
							cmd_inq.Fetch(tom01);
						}
						else
						{
							sprintf(s.msg, "无此合同号%s", (const char *)c_order_no1);
							throw CApplicationException(-1, s.msg, s.svc_name);
						}
						cmd_inq.Close();

						tsmpe10["PROD_CODE"] = tom01["PROD_CODE"].ToString();
						tsmpe10["PROD_CNAME"] = tom01["PROD_CNAME"].ToString();

					////	if (tom01["ORDER_TYPE_CODE"].ToString() =="CZA")  // 充当合同
					////	{
					////		tsmpe02.TRNP_MODE_CODE = tom01["TRNP_MODE_CODE"];
					////		tsmpe02["ORDER_NO"] = tom01["ORDER_NO"];
					////		tsmpe02.CONSIGN_CUST_CNAME = tom01["CONSIGN_CUST_CNAME"];
					////		tsmpe02.TERMINAL_NAME = tom01["DELIVERY_PLACE_NAME"];
					////		tsmpe02.PRIVATE_ROUTE_NAME = tom01["PRIVATE_ROUTE_NAME"];

					////		tsmpe02.Update("TRNP_MODE_CODE,ORDER_NO,CONSIGN_CUST_CNAME,TERMINAL_NAME,PRIVATE_ROUTE_NAME", "MAT_NO");
					////		tsmpe09.MAT_NO = tsmpe02["MAT_NO"];
					////		tsmpe09.Delete("MAT_NO");
					////	}
					////	else
					////	{
					////		sprintf(s.msg, "此合同号%s不是充当合同", (const char *)c_order_no1);
					////		throw CApplicationException(-1, s.msg, s.svc_name);
					////	}

					}

					if (d_mat_wt != tsmpe02["MAT_WT"].ToDecimal())
					{
						CFormattable arguments[] = { c_mat_no, d_mat_wt, tsmpe02["MAT_WT"].ToDecimal() };// 定义参数列表的数组
						CMessageFormat::Format(s.msg, "计划上的材料[{0}]重量[{1}]和准发的材料重量[{2}]不符", arguments, 3);//格式化字符串
						throw CApplicationException(-1, s.msg, s.svc_name);
					}



					/* *****	修改准发材料表发货状态及提单号 *********************************************** */
					sqlstr = "  UPDATE	tsmpe02 "
						"   SET	   bill_of_lading_no = @bill_of_lading_no, "
						"   confm_status      = '6', "
						"   rec_revise_time   = @datetime, "
						"   rec_revisor       = @c_user  "
						" ,	RAIN_COAT_FLAG    = @RAIN_COAT_FLAG "
						" , TERMINAL_NAME     = @TERMINAL_NAME "
						//" , VER_HOR_FLAG      = @VER_HOR_FLAG "
						"   WHERE	mat_no = @mat_no "
						;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					execute_sql.Parameters.Set("RAIN_COAT_FLAG", tsmpe10["RAIN_COAT_FLAG"].ToString().TrimOrBlank());
					execute_sql.Parameters.Set("TERMINAL_NAME", tsmpe10["DELIVY_PLACE_NAME"].ToString().TrimOrBlank());
					//execute_sql.Parameters.Set("VER_HOR_FLAG", c_ver_hor_flag.TrimOrBlank());	// 立卧标记
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					execute_sql.ExecuteNonQuery();

					/* *****	修改准发单据的状态 *********************************************** */
					sqlstr = " UPDATE	tsmpe00 a "
						"  SET confm_status	= (select nvl(max(confm_status),a.confm_status)  from tsmpe02 b where a.ready_bill_no = b.ready_bill_no ), "
						"  rec_revise_time        =  @datetime, "
						"  rec_revisor		    =  @c_user  "
						"  WHERE	ready_bill_no	= @ready_bill_no "
						;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("ready_bill_no", tsmpe02["READY_BILL_NO"].ToString());
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					execute_sql.ExecuteNonQuery();

					/* *****	修改准发计划的状态 *********************************************** */
					sqlstr = " UPDATE	tsmpe01 a "
						"  SET confm_status	= (select nvl(MIN(confm_status),a.confm_status)  from tsmpe02 b where a.confm_plan_no = b.confm_plan_no ), "
						"  rec_revise_time        =  @datetime, "
						"  rec_revisor		    =  @c_user  "
						"  WHERE	confm_plan_no	= @confm_plan_no "
						;
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("confm_plan_no", tsmpe02["CONFM_PLAN_NO"].ToString());
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					execute_sql.ExecuteNonQuery();

					/**************** 调用物料封装  函数*********************/
					//if (f_sm00_mm99(c_mat_no, 3, 1, s.msg, conn) != 0)
					//{
					//	throw CApplicationException(-1, s.msg, s.svc_name);
					//}
					mat_no.push_back(c_mat_no);
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


					//if (c_order_no1 == c_order_no)
					{
						sum_mat_wt = sum_mat_wt + d_mat_wt;
						sum_mat_num += 1;
					}


				}
				if (mark_flag == "1")
				{
					sprintf(s.msg, "此计划已经接收，不能重复！");
					return 0;
				}

				if (i == 0)
				{
					sprintf(s.msg, "没有传入材料记录");
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}

				ret = f_sm00_mm99(mat_no, 3, 1, s.msg, conn);
				if (ret != 0)
				{
					CFormattable	arguments[] = { s.msg };
					CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				// 判材料件数和计划上件数是否相同
				if (tsmpe10["PLAN_NUM"].ToDecimal() != sum_mat_num)
				{
					CFormattable	arguments[] = { tsmpe10["PLAN_NUM"].ToDecimal(), sum_mat_num, tsmpe10["BILL_OF_LADING_NO"].ToString() };
					CMessageFormat::Format(s.msg, "计划号{2}，计划材料数:{0}和材料合计数{1}不符", arguments, 3);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				// 判材料重量和计划重量是否相同
				if (tsmpe10["PLAN_WT"].ToDecimal() != sum_mat_wt)
				{
					CFormattable	arguments[] = { tsmpe10["PLAN_WT"].ToDecimal(), sum_mat_wt, tsmpe10["BILL_OF_LADING_NO"].ToString() };
					CMessageFormat::Format(s.msg, "计划号{2}，计划重量:{0}和材料合计重量{1}不符", arguments, 3);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				/* 新增出厂计划记录 */
				tsmpe10["REC_CREATE_TIME"] = datetime;						/* 创建日期 */
				tsmpe10["REC_CREATOR"] = c_user;							/* 创建人 */
				tsmpe10["DELIVY_PLAN_STATUS"] = "3";			/* 计划状态 */
				tsmpe10["DELIVY_NUM"] = 0;						/* 发货数量 */
				tsmpe10["DELIVY_WT"] = 0;						/* 发货重量 */
				tsmpe10["DELIVY_TUBE"] = 0;						/* 发货根数 */

				/* 插入计划表 */
				tsmpe10.TrimOrBlank();
				sqlstr = "INSERT TSMPE10   1";
				if (!tsmpe10.Insert())
				{
					sprintf(s.msg, "出厂计划号[%s] insert TABLE tsmpe10 失败.", (const char *)c_bill_of_lading_no);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}



			}//if-按件

			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" || tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2")	/* 按量 */
			{
				if (plan_num_count == 0)
				{
					sprintf(s.msg, "传入的件数不能为0；");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				for (i = 0; i < plan_num_count; i++)
				{
					/* *****11.	获得合同号 *********************************************** */
					tsmpe10["ORDER_NO"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_NO"].ToString();	// 合同号
					//tsmpe02.MAT_NO = bcls_rec->Tables[block_name_detail].Rows[i]["CUST_MAT_NO"].ToString();	// 材料号
					tsmpe10["PLAN_WT_D"] = bcls_rec->Tables[block_name_detail].Rows[i]["PLAN_WT_D"].ToDecimal();	// 合同计划量
					tsmpe10["ORDER_THICK"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_THICK"].ToDecimal();	// 厚度直径
					tsmpe10["ORDER_WIDTH"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_WIDHT"].ToDecimal();	// 宽度
					tsmpe10["ORDER_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN"].ToDecimal();	// 长度
					tsmpe10["ORDER_MIN_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN_MIN"].ToDecimal();	//最小长度
					tsmpe10["ORDER_MAX_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN_MAX"].ToDecimal();	//最大长度
					tsmpe10["CROSS_CODE"] = bcls_rec->Tables[block_name_detail].Rows[i]["CROSS_CODE"].ToString();	// 截面代码
					tsmpe10["PSC"] = bcls_rec->Tables[block_name_detail].Rows[i]["PSC"].ToString();	// 产品规范码
					tsmpe10["MSC"] = bcls_rec->Tables[block_name_detail].Rows[i]["MSC"].ToString();	// 冶金规范码
					tsmpe10["SG_SIGN"] = bcls_rec->Tables[block_name_detail].Rows[i]["SG_SIGN"].ToString();	// 牌号
					tsmpe10["SG_STD"] = bcls_rec->Tables[block_name_detail].Rows[i]["SG_STD"].ToString();	// 标准
					tsmpe10["WT_MODE"] = bcls_rec->Tables[block_name_detail].Rows[i]["WT_METHOD"].ToString();	// 计重方式
					tsmpe10["FIX_FLAG"] = bcls_rec->Tables[block_name_detail].Rows[i]["FIX_FLAG"].ToString();	// 定尺标记
					tsmpe10["RAIN_COAT_FLAG"] = bcls_rec->Tables[block_name_detail].Rows[i]["RAIN_COAT_FLAG"].ToString();	// 加盖雨布标志(苫盖)

					c_order_no1 = tsmpe10["ORDER_NO"];


					if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2" && (tsmpe10["MAT_KIND"].ToString() != "HP" || tsmpe10["TRNP_MODE_CODE"].ToString() == "22" || tsmpe10["TRNP_MODE_CODE"].ToString() == "12"))
					{
						CFormattable	arguments[] = { tsmpe10["MAT_KIND"].ToString(), tsmpe10["TRNP_MODE_CODE"].ToString() };
						CMessageFormat::Format(s.msg, "只有厚板【{0}】汽运【{1}】才允许按合同发货", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}



					if (tsmpe10["ORDER_NO"].ToString().Trim() == "")
					{
						sprintf(s.msg, _RES("GCRSS0000026")/*合同号不能为空。*/);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["ORDER_THICK"].ToDecimal() == 0)
					{
						CFormattable	arguments[] = { tsmpe10["ORDER_THICK"].ToDecimal(), tsmpe10["ORDER_NO"].ToString() };
						CMessageFormat::Format(s.msg, "厚度不符合要求【{0}】,合同号【{1}】", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["MAT_KIND"].ToString() == "HP" && tsmpe10["ORDER_WIDTH"].ToDecimal() == 0)
					{
						CFormattable	arguments[] = { tsmpe10["ORDER_WIDTH"].ToDecimal(), tsmpe10["ORDER_NO"].ToString() };
						CMessageFormat::Format(s.msg, "厚板宽度不符合要求【{0}】,合同号【{1}】", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["PLAN_WT_D"].ToDecimal() == 0)
					{
						CFormattable	arguments[] = { tsmpe10["PLAN_WT_D"].ToDecimal(), tsmpe10["ORDER_NO"].ToString() };
						CMessageFormat::Format(s.msg, "计划重量不符合要求【{0}】,合同号【{1}】", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["PSC"].ToString().Trim() == "" /*|| tsmpe10["MSC"].ToString().Trim() == "" */|| tsmpe10["SG_SIGN"].ToString().Trim() == "" || tsmpe10["SG_STD"].ToString().Trim() == "")
					{
						CFormattable	arguments[] = { tsmpe10["ORDER_NO"].ToString(), tsmpe10["PSC"].ToString(), tsmpe10["MSC"].ToString(), tsmpe10["SG_SIGN"].ToString(), tsmpe10["SG_STD"].ToString() };
						CMessageFormat::Format(s.msg, "合同号【{0}】,PSC【{1}】，MSC【{2}】，SG_SIGN【{3}】，SG_STD【{4}】不能为空", arguments, 5);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["FIX_FLAG"].ToString() != "0" && tsmpe10["FIX_FLAG"].ToString() != "1")
					{
						CFormattable	arguments[] = { tsmpe10["FIX_FLAG"].ToString(), tsmpe10["ORDER_NO"].ToString() };
						CMessageFormat::Format(s.msg, "定尺标记不符合要求【{0}】,合同号【{1}】", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["FIX_FLAG"].ToString() == "1")	// 定尺
					{
						tsmpe10["ORDER_MIN_LEN"] = tsmpe10["ORDER_LEN"];
						tsmpe10["ORDER_MAX_LEN"] = tsmpe10["ORDER_LEN"];
					}
					////// 根据仓库代码读取物料种类、厂别
					////sqlstr = "select mat_kind,factory_div from vsmpea9 where stock_no = @STOCK_NO ";
					////cmd_inq.SetCommandText(sqlstr);
					////cmd_inq.Parameters.Set("STOCK_NO", tsmpe10["STOCK_NO"].ToString());
					////cmd_inq.ExecuteReader();
					////Log::Debug("", "", "sqlstr = {0}", sqlstr);

					////if (cmd_inq.Read())
					////{
					////	tsmpe10["MAT_KIND"] = cmd_inq.GetString(1);
					////	tsmpe10["FACTORY_DIV"] = cmd_inq.GetString(2);
					////}
					////else
					////{
					////	CFormattable arguments[] = { tsmpe10["STOCK_NO"].ToString() };
					////	CMessageFormat::Format(s.msg, "读取物料种类，厂别出错，没有读到此 {0} 库区代码", arguments, 1);
					////	throw	CApplicationException(-1, s.msg, s.svc_name);
					////}
					////cmd_inq.Close();


					// 读取品名代码、产品名称
					sqlstr = "select * from tom01 where order_no = '" + c_order_no1 + "' ";
					cmd_inq.SetCommandText(sqlstr);
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					cmd_inq.ExecuteReader();
					if (cmd_inq.Read())
					{
						cmd_inq.Fetch(tom01);
					}
					else
					{
						sprintf(s.msg, "无此合同号%s", (const char *)c_order_no1);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					cmd_inq.Close();

					tsmpe10["PROD_CODE"] = tom01["PROD_CODE"].ToString();
					tsmpe10["PROD_CNAME"] = tom01["PROD_CNAME"].ToString();


					// 新增发货计划
					tsmpe10["REC_CREATE_TIME"] = datetime;						/* 创建日期 */
					tsmpe10["REC_CREATOR"] = c_user;							/* 创建人 */
					tsmpe10["DELIVY_PLAN_STATUS"] = "3";			/* 计划状态 */
					tsmpe10["DELIVY_NUM"] = 0;						/* 发货数量 */
					tsmpe10["DELIVY_WT"] = 0;						/* 发货重量 */
					tsmpe10["DELIVY_TUBE"] = 0;						/* 发货根数 */

					tsmpe10["PLAN_WT"] = tsmpe10["PLAN_WT_D"];

					tsmpe10.TrimOrBlank();

					sqlstr = "INSERT TSMPE10    2";
					Log::Debug("", "", "bill_of_lading_no={0},order_no = {1}", tsmpe10["BILL_OF_LADING_NO"].ToString(), tsmpe10["ORDER_NO"].ToString());

					/* 插入计划表 */
					if (!tsmpe10.Insert())
					{
						sprintf(s.msg, "出厂计划号[%s] insert TABLE tsmpe10 失败.", (const char *)c_bill_of_lading_no);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
				}//for
			}//if-按量


			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "3")	/* 按合约 */
			{
				if (tsmpe10["MAT_KIND"].ToString() != "HP" || (tsmpe10["TRNP_MODE_CODE"].ToString() != "22" && tsmpe10["TRNP_MODE_CODE"].ToString() != "12"))
				{
					CFormattable	arguments[] = { tsmpe10["MAT_KIND"].ToString(), tsmpe10["TRNP_MODE_CODE"].ToString() };
					CMessageFormat::Format(s.msg, "只有厚板【{0}】铁运【{1}】才允许按合约发货", arguments, 2);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				if (tsmpe10["PLAN_WT"].ToDecimal() == 0)
				{
					CFormattable	arguments[] = { tsmpe10["PLAN_WT"].ToDecimal()};
					CMessageFormat::Format(s.msg, "计划量不能为0", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);

				}
				if (tsmpe10["CONTRACT_NO"].ToString().Trim() == "" )
				{
					sprintf(s.msg, "合约号不能为空.");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}


				// 新增发货计划
				tsmpe10["REC_CREATE_TIME"] = datetime;						/* 创建日期 */
				tsmpe10["REC_CREATOR"] = c_user;							/* 创建人 */
				tsmpe10["DELIVY_PLAN_STATUS"] = "3";			/* 计划状态 */
				tsmpe10["DELIVY_NUM"] = 0;						/* 发货数量 */
				tsmpe10["DELIVY_WT"] = 0;						/* 发货重量 */
				tsmpe10["DELIVY_TUBE"] = 0;						/* 发货根数 */
				tsmpe10.TrimOrBlank();

				sqlstr = "INSERT TSMPE10    3";
				Log::Debug("", "", "bill_of_lading_no={0},order_no = {1}", tsmpe10["BILL_OF_LADING_NO"].ToString(), tsmpe10["ORDER_NO"].ToString());

				/* 插入计划表 */
				if (!tsmpe10.Insert())
				{
					sprintf(s.msg, "出厂计划号[%s] insert TABLE tsmpe10 失败.", (const char *)c_bill_of_lading_no);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

			}//if-按量

		}//if-提单接收

		if (c_operate_flag.Compare("A") == 0)	// 合同项次追加
		{
			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0")	/* 按件 */
			{
				sprintf(s.msg, "按件发货的合同不允许按项次追加");
				throw CApplicationException(-1, s.msg, s.svc_name);

				//////*********发货材料进行循环处理*********
				////for (i = 0; i < plan_num_count; i++)
				////{
				////	c_mat_no = bcls_rec->Tables[block_name_detail].Rows[i]["CUST_MAT_NO"].ToString().TrimOrBlank();
				////	c_order_no1 = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_NO"].ToString().TrimOrBlank();
				////	d_mat_wt = bcls_rec->Tables[block_name_detail].Rows[i]["PLAN_WT_D"].ToDecimal();	// 合同计划量
				////	//c_ver_hor_flag = bcls_rec->Tables[block_name_detail].Rows[i]["VER_HOR_FLAG"].ToString();	// 立卧标记
				////	Log::Info("", __FUNCTION__, "c_mat_no=[{0}],i=[{1}]", c_mat_no, i);
				////	Log::Info("", __FUNCTION__, "c_order_no=[{0}]],i=[{1}]", c_order_no, i);
				////	Log::Info("", __FUNCTION__, "d_mat_wt=[{0}]],i=[{1}]", d_mat_wt, i);

				////	if (0 == c_mat_no.Compare(" "))
				////	{
				////		//break;
				////	}


				////	/**************** 判断材料号是否合法 **********************/
				////	sqlstr = CString(" select * from tsmpe02 where mat_no = @mat_no ");
				////	execute_sql.SetCommandText(sqlstr);
				////	execute_sql.Parameters.Clear();
				////	execute_sql.Parameters.Set("mat_no", c_mat_no);
				////	execute_sql.ExecuteReader();
				////	if (execute_sql.Read())
				////	{
				////		execute_sql.Fetch(tsmpe02);
				////	}
				////	else
				////	{
				////		CFormattable	arguments[] = { c_mat_no, tsmpe10["BILL_OF_LADING_NO"].ToString() };
				////		CMessageFormat::Format(s.msg, "接收出厂计划{1}电文发现材料号{0}在准发材料表中不存在", arguments, 2);
				////		throw CApplicationException(-1, s.msg, s.svc_name);
				////	}
				////	execute_sql.Close();

				////	if (tsmpe02["BILL_OF_LADING_NO"].ToString().TrimOrBlank().Compare(" ") != 0)
				////	{
				////		if (tsmpe02["BILL_OF_LADING_NO"].ToString() == tsmpe10["BILL_OF_LADING_NO"].ToString())
				////		{
				////			mark_flag = "1";
				////			continue;
				////		}
				////		CFormattable	arguments[] = { c_mat_no, tsmpe02["BILL_OF_LADING_NO"].ToString(), tsmpe10["BILL_OF_LADING_NO"].ToString() };
				////		CMessageFormat::Format(s.msg, "接收出厂计划{2}电文发现材料号{0}已经在发货计划{1}中", arguments, 3);
				////		throw CApplicationException(-1, s.msg, s.svc_name);
				////	}

				////	if (tsmpe02["CONFM_STATUS"].ToString().Compare("4") != 0)
				////	{
				////		sprintf(s.msg, "材料号[%s]材料状态[%s]不为准发确认", (const char *)tsmpe02["MAT_NO"].ToString(), (const char *)tsmpe02["CONFM_STATUS"].ToString());
				////		throw CApplicationException(-1, s.msg, s.svc_name);
				////	}

				////	////if (tsmpe02["ORDER_NO"].ToString() != c_order_no1 /*&& tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "0"*/)
				////	////{
				////	////	CFormattable arguments[] = { tsmpe02["ORDER_NO"].ToString(), c_order_no1, tsmpe02["MAT_NO"].ToString() };// 定义参数列表的数组
				////	////	CMessageFormat::Format(s.msg, "计划上的合同号[{1}]和材料上的合同号[{0}]不一致,材料号=[{2}]", arguments, 3);



				////	////	// 更新材料表上的合同号(充当合同） 2021-9-6
				////	////	sqlstr = "select * from tom01 where order_no = '" + c_order_no1 + "' ";
				////	////	cmd_inq.SetCommandText(sqlstr);
				////	////	Log::Debug("", "", "sqlstr={0}", sqlstr);
				////	////	cmd_inq.ExecuteReader();
				////	////	if (cmd_inq.Read())
				////	////	{
				////	////		cmd_inq.Fetch(tom01);
				////	////	}
				////	////	else
				////	////	{
				////	////		sprintf(s.msg, "无此合同号%s", (const char *)c_order_no1);
				////	////		throw CApplicationException(-1, s.msg, s.svc_name);
				////	////	}
				////	////	cmd_inq.Close();

				////	////	if (tom01["ORDER_TYPE_CODE"].ToString() =="CZA")  // 充当合同
				////	////	{
				////	////		tsmpe02.TRNP_MODE_CODE = tom01["TRNP_MODE_CODE"];
				////	////		tsmpe02["ORDER_NO"] = tom01["ORDER_NO"];
				////	////		tsmpe02.CONSIGN_CUST_CNAME = tom01["CONSIGN_CUST_CNAME"];
				////	////		tsmpe02.TERMINAL_NAME = tom01["DELIVERY_PLACE_NAME"];
				////	////		tsmpe02.PRIVATE_ROUTE_NAME = tom01["PRIVATE_ROUTE_NAME"];

				////	////		tsmpe02.Update("TRNP_MODE_CODE,ORDER_NO,CONSIGN_CUST_CNAME,TERMINAL_NAME,PRIVATE_ROUTE_NAME", "MAT_NO");
				////	////		tsmpe09.MAT_NO = tsmpe02["MAT_NO"];
				////	////		tsmpe09.Delete("MAT_NO");
				////	////	}
				////	////	else
				////	////	{
				////	////		sprintf(s.msg, "此合同号%s不是充当合同", (const char *)c_order_no1);
				////	////		throw CApplicationException(-1, s.msg, s.svc_name);
				////	////	}

				////	////}

				////	if (d_mat_wt != tsmpe02["MAT_WT"].ToDecimal())
				////	{
				////		CFormattable arguments[] = { c_mat_no, d_mat_wt, tsmpe02["MAT_WT"].ToDecimal() };// 定义参数列表的数组
				////		CMessageFormat::Format(s.msg, "计划上的材料[{0}]重量[{1}]和准发的材料重量[{2}]不符", arguments, 3);//格式化字符串
				////		throw CApplicationException(-1, s.msg, s.svc_name);
				////	}



				////	/* *****	修改准发材料表发货状态及提单号 *********************************************** */
				////	sqlstr = "  UPDATE	tsmpe02 "
				////		"   SET	   bill_of_lading_no = @bill_of_lading_no, "
				////		"   confm_status      = '6', "
				////		"   rec_revise_time   = @datetime, "
				////		"   rec_revisor       = @c_user  "
				////		" ,	RAIN_COAT_FLAG    = @RAIN_COAT_FLAG "
				////		" , TERMINAL_NAME     = @TERMINAL_NAME "
				////		//" , VER_HOR_FLAG      = @VER_HOR_FLAG "
				////		"   WHERE	mat_no = @mat_no "
				////		;
				////	execute_sql.SetCommandText(sqlstr);
				////	execute_sql.Parameters.Clear();
				////	execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				////	execute_sql.Parameters.Set("c_user", c_user);
				////	execute_sql.Parameters.Set("datetime", datetime);
				////	execute_sql.Parameters.Set("mat_no", c_mat_no);
				////	execute_sql.Parameters.Set("RAIN_COAT_FLAG", tsmpe10["RAIN_COAT_FLAG"].ToString().TrimOrBlank());
				////	execute_sql.Parameters.Set("TERMINAL_NAME", tsmpe10["DELIVY_PLACE_NAME"].ToString().TrimOrBlank());
				////	//execute_sql.Parameters.Set("VER_HOR_FLAG", c_ver_hor_flag.TrimOrBlank());	// 立卧标记
				////	execute_sql.ExecuteNonQuery();

				////	/* *****	修改准发单据的状态 *********************************************** */
				////	sqlstr = " UPDATE	tsmpe00 a "
				////		"  SET confm_status	= (select nvl(max(confm_status),a.confm_status)  from tsmpe02 b where a.ready_bill_no = b.ready_bill_no ), "
				////		"  rec_revise_time        =  @datetime, "
				////		"  rec_revisor		    =  @c_user  "
				////		"  WHERE	ready_bill_no	= @ready_bill_no "
				////		;
				////	execute_sql.SetCommandText(sqlstr);
				////	execute_sql.Parameters.Clear();
				////	execute_sql.Parameters.Set("ready_bill_no", tsmpe02["READY_BILL_NO"].ToString());
				////	execute_sql.Parameters.Set("c_user", c_user);
				////	execute_sql.Parameters.Set("datetime", datetime);
				////	execute_sql.ExecuteNonQuery();

				////	/* *****	修改准发计划的状态 *********************************************** */
				////	sqlstr = " UPDATE	tsmpe01 a "
				////		"  SET confm_status	= (select nvl(MIN(confm_status),a.confm_status)  from tsmpe02 b where a.confm_plan_no = b.confm_plan_no ), "
				////		"  rec_revise_time        =  @datetime, "
				////		"  rec_revisor		    =  @c_user  "
				////		"  WHERE	confm_plan_no	= @confm_plan_no "
				////		;
				////	execute_sql.SetCommandText(sqlstr);
				////	execute_sql.Parameters.Clear();
				////	execute_sql.Parameters.Set("confm_plan_no", tsmpe02["CONFM_PLAN_NO"].ToString());
				////	execute_sql.Parameters.Set("c_user", c_user);
				////	execute_sql.Parameters.Set("datetime", datetime);
				////	execute_sql.ExecuteNonQuery();

				////	/**************** 调用物料封装  函数*********************/
				////	//if (f_sm00_mm99(c_mat_no, 3, 1, s.msg, conn) != 0)
				////	//{
				////	//	throw CApplicationException(-1, s.msg, s.svc_name);
				////	//}
				////	mat_no.push_back(c_mat_no);
				////	/**************** 调用物料封装  函数结束  **********************/

				////	bcls_rec->Tables[record_name].Rows[0]["mat_no"] = c_mat_no;
				////	bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "7";
				////	bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;

				////	ret = 0;
				////	ret = f_sm00_record(bcls_rec, bcls_ret, conn);
				////	if (ret < 0)
				////	{
				////		Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
				////		throw CApplicationException(-1, s.msg, s.svc_name);
				////	}


				////	//if (c_order_no1 == c_order_no)
				////	{
				////		sum_mat_wt = sum_mat_wt + d_mat_wt;
				////		sum_mat_num += 1;
				////	}


				////}
				////if (mark_flag == "1")
				////{
				////	sprintf(s.msg, "此计划已经接收，不能重复！");
				////	return 0;
				////}

				////if (i == 0)
				////{
				////	sprintf(s.msg, "没有传入材料记录");
				////	throw	CApplicationException(-1, s.msg, s.svc_name);
				////}

				////ret = f_sm00_mm99(mat_no, 3, 1, s.msg, conn);
				////if (ret != 0)
				////{
				////	CFormattable	arguments[] = { s.msg };
				////	CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
				////	throw CApplicationException(-1, s.msg, s.svc_name);
				////}

				////// 判材料件数和计划上件数是否相同
				////if (tsmpe10["PLAN_NUM"].ToDecimal() != sum_mat_num)
				////{
				////	CFormattable	arguments[] = { tsmpe10["PLAN_NUM"].ToDecimal(), sum_mat_num, tsmpe10["BILL_OF_LADING_NO"].ToString() };
				////	CMessageFormat::Format(s.msg, "计划号{2}，计划材料数:{0}和材料合计数{1}不符", arguments, 3);
				////	throw CApplicationException(-1, s.msg, s.svc_name);
				////}
				////// 判材料重量和计划重量是否相同
				////if (tsmpe10["PLAN_WT"].ToDecimal() != sum_mat_wt)
				////{
				////	CFormattable	arguments[] = { tsmpe10["PLAN_WT"].ToDecimal(), sum_mat_wt, tsmpe10["BILL_OF_LADING_NO"].ToString() };
				////	CMessageFormat::Format(s.msg, "计划号{2}，计划重量:{0}和材料合计重量{1}不符", arguments, 3);
				////	throw CApplicationException(-1, s.msg, s.svc_name);
				////}

				/////* 新增出厂计划记录 */
				////tsmpe10["REC_CREATE_TIME"] = datetime;						/* 创建日期 */
				////tsmpe10["REC_CREATOR"] = c_user;							/* 创建人 */
				////tsmpe10["DELIVY_PLAN_STATUS"] = "3";			/* 计划状态 */
				////tsmpe10["DELIVY_NUM"] = 0;						/* 发货数量 */
				////tsmpe10["DELIVY_WT"] = 0;						/* 发货重量 */
				////tsmpe10["DELIVY_TUBE"] = 0;						/* 发货根数 */

				/////* 插入计划表 */
				////tsmpe10.TrimOrBlank();
				////sqlstr = "INSERT TSMPE10";
				////if (!tsmpe10.Insert())
				////{
				////	sprintf(s.msg, "出厂计划号[%s] insert TABLE tsmpe10 失败.", (const char *)c_bill_of_lading_no);
				////	throw CApplicationException(-1, s.msg, s.svc_name);
				////}
			}//if-按件

			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1")	/* 按量 */
			{
				if (plan_num_count == 0)
				{
					sprintf(s.msg, "传入的件数不能为0；");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				for (i = 0; i < plan_num_count; i++)
				{
					/* *****11.	获得合同号 *********************************************** */
					tsmpe10["ORDER_NO"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_NO"].ToString();	// 合同号
					//tsmpe02.MAT_NO = bcls_rec->Tables[block_name_detail].Rows[i]["CUST_MAT_NO"].ToString();	// 材料号
					tsmpe10["PLAN_WT_D"] = bcls_rec->Tables[block_name_detail].Rows[i]["PLAN_WT_D"].ToDecimal();	// 合同计划量
					tsmpe10["ORDER_THICK"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_THICK"].ToDecimal();	// 厚度直径
					tsmpe10["ORDER_WIDTH"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_WIDTH"].ToDecimal();	// 宽度
					tsmpe10["ORDER_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN"].ToDecimal();	// 长度
					tsmpe10["ORDER_MIN_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN_MIN"].ToDecimal();	//最小长度
					tsmpe10["ORDER_MAX_LEN"] = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_LEN_MAX"].ToDecimal();	//最大长度
					tsmpe10["CROSS_CODE"] = bcls_rec->Tables[block_name_detail].Rows[i]["CROSS_CODE"].ToString();	// 截面代码
					tsmpe10["PSC"] = bcls_rec->Tables[block_name_detail].Rows[i]["PSC"].ToString();	// 产品规范码
					tsmpe10["MSC"] = bcls_rec->Tables[block_name_detail].Rows[i]["MSC"].ToString();	// 冶金规范码
					tsmpe10["SG_SIGN"] = bcls_rec->Tables[block_name_detail].Rows[i]["SG_SIGN"].ToString();	// 牌号
					tsmpe10["SG_STD"] = bcls_rec->Tables[block_name_detail].Rows[i]["SG_STD"].ToString();	// 标准
					tsmpe10["WT_MODE"] = bcls_rec->Tables[block_name_detail].Rows[i]["WT_METHOD"].ToString();	// 计重方式
					tsmpe10["FIX_FLAG"] = bcls_rec->Tables[block_name_detail].Rows[i]["FIX_FLAG"].ToString();	// 定尺标记
					tsmpe10["RAIN_COAT_FLAG"] = bcls_rec->Tables[block_name_detail].Rows[i]["RAIN_COAT_FLAG"].ToString();	// 加盖雨布标志(苫盖)

					c_order_no1 = tsmpe10["ORDER_NO"];

					if (tsmpe10["ORDER_NO"].ToString().Trim() == "")
					{
						sprintf(s.msg, _RES("GCRSS0000026")/*合同号不能为空。*/);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["ORDER_THICK"].ToDecimal() == 0)
					{
						CFormattable	arguments[] = { tsmpe10["ORDER_THICK"].ToDecimal(), tsmpe10["ORDER_NO"].ToString() };
						CMessageFormat::Format(s.msg, "厚度不符合要求【{0}】,合同号【{1}】", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["MAT_KIND"].ToString() == "HP" && tsmpe10["ORDER_WIDTH"].ToDecimal() == 0)
					{
						CFormattable	arguments[] = { tsmpe10["ORDER_WIDTH"].ToDecimal(), tsmpe10["ORDER_NO"].ToString() };
						CMessageFormat::Format(s.msg, "厚板宽度不符合要求【{0}】,合同号【{1}】", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["PLAN_WT_D"].ToDecimal() == 0)
					{
						CFormattable	arguments[] = { tsmpe10["PLAN_WT_D"].ToDecimal(), tsmpe10["ORDER_NO"].ToString() };
						CMessageFormat::Format(s.msg, "计划重量不符合要求【{0}】,合同号【{1}】", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["PSC"].ToString().Trim() == "" /*|| tsmpe10["MSC"].ToString().Trim() == ""*/ || tsmpe10["SG_SIGN"].ToString().Trim() == "" || tsmpe10["SG_STD"].ToString().Trim() == "")
					{
						CFormattable	arguments[] = { tsmpe10["ORDER_NO"].ToString(), tsmpe10["PSC"].ToString(), tsmpe10["MSC"].ToString(), tsmpe10["SG_SIGN"].ToString(), tsmpe10["SG_STD"].ToString() };
						CMessageFormat::Format(s.msg, "合同号【{0}】,PSC【{1}】，MSC【{2}】，SG_SIGN【{3}】，SG_STD【{4}】不能为空", arguments, 5);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["FIX_FLAG"].ToString() != "0" && tsmpe10["FIX_FLAG"].ToString() != "1")
					{
						CFormattable	arguments[] = { tsmpe10["FIX_FLAG"].ToString(), tsmpe10["ORDER_NO"].ToString() };
						CMessageFormat::Format(s.msg, "定尺标记不符合要求【{0}】,合同号【{1}】", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					if (tsmpe10["FIX_FLAG"].ToString() == "1")	// 定尺
					{
						tsmpe10["ORDER_MIN_LEN"] = tsmpe10["ORDER_LEN"];
						tsmpe10["ORDER_MAX_LEN"] = tsmpe10["ORDER_LEN"];
					}

					////// 根据仓库代码读取物料种类、厂别
					////sqlstr = "select mat_kind,factory_div from vsmpea9 where stock_no = @STOCK_NO ";
					////cmd_inq.SetCommandText(sqlstr);
					////cmd_inq.Parameters.Set("STOCK_NO", tsmpe10["STOCK_NO"].ToString());
					////cmd_inq.ExecuteReader();
					////Log::Debug("", "", "sqlstr = {0}", sqlstr);

					////if (cmd_inq.Read())
					////{
					////	tsmpe10["MAT_KIND"] = cmd_inq.GetString(1);
					////	tsmpe10["FACTORY_DIV"] = cmd_inq.GetString(2);
					////}
					////else
					////{
					////	CFormattable arguments[] = { tsmpe10["STOCK_NO"].ToString() };
					////	CMessageFormat::Format(s.msg, "读取物料种类，厂别出错，没有读到此 {0} 库区代码", arguments, 1);
					////	throw	CApplicationException(-1, s.msg, s.svc_name);
					////}
					////cmd_inq.Close();


					// 新增发货计划
					tsmpe10["REC_CREATE_TIME"] = datetime;						/* 创建日期 */
					tsmpe10["REC_CREATOR"] = c_user;							/* 创建人 */
					tsmpe10["DELIVY_PLAN_STATUS"] = "3";			/* 计划状态 */
					tsmpe10["DELIVY_NUM"] = 0;						/* 发货数量 */
					tsmpe10["DELIVY_WT"] = 0;						/* 发货重量 */
					tsmpe10["DELIVY_TUBE"] = 0;						/* 发货根数 */

					tsmpe10["PLAN_WT"] = tsmpe10["PLAN_WT_D"];

					tsmpe10.TrimOrBlank();

					sqlstr = "INSERT TSMPE10    4";
					Log::Debug("", "", "bill_of_lading_no={0},order_no = {1}", tsmpe10["BILL_OF_LADING_NO"].ToString(), tsmpe10["ORDER_NO"].ToString());

					/* 插入计划表 */
					if (!tsmpe10.Insert())
					{
						sprintf(s.msg, "出厂计划号[%s] insert TABLE tsmpe10 失败.", (const char *)c_bill_of_lading_no);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

				}//for
			}//if-按量

			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "3")	/* 按合约 */
			{
				sprintf(s.msg, "按合约不能追加合约项次！");
				throw CApplicationException(-1, s.msg, s.svc_name);

				//if (tsmpe10["MAT_KIND"].ToString() != "HP" && (tsmpe10["TRNP_MODE_CODE"].ToString() != "22" || tsmpe10["TRNP_MODE_CODE"].ToString() != "12"))
				//{
				//	CFormattable	arguments[] = { tsmpe10["MAT_KIND"].ToString(), tsmpe10["TRNP_MODE_CODE"].ToString() };
				//	CMessageFormat::Format(s.msg, "只有厚板【{0}】铁运【{1}】才允许按合约发货", arguments, 2);
				//	throw CApplicationException(-1, s.msg, s.svc_name);
				//}
				//if (tsmpe10["PLAN_WT"].ToDecimal() == 0)
				//{
				//	CFormattable	arguments[] = { tsmpe10["PLAN_WT"].ToDecimal() };
				//	CMessageFormat::Format(s.msg, "计划量不能为0", arguments, 1);
				//	throw CApplicationException(-1, s.msg, s.svc_name);

				//}
				//if (tsmpe10["CONTRACT_NO"].ToString().Trim() == "")
				//{
				//	sprintf(s.msg, "合约号不能为空.");
				//	throw CApplicationException(-1, s.msg, s.svc_name);
				//}


				//// 新增发货计划
				//tsmpe10["REC_CREATE_TIME"] = datetime;						/* 创建日期 */
				//tsmpe10["REC_CREATOR"] = c_user;							/* 创建人 */
				//tsmpe10["DELIVY_PLAN_STATUS"] = "3";			/* 计划状态 */
				//tsmpe10["DELIVY_NUM"] = 0;						/* 发货数量 */
				//tsmpe10["DELIVY_WT"] = 0;						/* 发货重量 */
				//tsmpe10["DELIVY_TUBE"] = 0;						/* 发货根数 */
				//tsmpe10.TrimOrBlank();

				//sqlstr = "INSERT TSMPE10";
				//Log::Debug("", "", "bill_of_lading_no={0},order_no = {1}", tsmpe10["BILL_OF_LADING_NO"].ToString(), tsmpe10["ORDER_NO"].ToString());

				///* 插入计划表 */
				//if (!tsmpe10.Insert())
				//{
				//	sprintf(s.msg, "出厂计划号[%s] insert TABLE tsmpe10 失败.", (const char *)c_bill_of_lading_no);
				//	throw CApplicationException(-1, s.msg, s.svc_name);
				//}

			}//if-按合约

		}//if-合同项次追加

		//*********出厂计划吊销*********
//		if (c_operate_flag.Compare("0") == 0)
//		{
//			sqlstr = CString(
//				" select delivy_plan_status from tsmpe10 where bill_of_lading_no = @bill_of_lading_no "
//				"  AND ORDER_NO IN (' ', @order_no) "
//				" order by delivy_plan_status desc "
//				);
//
//			//判断计划是否存在
//			execute_sql.SetCommandText(sqlstr);
//			execute_sql.Parameters.Clear();
//			execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
//			execute_sql.Parameters.Set("order_no", c_order_no);
//			Log::Debug("", "", "sqlstr={0}", sqlstr);
//			execute_sql.ExecuteReader();
//			if (execute_sql.Read())
//			{
//				c_delivy_plan_status = execute_sql.GetString(1);
//			}
//			else
//			{
//				CFormattable arguments[] = { c_bill_of_lading_no, c_order_no };
//				CMessageFormat::Format(s.msg, "此计划{0}合同{1}在发货计划表中不存在", arguments, 2);
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//			execute_sql.Close();
//
//			if (c_delivy_plan_status != "3")
//			{
//				CFormattable arguments[] = { c_bill_of_lading_no, c_order_no };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "发货计划号[{0}]合同号{1}已经执行不能删除", arguments, 2);//格式化字符串
//				throw CApplicationException(-1, s.msg, s.svc_name);
//			}
//
//
//			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0")	/* 按件 */
//			{
//
//				sqlstr7 = CString( /* 计算某计划下的计划量，发货量 */
//					"  update  tsmpe10 a set rec_revise_time        =  @datetime, "
//					"     	                   rec_revisor		  =  @c_user, "
//					"                        (plan_num,plan_wt) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe02 b where a.bill_of_lading_no = b.bill_of_lading_no and a.order_no = b.order_no ), "
//					"                        (delivy_num,delivy_wt ) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe12 b where a.bill_of_lading_no = b.bill_of_lading_no and a.order_no = b.order_no) "
//					"                    where bill_of_lading_no = @bill_of_lading_no "
//					" and order_no IN (' ',@order_no) "
//
//					);
//
//				sqlstr8 = CString( /* 计划量 = 发货量，状态置为 ‘5’ */
//					"  update  tsmpe10 a set DELIVY_PLAN_STATUS = '5' "
//					"                    where bill_of_lading_no = @bill_of_lading_no "
//					"                      and plan_num = delivy_num "
//					" and order_no IN (' ', @order_no) "
//					);
//
//				sqlstr9 = CString( /* 如果计划量为0，则删除该记录  */
//					"  delete from  tsmpe10 a   "
//					"                    where bill_of_lading_no = @bill_of_lading_no "
//					"                      and plan_num = 0 "
//					" and order_no IN (' ', @order_no) "
//					);
//
//
//				//*********发货材料进行循环处理*********
//				for (i = 0; i < plan_num_count; i++)
//				{
//
//					c_mat_no = bcls_rec->Tables[block_name_detail].Rows[i]["mat_no"].ToString().TrimOrBlank();
//					c_order_no = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_NUM"].ToString().TrimOrBlank();
//
//					if (0 == c_mat_no.Compare(" "))
//					{
//						//break;
//					}
//
//					/**************** 判断材料号是否合法 **********************/
//
//					sqlstr = "SELECT * FROM tsmpe02 WHERE mat_no = @mat_no";
//					execute_sql.SetCommandText(sqlstr);
//					execute_sql.Parameters.Clear();
//					execute_sql.Parameters.Set("mat_no", c_mat_no);
//					execute_sql.ExecuteReader();
//					if (execute_sql.Read())
//					{
//						execute_sql.Fetch(tsmpe02);
//					}
//					else
//					{
//						CFormattable	arguments[] = { c_mat_no };
//						CMessageFormat::Format(s.msg, "接收出厂计划电文发现材料号{0}在准发材料表中不存在", arguments, 1);
//						throw CApplicationException(-1, s.msg, s.svc_name);
//					}
//					execute_sql.Close();
//
//
//					//// 判此材料是否编入作业单，是报错
//					//if (tsmpe02.WORK_ID.Trim() != "")
//					//{
//					//	CFormattable	arguments[] = { c_mat_no, tsmpe02.WORK_ID };
//					//	CMessageFormat::Format(s.msg, "此材料号{0}已经编入作业单{1}不能删除", arguments, 1);
//					//	throw CApplicationException(-1, s.msg, s.svc_name);
//					//}
//
//
//					// 判此材料是否编入作业单，是删除作业单，并把作业单上的材料退出作业单
//					if (tsmpe02.WORK_ID.Trim() != "" )
//					{
//						if ( tsmpe02.TICKET_NO.Trim() == "")
//						{
//							sqlstr = "update tsmpe02 set work_id = ' ' "
//								" WHERE work_id = @WORK_ID ";
//							execute_sql.SetCommandText(sqlstr);
//							execute_sql.Parameters.Clear();
//							execute_sql.Parameters.Set("WORK_ID", tsmpe02.WORK_ID);
//							execute_sql.ExecuteNonQuery();
//
//							sqlstr = "delete from tsmpe10a  "
//								" WHERE work_id = @WORK_ID ";
//							execute_sql.SetCommandText(sqlstr);
//							execute_sql.Parameters.Clear();
//							execute_sql.Parameters.Set("WORK_ID", tsmpe02.WORK_ID);
//							execute_sql.ExecuteNonQuery();
//						}
//						else
//						{
//							CFormattable	arguments[] = { c_mat_no, tsmpe02.WORK_ID, tsmpe02.TICKET_NO };
//							CMessageFormat::Format(s.msg, "此材料号{0}已经编入作业单{1}并且已装车{2}，不能删除！", arguments, 3);
//							throw CApplicationException(-1, s.msg, s.svc_name);
//						}
//					}
//
//
//
//					bcls_rec->Tables[record_name].Rows[0]["mat_no"] = c_mat_no;
//					bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "8";
//					bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;
//
//					ret = 0;
//					ret = f_sm00_record(bcls_rec, bcls_ret, conn);
//					if (ret < 0)
//					{
//						Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
//						throw CApplicationException(-1, s.msg, s.svc_name);
//					}
//
//
//
//
//					/* *****	修改准发材料表发货状态 *********************************************** */
//					sqlstr = "  UPDATE	tsmpe02 "
//						"   SET	   bill_of_lading_no = ' ', "
//						"   confm_status      = '4', "
//						"   rec_revise_time   = @datetime, "
//						"   rec_revisor       = @c_user  "
//						" , vehicle_no = ' ' "
//						" , order_no = old_order_no "	// 2022-1-3
//						"   WHERE	mat_no = @mat_no ";
//					execute_sql.SetCommandText(sqlstr);
//					execute_sql.Parameters.Clear();
//					execute_sql.Parameters.Set("c_user", c_user);
//					execute_sql.Parameters.Set("datetime", datetime);
//					execute_sql.Parameters.Set("mat_no", c_mat_no);
//					execute_sql.ExecuteNonQuery();
//
//					/* *****	修改准发单据的状态 *********************************************** */
//					sqlstr = " UPDATE	tsmpe00 a "
//						"  SET confm_status	= (select nvl(max(confm_status),a.confm_status)  from tsmpe02 b where a.ready_bill_no = b.ready_bill_no ), "
//						"  rec_revise_time        =  @datetime, "
//						"  rec_revisor		    =  @c_user  "
//						"  WHERE	ready_bill_no	= @ready_bill_no ";
//					execute_sql.SetCommandText(sqlstr);
//					execute_sql.Parameters.Clear();
//					execute_sql.Parameters.Set("ready_bill_no", tsmpe02["READY_BILL_NO"].ToString());
//					execute_sql.Parameters.Set("c_user", c_user);
//					execute_sql.Parameters.Set("datetime", datetime);
//					execute_sql.ExecuteNonQuery();
//
//					/* *****	修改准发计划的状态 *********************************************** */
//					sqlstr = " UPDATE	tsmpe01 a "
//						"  SET confm_status	= (select nvl(min(confm_status),a.confm_status)  from tsmpe02 b where a.confm_plan_no = b.confm_plan_no ), "
//						"  rec_revise_time        =  @datetime, "
//						"  rec_revisor		    =  @c_user  "
//						"  WHERE	confm_plan_no	= @confm_plan_no ";
//					execute_sql.SetCommandText(sqlstr);
//					execute_sql.Parameters.Clear();
//					execute_sql.Parameters.Set("confm_plan_no", tsmpe02["CONFM_PLAN_NO"].ToString());
//					execute_sql.Parameters.Set("c_user", c_user);
//					execute_sql.Parameters.Set("datetime", datetime);
//					execute_sql.ExecuteNonQuery();
//
//					/* 计算该出厂计划相关的计划量 */
//					sqlstr = "  update  tsmpe10 a set rec_revise_time        =  @datetime "
//						"  ,rec_revisor		  =  @c_user "
//						"  ,plan_num = plan_num -1 "
//						"  ,plan_wt = plan_wt - @MAT_WT  "
//						"  where bill_of_lading_no = @bill_of_lading_no "
//						" and order_no in (' ',@order_no) ";
//					execute_sql.SetCommandText(sqlstr);
//					execute_sql.Parameters.Clear();
//					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
//					execute_sql.Parameters.Set("order_no", c_order_no);
//					execute_sql.Parameters.Set("c_user", c_user);
//					execute_sql.Parameters.Set("datetime", datetime);
//					execute_sql.Parameters.Set("MAT_WT", tsmpe02["MAT_WT"].ToDecimal());
//					execute_sql.ExecuteNonQuery();
//
//					////sqlstr = sqlstr8;
//					////execute_sql.SetCommandText(sqlstr);
//					////execute_sql.Parameters.Clear();
//					////execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
//					////execute_sql.Parameters.Set("order_no", c_order_no);
//					////execute_sql.ExecuteNonQuery();
//
//					sqlstr = "  delete from  tsmpe10 a   "
//						" where bill_of_lading_no = @bill_of_lading_no "
//						" and plan_num = 0 "
//						" and order_no in (' ', @order_no) ";
//					execute_sql.SetCommandText(sqlstr);
//					execute_sql.Parameters.Clear();
//					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
//					execute_sql.Parameters.Set("order_no", c_order_no);
//					execute_sql.ExecuteNonQuery();
//
//					/**************** 调用物料封装  函数*********************/
//					//if (f_sm00_mm99(c_mat_no, 3, -1, s.msg, conn) != 0)
//					//{
//					//	doFlag = -1;
//					//	throw CApplicationException(-1, s.msg, s.svc_name);
//					//}
//					mat_no.push_back(c_mat_no);
//					/**************** 调用物料封装  函数结束  **********************/
//
//					// 写仓库出库队列
//					if (v_crane_mark == "1" && (tsmpe02["STOCK_NO"].ToString() == "H10" || tsmpe02["STOCK_NO"].ToString() == "H12"))
//					{
//						bcls_rec_wm00.Tables[block_no_wm].Rows.Add();
//						int ii = bcls_rec_wm00.Tables[block_no_wm].Rows.get_Count() - 1;
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["MAT_NO"] = tsmpe02["MAT_NO"];
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["MAT_NUM"] = tsmpe02["MAT_TUBE"];
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["TRANS_TOOL"] = " ";
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["UNIT_CODE"] = " ";
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["NEXT_UNIT_CODE"] = " ";
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["STOCK_NO"] = tsmpe02["STOCK_NO"];
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["TO_STOCK_NO"] = " ";
//
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["OPER_FLAG"] = "D";
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["STOCK_OPER_ORDER"] = "2E";
//						bcls_rec_wm00.Tables[block_no_wm].Rows[ii]["STOCK_OPER_ORDER_DIV"] = " ";
//
//						// 发送无人行车电文
//						bcls_transfer_cx_send.Tables[0].Rows.Add();
//						ii = bcls_transfer_cx_send.Tables[0].Rows.get_Count() - 1;
//						bcls_transfer_cx_send.Tables[0].Rows[ii]["LOAD_PLAN_NO"] = tsmpe10["BILL_OF_LADING_NO"];
//						bcls_transfer_cx_send.Tables[0].Rows[ii]["MAT_NO"] = tsmpe02["MAT_NO"];
//
//
//
//						// 热轧行车电文 2021-9-8
//						if (tsmpe10["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "2" && tsmpe02["STOCK_NO"].ToString() == "H10")
//						{
//							// 是热轧1780 铁运时不生成行车命令，到作业单生成时再生成行车指令 2021-11-29
//						}
//						else
//						{
//							bcls_cmd.Tables["WM_CMD"].Rows.Add();
//							ii = bcls_cmd.Tables["WM_CMD"].Rows.get_Count() - 1;
//							bcls_cmd.Tables["WM_CMD"].Rows[ii]["MAT_NO"] = tsmpe02["MAT_NO"];
//							bcls_cmd.Tables["WM_CMD"].Rows[ii]["STOCK_OPER_ORDER"] = "2E";
//							bcls_cmd.Tables["WM_CMD"].Rows[ii]["STOCK_PLACE_NO_TO"] = "";
//							bcls_cmd.Tables["WM_CMD"].Rows[ii]["STOCK_NO_TO"] = tsmpe02["STOCK_NO"];
//							bcls_cmd.Tables["WM_CMD"].Rows[ii]["UNIT_CODE"] = "";
//							bcls_cmd.Tables["WM_CMD"].Rows[ii]["FLAG"] = "0";//1新增，0删除
//						}
//					}
//				}
//
//				ret = f_sm00_mm99(mat_no, 3, -1, s.msg, conn);
//				if (ret != 0)
//				{
//					CFormattable	arguments[] = { s.msg };
//					CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
//					throw CApplicationException(-1, s.msg, s.svc_name);
//				}
//
//				// 2022-2-18 down
//				tsmpe10["DELIVY_PLAN_TYPE"] = bcls_rec->Tables[block_name_master].Rows[0]["PLAN_TYPE"].ToString().TrimOrBlank();	// 计划类型
//				if (tsmpe10["DELIVY_PLAN_TYPE"].ToString() == "30")
//				{
//					for (i = 0; i < plan_num_count; i++)
//					{
//						tsmpe02["MAT_NO"] = bcls_rec->Tables[block_name_detail].Rows[i]["mat_no"].ToString().TrimOrBlank();
//						Log::Debug("", "", "delete mat_no={0}", tsmpe02["MAT_NO"].ToString());
//						tsmpe02.Delete("MAT_NO");
//					}
//				}
//				// 2022-2-18 up
//
//
//#if  defined(_LINE_HR)|| defined(_LINE_CR)
//				if (v_crane_mark == "1")
//				{
//					if (bcls_cmd.Tables["WM_CMD"].Rows.get_Count()>0)
//					{
//						//doFlag = f_wm_pmcmd_do(&bcls_cmd, bcls_ret, "1", conn);
//						if (doFlag != 0)
//						{
//							throw CApplicationException(-1, s.msg, log.Location);
//						}
//					}
//
//				}
//#endif
//
//
//				// 调用仓库函数
//				////if (bcls_rec_wm00.Tables[block_no_wm].Rows.get_Count() > 0)
//				////{
//				////	ret = f_wm00_queue(&bcls_rec_wm00, bcls_ret, conn);
//				////	if (ret != 0)
//				////	{
//				////		throw	CApplicationException(-1, s.msg, s.svc_name);
//				////	}
//				////}
//
//				////if (bcls_transfer_cx_send.Tables[0].Rows.get_Count() > 0)
//				////{
//				////	ret = f_wmhrhr_ls4j15_snd(&bcls_transfer_cx_send, bcls_ret, conn);
//				////	if (ret != 0)
//				////	{
//				////		throw	CApplicationException(-1, s.msg, s.svc_name);
//				////	}
//				////}
//
//
//
//			}//if-按件
//			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" || tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2")	/* 按量 */
//			{
//				for (i = 0; i < plan_num_count; i++)
//				{
//					c_order_no = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_NUM"].ToString().TrimOrBlank();
//					d_mat_wt = bcls_rec->Tables[block_name_detail].Rows[i]["mat_wt"].ToDecimal();
//
//					// 修改计划量
//					sqlstr = "update tsmpe10 set plan_wt = plan_wt - @plan_wt  "
//						"  WHERE bill_of_lading_no = @bill_of_lading_no "
//						"	AND ORDER_NO = @order_no ";
//					execute_sql.SetCommandText(sqlstr);
//					execute_sql.Parameters.Clear();
//					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
//					execute_sql.Parameters.Set("order_no", c_order_no);
//					execute_sql.Parameters.Set("plan_wt", d_mat_wt);
//					Log::Debug("", "", "sqlstr={0}", sqlstr);
//					execute_sql.ExecuteNonQuery();
//
//					// 当计划量等于0时删除计划
//					sqlstr = "delete from tsmpe10  "
//						"  WHERE bill_of_lading_no = @bill_of_lading_no "
//						"	AND ORDER_NO = @order_no "
//						"   AND PLAN_WT <= 0 ";
//					execute_sql.SetCommandText(sqlstr);
//					execute_sql.Parameters.Clear();
//					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
//					execute_sql.Parameters.Set("order_no", c_order_no);
//					Log::Debug("", "", "sqlstr={0}", sqlstr);
//					execute_sql.ExecuteNonQuery();
//
//
//				}
//
//
//			}//if-按量
//
//			sqlstr = "UPDATE TSMPE02 SET bill_of_lading_no = ' ',VEHICLE_NO = ' ',confm_status = '4' "
//				"  WHERE bill_of_lading_no = @bill_of_lading_no "
//				"	AND ORDER_NO = @order_no ";
//			execute_sql.SetCommandText(sqlstr);
//			execute_sql.Parameters.Clear();
//			execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
//			execute_sql.Parameters.Set("order_no", c_order_no);
//			execute_sql.ExecuteNonQuery();
//
//		}//提单吊销
//
		//*********计划材料删除*********
		if (c_operate_flag.Compare("D") == 0)	// 合同项次删除
		{
			sqlstr = CString(
				" select delivy_plan_status from tsmpe10 where bill_of_lading_no = @bill_of_lading_no "
				"  AND ORDER_NO IN (' ', @order_no) "
				" order by delivy_plan_status desc "
				);

			//判断计划是否存在
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
			execute_sql.Parameters.Set("order_no", c_order_no);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			execute_sql.ExecuteReader();
			if (execute_sql.Read())
			{
				c_delivy_plan_status = execute_sql.GetString(1);
			}
			else
			{
				CFormattable arguments[] = { c_bill_of_lading_no, c_order_no };
				CMessageFormat::Format(s.msg, "此计划{0}合同{1}在发货计划表中不存在", arguments, 2);
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}
			execute_sql.Close();

			if (c_delivy_plan_status != "4" && c_delivy_plan_status != "3")
			{
				CFormattable arguments[] = { c_bill_of_lading_no };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "发货计划号[{0}]已经完成，不能删除材料", arguments, 1);//格式化字符串
				throw CApplicationException(-1, s.msg, s.svc_name);
			}


			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0")	/* 按件 */
			{

				//sqlstr7 = CString( /* 计算某计划下的计划量，发货量 */
				//	"  update  tsmpe10 a set rec_revise_time        =  @datetime, "
				//	"     	                   rec_revisor		  =  @c_user, "
				//	"                        (plan_num,plan_wt) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe02 b where a.bill_of_lading_no = b.bill_of_lading_no  ), "
				//	"                        (delivy_num,delivy_wt ) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe12 b where a.bill_of_lading_no = b.bill_of_lading_no ) "
				//	"                    where bill_of_lading_no = @bill_of_lading_no "
				//	" and order_no IN (' ',@order_no) "

				//	);

				//sqlstr8 = CString( /* 计划量 = 发货量，状态置为 ‘5’ */
				//	"  update  tsmpe10 a set DELIVY_PLAN_STATUS = '5' "
				//	"                    where bill_of_lading_no = @bill_of_lading_no "
				//	"                      and plan_num = delivy_num "
				//	" and order_no IN (' ', @order_no) "
				//	);

				//sqlstr9 = CString( /* 如果计划量为0，则删除该记录  */
				//	"  delete from  tsmpe10 a   "
				//	"                    where bill_of_lading_no = @bill_of_lading_no "
				//	"                      and plan_num = 0 "
				//	" and order_no IN (' ', @order_no) "
				//	);


				//*********发货材料进行循环处理*********
				for (i = 0; i < bcls_rec->Tables[block_name_detail].Rows.get_Count(); i++)
				{

					c_mat_no = bcls_rec->Tables[block_name_detail].Rows[i]["CUST_MAT_NO"].ToString().TrimOrBlank();
					//c_order_no = bcls_rec->Tables[block_name_detail].Rows[i]["order_num"].ToString().TrimOrBlank();

					if (0 == c_mat_no.Compare(" "))
					{
						break;
					}

					/**************** 判断材料号是否合法 **********************/

					sqlstr = "SELECT * FROM tsmpe02 WHERE mat_no = @mat_no";
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					execute_sql.ExecuteReader();
					if (execute_sql.Read())
					{
						execute_sql.Fetch(tsmpe02);
						// 2021-12-8 down
						if (tsmpe02["BILL_OF_LADING_NO"].ToString().Trim() != c_bill_of_lading_no.Trim())
						{
							CFormattable	arguments[] = { c_mat_no, c_bill_of_lading_no };
							CMessageFormat::Format(s.msg, "材料{0}已经不在计划{1}中!", arguments, 2);
							throw CApplicationException(-1, s.msg, s.svc_name);
						}
						// 2021-12-8 up
					}
					else
					{
						CFormattable	arguments[] = { c_mat_no };
						CMessageFormat::Format(s.msg, "接收出厂计划电文发现材料号{0}在准发材料表中不存在", arguments, 1);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					execute_sql.Close();


					////// 判此材料是否编入作业单，是报错
					////if (tsmpe02.WORK_ID.Trim() != "")
					////{
					////	CFormattable	arguments[] = { c_mat_no, tsmpe02.WORK_ID };
					////	CMessageFormat::Format(s.msg, "此材料号{0}已经编入作业单{1}不能删除", arguments, 2);
					////	throw CApplicationException(-1, s.msg, s.svc_name);
					////}




					bcls_rec->Tables[record_name].Rows[0]["mat_no"] = c_mat_no;
					bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "8";	// 发货计划删除
					bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;

					ret = 0;
					ret = f_sm00_record(bcls_rec, bcls_ret, conn);
					if (ret < 0)
					{
						Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
						throw CApplicationException(-1, s.msg, s.svc_name);
					}




					/* *****	修改准发材料表发货状态 *********************************************** */
					sqlstr = "  UPDATE	tsmpe02 "
						"   SET	   bill_of_lading_no = ' ', "
						"   confm_status      = '4', "
						"   rec_revise_time   = @datetime, "
						"   rec_revisor       = @c_user  "
						" , vehicle_no = ' ' "
						" , ORDER_NO = OLD_ORDER_NO "	// 2022-1-3
						"   WHERE	mat_no = @mat_no ";
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.Parameters.Set("mat_no", c_mat_no);
					execute_sql.ExecuteNonQuery();

					/* *****	修改准发单据的状态 *********************************************** */
					sqlstr = " UPDATE	tsmpe00 a "
						"  SET confm_status	= (select nvl(max(confm_status),a.confm_status)  from tsmpe02 b where a.ready_bill_no = b.ready_bill_no ), "
						"  rec_revise_time        =  @datetime, "
						"  rec_revisor		    =  @c_user  "
						"  WHERE	ready_bill_no	= @ready_bill_no ";
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("ready_bill_no", tsmpe02["READY_BILL_NO"].ToString());
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.ExecuteNonQuery();

					/* *****	修改准发计划的状态 *********************************************** */
					sqlstr = " UPDATE	tsmpe01 a "
						"  SET confm_status	= (select nvl(min(confm_status),a.confm_status)  from tsmpe02 b where a.confm_plan_no = b.confm_plan_no ), "
						"  rec_revise_time        =  @datetime, "
						"  rec_revisor		    =  @c_user  "
						"  WHERE	confm_plan_no	= @confm_plan_no ";
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("confm_plan_no", tsmpe02["CONFM_PLAN_NO"].ToString());
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.ExecuteNonQuery();

					/* 计算该出厂计划相关的计划量 */
					sqlstr = "  update  tsmpe10 a set rec_revise_time        =  @datetime "
						"  ,rec_revisor		  =  @c_user "
						"  ,plan_num = plan_num -1 "
						"  ,plan_wt = plan_wt - @MAT_WT  "
						//"  ,DELIVY_PLAN_STATUS = '5' "
						"  where bill_of_lading_no = @bill_of_lading_no "
						" and order_no in (' ',@order_no) ";
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					execute_sql.Parameters.Set("order_no", c_order_no);
					execute_sql.Parameters.Set("c_user", c_user);
					execute_sql.Parameters.Set("datetime", datetime);
					execute_sql.Parameters.Set("MAT_WT", tsmpe02["MAT_WT"].ToDecimal());
					execute_sql.ExecuteNonQuery();

					////sqlstr = sqlstr8;
					////execute_sql.SetCommandText(sqlstr);
					////execute_sql.Parameters.Clear();
					////execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					////execute_sql.Parameters.Set("order_no", c_order_no);
					////execute_sql.ExecuteNonQuery();

					sqlstr = "  delete from  tsmpe10 a   "
						" where bill_of_lading_no = @bill_of_lading_no "
						" and plan_num = 0 "
						" and order_no in (' ', @order_no) ";
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					execute_sql.Parameters.Set("order_no", c_order_no);
					execute_sql.ExecuteNonQuery();

					/**************** 调用物料封装  函数*********************/
					//if (f_sm00_mm99(c_mat_no, 3, -1, s.msg, conn) != 0)
					//{
					//	doFlag = -1;
					//	throw CApplicationException(-1, s.msg, s.svc_name);
					//}
					mat_no.push_back(c_mat_no);
					/**************** 调用物料封装  函数结束  **********************/

				}

				ret = f_sm00_mm99(mat_no, 3, -1, s.msg, conn);
				if (ret != 0)
				{
					CFormattable	arguments[] = { s.msg };
					CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}


				// 计划量等于完成量时，置计划状态为完成
				sqlstr = "update tsmpe10 set DELIVY_PLAN_STATUS = '5' "
				" where bill_of_lading_no = @bill_of_lading_no "
					" and order_no in (' ',@order_no) "
					" and plan_wt = delivy_wt and plan_num = delivy_num ";
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				execute_sql.Parameters.Set("order_no", c_order_no);
				execute_sql.ExecuteNonQuery();


			}//if-按件

			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" )	/* 按量 */
			{
				for (i = 0; i < plan_num_count; i++)
				{
					c_order_no = bcls_rec->Tables[block_name_detail].Rows[i]["ORDER_NO"].ToString().TrimOrBlank();
					//d_mat_wt = bcls_rec->Tables[block_name_detail].Rows[i]["PLAN_WT_D"].ToDecimal();
					if (c_order_no.Trim() == "")
					{
						CFormattable	arguments[] = { c_order_no };
						CMessageFormat::Format(s.msg, "合同号不能为空:{0}", arguments, 1);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

					// 当计划状态等于3时删除计划
					sqlstr = "delete from tsmpe10  "
						"  WHERE bill_of_lading_no = @bill_of_lading_no "
						"	AND ORDER_NO = @order_no "
						"   AND DELIVY_PLAN_STATUS = '3' ";
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
					execute_sql.Parameters.Set("order_no", c_order_no);
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					if (execute_sql.ExecuteNonQuery() == 0)
					{
						CFormattable	arguments[] = { c_bill_of_lading_no, c_order_no };
						CMessageFormat::Format(s.msg, "删除计划{0}合同:{1}没有成功", arguments, 2);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

				}


			}//if-按量

			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "3")	/* 按量 */
			{
				if (tsmpe10["CONTRACT_NO"].ToString().Trim() == "")
				{
					CFormattable	arguments[] = { tsmpe10["CONTRACT_NO"].ToString() };
					CMessageFormat::Format(s.msg, "合约号不能为空:{0}", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				// 当计划状态等于3时删除计划
				sqlstr = "delete from tsmpe10  "
					"  WHERE bill_of_lading_no = @bill_of_lading_no "
					"	AND CONTRACT_NO = @contract_no "
					"   AND DELIVY_PLAN_STATUS = '3' ";
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				execute_sql.Parameters.Set("contract_no", tsmpe10["CONTRACT_NO"].ToString());
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				if (execute_sql.ExecuteNonQuery() == 0)
				{
					CFormattable	arguments[] = { c_bill_of_lading_no, tsmpe10["CONTRACT_NO"].ToString() };
					CMessageFormat::Format(s.msg, "删除计划{0}合约:{1}没有成功", arguments, 2);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

			}//if-按量


		}//提单删除

		if (c_operate_flag.Compare("U") == 0)	// 修改量、合约号
		{
			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "3")	/* 按合约 */
			{
				sqlstr = "update tsmpe10 set CONTRACT_NO =@CONTRACT_NO ,PLAN_WT = @PLAN_WT "
					" WHERE BILL_OF_LADING_NO = @c_bill_of_lading_no  ";
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Set("c_bill_of_lading_no", c_bill_of_lading_no);
				execute_sql.Parameters.Set("CONTRACT_NO", tsmpe10["CONTRACT_NO"].ToString());
				execute_sql.Parameters.Set("PLAN_WT", tsmpe10["PLAN_WT"].ToDecimal());
				Log::Info("", "", "sqlstr={0}", sqlstr);
				Log::Info("", "", "bill_of_lading_no={0},CONTRACT_NO={1}", c_bill_of_lading_no, tsmpe10["CONTRACT_NO"].ToString());
				if (execute_sql.ExecuteNonQuery() != 0)
				{
					CFormattable arguments[] = { c_bill_of_lading_no };
					CMessageFormat::Format(s.msg, "此计划{0}不符合修改条件", arguments, 1);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}

			}//if-按合约
			else
			{
				sprintf(s.msg, "不是按合约计划不能修改合约号、重量");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

		}//if-合同项次追加

		if (c_operate_flag.Compare("X") == 0 || c_operate_flag.Compare("E") == 0)	// 作废、结案
		{
			// 是按件发货计划时，删除准发材料表上的计划号、准发材料状态回到‘4’
			if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0")
			{
				sqlstr = "select mat_no from tsmpe02 where bill_of_lading_no = '" + c_bill_of_lading_no + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Info("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				while (cmd_inq.Read())
				{
					c_mat_no = cmd_inq.GetString(1);
					bcls_rec->Tables[record_name].Rows[0]["mat_no"] = c_mat_no;
					bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "8";	// 发货计划删除
					bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;

					ret = 0;
					ret = f_sm00_record(bcls_rec, bcls_ret, conn);
					if (ret < 0)
					{
						Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					mat_no.push_back(c_mat_no);
				}
				cmd_inq.Close();


				ret = f_sm00_mm99(mat_no, 3, -1, s.msg, conn);
				if (ret != 0)
				{
					CFormattable	arguments[] = { s.msg };
					CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}


				sqlstr = "update tsmpe02 set bill_of_lading_no = ' ' ,confm_status = '4',vehicle_no = ' ' where bill_of_lading_no = '" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' ";
				execute_sql.SetCommandText(sqlstr);
				Log::Info("", "", "sqlstr={0}", sqlstr);
				execute_sql.ExecuteNonQuery();
			}

			// 更新发货计划表上的计划状态
			sqlstr = "update tsmpe10 set DELIVY_PLAN_STATUS = '" + c_operate_flag + "' "
				" where bill_of_lading_no = '" + c_bill_of_lading_no + "' and DELIVY_PLAN_STATUS = '3' ";
			execute_sql.SetCommandText(sqlstr);
			Log::Info("", "", "sqlstr={0}", sqlstr);
			if (execute_sql.ExecuteNonQuery() == 0)
			{
				CFormattable	arguments[] = { c_bill_of_lading_no };
				CMessageFormat::Format(s.msg, "计划号{0}作废或结案没有成功，没有找到满足条件的计划！", arguments, 1);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}


		}//结案、作废

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

