/************************************************
*	程序名称：产成品发货处理——可发货材料查询	*
*	编制日期：2023-1-18   	                    *
*	编 制 人：13801					            *
*************************************************/

//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件

// service入口
BM2F_ENTERACE2(sm0008_inq_detail, f_sm00_plan_mat)

//自定义的函数
//int f_sm0008_inq_detail(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
//{
//	CTracer log(__FUNCTION__);
//
//	int	 doFlag = 0;				// 调用本函数的返回值
//	int	 fetchRowCount = 0;		// 调用本函数的返回值
//	int	 i = 0;
//	int	 blkNum = 0;	// 块号
//	int ret;
//
//	CString COMPANY_CODE("");
//	CString sqlstr("");              // 数据库SQL操作字符串
//	CModel tsmpe02("TSMPE02");
//	CModel tsmpe10("TSMPE10");
//	CModel tsmpe10_in("TSMPE10");
//	CModel tom01("TOM01");
//
//	/* 数据库操作类定义 */
//	CDbCommand cmd_inq(conn);
//
//	try
//	{
//		fetchRowCount = bcls_rec->Tables[blkNum].Rows.get_Count();
//		if (fetchRowCount == 0)
//		{
//			sprintf(s.msg, "没有传入参数");
//			throw	CApplicationException(-1, s.msg, log.Location);
//		}
//
//		// 读取传入的参数
//		tsmpe10_in.Reset();
//		tsmpe10_in.MergeFrom(bcls_rec->Tables[blkNum].Rows[0]);
//
//		// 打印传入的变量
//		tsmpe10_in.Print();
//
//		sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = '" + tsmpe10_in["BILL_OF_LADING_NO"].ToString() + "' ";
//		if (tsmpe10_in["ORDER_NO"].ToString().Trim() != "")
//		{
//			sqlstr += " AND ORDER_NO = '" + tsmpe10_in["ORDER_NO"].ToString() + "' ";
//		}
//		if	(tsmpe10_in["CONTRACT_NO"].ToString().Trim() != "")
//		{
//			sqlstr += " AND CONTRACT_NO = '" + tsmpe10_in["CONTRACT_NO"].ToString() + "' ";
//		}
//
//
//		CString table_name = "";
//		if (tsmpe10["MAT_KIND"].ToString() == "BW")	table_name = "TMMBW01";
//		if (tsmpe10["MAT_KIND"].ToString() == "SM")	table_name = "TMMSM01";
//		if (tsmpe10["MAT_KIND"].ToString() == "CR")	table_name = "TMMCR01";
//		if (tsmpe10["MAT_KIND"].ToString() == "HR")	table_name = "TMMHR01";
//		if (tsmpe10["MAT_KIND"].ToString() == "HP")	table_name = "TMMHP01";
//		if (tsmpe10["MAT_KIND"].ToString() == "TB")	table_name = "TMMTB01";
//		if (table_name == "")
//		{
//			sprintf(s.msg, "物料种类不能为空");
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//
//
//
//		// 按量发货标记为1时，到合同表上读取钢种、规格
//		if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1")
//		{
//			sqlstr = "SELECT * from tom01 where order_no = '" + tsmpe10["ORDER_NO"].ToString().Trim() + "' ";
//			cmd_inq.SetCommandText(sqlstr);
//			cmd_inq.ExecuteReader();
//			if (cmd_inq.Read())
//			{
//				cmd_inq.Fetch(tom01);
//			}
//			else
//			{
//				CFormattable arguments[] = { tsmpe10["ORDER_NO"].ToString() };
//				CMessageFormat::Format(s.msg, "无此合同信息{0}", arguments, 1);
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//			cmd_inq.Close();
//		}
//
//		////tom01["PSC"] = tsmpe10["PSC"];
//		////tom01["SG_SIGN"]=tsmpe10["SG_SIGN"];
//
//		// sql语句
//		sqlstr = "SELECT A.* ,B.STOCK_PLACE_NO,B.LAYERNO,B.HEAT_NO,B.IN_FLAG,B.STOCK_PLACE_POSITION "
//		" FROM TSMPE02 A RIGHT JOIN TMMBW01 B  ON A.MAT_NO = B.MAT_NO "
//		" WHERE A.MAT_KIND = '" + tsmpe10.MAT_KIND + "' "
//		" AND	B.RED_FLAG != '1' "
//
//
//		sqlstr = "SELECT A.* ,B.STOCK_PLACE_NO,B.LAYERNO,B.HEAT_NO,B.IN_FLAG,B.STOCK_PLACE_POSITION "
//		" FROM TSMPE02 A LEFT JOIN " + table_name + " B  ON A.MAT_NO = B.MAT_NO "
//		if (tsmpe10["MAT_KIND"].ToString() == "BW")
//		{
//			sqlstr = "SELECT B.PACK_NO,A.* ,B.STOCK_PLACE_NO,B.LAYERNO,B.HEAT_NO,B.IN_FLAG,B.STOCK_PLACE_POSITION "
//				"  ,B.STOCK_PLACE_NO || '*' || B.LAYERNO || '*' || B.STOCK_PLACE_POSITION || '*' || B.PACK_NO AS STOCK_PLACE_GROUP  FROM TSMPE02 A ," + table_name + " B ";
//		}
//		else
//		{
//			sqlstr = "SELECT A.* ,B.STOCK_PLACE_NO,B.LAYERNO,B.HEAT_NO,B.IN_FLAG,B.STOCK_PLACE_POSITION FROM TSMPE02 A ," + table_name + " B ";
//		}
//		sqlstr += " WHERE A.VEHICLE_NO <= ' '  "
//			" AND	A.RED_FLAG		!=	'1' "
//			" AND A.MAT_NO = B.MAT_NO "
//			" AND A.STATUS = ' ' ";
//
//
//		if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2")	// 1--按量跨合同，2--按量不跨合同，0--按件
//		{
//			sqlstr += " AND A.ORDER_NO = '" + tsmpe10["ORDER_NO"].ToString() + "' ";
//			sqlstr += " AND A.BILL_OF_LADING_NO = ' ' ";
//			sqlstr += " AND A.CONFM_STATUS	=	'4' ";
//			sqlstr += " AND A.STOCK_NO = '" + tsmpe10["STOCK_NO"].ToString() + "' ";
//		}
//		else if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "0")
//		{
//			sqlstr += " AND A.BILL_OF_LADING_NO = '" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' ";
//			sqlstr += " AND A.CONFM_STATUS	=	'6' ";
//			if (tsmpe10["ORDER_NO"].ToString().Trim() != "")
//			{
//				sqlstr += " AND A.ORDER_NO = '" + tsmpe10["ORDER_NO"].ToString() + "' ";
//			}
//		}
//		else if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1")
//		{
//			sqlstr += " AND A.BILL_OF_LADING_NO = ' ' ";
//			sqlstr += " AND A.CONFM_STATUS	=	'4' ";
//			//sqlstr += " AND A.DELIVY_QTY_FLAG = '1' ";
//			sqlstr += " AND A.STOCK_NO = '" + tsmpe10["STOCK_NO"].ToString() + "' ";
//			////sqlstr += " AND	a.SG_SIGN		=	'" + tom01["SG_SIGN"].ToString().Trim() + "' ";
//			////sqlstr += " AND	A.PSC		=	'" + tom01["PSC"].ToString().Trim() + "' ";
//			//sqlstr += " AND B.MSC		=	'" + tom01["MSC"].ToString().Trim() + "' ";
//			////sqlstr += " AND A.CUST_MAT_SPECS = '" + tsmpe10["CUST_MAT_SPECS"].ToString() + "' ";
//
//		}
//
//		sqlstr += " ORDER BY B.STOCK_PLACE_NO,B.LAYERNO DESC";
//
//
//		cmd_inq.SetCommandText(sqlstr);
//		//cmd_inq.Parameters.Set("tom01.ORDER_THICK", tom01.ORDER_THICK );
//		//cmd_inq.Parameters.Set("tom01.ORDER_THICK_MIN", tom01["ORDER_THICK"].ToDecimal() + tom01["ORDER_THICK_TOL_MIN"].ToDecimal());
//		//cmd_inq.Parameters.Set("tom01.ORDER_THICK_MAX", tom01["ORDER_THICK"].ToDecimal() + tom01["ORDER_THICK_TOL_MAX"].ToDecimal());
//
//		//cmd_inq.Parameters.Set("tom01.ORDER_WIDTH", tom01.ORDER_WIDTH );
//		//cmd_inq.Parameters.Set("tom01.ORDER_WIDTH_MIN", tom01["ORDER_WIDTH"].ToDecimal() + tom01["ORDER_WIDTH_TOL_MIN"].ToDecimal());
//		//cmd_inq.Parameters.Set("tom01.ORDER_WIDTH_MAX", tom01["ORDER_WIDTH"].ToDecimal() + tom01["ORDER_WIDTH_TOL_MAX"].ToDecimal());
//
//		//cmd_inq.Parameters.Set("tom01.ORDER_MIN_LEN", tom01["ORDER_MIN_LEN"].ToDecimal() + tom01["LEN_TOL_MINUS"].ToDecimal());
//		//cmd_inq.Parameters.Set("tom01.ORDER_MAX_LEN", tom01["ORDER_MAX_LEN"].ToDecimal() + tom01["LEN_TOL_PLUS"].ToDecimal());
//
//
//		//cmd_inq.Parameters.Set("tsmpe10.ORDER_THICK", tsmpe10["ORDER_THICK"].ToDecimal());
//		//cmd_inq.Parameters.Set("tsmpe10.ORDER_WIDTH", tsmpe10["ORDER_WIDTH"].ToDecimal());
//		//cmd_inq.Parameters.Set("tsmpe10.ORDER_MIN_LEN", tsmpe10["ORDER_MIN_LEN"].ToDecimal());
//		//cmd_inq.Parameters.Set("tsmpe10.ORDER_MAX_LEN", tsmpe10["ORDER_MAX_LEN"].ToDecimal());
//		Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
//
//		cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);
//
//		////if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" || tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2")
//		////{
//		////	for (size_t i = 0; i < bcls_ret->Tables[0].Rows.get_Count(); i++)
//		////	{
//		////		bcls_ret->Tables[0].Rows[i]["BILL_OF_LADING_NO"] = tsmpe10["BILL_OF_LADING_NO"];
//		////		bcls_ret->Tables[0].Rows[i]["ORDER_NO"] = tsmpe10["ORDER_NO"];
//
//		////		CString mark="";
//		////		tsmpe02["MAT_ACT_WT"] = bcls_ret->Tables[0].Rows[i]["MAT_ACT_WT"];
//		////		tsmpe02["MAT_THEORY_WT"] = bcls_ret->Tables[0].Rows[i]["MAT_THEORY_WT"];
//		////		tsmpe02["MAT_WT"] = bcls_ret->Tables[0].Rows[i]["MAT_WT"];
//		////		tsmpe02["MAT_NO"] = bcls_ret->Tables[0].Rows[i]["MAT_NO"];
//
//		////		if (tom01["WT_METHOD_CODE"].ToString() == "0" && tsmpe02["MAT_WT"].ToDecimal() != tsmpe02["MAT_ACT_WT"].ToDecimal() && tsmpe02["MAT_ACT_WT"].ToDecimal() != 0)
//		////		{
//		////			tsmpe02["MAT_WT"] = tsmpe02["MAT_ACT_WT"];
//		////			mark = "1";
//		////		}
//		////		if (tom01["WT_METHOD_CODE"].ToString() == "1" && tsmpe02["MAT_WT"].ToDecimal() != tsmpe02["MAT_THEORY_WT"].ToDecimal() && tsmpe02["MAT_THEORY_WT"].ToDecimal() != 0)
//		////		{
//		////			tsmpe02["MAT_WT"] = tsmpe02["MAT_THEORY_WT"];
//		////			mark = "1";
//		////		}
//
//		////		if ( mark == "1" )
//		////		{
//		////			bcls_ret->Tables[0].Rows[i]["MAT_WT"] = tsmpe02["MAT_WT"];
//
//		////			sqlstr = "update tsmpe02 set MAT_WT = @MAT_WT ,WT_MODE = @WT_MODE "
//		////				" WHERE MAT_NO = @MAT_NO ";
//		////			cmd_inq.SetCommandText(sqlstr);
//		////			cmd_inq.Parameters.Set("MAT_WT", tsmpe02["MAT_WT"].ToDecimal());
//		////			cmd_inq.Parameters.Set("WT_MODE", tom01["WT_METHOD_CODE"].ToString());
//		////			cmd_inq.Parameters.Set("MAT_NO", tsmpe02["MAT_NO"].ToString());
//		////			cmd_inq.ExecuteNonQuery();
//		////		}
//		////	}
//		////}
//
//		CFormattable arguments[] = { bcls_ret->Tables[0].Rows.get_Count() };// 定义参数列表的数组
//		CMessageFormat::Format(s.msg, "查询到[{0}]条记录。", arguments, 1);//格式化字符串
//		Log::Info("", __FUNCTION__, "{0}", s.msg);
//	}
//	catch (CDbException& ex)  //捕获数据库操作异常
//	{
//
//		CFormattable arguments[] = { ex.GetCode() };// 定义参数列表的数组
//		CMessageFormat::Format(s.msg, "数据库处理出错sqlcode=[{0}]", arguments, 1);//格式化字符串
//		CString str = sqlstr + "\r\n" + ex.GetMsg();
//
//		Log::Error("", __FUNCTION__, "error=[{0}]", str);
//
//		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
//		__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
//		s.flag = -1;
//		doFlag = -1;                  //数据库异常时返回-1，事务将被回滚
//	}
//	catch (const CApplicationException& ex)
//	{
//		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
//		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
//		doFlag = -1;
//	}
//
//	catch (const CException& ex)
//	{
//		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
//		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
//		doFlag = -1;
//	}
//	if (doFlag < 0)
//	{
//		CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
//		//CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
//		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
//	}
//
//	//返回-1时事务将回滚，返回为0是事务将提交
//	return doFlag;
//
//}
