/************************************************
*	程序名称：装车单查询						*
*	编制日期：2023-2-21   	                    *
*	编 制 人：13801					            *
*************************************************/

//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件
	// 提单表
#include "CDynaTable.h"

// service入口
BM2F_ENTERACE(sm0015_inq_master)

//自定义的函数
int f_sm0015_inq_master(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int	 doFlag = 0;				// 调用本函数的返回值
	int	 fetchRowCount = 0;		// 调用本函数的返回值
	int	 i = 0;
	int	 blkNum = 0;	// 块号
	int ret;

	CString date_from = "", date_to = "";
	CString mat_no = "";
	CString ticket_no = "";//提货单号

	CString sqlstr("");              // 数据库SQL操作字符串
	CString sqlstr_tno("");              
	CModel tsmpe11("TSMPE11");

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq1(conn);
	try
	{
		fetchRowCount = bcls_rec->Tables[blkNum].Rows.get_Count();
		if (fetchRowCount == 0)
		{
			sprintf(s.msg, "没有传入参数");
			throw	CApplicationException(-1, s.msg, log.Location);
		}
		PrintDataTable(bcls_rec->Tables[blkNum]);

		// 读取传入的参数
		tsmpe11.MergeFrom(bcls_rec->Tables[blkNum].Rows[0]);
		date_from = bcls_rec->Tables[blkNum].Rows[0]["DELIVY_TIME_FROM"];
		date_to = bcls_rec->Tables[blkNum].Rows[0]["DELIVY_TIME_TO"];

		Log::Debug("", "", "date_from = {0},date_to={1}", date_from, date_to);

		// sql语句
		sqlstr_tno = " SELECT TICKET_NO FROM TSMPE12 WHERE 1=1 ";
		sqlstr = "SELECT  T.* FROM 	vsmpe11 T WHERE  1=1 AND TICKET_NO IN ( SELECT  TICKET_NO FROM 	Tsmpe11 T WHERE  1=1 " ;

		if (date_from.Trim() != "")	sqlstr += " AND	T.DELIVY_TIME >= '" + date_from.Trim() + "'";
		if (date_to.Trim() != "")	sqlstr += " AND	T.DELIVY_TIME <= '" + date_to.Trim() + "'";
		if (tsmpe11["BILL_OF_LADING_NO"].ToString().Trim() != "")	sqlstr += " AND	T.BILL_OF_LADING_NO LIKE '" + tsmpe11["BILL_OF_LADING_NO"].ToString().Trim() + "%'";
		if (tsmpe11["CONSIGNE_NAME"].ToString().Trim() != "")	sqlstr += " AND	T.CONSIGNE_NAME LIKE '" + tsmpe11["CONSIGNE_NAME"].ToString().Trim() + "%'";
		if (tsmpe11["TICKET_NO"].ToString().Trim() != "")	sqlstr += " AND	T.TICKET_NO LIKE '" + tsmpe11["TICKET_NO"].ToString().Trim() + "%'";
		if (tsmpe11["VEHICLE_NO"].ToString().Trim() != "")	sqlstr += " AND	T.VEHICLE_NO LIKE '%" + tsmpe11["VEHICLE_NO"].ToString().Trim() + "%' ";
		if (tsmpe11["ORDER_NO"].ToString().Trim() != "")	sqlstr += " AND	T.ORDER_NO LIKE '%" + tsmpe11["ORDER_NO"].ToString().Trim() + "%' ";
		if (tsmpe11["FACTORY_DIV"].ToString().Trim() != "")	sqlstr += " AND	T.FACTORY_DIV LIKE '" + tsmpe11["FACTORY_DIV"].ToString().Trim() + "'";
		if (tsmpe11["STOCK_NO"].ToString().Trim() != "")	sqlstr += " AND	T.STOCK_NO = '" + tsmpe11["STOCK_NO"].ToString().Trim() + "'";
		if (tsmpe11["DELIVY_PLACE_NAME"].ToString().Trim() != "")	sqlstr += " AND T.DELIVY_PLACE_NAME LIKE '" + tsmpe11["DELIVY_PLACE_NAME"].ToString().Trim() + "%' ";
		if (tsmpe11["TRNP_MODE_CODE"].ToString().Trim() != "")
		{
			if (tsmpe11["TRNP_MODE_CODE"].ToString().Trim() == "2")
			{
				sqlstr += " AND	T.TRNP_MODE_CODE IN ( '2','22','12','92') ";
			}
			else if (tsmpe11["TRNP_MODE_CODE"].ToString().Trim() == "1")
			{
				sqlstr += " AND	T.TRNP_MODE_CODE IN ( '1','11','21','91') ";
			}
			else
			{
				sqlstr += " AND T.TRNP_MODE_CODE = '" + tsmpe11["TRNP_MODE_CODE"].ToString().Trim() + "' ";
			}
		}
		if (tsmpe11["MAT_KIND"].ToString().Trim() != "")
		{
			if (tsmpe11["MAT_KIND"].ToString().Trim() == "SM")
			{
				sqlstr = sqlstr + " AND	exists (select 1 from tsi0021 b where t.stock_no = b.stock_no and b.mat_line_type = 'SM' AND  t.mat_kind in ('SM','SN') ) ";
			}
			else
			{
				sqlstr = sqlstr + " AND	T.MAT_KIND = '" + tsmpe11["MAT_KIND"].ToString().Trim() + "' ";
			}
		}
		if (tsmpe11["STACKING_STATUS"].ToString().Trim() != "")
		{
			sqlstr += " AND STACKING_STATUS = '" + tsmpe11["STACKING_STATUS"].ToString() + "' ";
		}

		if (bcls_rec->Tables[0].Columns.Contains("MAT_NO"))
		{
			mat_no = bcls_rec->Tables[0].Rows[0]["MAT_NO"].ToString();
			Log::Debug("", "", "mat_no = {0}", mat_no);

			if (mat_no.Trim() != "")
			{
				sqlstr_tno += " AND MAT_NO LIKE '" + mat_no.Trim() + "%' ";

				int blkNum = bcls_ret->Tables.IndexOf("TICKET_NO");
				if (blkNum < 0)
				{
					bcls_ret->Tables.Add("TICKET_NO");
					bcls_ret->Tables["TICKET_NO"].Columns.Add(DT_STRING, "TICKET_NO");
				}
				Log::Debug("", __FUNCTION__, "sqlstr_tno 1 ={0}", sqlstr_tno);

				cmd_inq.SetCommandText(sqlstr_tno);
				cmd_inq.ExecuteQuery(bcls_ret->Tables["TICKET_NO"]);
				int count = bcls_ret->Tables["TICKET_NO"].Rows.get_Count();
				Log::Debug("", __FUNCTION__, "count = { 0 }", count);
				ticket_no = "*";
				for (int i = 0; i < count; i++)
				{
					ticket_no = bcls_ret->Tables["TICKET_NO"].Rows[i]["TICKET_NO"];
					if (i == 0)
					{
						ticket_no = "'" + ticket_no.Trim() + "'";
					}
					else
					{
						ticket_no = ticket_no + ",'" + ticket_no.Trim() + "'";
					}

				}
				Log::Debug("", __FUNCTION__, "sqlstr_tno  ={0}", sqlstr_tno);
				Log::Debug("", __FUNCTION__, "ticket_no 1 ={0}", ticket_no);

				if (ticket_no.Trim() != "")	sqlstr = sqlstr + " AND	T.TICKET_NO IN ( " + ticket_no.Trim() + ") ";
				Log::Debug("", __FUNCTION__, "ticket_no 2 ={0}", ticket_no);
				cmd_inq.Close();
			}
			
		}
		
		sqlstr += " ) ORDER BY T.DELIVY_TIME DESC ";
		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);

		cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);

		if (bcls_ret->Tables[0].Rows.get_Count() > 0)
		{
			CString trnp_mode_code = "";
			CString status = "";
			CString vehicle_no = "";
			CString TICKET_NO = "";
			CString WORK_ID = "";
			bcls_ret->Tables[0].Columns.Add(DT_STRING, "STATUS");
			bcls_ret->Tables[0].Columns.Add(DT_STRING, "WORK_ID");
			bcls_ret->Tables[0].Columns.Add(DT_DECIMAL, "STACKING_WT_FT");


			// 2022-4-8
			if (!bcls_ret->Tables[0].Columns.Contains("LANE_NO"))	// 股道代码
			{
				bcls_ret->Tables[0].Columns.Add(DT_STRING, "LANE_NO");
			}
			// 2022-4-8

			for (int i = 0; i < bcls_ret->Tables[0].Rows.get_Count(); i++)
			{
				trnp_mode_code = bcls_ret->Tables[0].Rows[i]["TRNP_MODE_CODE"].ToString().Trim();
				tsmpe11["STACKING_STATUS"] = bcls_ret->Tables[0].Rows[i]["STACKING_STATUS"].ToString().Trim();
				vehicle_no = bcls_ret->Tables[0].Rows[i]["VEHICLE_NO"].ToString().Trim();
				TICKET_NO = bcls_ret->Tables[0].Rows[i]["TICKET_NO"].ToString().Trim();
				if (trnp_mode_code.SubstringNE(1, 1) == "2" && tsmpe11["STACKING_STATUS"].ToString() == "1")
				{
					sqlstr = "select status from tsm00b4 where vehicle_no='" + vehicle_no + "'";
					Log::Debug("", __FUNCTION__, "作业单sqlstrtd={0}", sqlstr);
					cmd_inq1.SetCommandText(sqlstr);
					cmd_inq1.ExecuteReader();
					if (cmd_inq1.Read())
					{
						status = cmd_inq1.GetString(1).Trim();
					}
					else
					{
						status = " ";
					}
					cmd_inq1.Close();
					bcls_ret->Tables[0].Rows[i]["STATUS"] = status;
				}
				else
				{
					bcls_ret->Tables[0].Rows[i]["STATUS"] = "";
				}
				bcls_ret->Tables[0].Rows[i]["STACKING_WT_FT"] = bcls_ret->Tables[0].Rows[i]["STACKING_WT"].ToDecimal() + bcls_ret->Tables[0].Rows[i]["STACKING_DISCREP_WT"].ToDecimal();


				// 硅钢产线读取铁运读取股道号 2022-4-8
				CString PART_NAME = getenv("BM2_PART_NAME");
				Log::Debug("", "", "PART_NAME={0}", PART_NAME);
				CString FACTORY_DIV = "";
				FACTORY_DIV = bcls_ret->Tables[0].Rows[i]["FACTORY_DIV"].ToString().Trim();
				if ((PART_NAME == "AGP9Z" || FACTORY_DIV == "A61" || FACTORY_DIV == "A62") && trnp_mode_code.SubstringNE(1, 1) == "2")
				{
					sqlstr = "select LANE_NO from tsm00b4 where TICKET_NO = '" + TICKET_NO + "' ";
					cmd_inq.SetCommandText(sqlstr);
					Log::Info("", "", "sqlstr={0}", sqlstr);
					cmd_inq.ExecuteReader();
					if (cmd_inq.Read())
					{
						bcls_ret->Tables[0].Rows[i]["LANE_NO"] = cmd_inq.GetString(1);
					}
					else
					{
						cmd_inq.Close();
						sqlstr = "select LANE_NO from hsm00b4 where TICKET_NO = '" + tsmpe11["TICKET_NO"].ToString() + "' ";
						cmd_inq.SetCommandText(sqlstr);
						Log::Info("", "", "sqlstr={0}", sqlstr);
						cmd_inq.ExecuteReader();
						if (cmd_inq.Read())
						{
							bcls_ret->Tables[0].Rows[i]["LANE_NO"] = cmd_inq.GetString(1);
						}
					}
					cmd_inq.Close();
				}
				// 2022-4-8

				/*sqlstr = "select t.work_id from tsmpe12 t where t.ticket_no = '" + TICKET_NO  + "' group by t.work_id ";
				Log::Debug("", __FUNCTION__, "sqlstrtd={0}", sqlstr);
				cmd_inq1.SetCommandText(sqlstr);
				cmd_inq1.ExecuteReader();
				if (cmd_inq1.Read())
				{
				WORK_ID = cmd_inq1.GetString(1).Trim();
				}
				else
				{
				WORK_ID = " ";
				}
				cmd_inq1.Close();
				bcls_ret->Tables[0].Rows[i]["WORK_ID"] = WORK_ID;*/
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
