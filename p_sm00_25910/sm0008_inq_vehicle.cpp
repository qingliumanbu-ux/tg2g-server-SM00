/************************************************
*	程序名称：产成品发货处理——车辆信息查询	*
*	编制日期：2020-1-15   	                    *
*	编 制 人：13801					            *
*************************************************/

//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件
	// 车辆表

// service入口
BM2F_ENTERACE(sm0008_inq_vehicle)

//自定义的函数
int f_sm0008_inq_vehicle(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);

	int	 doFlag = 0;				// 调用本函数的返回值
	int	 fetchRowCount = 0;		// 调用本函数的返回值
	int	 i = 0;
	int	 blkNum = 0;	// 块号
	int ret;

	CString sqlstr("");              // 数据库SQL操作字符串
	CModel tsm00b4("TSM00B4");

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);

	try
	{
		fetchRowCount = bcls_rec->Tables[blkNum].Rows.get_Count();
		if (fetchRowCount == 0)
		{
			sprintf(s.msg, "没有传入参数");
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		// 读取传入的参数
		tsm00b4.Reset();
		tsm00b4.MergeFrom(bcls_rec->Tables[blkNum].Rows[0]);

		// 打印传入的变量
		tsm00b4.Print();

		// sql语句
		sqlstr = "SELECT  T.* FROM 	tsm00b4 T WHERE  T.STATUS = '2' ";

		//if (tsm00b4["MAT_KIND"].ToString().Trim() != "")
		//{
		//	sqlstr += " AND	T.MAT_KIND	= '" + tsm00b4["MAT_KIND"].ToString().Trim() + "'";
		//}

		//if (tsm00b4["FACTORY_DIV"].ToString().Trim() != "")
		//{
		//	sqlstr += " AND	T.FACTORY_DIV	= '" + tsm00b4["FACTORY_DIV"].ToString().Trim() + "'";
		//}
		if (tsm00b4["VEHICLE_NO"].ToString().Trim() != "")
		{
			sqlstr += " AND	T.VEHICLE_NO = '" + tsm00b4["VEHICLE_NO"].ToString().Trim() + "' ";
		}

		if (tsm00b4["STOCK_NO"].ToString().Trim() != "")
		{
			sqlstr += " AND	T.STOCK_NO IN ( '" + tsm00b4["STOCK_NO"].ToString().Trim() + "',' ') ";
		}

		tsm00b4["TRNP_MODE_CODE"] = tsm00b4["TRNP_MODE_CODE"].ToString().TrimOrBlank().SubstringNE(1, 1);
		Log::Debug("", __FUNCTION__, "TRNP_MODE_CODE={0}", tsm00b4["TRNP_MODE_CODE"].ToString().Trim());

		sqlstr += " AND	T.TRNP_MODE_CODE = '" + tsm00b4["TRNP_MODE_CODE"].ToString().Trim() + "' ";
		if (tsm00b4["TRNP_MODE_CODE"].ToString().Trim() == "1")
		{
			if (bcls_rec->Tables[blkNum].Columns.Contains("BILL_OF_LADING_NO"))
			{
				sqlstr += " AND	T.BILL_OF_LADING_NO = '" + tsm00b4["BILL_OF_LADING_NO"].ToString() + "' ";
			}
			else
			{
				sqlstr += "ORDER BY ARRIVAL_TIME ASC,REC_CREATE_TIME DESC ";
			}
		}
		else
		{
			sqlstr += "ORDER BY LANE_NO DESC,SEQ_NO DESC,REC_CREATE_TIME DESC ";
		}


		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);

		cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);

		CFormattable arguments[] = { bcls_ret->Tables[0].Rows.get_Count() };// 定义参数列表的数组
		CMessageFormat::Format(s.msg, "查询到[{0}]条记录。", arguments, 1);//格式化字符串
		Log::Info("", __FUNCTION__, "{0}", s.msg);
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
		CMessageFormat::Format(s.msg, "{0} :{1}", arguments, 2);
	}

	//返回-1时事务将回滚，返回为0是事务将提交
	return doFlag;

}
