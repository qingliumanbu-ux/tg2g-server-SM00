/*
程序名称:		f_sm00_md_del
隶属子系统:		PES
产品名称:		BSM1
功能描述:		装车撤销
外部接口:		无
相关数据库表:
无
主要逻辑说明:
备注:
修改历史:
*/
#include "stdafx.h"


//程序用头文件




#include "epex.h"


#ifdef _LINE_SM
int f_sm00_xxjlwt_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	// 计量委托
#endif
int f_sm00_record(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 写履历记录 */

/* -EP_SYSTEM_HEAD_END */
int f_sm00_md_del(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);
	/*程序用变量*/
	int blkNum = 0;
	int	ret = 0;
	int	doFlag = 0;
	int	fetchRowCount = 0;
	int	fetchRowCount1 = 0;
	int	blkSeq = 0;
	int	i = 0;
	int	temp = 0;
	CString	blk_name=	"md_ok";			/* 定义传入的块名 */
	CString	str="";
	CString	record_name = "sm00_record";
	CString	datetime = "";
	CString	v_userid = "";

	CString	c_cust_mat_no = " ";				/*材料号*/
	CString	c_bill_of_lading_no = "";			/* 提单号 */
	CString	c_stacking_no = "";
	CString  v_stacking_no = "";
	CString	v_code = "";//码单是否确认标记
	CString v_order_no_erp = "";
	CDecimal v_mat_net_wt = 0;
	CString	tc_no = "MEJL01";
	CString ticket_no = "";	// 装车单号

	CModel tsmpe10("TSMPE10");
	CModel tsmpe02("TSMPE02");/*产成品材料表*/
	CModel tsmpe12("TSMPE12");/*码单材料表*/
	CModel tsmpe11("TSMPE11");/*码单表*/
	CModel tsi0021("TSI0021");
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_upd(conn);
	CDbCommand cmd_del(conn);
	CDbCommand cmd_loop(conn);
	CString sqlstr;
	try
	{
		/*获得传入参数*/
		v_userid = CString(s.userid);	//取操作者工号变量
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		CString datetime1 = CDateTime::Parse(datetime).AddHours(24).ToString("yyyyMMddHHmmss");

		int v_count = bcls_rec->Tables[0].Rows.get_Count();
		if ( v_count == 0 )
		{
			sprintf(s.msg, "没有传入参数");
			throw	CApplicationException(-1,s.msg,log.Location);
		}



		// 计量委托
		EIClass	bcls_rec_jlwt;
		bcls_rec_jlwt.Tables[0].set_TableName("JLWT");
		bcls_rec_jlwt.Tables[0].Columns.Add(DT_STRING, "TICKET_NO");
		bcls_rec_jlwt.Tables[0].Columns.Add(DT_STRING, "OPER_FLAG");

		// 发货履历
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}


		// 发送计量委托删除电文
		for (i = 0; i < v_count; i++)
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString();	// 装车单号
			if (ticket_no.Trim() == "")
			{
				sprintf(s.msg, "装车单号不能为空");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			sqlstr = "SELECT * "
				" FROM TSMPE11 WHERE ticket_no = '" + ticket_no + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe11);
			}
			cmd_inq.Close();

			// 有称重委托时，发送委托取消电文
			if (tsmpe11["PONDER_NO"].ToString().Trim() != "")
			{
				bcls_rec_jlwt.Tables[0].Rows.Add();
				int ii = bcls_rec_jlwt.Tables[0].Rows.get_Count() - 1;
				bcls_rec_jlwt.Tables[0].Rows[ii]["TICKET_NO"] = ticket_no;
				bcls_rec_jlwt.Tables[0].Rows[ii]["OPER_FLAG"] = "D";
			}
		}

#ifdef _LINE_SM
		if (bcls_rec_jlwt.Tables[0].Rows.get_Count() >0)
		{
			//ret = f_sm00_xxjlwt_snd(&bcls_rec_jlwt, bcls_ret, conn);
			if (ret < 0)
			{
				throw	CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif


		for ( i = 0; i < v_count; i++ )
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString();	// 装车单号

			Log::Debug("", __FUNCTION__, " 第 [{0}] 条,共[{1}]条，装车单号[{2}]", i + 1, v_count, ticket_no);

			if (ticket_no.Trim() == "")
			{
				sprintf(s.msg, "装车单号不能为空");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}


			/* 判是按量发货的提单时，删除材料表上的提单号 */
			sqlstr = "SELECT * "
				" FROM TSMPE11 WHERE ticket_no = '" + ticket_no+ "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}",sqlstr);
			cmd_inq.ExecuteReader();
			if ( cmd_inq.Read() )
			{
				cmd_inq.Fetch(tsmpe11);
			}
			cmd_inq.Close();

			sqlstr = "SELECT * "
				" FROM TSMPE02 WHERE ticket_no = '" + ticket_no + "' ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe02);
			}
			cmd_inq.Close();


			if (tsmpe11["STACKING_STATUS"].ToString() == "1" )
			{
				////// 更新作业单上的状态
				////sqlstr = " update tsmpe10a set STATUS = '3' where work_id = '" + tsmpe02["WORK_ID"].ToString() + "' ";
				////cmd_upd.SetCommandText(sqlstr);
				////Log::Debug("", "", "sqlstr = {0}", sqlstr);
				////cmd_upd.ExecuteNonQuery();

				//// 更新产成品材料表上的码单号、车号为空
				//sqlstr = " UPDATE TSMPE02 SET STACKING_NO = ' ' , VEHICLE_NO = ' ' "
				//	" ,ticket_no = ' ' "
				//	" WHERE WORK_ID = '" + tsmpe02["WORK_ID"].ToString() + "' ";
				//cmd_upd.SetCommandText(sqlstr);
				//Log::Debug("", "", "sqlstr = {0}", sqlstr);
				//cmd_upd.ExecuteNonQuery();


				// 更新计划表上的计划状态，完成量、完成件数
				sqlstr = "SELECT DISTINCT BILL_OF_LADING_NO FROM TSMPE11 WHERE STACKING_STATUS = '1' AND ticket_no = '" + ticket_no + "' ";
				cmd_loop.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr = {0}", sqlstr);
				cmd_loop.ExecuteReader();
				while (cmd_loop.Read())
				{
					tsmpe10["BILL_OF_LADING_NO"] = cmd_loop.GetString(1);
					
					tsmpe10["PLAN_WT"] = 0;
					tsmpe10["PLAN_NUM"] = 0;
					tsmpe10["DELIVY_PLAN_STATUS"] = "3";
					sqlstr = "Update tsmpe10 where BILL_OF_LADING_NO = '" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' ";
					tsmpe10.Update("DELIVY_PLAN_STATUS,PLAN_NUM,PLAN_WT", "BILL_OF_LADING_NO");
				}
				cmd_loop.Close();


				// 删除码单表记录
				sqlstr = " DELETE FROM TSMPE11 WHERE STACKING_STATUS = '1' AND ticket_no = '"+ ticket_no + "' ";
				cmd_del.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr = {0}", sqlstr);
				if (cmd_del.ExecuteNonQuery() == 0)
				{
					sprintf(s.msg, "没有找到符合条件的装车单%s记录!", (const char *)ticket_no);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				// 删除码单材料记录
				sqlstr = " DELETE FROM TSMPE12 WHERE ticket_no = '" + ticket_no + "' ";
				cmd_del.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr = {0}", sqlstr);
				cmd_del.ExecuteNonQuery();

				// 记履历
				sqlstr = "select * from tsmpe02 WHERE ticket_no = '" + ticket_no + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr = {0}", sqlstr);
				cmd_inq.ExecuteReader();
				while (cmd_inq.Read())
				{
					cmd_inq.Fetch(tsmpe02);


					/*	调用函数新增履历记录												*/
					bcls_rec->Tables[record_name].Rows[0]["mat_no"] = tsmpe02["MAT_NO"];
					bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "F";	// 卸车
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


				// 更新产成品材料表上的码单号、车号为空
				sqlstr = " UPDATE TSMPE02 SET STACKING_NO = ' ' , VEHICLE_NO = ' ' "
					" ,ticket_no = ' ' "
					" WHERE ticket_no = '" + ticket_no + "' ";
				cmd_upd.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr = {0}", sqlstr);
				cmd_upd.ExecuteNonQuery();

			}
		}

		sprintf(s.msg, "码单撤销处理成功");

	}
	catch ( CDbException& ex )  //捕获数据库操作异常
	{
		CFormattable arguments[] ={ ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		Log::Error("", __FUNCTION__, "error=[{0}]", s.sysmsg);
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch ( CApplicationException& ex )  //捕获应用错误
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch ( CException& ex )
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	if (doFlag < 0)
	{
		//CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
	}

	return doFlag;

}
