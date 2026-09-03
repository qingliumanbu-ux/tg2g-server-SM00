/************************************************
*	程序名称：产成品发货处理——发货材料装车	*
*	编制日期：2021-1-19   	                    *
*	编 制 人：13801					            *
*************************************************/

//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件
	// 产成品材料表
	// 提单表
	// 车辆信息表

// service入口
BM2F_ENTERACE2(sm0008_upd_up,f_sm00_plan_mat_add)

//自定义的函数
//int f_sm0008_upd_up(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
//{
//	CTracer log(__FUNCTION__);
//
//	int	 doFlag = 0;				// 调用本函数的返回值
//	int	 fetchRowCount = 0;		// 调用本函数的返回值
//	int	 i = 0;
//	int	 blkNum = 0;	// 块号
//	int ret;
//
//	CString sqlstr("");              // 数据库SQL操作字符串
//	CModel tsmpe02("TSMPE02");
//	CModel tsmpe02_in("TSMPE02");
//	CModel tsmpe10("TSMPE10");
//	CModel tsmpe10_chk("TSMPE10");
//	CModel tsm00b4("TSM00B4");
//
//	/* 数据库操作类定义 */
//	CDbCommand cmd_inq(conn);
//	CDbCommand cmd_inq1(conn);
//	CDbCommand cmd_upd(conn);
//	CDbCommand cmd_loop(conn);
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
//
//
//		// 检查已经装车的计划组合码是否和这次选择的组合码是否一致，否报错
//		tsmpe02_in["VEHICLE_NO"] = bcls_rec->Tables[blkNum].Rows[0]["VEHICLE_NO"];
//		tsmpe02_in["ORDER_NO"] = bcls_rec->Tables[blkNum].Rows[0]["ORDER_NO"];
//		tsmpe02_in["BILL_OF_LADING_NO"] = bcls_rec->Tables[blkNum].Rows[0]["BILL_OF_LADING_NO"];
//		tsmpe02_in["STOCK_NO"] = bcls_rec->Tables[blkNum].Rows[0]["STOCK_NO"];
//
//		if (tsmpe02_in["VEHICLE_NO"].ToString().Trim() == "")
//		{
//			sprintf(s.msg, "车号不能为空");
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//
//		sqlstr = "select b.* from tsmpe02 a,tsmpe10 b where a.VEHICLE_NO = '" + tsmpe02_in["VEHICLE_NO"].ToString().Trim() + "' "
//			" and a.BILL_OF_LADING_NO = b.BILL_OF_LADING_NO ";
//		cmd_inq.SetCommandText(sqlstr);
//		Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
//		cmd_inq.ExecuteReader();
//		if (cmd_inq.Read())
//		{
//			cmd_inq.Fetch(tsmpe10_chk);
//			cmd_inq.Close();
//
//			sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = '" + tsmpe02_in["BILL_OF_LADING_NO"].ToString() + "' ";
//			cmd_inq.SetCommandText(sqlstr);
//			Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
//			cmd_inq.ExecuteReader();
//			if (cmd_inq.Read())
//			{
//				cmd_inq.Fetch(tsmpe10);
//			}
//			cmd_inq.Close();
//
//
//			// 检查相关字段是否符合车辆合装要求
//			CString msg = "", mark = "";
//			if (tsmpe10["DELIVY_PLACE_NAME"].ToString().Trim() != tsmpe10_chk["DELIVY_PLACE_NAME"].ToString().Trim())
//			{
//				msg = msg + "交货地点不同\r\n";
//				mark = "1";
//			}
//
//			if (tsmpe10["PRIVATE_ROUTE_NAME"].ToString().Trim() != tsmpe10_chk["PRIVATE_ROUTE_NAME"].ToString().Trim())
//			{
//				msg = msg + "专用线不同\r\n";
//				mark = "1";
//			}
//			if (tsmpe10["CONSIGNE_NAME"].ToString().Trim() != tsmpe10_chk["CONSIGNE_NAME"].ToString().Trim())
//			{
//				msg = msg + "收货单位名称不同\r\n";
//				mark = "1";
//			}
//			if (tsmpe10["BALANCE_USER_NAME"].ToString().Trim() != tsmpe10_chk["BALANCE_USER_NAME"].ToString().Trim())
//			{
//				msg = msg + "结算用户名称不同\r\n";
//				mark = "1";
//			}
//
//			if (mark == "1")
//			{
//				strcpy(s.msg, msg);
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//
//		}
//		else
//		{
//			tsmpe10_chk.Reset();
//		}
//		cmd_inq.Close();
//
//		////sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = '" + tsmpe02_in["BILL_OF_LADING_NO"].ToString() + "' ";
//		////cmd_inq.SetCommandText(sqlstr);
//		////Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
//		////cmd_inq.ExecuteReader();
//		////if (cmd_inq.Read())
//		////{
//		////	cmd_inq.Fetch(tsmpe10);
//		////}
//		////cmd_inq.Close();
//
//
//		////// 检查相关字段是否符合车辆合装要求
//		////CString msg = "", mark = "";
//		////if (tsmpe10["DELIVY_PLACE_NAME"].ToString().Trim() != tsmpe10_chk["DELIVY_PLACE_NAME"].ToString().Trim())
//		////{
//		////	msg = msg + "交货地点不同\r\n";
//		////	mark = "1";
//		////}
//
//		////if (tsmpe10["PRIVATE_ROUTE_NAME"].ToString().Trim() != tsmpe10_chk["PRIVATE_ROUTE_NAME"].ToString().Trim())
//		////{
//		////	msg = msg + "专用线不同\r\n";
//		////	mark = "1";
//		////}
//		////if (tsmpe10["CONSIGNE_NAME"].ToString().Trim() != tsmpe10_chk["CONSIGNE_NAME"].ToString().Trim())
//		////{
//		////	msg = msg + "收货单位名称不同\r\n";
//		////	mark = "1";
//		////}
//		////if (tsmpe10["BALANCE_USER_NAME"].ToString().Trim() != tsmpe10_chk["BALANCE_USER_NAME"].ToString().Trim())
//		////{
//		////	msg = msg + "结算用户名称不同\r\n";
//		////	mark = "1";
//		////}
//
//		////if (mark == "1")
//		////{
//		////	strcpy(s.msg, msg);
//		////	throw	CApplicationException(-1, s.msg, s.svc_name);
//		////}
//
//
//		// 检查计划是否存在否报错
//		sqlstr = "select count(1) from TSMPE10 where BILL_OF_LADING_NO = @BILL_OF_LADING_NO ";
//
//		cmd_inq.SetCommandText(sqlstr);
//		cmd_inq.Parameters.Set("BILL_OF_LADING_NO", tsmpe02_in["BILL_OF_LADING_NO"].ToString());
//		Log::Debug("", "", "sqlstr = [{0}]", sqlstr);
//		if (cmd_inq.ExecuteScalar() == 0)
//		{
//			CFormattable arguments[] = { tsmpe02_in["BILL_OF_LADING_NO"].ToString() };// 定义参数列表的数组
//			CMessageFormat::Format(s.msg, "此计划{0}已经做了撤销，不能做预装车", arguments, 1);//格式化字符串
//			throw	CApplicationException(-1, s.msg, s.svc_name);
//		}
//		cmd_inq.Close();
//
//
//
//		// 检查铁运车皮是否装车完毕，是不允许再装车
//		CString status = "0";
//		sqlstr = "select status from tsm00b4 where STATUS < '9' AND VEHICLE_NO = @VEHICLE_NO ";
//		cmd_inq.SetCommandText(sqlstr);
//		cmd_inq.Parameters.Set("VEHICLE_NO", tsmpe02_in["VEHICLE_NO"].ToString());
//		Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
//		cmd_inq.ExecuteReader();
//		if (cmd_inq.Read())
//		{
//			status = cmd_inq.GetString(1);
//			if (status == "3")
//			{
//				CFormattable arguments[] = { tsmpe02_in["VEHICLE_NO"].ToString() };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "此车皮{0}已经装车生成码单不能再次装车", arguments, 1);//格式化字符串
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//			if (status != "2")
//			{
//				CFormattable arguments[] = { tsmpe02_in["VEHICLE_NO"].ToString() };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "此车皮{0}不是可用状态", arguments, 1);//格式化字符串
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//		}
//		cmd_inq.Close();
//
//
//
//		for (i = 0; i < fetchRowCount; i++)
//		{
//			// 读取传入的参数
//			tsmpe02_in["MAT_NO"] = bcls_rec->Tables[blkNum].Rows[i]["MAT_NO"];
//			Log::Debug("", "", "MAT_NO={0},i={1}", tsmpe02_in["MAT_NO"].ToString(), i + 1);
//
//			tsmpe02_in["ORDER_NO"] = bcls_rec->Tables[blkNum].Rows[i]["ORDER_NO"];
//			Log::Debug("", "", "ORDER_NO={0},i={1}", tsmpe02_in["ORDER_NO"].ToString(), i + 1);
//
//			tsmpe02_in["BILL_OF_LADING_NO"] = bcls_rec->Tables[blkNum].Rows[i]["BILL_OF_LADING_NO"];
//			Log::Debug("", "", "BILL_OF_LADING_NO={0},i={1}", tsmpe02_in["BILL_OF_LADING_NO"].ToString(), i + 1);
//
//			if (i==0)
//			{
//				tsmpe02_in["VEHICLE_NO"] = bcls_rec->Tables[blkNum].Rows[i]["VEHICLE_NO"];
//				Log::Debug("", "", "VEHICLE_NO={0}", tsmpe02_in["VEHICLE_NO"].ToString());
//
//				tsmpe02_in["DELIVY_QTY_FLAG"] = bcls_rec->Tables[blkNum].Rows[i]["DELIVY_QTY_FLAG"];
//				Log::Debug("", "", "DELIVY_QTY_FLAG={0}", tsmpe02_in["DELIVY_QTY_FLAG"].ToString());
//			}
//
//
//			if (tsmpe02_in["MAT_NO"].ToString().Trim()=="")
//			{
//				sprintf(s.msg, "传入参数材料号不能为空");
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//
//
//			// 按材料号到产成品材料表上读取材料记录
//			sqlstr = "SELECT T.* FROM TSMPE02 T WHERE MAT_NO = '" + tsmpe02_in["MAT_NO"].ToString().Trim() + "' ";
//			cmd_inq.SetCommandText(sqlstr);
//			Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
//			cmd_inq.ExecuteReader();
//			if (cmd_inq.Read())
//			{
//				cmd_inq.Fetch(tsmpe02);
//			}
//			else
//			{
//				CFormattable arguments[] = { tsmpe02_in["MAT_NO"].ToString() };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "没有读到材料=[{0}的记录]", arguments, 1);//格式化字符串
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//			cmd_inq.Close();
//
//			// 判材料上的库区是否和计划上的一致，否不能装车
//			if (tsmpe02["STOCK_NO"].ToString().Trim() != tsmpe02_in["STOCK_NO"].ToString().Trim())
//			{
//				CFormattable arguments[] = { tsmpe02_in["MAT_NO"].ToString(), tsmpe02["STOCK_NO"].ToString(), tsmpe02_in["STOCK_NO"].ToString() };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "材料=[{0}的仓库代码[{1}]和计划上的库区[{2}]不一致", arguments, 3);//格式化字符串
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//
//			// 判材料是否允许装车
//			if (tsmpe02["RED_FLAG"].ToString().Trim() == "1")
//			{
//				CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "此材料=[{0}]已经提红冲请求不允许装车", arguments, 1);//格式化字符串
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//			if (tsmpe02["VEHICLE_NO"].ToString().Trim() != "")
//			{
//				CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString(),tsmpe02["VEHICLE_NO"].ToString() };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "此材料=[{0}]已经装在[{1}]车上了", arguments, 2);//格式化字符串
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//
//
//			// 判材料是否入库，否不允许装车
//			CString IN_FLAG = "1";
//			CString table_name = "TMM" + tsmpe02["MAT_KIND"].ToString() + "01";
//			sqlstr = "SELECT IN_FLAG FROM " + table_name + " where mat_no = @mat_no ";
//			cmd_inq.SetCommandText(sqlstr);
//			cmd_inq.Parameters.Set("mat_no", tsmpe02["MAT_NO"].ToString());
//			cmd_inq.ExecuteReader();
//			if (cmd_inq.Read())
//			{
//				IN_FLAG = cmd_inq.GetString(1);
//			}
//			cmd_inq.Close();
//
//			if (IN_FLAG != "1")
//			{
//				CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "此材料=[{0}]还没有入库，不能装车", arguments, 1);//格式化字符串
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//
//
//
//			// 更新材料上的记录
//			CString	update_where, update_field;
//			update_where = "MAT_NO";
//			update_field = "VEHICLE_NO";
//
//			tsmpe02["VEHICLE_NO"] = tsmpe02_in["VEHICLE_NO"];
//			tsmpe02["BILL_OF_LADING_NO"] = tsmpe02_in["BILL_OF_LADING_NO"];
//			tsmpe02["ORDER_NO"] = tsmpe02_in["ORDER_NO"];
//			tsmpe02["DELIVY_QTY_FLAG"] = tsmpe02_in["DELIVY_QTY_FLAG"];
//
//			if (tsmpe02_in["DELIVY_QTY_FLAG"].ToString() == "1")
//			{
//				update_field += ",BILL_OF_LADING_NO,ORDER_NO,DELIVY_QTY_FLAG";
//			}
//			tsmpe02.Update(update_field, update_where);
//
//
//			////sqlstr = "UPDATE TSMPE02 SET VEHICLE_NO = @VEHICLE_NO ,BILL_OF_LADING_NO = @BILL_OF_LADING_NO "
//			////	" ,ORDER_NO = @ORDER_NO ,DELIVY_QTY_FLAG = @DELIVY_QTY_FLAG "
//			////	" WHERE MAT_NO = @MAT_NO ";
//			////cmd_upd.SetCommandText(sqlstr);
//			////cmd_upd.Parameters.Set("VEHICLE_NO", tsmpe02_in["VEHICLE_NO"].ToString());
//			////cmd_upd.Parameters.Set("BILL_OF_LADING_NO", tsmpe02_in["BILL_OF_LADING_NO"].ToString());
//			////cmd_upd.Parameters.Set("ORDER_NO", tsmpe02_in["ORDER_NO"].ToString());
//			////cmd_upd.Parameters.Set("DELIVY_QTY_FLAG", tsmpe02_in["DELIVY_QTY_FLAG"].ToString());
//			////cmd_upd.Parameters.Set("MAT_NO", tsmpe02["MAT_NO"].ToString());
//			////Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
//			////cmd_upd.ExecuteNonQuery();
//
//
//		}
//
//
//		sprintf(s.msg, "预装车处理成功");
//
//
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
