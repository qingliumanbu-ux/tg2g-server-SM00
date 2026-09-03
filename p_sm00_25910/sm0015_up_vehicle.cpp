/************************************************
*	程序名称：装车单车号修改					*
*	编制日期：2023-2-24   	                    *
*	编 制 人：013801				            *
*************************************************/

//框架公用头文件，勿删
#include "stdafx.h"

int f_sm00_record(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	/* 写履历记录 */
int f_xxsm02_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);

// service入口
BM2F_ENTERACE(sm0015_up_vehicle)

//自定义的函数
int f_sm0015_up_vehicle(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int	 doFlag = 0;				// 调用本函数的返回值
	int	 fetchRowCount = 0;
	int	 i = 0;
	int	 blkNum = 0;	// 块号
	int ret;

	CString bill_of_lading_no("");
	CString task_no = "";
	CString vehicle_no = "";
	CString	record_name = "sm00_record";
	CString	ticket_no = "";

	CString sqlstr("");              // 数据库SQL操作字符串
	CModel tsmpe02("TSMPE02");
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");
	//CTOM01 tom01(conn);

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);

	try
	{
		/* 发货记录履历块 */
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}

		// 发送电文
		ret = f_xxsm02_snd(bcls_rec, bcls_ret, conn);
		if (ret < 0)
		{
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}

		for (i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			tsmpe02.Reset();
			tsmpe11.Reset();
			tsmpe12.Reset();
			task_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString().Trim();
			vehicle_no = bcls_rec->Tables[0].Rows[i]["VEHICLE_NO"].ToString().ToUpper().Trim();
			Log::Trace("", "task_no", "第【{0}】次task_no{1}", i, task_no);
			Log::Trace("", "tvehicle_no", "第【{0}】次vehicle_no{1}", i, vehicle_no);
			tsmpe02["TICKET_NO"] = task_no;
			tsmpe11["TICKET_NO"] = task_no;
			tsmpe12["TICKET_NO"] = task_no;
			if (tsmpe02.QueryCount("TICKET_NO")<=0)
			{
				strcpy(s.msg, "装车单号" + task_no + "在准发材料表中不存在");
				Log::Error("", __FUNCTION__, "装车单号{0}在准发材料表中不存在", task_no);
				//throw CApplicationException(-1, s.msg, s.svc_name);
			}
			else
			{
				tsmpe02["VEHICLE_NO"] = vehicle_no;
				tsmpe02.Update("VEHICLE_NO", "TICKET_NO");
			}
			//更新码单材料表
			if (tsmpe12.QueryCount("TICKET_NO") <= 0)
			{
				strcpy(s.msg, "装车单号" + task_no + "在码单表中不存在");
				Log::Error("", __FUNCTION__, "装车单号{0}在码单表中不存在", task_no);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			else
			{
				tsmpe12["VEHICLE_NO"] = vehicle_no;
				tsmpe12.Update("VEHICLE_NO", "TICKET_NO");
			}
			//更新码单表
			if (tsmpe11.QueryCount("TICKET_NO") <= 0)
			{
				strcpy(s.msg, "装车单号" + task_no + "在码单表中不存在");
				Log::Error("", __FUNCTION__, "装车单号{0}在码单表中不存在", task_no);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			else
			{
				tsmpe11["VEHICLE_NO"] = vehicle_no;
				tsmpe11.Update("VEHICLE_NO", "TICKET_NO");
			}


			// 记录履历
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString().Trim();
			sqlstr = "SELECT MAT_NO FROM TSMPE12 WHERE TICKET_NO ='" + ticket_no + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			while (cmd_inq.Read())
			{
				tsmpe12["MAT_NO"] = cmd_inq.GetString(1);

				/*	调用函数新增履历记录	*/
				bcls_rec->Tables[record_name].Rows[0]["mat_no"] = tsmpe12["MAT_NO"].ToString();
				bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "D";	// 车号修正
				bcls_rec->Tables[record_name].Rows[0]["userid"] = s.userid;

				ret = 0;
				ret = f_sm00_record(bcls_rec, bcls_ret, conn);
				if (ret < 0)
				{
					Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
			}
			cmd_inq.Close();
		}
		sprintf(s.msg, "车号修改成功");

	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{

		CFormattable arguments[] = { ex.GetCode() };// 定义参数列表的数组
		CMessageFormat::Format(s.msg, "数据库处理出错sqlcode=[{0}]", arguments, 1);//格式化字符串
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
		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
	}

	//返回-1时事务将回滚，返回为0是事务将提交
	return doFlag;

}
