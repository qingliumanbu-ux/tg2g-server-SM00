/************************************************
*	程序名称：产成品发货处理——发货材料卸车	*
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
BM2F_ENTERACE2(sm0008_upd_dn, f_sm00_plan_mat_del)

//自定义的函数
//int f_sm0008_upd_dn(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
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
//	CModel tsm00b4("TSM00B4");
//
//	/* 数据库操作类定义 */
//	CDbCommand cmd_inq(conn);
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
//		for (i = 0; i < fetchRowCount; i++)
//		{
//			// 读取传入的参数
//			tsmpe02_in["MAT_NO"] = bcls_rec->Tables[blkNum].Rows[i]["MAT_NO"];
//			Log::Debug("", "", "MAT_NO={0},i={1}", tsmpe02_in["MAT_NO"].ToString(), i + 1);
//
//			tsmpe02_in["ORDER_NO"] = bcls_rec->Tables[blkNum].Rows[i]["ORDER_NO"];
//			Log::Debug("", "", "MAT_NO={0},i={1}", tsmpe02_in["ORDER_NO"].ToString(), i + 1);
//
//			tsmpe02_in["BILL_OF_LADING_NO"] = bcls_rec->Tables[blkNum].Rows[i]["BILL_OF_LADING_NO"];
//			Log::Debug("", "", "MAT_NO={0},i={1}", tsmpe02_in["BILL_OF_LADING_NO"].ToString(), i + 1);
//
//			if (i == 0)
//			{
//				tsmpe02_in["VEHICLE_NO"] = bcls_rec->Tables[blkNum].Rows[i]["VEHICLE_NO"];
//				Log::Debug("", "", "VEHICLE_NO={0}", tsmpe02_in["VEHICLE_NO"].ToString());
//			}
//
//
//			if (tsmpe02_in["MAT_NO"].ToString().Trim() == "")
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
//
//			// 判材料是否允许卸车
//			if (tsmpe02["VEHICLE_NO"].ToString().Trim() != tsmpe02_in["VEHICLE_NO"].ToString() )
//			{
//				CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString(), tsmpe02["VEHICLE_NO"].ToString() };// 定义参数列表的数组
//				CMessageFormat::Format(s.msg, "此材料=[{0}不在这个{1}车上]", arguments, 2);//格式化字符串
//				throw	CApplicationException(-1, s.msg, s.svc_name);
//			}
//
//			if (tsmpe02["DELIVY_QTY_FLAG"].ToString() == "1" )
//			{
//				tsmpe02["BILL_OF_LADING_NO"] = " ";
//				tsmpe02["ORDER_NO"] = tsmpe02["OLD_ORDER_NO"];		// 字段存放准发时的合同号
//			}
//			if (tsmpe02["DELIVY_QTY_FLAG"].ToString() == "2" )
//			{
//				tsmpe02["BILL_OF_LADING_NO"] = " ";
//			}
//
//			// 更新材料上的记录
//			sqlstr = "UPDATE TSMPE02 SET VEHICLE_NO = ' ' ,BILL_OF_LADING_NO = @BILL_OF_LADING_NO,ORDER_NO = @ORDER_NO "
//				" WHERE MAT_NO = @MAT_NO ";
//			cmd_upd.SetCommandText(sqlstr);
//			cmd_upd.Parameters.Set("VEHICLE_NO", tsmpe02_in["VEHICLE_NO"].ToString());
//			cmd_upd.Parameters.Set("BILL_OF_LADING_NO", tsmpe02["BILL_OF_LADING_NO"].ToString());
//			cmd_upd.Parameters.Set("ORDER_NO", tsmpe02["ORDER_NO"].ToString());
//			cmd_upd.Parameters.Set("MAT_NO", tsmpe02["MAT_NO"].ToString());
//			Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
//			cmd_upd.ExecuteNonQuery();
//
//
//		}
//
//
//		sprintf(s.msg, "材料卸车处理成功");
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
