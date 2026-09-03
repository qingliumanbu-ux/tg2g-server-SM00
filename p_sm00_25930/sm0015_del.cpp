/*
程序名称:		sm0015_del
产品名称:		BSM1
功能描述:		汽运装车撤销
外部接口:		无
相关数据库表:
无
主要逻辑说明:
	发送码单删除电文
	发送计划结案取消电文
	发送门禁取消电文
	根据装车单上的计划号去恢复计划状态、完成量、件等数据
	记录卸车履历
	材料从码单材料表退回到发货材料表，根据计划类型清计划号、材料状态等
	调用物料函数材料从历史档拉回当前档
	调用仓库函数写材料入库队列
备注:
修改历史:
修改人	linsy修改日期	2017-06-06	内容
*/
#include "stdafx.h"
//程序用头文件
#include "epex.h"

int f_sm00_xxjlwt_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	// 计量委托
int f_wm00_queue(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	// 仓库出入库队列
int f_sm00_record(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);/* 写履历记录 */

int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int f_sm00_mm99(vector <CString> pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */

int f_xxsm01_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 码单实绩电文 */
int f_xxsm04_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);					/* 计划结案电文 */
int f_xx00s4_snd(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);					/* 给L4发送发货实绩电文 */

// service入口
BM2F_ENTERACE(sm0015_del)
/* -EP_SYSTEM_HEAD_END */
int f_sm0015_del(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
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
	vector <CString> mat_no;

	CModel tsmpe10("TSMPE10");
	CModel tsmpe02("TSMPE02");/*产成品材料表*/
	CModel tsmpe12("TSMPE12");/*码单材料表*/
	CModel tsmpe11("TSMPE11");/*码单表*/
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_upd(conn);
	CDbCommand cmd_del(conn);
	CDbCommand cmd_loop(conn);
	CDbCommand cmd_loop1(conn);
	CDbCommand cmd_loop2(conn);
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


		//入库队列
		CString blk_name_wm = "WM00QUE";
		EIClass bcls_stock_que;
		bcls_stock_que.Tables.Add(blk_name_wm);
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "MAT_NO");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "TO_STOCK_NO");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "STOCK_NO");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "UNIT_CODE");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "OPER_FLAG");
		bcls_stock_que.Tables[blk_name_wm].Columns.Add(DT_STRING, "STOCK_OPER_ORDER");
		bcls_stock_que.Tables[blk_name_wm].Rows.Clear();

		//计量委托
		EIClass	bcls_rec_jlwt;
		bcls_rec_jlwt.Tables[0].set_TableName("JLWT");
		bcls_rec_jlwt.Tables[0].Columns.Add(DT_STRING, "TICKET_NO");
		bcls_rec_jlwt.Tables[0].Columns.Add(DT_STRING, "OPER_FLAG");

		//发货履历
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}

		// 门禁电文


		// 码单电文
		// 码单电文发送函数块
		if (!bcls_rec->Tables.Contains("0"))
		{
			bcls_rec->Tables.Add("0");
			bcls_rec->Tables["0"].Columns.Add(DT_STRING, "stacking_no");
			bcls_rec->Tables["0"].Columns.Add(DT_STRING, "tc_no");
			bcls_rec->Tables["0"].Columns.Add(DT_STRING, "oper_flag");
		}

		for (i = 0; i < v_count; i++)
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString();	// 装车单号
			if (ticket_no.Trim() == "")
			{
				strcpy(s.msg, "装车单号不能为空");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			sqlstr = " SELECT * FROM TSMPE11 WHERE TICKET_NO = '" + ticket_no + "' ";
			cmd_loop.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_loop.ExecuteReader();
			while (cmd_loop.Read())
			{
				cmd_loop.Fetch(tsmpe11);

				// 读取提单记录
				sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = '" + tsmpe11["BILL_OF_LADING_NO"].ToString() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					cmd_inq.Fetch(tsmpe10);
				}
				else
				{
					CFormattable	arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString() };
					CMessageFormat::Format(s.msg, "无此提单记录:{0}", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				cmd_inq.Close();


				bcls_rec->Tables["0"].Rows.Clear();
				bcls_rec->Tables["0"].Rows.Add();
				int jj = bcls_rec->Tables["0"].Rows.get_Count() - 1;
				bcls_rec->Tables["0"].Rows[jj]["stacking_no"] = tsmpe11["STACKING_NO"].ToString();

				tc_no = tsmpe10["REC_CREATOR"].ToString().SubstringNE(2, 2) + tsmpe10["REC_CREATOR"].ToString().SubstringNE(0, 2) + "01";
				bcls_rec->Tables["0"].Rows[jj]["tc_no"] = tc_no;
				bcls_rec->Tables["0"].Rows[jj]["oper_flag"] = "D";

				//liguangyuan 20230908 add 判定计划是L4接收的还是销售物流接收的
				if (tsmpe10["REMARK1"].ToString() == "L4")
					ret = f_xx00s4_snd(bcls_rec, bcls_ret, conn);
				else
					ret = f_xxsm01_snd(bcls_rec, bcls_ret, conn);;
				if (ret != 0)
				{
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

			}
			cmd_loop.Close();
		}


		// 计划结案电文
		// 按装车单读取发货计划
		EIClass bcls_rec_fhja;
		bcls_rec_fhja.Tables[0].Columns.Add(DT_STRING, "BILL_OF_LADING_NO");
		bcls_rec_fhja.Tables[0].Columns.Add(DT_STRING, "ORDER_NO");
		bcls_rec_fhja.Tables[0].Columns.Add(DT_STRING, "OPER_FLAG");
		for (i = 0; i < v_count; i++)
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString();	// 装车单号
			if (ticket_no.Trim() == "")
			{
				strcpy(s.msg, "装车单号不能为空");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			CString bill_of_lading_no = "";
			sqlstr = " SELECT DISTINCT BILL_OF_LADING_NO FROM TSMPE11 WHERE TICKET_NO = '" + ticket_no + "' ";
			cmd_loop.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_loop.ExecuteReader();
			while (cmd_loop.Read())
			{
				bill_of_lading_no = cmd_loop.GetString(1);
				// 按计划号到计划表上读取合同号，合约号
				CString order_no = "", contract_no = "", delivy_qty_flag = "";
				sqlstr = "select DISTINCT ORDER_NO,CONTRACT_NO,DELIVY_QTY_FLAG,REMARK1 from tsmpe10 where BILL_OF_LADING_NO = '" + bill_of_lading_no + "' ";
				cmd_loop1.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_loop1.ExecuteReader();

				while (cmd_loop1.Read())
				{
					order_no = cmd_loop1.GetString(1);
					contract_no = cmd_loop1.GetString(2);
					delivy_qty_flag = cmd_loop1.GetString(3);
					if (delivy_qty_flag == "2")	order_no = contract_no;

					bcls_rec_fhja.Tables[0].Rows.Clear();
					bcls_rec_fhja.Tables[0].Rows.Add();
					int ii = bcls_rec_fhja.Tables[0].Rows.get_Count() - 1;
					bcls_rec_fhja.Tables[0].Rows[ii]["BILL_OF_LADING_NO"] = bill_of_lading_no;
					bcls_rec_fhja.Tables[0].Rows[ii]["ORDER_NO"] = order_no;
					bcls_rec_fhja.Tables[0].Rows[ii]["OPER_FLAG"] = "D";

					//liguangyuan 20230908 add 判定计划是L4接收的还是销售物流接收的
					if (cmd_loop1.GetString(4) == "L4")
					{

					}
					else 
					{
						ret = 0;
						ret = f_xxsm04_snd(&bcls_rec_fhja, bcls_ret, conn);
						if (ret < 0)
						{
							Log::Debug("", __FUNCTION__, "f_xxsm04_snd函数调用出错.");
							throw CApplicationException(-1, s.msg, s.svc_name);
						}
					}

				}
				cmd_loop1.Close();
			}
			cmd_loop.Close();
		}



		// 发送计量委托删除电文
		for (i = 0; i < v_count; i++)
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString();	// 装车单号
			if (ticket_no.Trim() == "")
			{
				strcpy(s.msg, "装车单号不能为空");
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

#ifdef SM_JLWT_PD
		if (bcls_rec_jlwt.Tables[0].Rows.get_Count() >0)
		{
			ret = f_sm00_xxjlwt_snd(&bcls_rec_jlwt, bcls_ret, conn);
			if (ret < 0)
			{
				throw	CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		// 
		for (i = 0; i < v_count; i++)
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString();	// 装车单号
			Log::Debug("", __FUNCTION__, " 第 [{0}] 条,共[{1}]条，装车单号[{2}]", i + 1, v_count, ticket_no);
			if (ticket_no.Trim() == "")
			{
				strcpy(s.msg, "装车单号不能为空");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			// 根据装车单读取提单
			mat_no.clear();
			sqlstr = " SELECT * FROM TSMPE11 WHERE TICKET_NO = '" + ticket_no + "' ";
			cmd_loop.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_loop.ExecuteReader();
			while (cmd_loop.Read())
			{
				cmd_loop.Fetch(tsmpe11);
				if (tsmpe11["TRNP_MODE_CODE"].ToString().SubstringNE(1,1) != "1" )	// 运输方式不是汽运
				{
					strcpy(s.msg, "运输方式不是汽运！");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}


				// 读取提单记录
				sqlstr = "select * from tsmpe10 where BILL_OF_LADING_NO = '" + tsmpe11["BILL_OF_LADING_NO"].ToString() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					cmd_inq.Fetch(tsmpe10);
				}
				else
				{
					CFormattable	arguments[] = { tsmpe11["BILL_OF_LADING_NO"].ToString() };
					CMessageFormat::Format(s.msg, "无此提单记录:{0}", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				cmd_inq.Close();

				//liguangyuan 20230907 add 汽运的删除码单时将车辆信息拉回（汽运且状态为已确认）
				if (tsmpe11["TRNP_MODE_CODE"].ToString().SubstringNE(1, 1) == "1" &&
					tsmpe11["STACKING_STATUS"].ToString() == "0")
				{
					sqlstr = " INSERT INTO TSM00B4 "
						" SELECT * FROM HSM00B4 "
						" WHERE BILL_OF_LADING_NO='" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' "
						"   AND VEHICLE_NO='" + tsmpe11["VEHICLE_NO"].ToString() + "'";
					cmd_inq.SetCommandText(sqlstr);
					cmd_inq.ExecuteNonQuery();

					sqlstr = " DELETE FROM HSM00B4 "
						" WHERE BILL_OF_LADING_NO='" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' "
						"   AND VEHICLE_NO='" + tsmpe11["VEHICLE_NO"].ToString() + "'";
					cmd_inq.SetCommandText(sqlstr);
					cmd_inq.ExecuteNonQuery();

					sqlstr = " UPDATE TSM00B4 "
						" SET status='2', TICKET_NO=' ',TERMINAL_NAME=' ' "
						" WHERE BILL_OF_LADING_NO='" + tsmpe10["BILL_OF_LADING_NO"].ToString() + "' "
						"   AND VEHICLE_NO='" + tsmpe11["VEHICLE_NO"].ToString() + "'";
					cmd_inq.SetCommandText(sqlstr);
					cmd_inq.ExecuteNonQuery();
				}

				// 更新计划表上的计划状态及完成量
				sqlstr = "UPDATE TSMPE10 SET DELIVY_PLAN_STATUS = '3' , DELIVY_WT = 0,DELIVY_NUM = 0 "
					" WHERE BILL_OF_LADING_NO = '" + tsmpe11["BILL_OF_LADING_NO"].ToString() + "' ";
				cmd_upd.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_upd.ExecuteNonQuery();


				// 写仓库入库队列
				sqlstr = "select * from tsmpe12 WHERE ticket_no = '" + ticket_no + "' ";
				cmd_loop1.SetCommandText(sqlstr);
				Log::Debug("", "", "sqlstr = {0}", sqlstr);
				cmd_loop1.ExecuteReader();
				while (cmd_loop1.Read())
				{
					cmd_loop1.Fetch(tsmpe12);
					tsmpe02.CopyFrom(tsmpe12);

					/*增加写入库队列 */
					bcls_stock_que.Tables[blk_name_wm].Rows.Add();
					int ii = bcls_stock_que.Tables[blk_name_wm].Rows.get_Count() - 1;
					bcls_stock_que.Tables[blk_name_wm].Rows[ii]["MAT_NO"] = tsmpe02["MAT_NO"];
					bcls_stock_que.Tables[blk_name_wm].Rows[ii]["TO_STOCK_NO"] = tsmpe02["STOCK_NO"];
					bcls_stock_que.Tables[blk_name_wm].Rows[ii]["STOCK_NO"] = tsmpe02["STOCK_NO"];
					bcls_stock_que.Tables[blk_name_wm].Rows[ii]["UNIT_CODE"] = " ";
					bcls_stock_que.Tables[blk_name_wm].Rows[ii]["OPER_FLAG"] = "1";
					bcls_stock_que.Tables[blk_name_wm].Rows[ii]["STOCK_OPER_ORDER"] = "1Z";	// 返厂入库


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
					mat_no.push_back(tsmpe02["MAT_NO"].ToString());

					// 新增材料表
					if (tsmpe10["DELIVY_QTY_FLAG"].ToString() == "1" 
						|| tsmpe10["DELIVY_QTY_FLAG"].ToString() == "2"
						|| tsmpe10["DELIVY_QTY_FLAG"].ToString() == "3"
						)
					{
						tsmpe02["BILL_OF_LADING_NO"] = " ";
					}
					tsmpe02["STACKING_NO"] = " ";
					tsmpe02["VEHICLE_NO"] = " ";
					tsmpe02["TICKET_NO"] = " ";
					sqlstr = "INSERT tsmpe02 INTO mat_no='" + tsmpe02["MAT_NO"].ToString() + "' ";
					tsmpe02.Insert();

					// 删除码单材料记录
					sqlstr = "DELETE tsmpe12 where mat_no='" + tsmpe12["MAT_NO"].ToString() + "' AND STACKING_NO = '" + tsmpe12["STACKING_NO"].ToString() + "' ";
					tsmpe12.Delete("MAT_NO,STACKING_NO");
				}
				cmd_loop1.Close();

				// 删除码单记录
				sqlstr = "delete from STACKING_NO = '" + tsmpe11["STACKING_NO"].ToString() + "' ";
				tsmpe11.Delete("STACKING_NO");
			}
			cmd_loop.Close();


			ret = f_sm00_mm99(mat_no, 3, -3, s.msg, conn);
			if (ret != 0)
			{
				CFormattable	arguments[] = { s.msg };
				CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			if (bcls_stock_que.Tables[blk_name_wm].Rows.get_Count() > 0)
			{
				doFlag = f_wm00_queue(&bcls_stock_que, bcls_ret, conn);
				if (doFlag != 0)
				{
					throw CApplicationException(doFlag, s.msg, log.Location);
				}
			}
			else
			{
				sprintf(s.msg, "没有读取到车上的材料记录[%s]", (const char *)ticket_no);
				//throw	CApplicationException(-1, s.msg, log.Location);
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

	return doFlag;

}
