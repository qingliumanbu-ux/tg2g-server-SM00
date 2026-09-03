/************************************************
*	程序名称：产成品发货处理——提单查询		*
*	编制日期：2023-1-18   	                    *
*	编 制 人：13801					            *
*************************************************/

//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件
	// 提单表

// service入口
BM2F_ENTERACE(sm0008_inq_master)

//自定义的函数
int f_sm0008_inq_master(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int	 doFlag = 0;				// 调用本函数的返回值
	int	 fetchRowCount = 0;		// 调用本函数的返回值
	int	 i = 0;
	int	 blkNum = 0;	// 块号
	int ret;

	CString date_from = "", date_to = "";

	CString sqlstr("");              // 数据库SQL操作字符串
	CModel tsmpe10("TSMPE10");

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
		tsmpe10.Reset();
		tsmpe10.MergeFrom(bcls_rec->Tables[blkNum].Rows[0]);
		if (bcls_rec->Tables[blkNum].Columns.Contains("DATE_FROM"))	date_from = bcls_rec->Tables[blkNum].Rows[0]["DATE_FROM"];
		if (bcls_rec->Tables[blkNum].Columns.Contains("DATE_TO"))	date_to = bcls_rec->Tables[blkNum].Rows[0]["DATE_TO"];

		Log::Debug("", "", "MAT_KIND=[{0}]", tsmpe10["MAT_KIND"].ToString());

		// sql语句
		sqlstr = "SELECT  T.* FROM 	tsmpe10 T WHERE  T.DELIVY_PLAN_STATUS < '5' ";

		if (tsmpe10["BILL_OF_LADING_NO"].ToString().Trim() != "")
		{
			sqlstr += " AND	T.BILL_OF_LADING_NO = '" + tsmpe10["BILL_OF_LADING_NO"].ToString().Trim() + "'";
		}

		if (tsmpe10["MAT_KIND"].ToString().Trim() != "")
		{
			sqlstr += " AND	T.MAT_KIND	= '" + tsmpe10["MAT_KIND"].ToString().Trim() + "'";
		}

		if (tsmpe10["FACTORY_DIV"].ToString().Trim() != "")
		{
			sqlstr += " AND	T.FACTORY_DIV	= '" + tsmpe10["FACTORY_DIV"].ToString().Trim() + "'";
		}

		if (tsmpe10["STOCK_NO"].ToString().Trim() != "")
		{
			//sqlstr += " AND	T.STOCK_NO	= '" + tsmpe10["STOCK_NO"].ToString().Trim() + "' ";
			//sqlstr += " AND T.STOCK_NO IN (SELECT STOCK_ADDR FROM TSI0021 WHERE STOCK_NO = '" + tsmpe10["STOCK_NO"].ToString().Trim() + "') ";
			sqlstr += " AND (T.STOCK_NO IN (SELECT STOCK_NO FROM TSI0021 WHERE STOCK_ADDR = '" + tsmpe10["STOCK_NO"].ToString().Trim() + "') "
				"                      OR  T.STOCK_NO = '" + tsmpe10["STOCK_NO"].ToString().Trim() + "' ) ";
		}

		if (tsmpe10["TRNP_MODE_CODE"].ToString().Trim() != "")
		{
			if (tsmpe10["TRNP_MODE_CODE"].ToString().Trim() == "2")
			{
				sqlstr += " AND	T.TRNP_MODE_CODE IN ( '2','22','12') ";
			}
			else if (tsmpe10["TRNP_MODE_CODE"].ToString().Trim() == "1")
			{
				sqlstr += " AND	T.TRNP_MODE_CODE IN ( '1','11','21','91') ";
			}
			else
			{
				sqlstr += " AND T.TRNP_MODE_CODE = '" + tsmpe10["TRNP_MODE_CODE"].ToString().Trim() + "' ";
			}
		}

		if (tsmpe10["VEHICLE_NO"].ToString().Trim() != "" )
		{
			sqlstr += " AND T.VEHICLE_NO LIKE '" + tsmpe10["VEHICLE_NO"].ToString().Trim() + "' ";
		}

		if (date_from.Trim() != "")
		{
			sqlstr += " AND T.REC_CREATE_TIME >= '" + date_from.Trim() + "' ";
		}
		if (date_to.Trim() != "")
		{
			sqlstr += " AND SUBSTR(T.REC_CREATE_TIME,1, length(@date_to)) <= '" + date_to.Trim() + "' ";
			cmd_inq.Parameters.Set("date_to", date_to.Trim());
		}

		sqlstr += " ORDER BY T.REC_CREATE_TIME ";
		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);

		cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);

		if (!bcls_ret->Tables[0].Columns.Contains("COLOR_MARK"))
		{
			bcls_ret->Tables[0].Columns.Add(DT_STRING, "COLOR_MARK");
		}
		CString v_bill_of_lading_no = "";
		CString v_trnp_mode_code = "";
		for (int i = 0; i < bcls_ret->Tables[0].Rows.get_Count(); i++)
		{
			v_bill_of_lading_no = bcls_ret->Tables[0].Rows[i]["BILL_OF_LADING_NO"].ToString().Trim();
			v_trnp_mode_code = bcls_ret->Tables[0].Rows[i]["TRNP_MODE_CODE"].ToString().Trim();
			if (v_trnp_mode_code.SubstringNE(1, 1) == "2")
			{
				sqlstr = "select * from tsmpe15 where BILL_OF_LADING_NO = '" + v_bill_of_lading_no + "' ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.ExecuteReader();
				if (!cmd_inq.Read())
				{
					bcls_ret->Tables[0].Rows[i]["COLOR_MARK"] = "1";	// 铁运未有车皮
				}
				cmd_inq.Close();

			}
		}


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
		CMessageFormat::Format(s.msg, "{0}:{1}", arguments, 2);
	}

	//返回-1时事务将回滚，返回为0是事务将提交
	return doFlag;

}
