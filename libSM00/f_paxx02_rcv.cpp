/*************************************************
Copyright:   Baosight Software LTD.co Copyright (c) 2010
Author:
Version:     3.0
Date:
Description: 厂内汽运配车信息
**************************************************/

//框架公用头文件，勿删
#include "stdafx.h"
#include "epex.h"

//程序用头文件



//外部函数声明

// service入口

BM2_FUNCTION_EXPORT
int f_paxx02_rcv(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{

	CTracer log(__FUNCTION__);

	/* 程序内部变量 */
	int   doFlag = 0;
	int   fetchRowCount = 0;
	int   outBlockRow = 0;
	int   ren = 0;
	int   v_count = 0;
	int   blkNum = 0;
	int   v_flag = 0;

	/* 业务变量 */
	CString  datetime("");
	CString	OPER_FLAG = "";

	/* 实体类定义 */
	CModel tsm00b4("TSM00B4");
	CModel hsm00b4("HSM00B4");


	// 数据库SQL操作字符串
	CString  sqlstr("");

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_loop(conn);
	CDbCommand cmd_upd(conn);


	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		//获得传入的数据行数

		tsm00b4.MergeFrom(bcls_rec->Tables[0].Rows[0]);
		OPER_FLAG = bcls_rec->Tables[0].Rows[0]["OPERATION_FLAG"];	// I--新增，D--删除，E--结束

		if (OPER_FLAG != "I" && OPER_FLAG != "D" && OPER_FLAG != "E")
		{
			sprintf(s.msg, "操作标志出错,I--新增，D--删除，E--结束！");
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		/* 数据校验 */
		if (tsm00b4["VEHICLE_NO"].ToString().Trim() == "")
		{
			sprintf(s.msg, "车号不能为空");
			throw	CApplicationException(-1, s.msg, log.Location);
		}
		/* 数据校验 */
		if (tsm00b4["TRUCK_NO_ID"].ToString().Trim() == "")
		{
			sprintf(s.msg, "作业车次号不能为空");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		tsm00b4["USE_MARK"] = "1";	// 1-可用 ， 0--不可用
		tsm00b4["TRNP_MODE_CODE"] = "1";//运输方式
		tsm00b4["DIS_SOURCE"] = "DY";//配车来源，DY代运，ZT自提
		// 新增
		if (OPER_FLAG == "I")
		{
			sqlstr = "select TICKET_NO from TSM00B4 WHERE "
				" VEHICLE_NO = '" + tsm00b4["VEHICLE_NO"].ToString().Trim() + "' "
				" AND TRUCK_NO_ID='" + tsm00b4["TRUCK_NO_ID"].ToString().Trim() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				if (cmd_inq.GetString(1).Trim() == "")
				{
					tsm00b4.Delete("VEHICLE_NO,TRUCK_NO_ID");
				}
				else
				{
					CFormattable arguments[] = { tsm00b4["VEHICLE_NO"].ToString(), tsm00b4["TRUCK_NO_ID"].ToString() };
					CMessageFormat::Format(s.msg, "车号{0}，作业车次号{1}已经存在", arguments, 2);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
			}
			cmd_inq.Close();

			// 根据库区来判定物料种类、厂别
			sqlstr = " select mat_kind,factory_div from TWM01 where stock_no = @stock_no ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("stock_no", tsm00b4["STOCK_NO"].ToString());
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				tsm00b4["MAT_KIND"] = cmd_inq.GetString(1);
				tsm00b4["FACTORY_DIV"] = cmd_inq.GetString(2);
			}
			cmd_inq.Close();

			tsm00b4["REC_CREATE_TIME"] = datetime;
			tsm00b4["REC_CREATOR"] = s.username;
			tsm00b4["STATUS"] = "2";		//0 :接收

			tsm00b4.TrimOrBlank();
			tsm00b4.Print();
			sqlstr = " INSERT tsm00b4 ";
			tsm00b4.Insert();

		}

		// 结束
		if (OPER_FLAG == "E")
		{
			/* 读取车辆信息 */
			if (!tsm00b4.Query("VEHICLE_NO,TRUCK_NO_ID"))
			{
				CFormattable arguments[] = { tsm00b4["VEHICLE_NO"].ToString(), tsm00b4["TRUCK_NO_ID"].ToString() };
				CMessageFormat::Format(s.msg, "没有此车号{0}，作业车次号{1}的车辆信息", arguments, 2);
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			//if (tsm00b4["TICKET_NO"].ToString().Trim() == "")//空车归档
			{
				/* 写入历史档*/
				hsm00b4.CopyFrom(tsm00b4);
				hsm00b4["REC_REVISE_TIME"] = datetime;
				hsm00b4["REC_REVISOR"] = s.userid;
				//hsm00b4["STATUS"] = "9";
				sqlstr = "INSERT HSM00B4 ";
				hsm00b4.TrimOrBlank();
				hsm00b4.Insert();

				/* 删除此车辆记录 */
				sqlstr = "INSERT TSM00B4 ";
				tsm00b4.Delete("VEHICLE_NO,TRUCK_NO_ID");
			}


		}

		// 删除
		if (OPER_FLAG == "D")
		{
			sqlstr = "select TICKET_NO from TSM00B4 WHERE "
				" VEHICLE_NO = '" + tsm00b4["VEHICLE_NO"].ToString().Trim() + "' "
				" AND TRUCK_NO_ID='" + tsm00b4["TRUCK_NO_ID"].ToString().Trim() + "' ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				if (cmd_inq.GetString(1).Trim() == "")
				{
					tsm00b4.Delete("VEHICLE_NO,TRUCK_NO_ID");
				}
				else
				{
					CFormattable arguments[] = { tsm00b4["VEHICLE_NO"].ToString(), tsm00b4["TRUCK_NO_ID"].ToString() };
					CMessageFormat::Format(s.msg, "车号{0}，作业车次号{1}不满足删除条件", arguments, 2);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
			}
			cmd_inq.Close();
		}

		strcpy(s.msg, "电文接收成功！");//处理成功。
		s.flag = 0;
		return 0;

	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("数据库操作失败{0}")/*信息读取失败。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1; //数据库异常时返回-1，事务将被回滚
	}
	catch (const CApplicationException& ex)
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

	return(doFlag);
}
