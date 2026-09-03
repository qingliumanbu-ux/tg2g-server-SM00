/************************************************
*	程序名称：产成品发货处理——码单生成检查		*
*	编制日期：2020-7-18   	                    *
*	编 制 人：13801					            *
*************************************************/

//框架公用头文件，勿删
#include "stdafx.h"
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//程序用头文件
#include "tsmpe02.h"	// 产成品材料表
#include "tsmpe10.h"	// 提单表

// service入口
BM2F_ENTERACE(sm0008_pro_check)

//自定义的函数
int f_sm0008_pro_check(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int	 doFlag = 0;				// 调用本函数的返回值
	int	 fetchRowCount = 0;		// 调用本函数的返回值
	int	 i = 0;
	int	 blkNum = 0;	// 块号
	int ret;

	CString sqlstr("");              // 数据库SQL操作字符串
	CTSMPE02 tsmpe02(conn);
	CTSMPE02 tsmpe02_in(conn);
	CTSMPE10 tsmpe10(conn);
	CTSMPE10 tsmpe10_chk(conn);

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq1(conn);
	CDbCommand cmd_upd(conn);
	CDbCommand cmd_loop(conn);

	try
	{
		fetchRowCount = bcls_rec->Tables[blkNum].Rows.get_Count();
		if (fetchRowCount == 0)
		{
			sprintf(s.msg, "没有传入参数");
			throw	CApplicationException(-1, s.msg, log.Location);
		}



		// 检查已经装车的计划组合码是否和这次选择的组合码是否一致，否报错
		tsmpe02_in.VEHICLE_NO = bcls_rec->Tables[blkNum].Rows[0]["VEHICLE_NO"];
		if (tsmpe02_in.VEHICLE_NO.Trim() == "")
		{
			sprintf(s.msg, "车号不能为空");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		// 按车号读取材料上的计划号
		sqlstr = "select distinct BILL_OF_LADING_NO from tsmpe02 where VEHICLE_NO = '" + tsmpe02_in.VEHICLE_NO.Trim() + "' ";
		cmd_loop.SetCommandText(sqlstr);
		Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
		cmd_loop.ExecuteReader();
		while (cmd_loop.Read())
		{
			tsmpe02.BILL_OF_LADING_NO = cmd_loop.GetString(1);

			sqlstr = " select * FROM tsmpe10 WHERE BILL_OF_LADING_NO = '" + tsmpe02.BILL_OF_LADING_NO + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", __FUNCTION__, "sqlstr1={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe10);
			}
			cmd_inq.Close();
			if (i == 0)
			{
				i = i + 1;
				tsmpe10_chk.CopyFrom(tsmpe10);
				continue;
			}
			else
			{
				// 检查相关字段是否符合车辆合装要求
				CString msg = "", mark = "";
				if (tsmpe10.DELIVY_PLACE_NAME.Trim() != tsmpe10_chk.DELIVY_PLACE_NAME.Trim())
				{
					msg = msg + "交货地点不同\r\n";
					mark = "1";
				}

				if (tsmpe10.PRIVATE_ROUTE_NAME.Trim() != tsmpe10_chk.PRIVATE_ROUTE_NAME.Trim())
				{
					msg = msg + "专用线不同\r\n";
					mark = "1";
				}
				if (tsmpe10.CONSIGNE_NAME.Trim() != tsmpe10_chk.CONSIGNE_NAME.Trim())
				{
					msg = msg + "收货单位名称不同\r\n";
					mark = "1";
				}
				if (tsmpe10.BALANCE_USER_NAME.Trim() != tsmpe10_chk.BALANCE_USER_NAME.Trim())
				{
					msg = msg + "结算用户名称不同\r\n";
					mark = "1";
				}

				if (mark == "1")
				{
					strcpy(s.msg, msg);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}

			}
		}
		cmd_loop.Close();

		if (i==0)
		{
			sprintf(s.msg, "无此车号 %s 的材料记录", (const char *)tsmpe02_in.VEHICLE_NO);
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}


		sprintf(s.msg, "合车检查成功");


	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{

		CFormattable arguments[] = { ex.GetCode() };// 定义参数列表的数组
		CMessageFormat::Format(s.msg, "数据库处理出错sqlcode=[{0}]", arguments, 1);//格式化字符串
		CString str = sqlstr + "\r\n" + ex.GetMsg();

		Log::Error("", __FUNCTION__, "error=[{0}]", str);

		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
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
		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
	}

	//返回-1时事务将回滚，返回为0是事务将提交
	return doFlag;

}
