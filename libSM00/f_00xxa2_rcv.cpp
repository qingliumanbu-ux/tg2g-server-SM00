/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2020
Author:      013801
Version:     1.0
Date:        2020-02-13 16:08:47
Description: 准发计划体电文接收函数
2021-12-16	13801	库区代码为000时做自动准发确认
2021-12-26	13801	读取标签用标准取消
2022-3-15	13801	修改准发结束后调用 f_sm00_count 函数
2022-3-16	13801	优化调用物料函数为一起调用
2022-03-29	13801	期货合同热轧大类品名为热轧板、热轧调制板和热轧切板的长度用实际值
2022-4-12	13801	陈功旭要求热轧牌号取合同上的 CHART_TYPE_DESC 字段
**************************************************/


/***** C/C++ 的标准头文件部分 *****/
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"
#include "epex.h"



//#include "CDynaTable.h"


BM2_FUNCTION_EXPORT
int f_sm00_record(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);
int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int f_sm00_mm99(vector <CString> pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection * conn);
int f_sm00_confirm(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);
#ifdef _LINE_HR
int f_mmhrauth_cfm(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);	// 热轧自动准发确认
#endif

int f_00xxa2_rcv(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);
	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, i = 0, ret = 0, row_sum = 0;
	CString	record_name = "sm00_record";
	CString v_flag = "";	// 电文发送标记

	CString c_datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	EIClass sm_bcls_rec;

	//CModel tsmpe00("TSMPE00");

	CModel tsmpe02 = CModel("TSMPE02");
	CModel tsmpe01 = CModel("TSMPE01");
	CModel tsmpe00 = CModel("TSMPE00");
	CModel tep0002 = CModel("TEP0002");

	EIClass bcls_zrec, bcls_zret;
	EIClass bcls_ret_buffer;

	/* ***** 电文变量定义 ***** */
	CString  ct_pono = " ", ct_complex_decide_code = " ", ct_factory_div = " ", ct_mat_kind = " ", table_name = "TMMBW01";

	CDecimal dt_pack_mat_wt = 0;  // 包材重量

	CString    c_mat_kind;
	CString    c_confm_plan_no;
	CString    c_ready_bill_no;
	CString    c_order_no;
	CString    c_export_flag;
	CString    c_order_type_code;
	CString    c_chn_or_eng_flag;
	CString    c_strateg_user_flag;
	CString    c_wt_method_code;
	CString    c_dis_flag;
	CDecimal   d_order_wt = 0;
	CString    c_delivy_date;
	CDecimal   d_delivy_wt_tol_plus = 0;
	CDecimal   d_delivy_wt_tol_minus = 0;
	CString    c_order_unit_code;
	CString    c_sg_std;
	CString    c_sg_sign;
	CString    c_psc, c_msc, c_apn, c_apn_desc, c_wkm;
	CString    c_prod_code;
	CString    c_prod_cname, c_prod_ename;
	CString    c_trnp_mode_code;
	CDecimal   d_order_thick = 0;
	CDecimal   d_order_width = 0;
	int        i_order_len = 0;
	CDecimal   d_order_min_len = 0;
	CDecimal   d_order_max_len = 0;
	CString    c_order_eng_thick;
	CString    c_order_eng_width;
	CString    c_order_eng_len;
	CString    c_order_eng_min_len;
	CString    c_order_eng_max_len;
	CString    c_private_route_name;
	CString    c_delivy_place_code;
	CString    c_delivy_place_name;
	CString    c_consign_user_code;
	CString    c_consign_cust_cname;
	CString    c_consign_cust_ename;
	CString    c_order_cust_code;
	CString    c_order_cust_cname;
	CString    c_order_cust_ename;
	CString    c_fin_cust_code;
	CString    c_fin_cust_cname;
	CString    c_fin_cust_ename;
	CString    c_order_special;
	CString    c_order_rem;
	CString    c_order_add_rem;
	int	       i_lable_num = 0;
	CString    c_special_label_instructions;
	CString    c_add_special_label;
	CString    c_mark_1;
	CString    c_mark_2;
	CString    c_order_no_custom;
	CDecimal   d_total_mat_wt = 0;
	int        i_total_mat_num = 0;
	CString    c_part_no;
	CString    c_part_name;
	CString    c_licence_no;
	CString    c_delivery_place_name;
	CString    c_special_label_format_code;
	CString    c_label_format_code;
	CString    c_label_mark;
	CString    c_face_treat_code;
	CString    c_face_treat_desc;
	CString    c_surface_accu_class_code;
	CString    c_surface_accu_class_desc;
	CString    c_plate_wt_code;
	CString    c_plate_wt_desc;
	CString    c_surf_struc_code;
	CString    c_surf_struc_desc;
	CString    c_topcoat_code;
	CString    c_topcoat_desc;
	CString    c_botcoat_code;
	CString    c_botcoat_desc;
	CString    c_contract_no;
	CString    c_consign_user_addr;
	CString    c_pack_type_code;
	CString    c_plate_type_code;
	CString    c_plate_type_desc;
	CString    c_film_proc_code;
	CString    c_film_proc_desc;
	CString    c_blunt_mode_code;
	CString    c_coat_struc_desc;
	CString    c_paint_kind_desc;
	CString    c_order_section;
	CString    c_pm_remark_1;
	CString    c_pm_remark_2;
	CString    c_pm_remark_3;
	CString    c_pm_remark_4;
	CString    c_pm_remark_5;
	CString    c_pm_remark_6;
	CString    c_pm_remark_7;
	CDecimal   d_pm_remark_8 = 0;
	CDecimal   d_pm_remark_9 = 0;
	int        i_pm_remark_a = 0;
	CString    c_end_flag;

	CString		c_id_purchse;


	CString    c_mat_no;
	CDecimal   d_mat_wt = 0;
	CString    c_order_section_act_0;
	CString    c_heat_no;
	CDecimal   dt_mat_thick = 0;
	CDecimal   dt_mat_width = 0;
	CDecimal   dt_mat_len = 0;
	CDecimal   d_mat_act_thick = 0;
	CDecimal   d_mat_act_width = 0;
	int        i_mat_act_len = 0;
	CString    c_pm_ramark_01;
	CString    c_pm_ramark_02;
	CString    c_pm_ramark_03;
	CString    c_pm_ramark_04;
	CString    c_pm_ramark_05;
	CString    c_pm_ramark_06;
	CString    c_pm_ramark_07;
	CDecimal   d_pm_ramark_08 = 0;
	CDecimal   d_pm_ramark_09 = 0;
	int        i_pm_ramark_0a = 0;
	CDecimal   dt_mat_theory_wt = 0, dt_mat_act_wt = 0;

	CDecimal   d_lack_wt = 0;



	CString    c_metric_or_eng_flag;
	CString    c_part_num;
	CString    c_vendor_code;
	CString    c_std_print_lable;
	CString    c_label_special_remark;
	CString    c_lable_pos_code;


	CString	   c_delivy_qty_flag = "0";


	vector <CString> mat_no;


	/* ***** 程序变量 ***** */
	CString c_user = " ", c_tc_no = " ";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr_1(""), sqlstr_2(""), sqlstr_3(""), sqlstr1(""), sqlstr2("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	/* ***** 应用程序开始处理 ***** */
	try
	{
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}

		/* ***** 获取电文号 ***** */
		c_tc_no = s.username;
		c_user = c_tc_no;


		/* ***** 解析电文 ***** */
		tsmpe00.Reset();
		tsmpe00["MAT_KIND"] = bcls_rec->Tables[0].Rows[0]["MAT_KIND"].ToString().TrimOrBlank();	// 物料种类
		tsmpe00["CONFM_PLAN_NO"] = bcls_rec->Tables[0].Rows[0]["CONFM_PLAN_NO"].ToString().TrimOrBlank();	// 准发计划号
		tsmpe00["READY_BILL_NO"] = bcls_rec->Tables[0].Rows[0]["READY_BILL_NO"].ToString().TrimOrBlank();	// 准发单据号
		tsmpe00["ORDER_NO"] = bcls_rec->Tables[0].Rows[0]["ORDER_NO"].ToString().TrimOrBlank();	// 合同号
		//tsmpe00["MAT_KIND"] = bcls_rec->Tables[0].Rows[0]["lack_wt"].ToString().TrimOrBlank();	// 合同准发欠量
		tsmpe00["ORDER_WT"] = bcls_rec->Tables[0].Rows[0]["ORDER_WT"].ToDecimal();	// 订货重量
		tsmpe00["ORDER_WIDTH"] = bcls_rec->Tables[0].Rows[0]["ORDER_WIDTH"].ToDecimal();	// 订货宽度
		tsmpe00["ORDER_LEN"] = bcls_rec->Tables[0].Rows[0]["ORDER_LEN"].ToDouble();	// 订货长度
		tsmpe00["ORDER_THICK"] = bcls_rec->Tables[0].Rows[0]["ORDER_THICK"].ToDecimal();	// 订货厚度
		tsmpe00["SG_STD"] = bcls_rec->Tables[0].Rows[0]["SG_STD"].ToString().TrimOrBlank();	// 标准
		tsmpe00["SG_SIGN"] = bcls_rec->Tables[0].Rows[0]["SG_SIGN"].ToString().TrimOrBlank();	// 牌号
		tsmpe00["PROD_CODE"] = bcls_rec->Tables[0].Rows[0]["PROD_CODE"].ToString().TrimOrBlank();	// 品名代码
		tsmpe00["DELIVY_DATE"] = bcls_rec->Tables[0].Rows[0]["DELIVY_DATE"].ToString().TrimOrBlank();	// 交货日期
		tsmpe00["PLAN_NUM"] = bcls_rec->Tables[0].Rows[0]["TOTAL_MAT_NUM"].ToDouble();	// 单据内材料数
		c_end_flag = bcls_rec->Tables[0].Rows[0]["end_flag"].ToString().TrimOrBlank();	// 结束标记

		c_confm_plan_no = tsmpe00["CONFM_PLAN_NO"];
		
		

		// 检查准发计划头电文是否已经收到
		tsmpe01["CONFM_PLAN_NO"] = tsmpe00["CONFM_PLAN_NO"];
		if (!tsmpe01.Query("CONFM_PLAN_NO"))
		{
			CFormattable arguments[] = { tsmpe01["CONFM_PLAN_NO"].ToString() };
			CMessageFormat::Format(s.msg, "此准发计划号[{0}]电文还没有接收", arguments, 1);
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}

		//检查准发单据是否存在，已存在报错
		sqlstr = "SELECT count(1) FROM tsmpe00 WHERE ready_bill_no = @read_bill_no";
		execute_sql.SetCommandText(sqlstr);
		execute_sql.Parameters.Clear();
		execute_sql.Parameters.Set("read_bill_no", tsmpe00["READY_BILL_NO"].ToString());
		row_count = execute_sql.ExecuteScalar().ToInt32();

		if (row_count > 0)
		{
			CFormattable arguments[] = { tsmpe00["READY_BILL_NO"].ToString() }; // 定义参数列表的数组
			CMessageFormat::Format(s.msg, "单据号[{0}]已存在.", arguments, 1);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		// 根据品名代码读取产品名称
		sqlstr = " select code_desc_1_content , code_desc_2_content "
			" FROM	TEP0002 "
			" WHERE code_class = 'QM02' "
			" AND	CODE = @c_prod_code ";
		execute_sql.SetCommandText(sqlstr);
		execute_sql.Parameters.Clear();
		execute_sql.Parameters.Set("c_prod_code", tsmpe00["PROD_CODE"]);
		execute_sql.ExecuteReader();
		if (execute_sql.Read())
		{
			tsmpe00["PROD_CNAME"] = execute_sql.GetString(1);
			tsmpe00["PROD_ENAME"] = execute_sql.GetString(2);
		}
		execute_sql.Close();


		// 根据仓库代码读取厂别
		sqlstr = "select factory_div from vsmpea9 where stock_no = @stock_no ";
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("stock_no", tsmpe01["STOCK_NO"]);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			tsmpe00["FACTORY_DIV"] = cmd_inq.GetString(1);
		}
		else
		{
			sprintf(s.msg, "此库区【%s】不是可发货库", (const char *)tsmpe01["STOCK_NO"].ToString());
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}
		cmd_inq.Close();

		
		// 读取合同性质代码
		sqlstr = "select ORDER_TYPE_CODE,EXPORT_FLAG,WT_METHOD_CODE,TRNP_MODE_CODE,CONTRACT_NO,DELIVY_QTY_FLAG,PROD_CNAME,PROD_ENAME,PICK_DELIVERY_FLAG from tom01 where order_no = '" + tsmpe00["ORDER_NO"].ToString() + "' ";
		cmd_inq.SetCommandText(sqlstr);
		Log::Info("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			tsmpe00["ORDER_TYPE_CODE"] = cmd_inq.GetString(1);
			tsmpe00["EXPORT_FLAG"] = cmd_inq.GetString(2);
			tsmpe00["WT_METHOD_CODE"] = cmd_inq.GetString(3);
			tsmpe00["TRNP_MODE_CODE"] = cmd_inq.GetString(4);
			c_contract_no = cmd_inq.GetString(5);
			c_delivy_qty_flag = cmd_inq.GetString(6);
			tsmpe00["PROD_CNAME"] = cmd_inq.GetString(7);
			tsmpe00["PROD_ENAME"] = cmd_inq.GetString(8);
			c_delivy_qty_flag = cmd_inq.GetString(9); //pick_delivery_flag按量标记用此字段20230922 liguangyuan
		}
		else
		{
			sprintf(s.msg, "无此合同【%s】的信息！", (const char *)tsmpe00["ORDER_NO"].ToString());
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}
		cmd_inq.Close();


		// 新增准发记录
		tsmpe00["REC_CREATOR"] = c_user;
		tsmpe00["REC_CREATE_TIME"] = c_datetime;
		tsmpe00["REC_REVISE_TIME"] = c_datetime;
		tsmpe00["REC_REVISOR"] = c_user;
		tsmpe00["CONFM_STATUS"] = "2";
		tsmpe00["STOCK_NO"] = tsmpe01["STOCK_NO"];


		tsmpe00["WT_METHOD_NAME"] = (tsmpe00["WT_METHOD_CODE"].ToString() == "0") ? "实重" : "理重";
		//tsmpe00.DIS_FLAG = c_dis_flag;//区分标志
		//tsmpe00["ORDER_WT"] = d_order_wt;//订货重量
		//tsmpe00["DELIVY_DATE"] = c_delivy_date; // 交货日期
		//tsmpe00["DELIVY_WT_TOL_PLUS"] = d_delivy_wt_tol_plus;// 交货重量正公差
		//tsmpe00["DELIVY_WT_TOL_MINUS"] = d_delivy_wt_tol_minus;//交货重量负公差
		//tsmpe00["ORDER_UNIT_CODE"] = c_order_unit_code;//订货计量单位代码
		//tsmpe00["SG_STD"] = c_sg_std;//标准
		//tsmpe00["SG_SIGN"] = c_sg_sign;//牌号（钢级）
		//tsmpe00.PSC = c_psc;//产品规范码
		//tsmpe00.MSC = c_msc;//冶金规范码
		//tsmpe00["STD_PRINT_LABLE"] = c_std_print_lable;	// 标签打印用标准
		//tsmpe00["APN"] = c_apn;// 产品最终用途码
		//tsmpe00["APN_DESC"] = c_apn_desc;//产品最终用途说明
		//tsmpe00.WKM = c_wkm;//客户特殊要求码
		//tsmpe00["PROD_CODE"] = c_prod_code;//品名代码
		//tsmpe00["PROD_CNAME"] = c_prod_cname;//品名中文
		//tsmpe00["PROD_ENAME"] = c_prod_ename;//品名英文
		//tsmpe00["TRNP_MODE_CODE"] = c_trnp_mode_code;//运输方式代码
		//tsmpe00["ORDER_THICK"] = d_order_thick;//订货厚度
		//tsmpe00["ORDER_WIDTH"] = d_order_width;//订货宽度
		//tsmpe00["ORDER_LEN"] = i_order_len;//订货长度
		//tsmpe00["ORDER_MIN_LEN"] = d_order_min_len;//订货最小长度
		//tsmpe00["ORDER_MAX_LEN"] = d_order_max_len;//订货最大长度
		//tsmpe00["ORDER_ENG_THICK"] = c_order_eng_thick;//订货英制厚度
		//tsmpe00["ORDER_ENG_WIDTH"] = c_order_eng_width;//订货英制宽度
		//tsmpe00["ORDER_ENG_LEN"] = c_order_eng_len;//订货英制长度
		//tsmpe00["ORDER_ENG_MIN_LEN"] = c_order_eng_min_len;//订货英制最小长度
		//tsmpe00["ORDER_ENG_MAX_LEN"] = c_order_eng_max_len;//订货英制最大长度
		//tsmpe00["PRIVATE_ROUTE_NAME"] = c_private_route_name;//专用线名称
		//tsmpe00["DELIVY_PLACE_CODE"] = c_delivy_place_code;//交货地点代码
		//tsmpe00["DELIVY_PLACE_NAME"] = c_delivy_place_name;//交货地点名称
		//tsmpe00["CONSIGN_USER_CODE"] = c_consign_user_code;//收货用户代码
		//tsmpe00["CONSIGN_CUST_CNAME"] = c_consign_cust_cname;//收货用户中文名称
		//tsmpe00["CONSIGN_CUST_ENAME"] = c_consign_cust_ename;//收货用户英文名称
		//tsmpe00["ORDER_CUST_CODE"] = c_order_cust_code;//订货用户代码
		//tsmpe00["ORDER_CUST_CNAME"] = c_order_cust_cname;//订货用户中文名称
		//tsmpe00["ORDER_CUST_ENAME"] = c_order_cust_ename;//订货用户英文名称
		//tsmpe00["FIN_CUST_CODE"] = c_fin_cust_code;//最终用户代码
		//tsmpe00["FIN_CUST_CNAME"] = c_fin_cust_cname;//最终用户中文名称
		//tsmpe00["FIN_CUST_ENAME"] = c_fin_cust_ename;//最终用户英文名称
		//tsmpe00["ORDER_SPECIAL"] = c_order_special;//合同特殊要求
		//tsmpe00["ORDER_REM"] = c_order_rem;//合同备注
		//tsmpe00["ORDER_ADD_REM"] = c_order_add_rem;//合同补充说明
		//tsmpe00["LABLE_NUM"] = i_lable_num;//标签数量
		//tsmpe00.SPECIAL_LABEL_INSTRUCTIONS = c_special_label_instructions;//特殊标签指示
		//tsmpe00.ADD_SPECIAL_LABEL = c_add_special_label;//增加特殊标签
		//tsmpe00["MARK_1"] = c_mark_1;//唛头1
		//tsmpe00["MARK_2"] = c_mark_2;//唛头2
		//tsmpe00["ORDER_NO"]_CUSTOM = c_order_no_custom;//客户订单号
		//tsmpe00["TOTAL_MAT_WT"] = d_total_mat_wt;//合计材料重量
		//tsmpe00["TOTAL_MAT_NUM"] = i_total_mat_num;//合计材料个数
		//tsmpe00["PART_NO"] = c_part_no;//零部件号
		//tsmpe00.PART_NAME = c_part_name;//零件名称
		//tsmpe00.LICENCE_NO = c_licence_no;//许可证编号
		//tsmpe00["DELIVY_PLACE_NAME"] = c_delivery_place_name; //到站港
		//tsmpe00.SPECIAL_LABEL_FORMAT_CODE = c_special_label_format_code;//特殊标签格式代码
		//tsmpe00["LABEL_FORMAT_CODE"] = c_label_format_code;//标签指示
		//tsmpe00.LABEL_MARK = c_label_mark;//标签备注
		//tsmpe00.FACE_TREAT_CODE = c_face_treat_code;//表面处理代码
		//tsmpe00.FACE_TREAT_DESC = c_face_treat_desc;//表面处理描述
		//tsmpe00.SURFACE_ACCU_CLASS_CODE = c_surface_accu_class_code;//表面质量等级代码
		//tsmpe00.SURFACE_ACCU_CLASS_DESC = c_surface_accu_class_desc;//表面质量等级描述
		//tsmpe00.PLATE_WT_CODE = c_plate_wt_code;//镀层重量代码
		//tsmpe00.PLATE_WT_DESC = c_plate_wt_desc;//镀层重量描述
		//tsmpe00.SURF_STRUC_CODE = c_surf_struc_code;//表面结构代码
		//tsmpe00.SURF_STRUC_DESC = c_surf_struc_desc;//表面结构描述
		//tsmpe00.TOPCOAT_CODE = c_topcoat_code;//上表面面漆代码
		//tsmpe00.TOPCOAT_DESC = c_topcoat_desc;//上表面面漆代码描述
		//tsmpe00.BOTCOAT_CODE = c_botcoat_code;// 下表面面漆代码
		//tsmpe00.BOTCOAT_DESC = c_botcoat_desc;//下表面面漆代码描述
		//tsmpe00.CONTRACT_NO = c_contract_no;//合约号
		//tsmpe00["CONSIGN_USER_ADDR"] = c_consign_user_addr;//收货用户地址
		//tsmpe00["PACK_TYPE_CODE"] = c_pack_type_code;//包装方式代码
		//tsmpe00.PLATE_TYPE_CODE = c_plate_type_code;//镀层类型代码
		//tsmpe00.PLATE_TYPE_DESC = c_plate_type_desc;//镀层类型描述
		//tsmpe00.FILM_PROC_CODE = c_film_proc_code;//覆膜处理代码
		//tsmpe00.FILM_PROC_DESC = c_film_proc_desc;//覆膜处理描述
		//tsmpe00.BLUNT_MODE_CODE = c_blunt_mode_code;//钝化方式
		//tsmpe00.COAT_STRUC_DESC = c_coat_struc_desc;//涂层结构
		//tsmpe00["PAINT_KIND_DESC"] = c_paint_kind_desc;//面漆种类
		//tsmpe00.ORDER_SECTION = c_order_section;// 成品截面
		//tsmpe00.PM_REMARK_1 = c_pm_remark_1;//PM备用字段1
		//tsmpe00.PM_REMARK_2 = c_pm_remark_2;//PM备用字段2
		//tsmpe00.PM_REMARK_3 = c_pm_remark_3;//PM备用字段3
		//tsmpe00.PM_REMARK_4 = c_pm_remark_4;//PM备用字段4
		//tsmpe00.PM_REMARK_5 = c_pm_remark_5;//PM备用字段5
		//tsmpe00.PM_REMARK_6 = c_pm_remark_6;//PM备用字段6
		//tsmpe00.PM_REMARK_7 = c_pm_remark_7;//PM备用字段7
		//tsmpe00.PM_REMARK_8 = d_pm_remark_8;//PM备用字段8
		//tsmpe00.PM_REMARK_9 = d_pm_remark_9;//PM备用字段9
		//tsmpe00.PM_REMARK_A = i_pm_remark_a;//PM备用字段a
		//tsmpe00.ID_PURCHSE = c_id_purchse;//购单号



		sqlstr = "INSERT INTO TSMPE00 ready_bill_no = '" + tsmpe00["READY_BILL_NO"].ToString() + "' ";
		tsmpe00.TrimOrBlank();

		//tsmpe00.Print();
		tsmpe00.Insert();


		/**************** 接收准发材料 **********************/
		row_sum = bcls_rec->Tables[0].Rows.get_Count();
		//row_sum = tsmpe00["TOTAL_MAT_NUM"].ToDecimal().ToInt32();
		tsmpe02.Reset();
		tsmpe02.CopyFrom(tsmpe00);

		bcls_rec->Tables[record_name].Rows.Clear();		// 2022-3-17

		for (i = 0; i < row_sum; i++)
		{
			c_mat_no = bcls_rec->Tables[0].Rows[i]["mat_no"].ToString().TrimOrBlank();  // 材料号
			d_mat_wt = bcls_rec->Tables[0].Rows[i]["mat_wt"].ToDecimal();  //   材料重量

			Log::Info("", __FUNCTION__, "第[{0}]个，共[{1}]个", i + 1, bcls_rec->Tables[0].Rows.get_Count());
			Log::Info("", __FUNCTION__, "材料号=[{0}] , 重量=[{1}]", c_mat_no, d_mat_wt);

			if (c_mat_no.Compare(" ") == 0)
			{
				break;
				//sprintf(s.msg, "材料号不能为空");
				//throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			table_name = "TMM" + tsmpe00["MAT_KIND"].ToString() + "01";
			CString c_stock_no = "";
			c_mat_kind = tsmpe00["MAT_KIND"];

			Log::Debug("", __FUNCTION__, "table_name=[{0}]", table_name);


			// 读取物料信息
			sqlstr = " SELECT * FROM " + table_name + " WHERE mat_no = '" + c_mat_no + "' ";

			execute_sql.SetCommandText(sqlstr);
			Log::Debug("", __FUNCTION__, "sqlstr=[{0}]", sqlstr);
			execute_sql.ExecuteQuery(bcls_ret_buffer.Tables[0]);

			if (bcls_ret_buffer.Tables[0].Rows.get_Count() != 1)
			{
				CFormattable arguments[] = { c_mat_no }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, "读取物料材料表出错，无此材料号[{0}]", arguments, 1);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			ct_pono = bcls_ret_buffer.Tables[0].Rows[0]["PONO"].ToString();
			ct_complex_decide_code = bcls_ret_buffer.Tables[0].Rows[0]["COMPLEX_DECIDE_CODE"].ToString();
			dt_mat_thick = bcls_ret_buffer.Tables[0].Rows[0]["MAT_THICK"].ToDecimal();
			dt_mat_width = bcls_ret_buffer.Tables[0].Rows[0]["MAT_WIDTH"].ToDecimal();
			dt_mat_len = bcls_ret_buffer.Tables[0].Rows[0]["MAT_LEN"].ToDecimal();
			dt_mat_theory_wt = bcls_ret_buffer.Tables[0].Rows[0]["MAT_THEORY_WT"].ToDecimal();
			dt_mat_act_wt = bcls_ret_buffer.Tables[0].Rows[0]["MAT_ACT_WT"].ToDecimal();
			tsmpe02["MAT_TUBE"] = bcls_ret_buffer.Tables[0].Rows[0]["MAT_NUM"].ToDecimal();
			tsmpe02["PSC"] = bcls_ret_buffer.Tables[0].Rows[0]["PSC"].ToString();
			tsmpe02["ORDER_NO"] = bcls_ret_buffer.Tables[0].Rows[0]["ORDER_NO"].ToString();
			c_sg_sign = bcls_ret_buffer.Tables[0].Rows[0]["SG_SIGN"].ToString();
			dt_pack_mat_wt = bcls_ret_buffer.Tables[0].Rows[0]["PACK_MAT_WT"].ToDecimal();
			i_mat_act_len = bcls_ret_buffer.Tables[0].Rows[0]["MAT_ACT_LEN"].ToDecimal().ToInt32();
			c_stock_no = bcls_ret_buffer.Tables[0].Rows[0]["STOCK_NO"].ToString();

			if (c_mat_kind == "SM")
			{
				tsmpe02["FIX_FLAG"] = bcls_ret_buffer.Tables[0].Rows[0]["FIX_FLAG"].ToString();
			}
			if (c_mat_kind == "BW")
			{
				tsmpe02["CROSS_CODE"] = bcls_ret_buffer.Tables[0].Rows[0]["CROSS_CODE"].ToString();
				tsmpe02["FIX_FLAG"] = bcls_ret_buffer.Tables[0].Rows[0]["FIX_FLAG"].ToString();
			}


			////if (c_mat_kind == "SM")
			////{
			////	sqlstr = " SELECT  'SM',pono,complex_decide_code,mat_thick,mat_width,mat_len,mat_theory_wt,mat_act_wt ,1 as mat_num , ' ',PSC,ORDER_NO,SG_SIGN,PACK_MAT_WT,mat_act_len,STOCK_NO,FIX_FLAG FROM " + table_name + " WHERE mat_no = @mat_no ";
			////}
			////else if (c_mat_kind == "BW")
			////{
			////	sqlstr = " SELECT  'BW',pono,complex_decide_code,mat_thick,mat_width,mat_len,mat_theory_wt,mat_act_wt ,mat_num ,CROSS_CODE,PSC,ORDER_NO,SG_SIGN,PACK_MAT_WT,mat_act_len,STOCK_NO,FIX_FLAG  FROM " + table_name + " WHERE mat_no = @mat_no ";
			////}
			////else if (c_mat_kind == "HR")
			////{
			////	sqlstr = " SELECT  'HR',pono,complex_decide_code,mat_thick,mat_width,mat_len,mat_theory_wt,mat_act_wt ,mat_num ,' ',PSC,ORDER_NO,SG_SIGN,PACK_MAT_WT,mat_act_len,STOCK_NO,' ' FROM " + table_name + " WHERE mat_no = @mat_no ";
			////}
			////else if (c_mat_kind == "CR")
			////{
			////	sqlstr = " SELECT  'CR',pono,complex_decide_code,mat_ACT_thick,mat_ACT_width,mat_ACT_len,mat_theory_wt,mat_act_wt ,mat_num ,' ',PSC,ORDER_NO,SG_SIGN,PACK_MAT_WT,mat_act_len,STOCK_NO,' ' FROM " + table_name + " WHERE mat_no = @mat_no ";
			////}
			////else if (c_mat_kind == "HP")
			////{
			////	sqlstr = " SELECT  'HP',pono,complex_decide_code,mat_ACT_thick,mat_ACT_width,mat_ACT_len,mat_theory_wt,mat_act_wt ,1 AS mat_num ,' ',PSC,ORDER_NO,SG_SIGN,PACK_MAT_WT,mat_act_len,STOCK_NO,' ' FROM " + table_name + " WHERE mat_no = @mat_no ";
			////}
			////else if (c_mat_kind == "BS")
			////{
			////	sqlstr = " SELECT  'BS',pono,complex_decide_code,mat_ACT_thick,mat_ACT_width,mat_ACT_len,mat_theory_wt,mat_act_wt ,1 AS mat_num ,' ',PSC,ORDER_NO,SG_SIGN,PACK_MAT_WT,mat_act_len,STOCK_NO,' ' FROM " + table_name + " WHERE mat_no = @mat_no ";
			////}
			////else if (c_mat_kind == "BT")
			////{
			////	sqlstr = " SELECT  'BT',pono,complex_decide_code,mat_ACT_thick,mat_ACT_width,mat_ACT_len,mat_theory_wt,mat_act_wt ,1 AS mat_num ,' ',PSC,ORDER_NO,SG_SIGN,PACK_MAT_WT,mat_act_len,STOCK_NO,' ' FROM " + table_name + " WHERE mat_no = @mat_no ";
			////}
			////else if (c_mat_kind == "SN")
			////{
			////	sqlstr = " SELECT  'SN',pono,complex_decide_code,mat_thick,mat_width,mat_len,mat_theory_wt,mat_act_wt ,1 as mat_num , ' ',PSC,ORDER_NO,SG_SIGN,PACK_MAT_WT,mat_act_len,STOCK_NO,' ' FROM " + table_name + " WHERE mat_no = @mat_no ";
			////}
			////else
			////{
			////	sprintf(s.msg, "无此物料种类[%s]代码的逻辑，请维护程序", (const char *)c_mat_kind);
			////	throw	CApplicationException(-1, s.msg, s.svc_name);
			////}


			////Log::Debug("", __FUNCTION__, "sqlstr=[{0}]", sqlstr);
			////execute_sql.SetCommandText(sqlstr);
			////execute_sql.Parameters.Clear();
			////execute_sql.Parameters.Set("mat_no", c_mat_no);
			////execute_sql.Parameters.Set("mat_kind", c_mat_kind);
			////execute_sql.ExecuteReader();

			////if (execute_sql.Read())
			////{
			////	ct_mat_kind = execute_sql.GetString(1);
			////	ct_pono = execute_sql.GetString(2);
			////	ct_complex_decide_code = execute_sql.GetString(3);
			////	dt_mat_thick = execute_sql.GetDecimal(4);
			////	dt_mat_width = execute_sql.GetDecimal(5);
			////	dt_mat_len = execute_sql.GetDecimal(6);
			////	dt_mat_theory_wt = execute_sql.GetDecimal(7);
			////	dt_mat_act_wt = execute_sql.GetDecimal(8);
			////	tsmpe02["MAT_TUBE"] = execute_sql.GetInt32(9);
			////	tsmpe02["CROSS_CODE"] = execute_sql.GetString(10);
			////	tsmpe02["PSC"] = execute_sql.GetString(11);
			////	tsmpe02["ORDER_NO"] = execute_sql.GetString(12);
			////	c_sg_sign = execute_sql.GetString(13);
			////	dt_pack_mat_wt = execute_sql.GetDecimal(14);
			////	i_mat_act_len = execute_sql.GetInt32(15);
			////	c_stock_no = execute_sql.GetString(16);
			////	tsmpe02["FIX_FLAG"] = execute_sql.GetString(17);
			////}
			////else
			////{
			////	CFormattable arguments[] = { c_mat_no }; // 定义参数列表的数组
			////	CMessageFormat::Format(s.msg, "读取物料材料表出错，无此材料号[{0}]", arguments, 1);
			////	throw CApplicationException(-1, s.msg, s.svc_name);
			////}
			////execute_sql.Close();

			if (c_stock_no != tsmpe00["STOCK_NO"].ToString() )
			{
				CFormattable arguments[] = { c_mat_no, c_stock_no,tsmpe00["STOCK_NO"].ToString() }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, "材料 {0} 上的库区号 {1} 和准发上的库区 {2} 不一致！", arguments, 3);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			// 判材料上的合同号和准发计划上的合同号是否一致，否报错
			if (tsmpe02["ORDER_NO"].ToString() != tsmpe00["ORDER_NO"].ToString())
			{
				CFormattable arguments[] = { c_mat_no, tsmpe02["ORDER_NO"].ToString(), tsmpe00["ORDER_NO"].ToString() }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, "材料号[{0}],上合同号[{2}]和物料表上合同号[{1}]不一致", arguments, 3);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}


			tsmpe02["REC_CREATOR"] = c_user;
			tsmpe02["REC_CREATE_TIME"] = c_datetime;
			tsmpe02["REC_REVISE_TIME"] = c_datetime;
			tsmpe02["REC_REVISOR"] = c_user;
			tsmpe02["ARCHIVE_FLAG"] = "0";
			tsmpe02["MAT_NO"] = c_mat_no;
			tsmpe02["CONFM_STATUS"] = "2";
			//tsmpe02["FACTORY_DIV"] = ct_factory_div;
			//tsmpe02["MAT_KIND"] = c_mat_kind;
			tsmpe02["STOCK_NO"] = tsmpe01["STOCK_NO"];
			//tsmpe02["CONFM_PLAN_NO"] = c_confm_plan_no;
			//tsmpe02["READY_BILL_NO"] = c_ready_bill_no;
			tsmpe02["BILL_OF_LADING_NO"] = " ";
			tsmpe02["STACKING_NO"] = " ";
			tsmpe02["LEAVE_FACTORY_CARD"] = " ";
			tsmpe02["CONFM_TIME"] = " ";
			tsmpe02["BILL_CONFM_DATE"] = " ";
			tsmpe02["CONFM_SHIFT"] = " ";
			tsmpe02["CONFM_GROUP"] = " ";
			tsmpe02["CONFM_MAKER"] = " ";
			tsmpe02["DELIVY_TIME"] = " ";
			tsmpe02["OUT_FACT_DATE"] = " ";
			tsmpe02["DELIVY_SHIFT"] = " ";
			tsmpe02["DELIVY_GROUP"] = " ";
			tsmpe02["DELIVY_MAKER"] = " ";
			tsmpe02["VEHICLE_NO"] = " ";

			tsmpe02["MAT_WT"] = d_mat_wt;
			tsmpe02["WT_MODE"] = tsmpe00["WT_METHOD_CODE"];
			tsmpe02["OLD_ORDER_NO"] = tsmpe02["ORDER_NO"];
			tsmpe02["PROD_CODE"] = tsmpe00["PROD_CODE"];
			tsmpe02["PROD_CNAME"] = tsmpe00["PROD_CNAME"];
			tsmpe02["PROD_ENAME"] = tsmpe00["PROD_ENAME"];
			tsmpe02["SG_SIGN"] = c_sg_sign;
			tsmpe02["RED_FLAG"] = "0";
			tsmpe02["RED_CAUSE_CODE"] = " ";
			tsmpe02["RED_CAUSE_DESC"] = " ";
			tsmpe02["PRINT_NUM"] = 0;
			tsmpe02["CONFM_SCAN_MARK"] = " ";
			tsmpe02["DELIVY_SCAN_MARK"] = " ";
			//tsmpe02["MATCH_ERROR_SEQ"] =    ;

			tsmpe02["FORCE_PASS_FLAG"] = "0";
			tsmpe02["EXE_TIME_PLAN"] = " ";
			//tsmpe02["DELIVY_QTY_FLAG"] = c_delivy_qty_flag == "1" ? "1" : "0";	// 按量发货（捡配）标记 2013-08-14增加
			tsmpe02["DELIVY_QTY_FLAG"] = c_delivy_qty_flag;

			tsmpe02["PONO"] = ct_pono;
			tsmpe02["COMPLEX_DECIDE_CODE"] = ct_complex_decide_code;
			tsmpe02["MAT_THICK"] = dt_mat_thick;
			tsmpe02["MAT_WIDTH"] = dt_mat_width;
			tsmpe02["MAT_LEN"] = dt_mat_len;
			//tsmpe02["MAT_ACT_LEN"] = i_mat_act_len;
			tsmpe02["MAT_THEORY_WT"] = dt_mat_theory_wt;
			//tsmpe02["MAT_GROSS_WT"] = dt_pack_mat_wt + tsmpe02["MAT_WT"].ToDouble();		// 材料毛重
			tsmpe02["MAT_ACT_WT"] = dt_mat_act_wt;

			tsmpe02["CONTRACT_NO"] = c_contract_no;	//合约号

			tsmpe02["CUST_MAT_SPECS"] = CString::Format("%g", tsmpe02["MAT_THICK"].ToDouble());
			if (tsmpe02["MAT_WIDTH"].ToDouble() != 0)
			{
				tsmpe02["CUST_MAT_SPECS"] = tsmpe02["CUST_MAT_SPECS"].ToString() + "*" + CString::Format("%g", tsmpe02["MAT_WIDTH"].ToDouble());
			}
			else
			{
				tsmpe02["CUST_MAT_SPECS"] = "φ" + tsmpe02["CUST_MAT_SPECS"].ToString();
			}
			if (tsmpe02["MAT_LEN"].ToDouble() != 0)
			{
				tsmpe02["CUST_MAT_SPECS"] = tsmpe02["CUST_MAT_SPECS"].ToString() + "*" + CString::Format("%g", tsmpe02["MAT_LEN"].ToDouble());
			}

			sqlstr = "INSERT TSMPE02  MAT_NO = '" + tsmpe02["MAT_NO"].ToString() + "' ";
			tsmpe02.TrimOrBlank();

			if (!tsmpe02.Insert())
			{
				CFormattable arguments[] = { c_mat_no }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, "新增准发材料记录失败，材料号[{0}]", arguments, 1);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			/* ***** 调用材料模块函数 ***** */
			mat_no.push_back(tsmpe02["MAT_NO"]);

			if (f_sm00_mm99(c_mat_no, 3, 4, s.msg, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			bcls_rec->Tables[record_name].Rows.Add();
			int ii = bcls_rec->Tables[record_name].Rows.get_Count() - 1;

			bcls_rec->Tables[record_name].Rows[ii]["mat_no"] = c_mat_no;
			bcls_rec->Tables[record_name].Rows[ii]["event_mark"] = "0";
			bcls_rec->Tables[record_name].Rows[ii]["userid"] = c_user;

			//ret = 0;
			//ret = f_sm00_record(bcls_rec, bcls_ret, conn);
			//if (ret < 0)
			//{
			//	Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
			//	throw CApplicationException(-1, s.msg, s.svc_name);
			//}
		}
		ret = 0;
		ret = f_sm00_record(bcls_rec, bcls_ret, conn);
		if (ret < 0)
		{
			Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		bcls_rec->Tables[record_name].Clear();

		/**************** 循环结束 **********************************************/
		if (f_sm00_mm99(mat_no, 3, 4, s.msg, conn) != 0)
		{
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		/**************** 判断是否是该计划下的最后一个单据 **********************/

		// 按准发计划号读取单据数，和准发计划比较是否一致
		int i_plan_num = 0;
		sqlstr = "select count(1) from tsmpe00 where confm_plan_no = @confm_plan_no ";
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("confm_plan_no", tsmpe00["CONFM_PLAN_NO"]);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			i_plan_num = cmd_inq.GetInt32(1);
		}
		cmd_inq.Close();

		Log::Debug("", "", "i_plan_num={0}/*,PLAN_BILL_NUM = {1}*/,c_end_flag = {1}", i_plan_num, /*tsmpe01["PLAN_BILL_NUM"].ToString(),*/ c_end_flag);
		if (c_end_flag.Compare("1") == 0 /*&& i_plan_num == tsmpe01["PLAN_BILL_NUM"].ToDouble()*/)
		{
			sqlstr = CString(
				" UPDATE tsmpe01 SET confm_status = '2' WHERE confm_plan_no = @confm_plan_no AND confm_status < '4' "
				);

			sqlstr1 = CString(
				" UPDATE tsmpe00 SET confm_status = '2'  WHERE confm_plan_no = @confm_plan_no AND confm_status < '4' "
				);

			sqlstr2 = CString(
				" UPDATE tsmpe02 SET confm_status = '2'  WHERE confm_plan_no = @confm_plan_no AND confm_status < '4' "
				);


			/**************** 修改准发计划表、准发单据表、准发材料表的状态 **********************/
			Log::Debug("", __FUNCTION__, "wuhao1");
 			sqlstr = sqlstr;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("mat_kind", c_mat_kind);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteNonQuery();

			Log::Debug("", __FUNCTION__, "wuhao2");

			sqlstr = sqlstr1;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteNonQuery();

			Log::Debug("", __FUNCTION__, "wuhao3");

			sqlstr = sqlstr2;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteNonQuery();
			Log::Debug("", __FUNCTION__, "wuhao4");
		}

		//更新准发计划表,准发单据表计划重量 材料总重量
		Log::Debug("", __FUNCTION__, "wuhao5");

		if (c_end_flag == "1")	// 2022-3-15
		{
			doFlag = f_sm00_count(c_confm_plan_no, c_user, conn);
		}
		Log::Debug("", __FUNCTION__, "wuhao6");

		if (doFlag < 0)

		{
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		Log::Debug("", __FUNCTION__, "wuhao7");



		/* 事务结束，另起一个事务*/
		CTransactionManager::Commit(0);
		CTransactionManager::Begin(0, 0);

////		// 发送接收应答电文
////		CString tc_no = "";
////		tc_no = c_tc_no.SubstringNE(2, 2) + c_tc_no.SubstringNE(0, 2) + "04";
////		// 生成电文发送对象
////		EPEX epex(&s);
////
////		// 初始化电文格式
////		if (epex.Initialize(tc_no) < 0)   //电文号
////		{
////			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
////			CMessageFormat::Format(s.msg, "初始化电文时失败！ 原因描述：[ {0}]", arguments, 1);//格式化字符串
////			throw	CApplicationException(-1, s.msg, log.Location);
////		}
////
////		/* 数据压电文 */
////		if (   epex.SetValue("tc_no", 0, tc_no) < 0		// 电文号
////			|| epex.SetValue("confm_plan_no", 0, tsmpe00["CONFM_PLAN_NO"].ToString()) < 0	// 准发计划号
////			|| epex.SetValue("bill_no", 0, tsmpe00["READY_BILL_NO"].ToString()) < 0	// 单据号
////			|| epex.SetValue("match_flag", 0, "0") < 0	// 应答状态
////			|| epex.SetValue("mat_num", 0, tsmpe00["PLAN_NUM"].ToDecimal()) < 0	// 计划件数
////			|| epex.SetValue("mat_wt", 0, tsmpe00["PLAN_WT"].ToDecimal()) < 0	// 计划重量
////			|| epex.SetValue("remark_1", 0, s.msg) < 0	// 备注
////			)
////		{
////			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
////			CMessageFormat::Format(s.msg, "写入电文体数据时出错! 原因描述： [{0}]", arguments, 1);//格式化字符串
////			throw CApplicationException(-1, s.msg, log.Location);
////		}
////
////		// 发送电文
////		if (epex.SendTele() < 0)
////		{
////			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
////			CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
////			throw	CApplicationException(-1, s.msg, log.Location);
////		}
////
////		// 释放
////		epex.Uninitialize();
////
////		v_flag = "1";
////
////		/* 事务结束，另起一个事务*/
////		CTransactionManager::Commit(0);
////		CTransactionManager::Begin(0, 0);


		Log::Debug("", __FUNCTION__, "c_end_flag={0}",c_end_flag);
		Log::Debug("", __FUNCTION__, "tsmpe02.MAT_KIND={0}", tsmpe02["MAT_KIND"]);
		Log::Debug("", __FUNCTION__, "c_mat_kind={0}", c_mat_kind);

////		//热轧产线
////		if (tsmpe02.MAT_KIND == "HR")
////		{
////#ifdef _LINE_HR
////			if (f_mmhrauth_cfm(bcls_rec, bcls_ret, conn) != 0)
////			{
////				throw CApplicationException(-1, s.msg, log.Location);
////			}
////#endif
////		}
////		else
		{
			//首先判断是否是该计划的最后一条单据
			if (c_end_flag.Compare("1") == 0 /*|| i_plan_num == tsmpe01.PLAN_BILL_NUM*/)
			{
				//add by chenkai:2015-07-31 根据SM29自动准发标记判断是否自动准发
				tep0002.Reset();
				tep0002["CODE_CLASS"] = "SMPE29";
				tep0002["CODE"] = c_mat_kind;
				Log::Trace("", "__FUNCTION__", "tep0002.CODE=[{0}]", tep0002["CODE"]);
				if (tep0002.Query("CODE_CLASS,CODE"))
				{
					Log::Trace("", "__FUNCTION__", "tep0002.CODE_DESC_2_CONTENT=[{0}]", tep0002["CODE_DESC_2_CONTENT"]);
					if (tep0002["CODE_DESC_2_CONTENT"].ToString().Trim() == "1" )  //启用标志
					{
						bcls_zrec.Tables[0].Columns.Add(DT_STRING, "confm_plan_no");
						bcls_zrec.Tables[0].Rows.Add();
						bcls_zrec.Tables[0].Rows[0]["confm_plan_no"] = c_confm_plan_no;
						Log::Trace("", "__FUNCTION__", "confm_plan_no=[{0}]", c_confm_plan_no);
						if (f_sm00_confirm(&bcls_zrec, &bcls_zret, conn) < 0)
						{
							throw CException(-1, s.msg, s.svc_name);
						}
					}
				}
			}

		}
	}

	catch (CDbException& ex)  //捕获数据库操作异常
	{

		CFormattable arguments[] = { ex.GetCode() , ex.GetMsg() };// 定义参数列表的数组
		CMessageFormat::Format(s.msg, "数据库处理出错sqlcode=[{0}] \r\n sqlMsg={1}", arguments, 2);//格式化字符串
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


		/////* 事务结束，另起一个事务*/
		////CTransactionManager::Abort(0);
		////CTransactionManager::Begin(0, 0);

		////// 发送接收应答电文
		////CString tc_no = "";
		////tc_no = c_tc_no.SubstringNE(2, 2) + c_tc_no.SubstringNE(0, 2) + "04";
		////// 生成电文发送对象
		////EPEX epex(&s);

		////// 初始化电文格式
		////if (epex.Initialize(tc_no) < 0)   //电文号
		////{
		////	CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
		////	CMessageFormat::Format(s.msg, "初始化电文时失败！ 原因描述：[ {0}]", arguments, 1);//格式化字符串
		////	throw	CApplicationException(-1, s.msg, log.Location);
		////}

		/////* 数据压电文 */
		////if (epex.SetValue("tc_no", 0, c_tc_no) < 0		// 电文号
		////	|| epex.SetValue("confm_plan_no", 0, tsmpe00["CONFM_PLAN_NO"].ToString()) < 0	// 准发计划号
		////	|| epex.SetValue("bill_no", 0, tsmpe00["READY_BILL_NO"].ToString()) < 0	// 单据号
		////	|| epex.SetValue("match_flag", 0, doFlag) < 0	// 应答状态
		////	|| epex.SetValue("mat_num", 0, tsmpe00["PLAN_NUM"].ToDecimal()) < 0	// 计划件数
		////	|| epex.SetValue("mat_wt", 0, tsmpe00["PLAN_WT"].ToDecimal()) < 0	// 计划重量
		////	|| epex.SetValue("remark_1", 0, s.msg) < 0	// 备注
		////	)
		////{
		////	CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
		////	CMessageFormat::Format(s.msg, "写入电文体数据时出错! 原因描述： [{0}]", arguments, 1);//格式化字符串
		////	throw CApplicationException(-1, s.msg, log.Location);
		////}

		////// 发送电文
		////if (epex.SendTele() < 0)
		////{
		////	CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
		////	CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
		////	throw	CApplicationException(-1, s.msg, log.Location);
		////}

		////// 释放
		////epex.Uninitialize();

		/////* 事务结束，另起一个事务*/
		////CTransactionManager::Commit(0);
		////CTransactionManager::Begin(0, 0);

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

////	if (v_flag != "1")
////	{
////		/* 事务结束，另起一个事务*/
////		CTransactionManager::Abort(0);
////		CTransactionManager::Begin(0, 0);
////
////		// 发送接收应答电文
////		CString tc_no = "";
////		tc_no = c_tc_no.SubstringNE(2, 2) + c_tc_no.SubstringNE(0, 2) + "04";
////		// 生成电文发送对象
////		EPEX epex(&s);
////
////		// 初始化电文格式
////		if (epex.Initialize(tc_no) < 0)   //电文号
////		{
////			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
////			CMessageFormat::Format(s.msg, "初始化电文时失败！ 原因描述：[ {0}]", arguments, 1);//格式化字符串
////			throw	CApplicationException(-1, s.msg, log.Location);
////		}
////
////		/* 数据压电文 */
////		if (epex.SetValue("tc_no", 0, c_tc_no) < 0		// 电文号
////			|| epex.SetValue("confm_plan_no", 0, tsmpe00["CONFM_PLAN_NO"].ToString()) < 0	// 准发计划号
////			|| epex.SetValue("bill_no", 0, tsmpe00["READY_BILL_NO"].ToString()) < 0	// 单据号
////			|| epex.SetValue("match_flag", 0, doFlag) < 0	// 应答状态
////			|| epex.SetValue("mat_num", 0, tsmpe00["PLAN_NUM"].ToDecimal()) < 0	// 计划件数
////			|| epex.SetValue("mat_wt", 0, tsmpe00["PLAN_WT"].ToDecimal()) < 0	// 计划重量
////			|| epex.SetValue("remark_1", 0, s.msg) < 0	// 备注
////			)
////		{
////			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
////			CMessageFormat::Format(s.msg, "写入电文体数据时出错! 原因描述： [{0}]", arguments, 1);//格式化字符串
////			throw CApplicationException(-1, s.msg, log.Location);
////		}
////
////		// 发送电文
////		if (epex.SendTele() < 0)
////		{
////			CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
////			CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
////			throw	CApplicationException(-1, s.msg, log.Location);
////		}
////
////		// 释放
////		epex.Uninitialize();
////
////		/* 事务结束，另起一个事务*/
////		CTransactionManager::Commit(0);
////		CTransactionManager::Begin(0, 0);
////
////		doFlag = 0;	// 2022-4-2  负应答不报错 冷轧P8测试
////	}
	return doFlag;
}

