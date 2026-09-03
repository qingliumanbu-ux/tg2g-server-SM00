/*
程序名称:		f_sm00_madan_red
隶属子系统:		SM00
产品名称:		PES
功能描述:		接收L4码单红冲
外部接口:		无
相关数据库表:
无
主要逻辑说明:
备注:
修改历史:
修改人			修改日期		内容
BM2IDE	2012-04-25		当前程序被创建。
码单红冲流程：MMS的发货做码单红冲，发送码单红冲电文，PES发货接受电文的后台中调用物料跟踪，写入库队列
码单红冲业务：物料状态红冲到准发确认后
*/
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除
#include "epex.h"







//名称空间引用




/*  函数申明  */
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection* conn);
int f_sm00_record(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
int f_wm00_queue(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);	// 仓库出入库队列

int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection* conn);	/* 抛物料跟踪打包函数 */
int f_sm00_mm99(vector <CString> pack_num, int para_type, int event_id, CString msg, CDbConnection* conn);	/* 抛物料跟踪打包函数 */

//BM2F_ENTERACE_TELE(cm_0020sc_rcv)

int f_00xxsc_rcv(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义
	/*定义函数名*/

	/*程序用变量*/
	int		i = 0;
	CString	record_name = "sm00_record";
	CString	blkName = "madan_red";
	CString op_flag = "";
	int		ret = 0;
	int		doFlag = 0;

	CModel tsmpe02("TSMPE02");
	CModel tsmpe12("TSMPE12");
	CModel tsmpe11("TSMPE11");
	CModel tsmpe01("TSMPE01");
	CModel tsmpe00("TSMPE00");
	CModel tsmpe10("TSMPE10");

	/*在SQL语句中使用的变量*/
	CString v_mat_no = "";
	CString c_userid = "";
	CString	sqlstr("");
	vector <CString> mat_no;

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);

	try
	{
		/*添加并设置块名*/
		if (bcls_rec->Tables.IndexOf(blkName) < 0)	bcls_rec->Tables[0].set_TableName(blkName);

		/*获得传入参数*/
		c_userid = s.userid;
		/* 定义写履历块 */
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
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

		//	读取传入的参数
		int	rows = bcls_rec->Tables[blkName].Rows.get_Count();
		for (i = 0; i < rows; i++)
		{
			Log::Trace("", __FUNCTION__, "第[{0}]条记录，共[{1}]条记录", i + 1, rows);
			if (bcls_rec->Tables[blkName].Columns.Contains("MAT_NO"))
			{
				tsmpe02["MAT_NO"] = bcls_rec->Tables[blkName].Rows[i]["MAT_NO"];
			}
			else
			{
				sprintf(s.msg, "没有传入材料号参数");
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			if (bcls_rec->Tables[blkName].Columns.Contains("STACKING_NO"))
			{
				tsmpe02["STACKING_NO"] = bcls_rec->Tables[blkName].Rows[i]["STACKING_NO"];
			}
			else
			{
				sprintf(s.msg, "没有传入码单号参数");
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			if (bcls_rec->Tables[blkName].Columns.Contains("OP_FLAG"))
			{
				op_flag = bcls_rec->Tables[blkName].Rows[i]["OP_FLAG"];
			}
			else
			{
				sprintf(s.msg, "没有传入标记参数");
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			Log::Trace("", __FUNCTION__, "材料号=[{0}] , 码单号[{1}],标记[{2}]", tsmpe02["MAT_NO"].ToString(), tsmpe02["STACKING_NO"].ToString(), op_flag);

			/* ***** 检查输入参数合法性 ***** */
			tsmpe02.TrimOrBlank();
			if (tsmpe02["MAT_NO"].ToString().Trim() == "")
			{
				strcpy(s.msg, "材料号不能为空");
				strcpy(s.sysmsg, "材料号不能为空!");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			if (tsmpe02["STACKING_NO"].ToString().Trim() == "")
			{
				strcpy(s.msg, "码单号不能为空");
				strcpy(s.sysmsg, "码单号不能为空!");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			sqlstr = " SELECT * FROM TSMPE12 ";
			tsmpe12["MAT_NO"] = tsmpe02["MAT_NO"];
			tsmpe12["STACKING_NO"] = tsmpe02["STACKING_NO"];
			if (tsmpe12.Query("MAT_NO,STACKING_NO") == false)
			{
				sprintf(s.msg, "读取材料记录出错，材料号[%s],码单号[%s]", (const char*)tsmpe02["MAT_NO"].ToString(), (const char*)tsmpe02["STACKING_NO"].ToString());
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}
			tsmpe02.CopyFrom(tsmpe12);

			sqlstr = " UPDATE TSMPE01 SET CONFM_STATUS = '4' WHERE CONFM_PLAN_NO = '" + tsmpe01["CONFM_PLAN_NO"].ToString() + "' ";
			tsmpe01["CONFM_STATUS"] = "4";
			tsmpe01["CONFM_PLAN_NO"] = tsmpe02["CONFM_PLAN_NO"];
			if (tsmpe01.Update("CONFM_STATUS", "CONFM_PLAN_NO") == 0)
			{
				sprintf(s.msg, "更新准发计划表记录出错，计划号[%s]", (const char*)tsmpe01["CONFM_PLAN_NO"].ToString());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			sqlstr = " UPDATE TSMPE00 SET CONFM_STATUS = '4' WHERE READY_BILL_NO = '" + tsmpe00["READY_BILL_NO"].ToString() + "' ";
			tsmpe00["CONFM_STATUS"] = "4";
			tsmpe00["READY_BILL_NO"] = tsmpe02["READY_BILL_NO"];
			if (tsmpe00.Update("CONFM_STATUS", "READY_BILL_NO") == 0)
			{
				sprintf(s.msg, "更新准发单据表记录出错，单据号[%s]", (const char*)tsmpe00["READY_BILL_NO"].ToString());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			if (op_flag.Trim() == "2")
			{
				sqlstr = " UPDATE TSMPE10 SET DELIVY_PLAN_STATUS = '4' ,DELIVY_NUM = DELIVY_NUM -1 ,DELIVY_WT = DELIVY_WT - @tsmpe02.MAT_WT "
					" WHERE BILL_OF_LADING_NO = @tsmpe02.BILL_OF_LADING_NO ";
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Set("tsmpe02.BILL_OF_LADING_NO", tsmpe02["BILL_OF_LADING_NO"].ToString());
				execute_sql.Parameters.Set("tsmpe02.MAT_WT", tsmpe02["MAT_WT"].ToDecimal());
				if (execute_sql.ExecuteNonQuery() == 0)
				{
					CFormattable arguments[] = { tsmpe02["BILL_OF_LADING_NO"].ToString(), tsmpe02["MAT_WT"].ToDecimal() };
					CMessageFormat::Format(s.msg, "更新发货计划表不成功,提单号[{0}],材料重量[{1}]", arguments, 2);

					throw	CApplicationException(-1, s.msg, s.svc_name);
				}
				tsmpe02["CONFM_STATUS"] = "6";	//准发状态
			}
			else
			{
				tsmpe02["BILL_OF_LADING_NO"] = " ";	//提单号
				tsmpe02["CONFM_STATUS"] = "4";	//准发状态
			}
			
			tsmpe02["TICKET_NO"] = " ";
			tsmpe02["DELIVY_TIME"] = " ";	//出厂时刻
			//tsmpe02["BILL_OF_LADING_NO"] =	" ";	//提单号
			tsmpe02["STACKING_NO"] = " ";	//码单号
			//tsmpe02["CONFM_STATUS"]="4";	//准发状态
			tsmpe02["OUT_FACT_DATE"] = " ";	//出厂日期
			tsmpe02["DELIVY_SHIFT"] = " ";	//出厂班次
			tsmpe02["DELIVY_GROUP"] = " ";	//出厂班组
			tsmpe02["DELIVY_MAKER"] = " ";	//出厂责任者
			tsmpe02["VEHICLE_NO"] = " ";	//车船号
			tsmpe02["OUT_MARK"] = " ";	//出库标志
			tsmpe02["RED_FLAG"] = "0";	//红冲标记

			tsmpe02.TrimOrBlank();
			sqlstr = " INSERT INTO TSMPE02 ";
			if (tsmpe02.Insert() == false)
			{
				sprintf(s.msg, "新增材料记录出错，材料号[%s]", (const char*)tsmpe02["MAT_NO"].ToString());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			//更新准发计划表,准发单据表计划重量 材料总重量
			ret = f_sm00_count(tsmpe02["CONFM_PLAN_NO"].ToString(), c_userid, conn);
			if (ret < 0)
			{
				strcpy(s.msg, "f_sm00_count函数调用出错");
				strcpy(s.sysmsg, "f_sm00_count函数调用出错!");
				throw CApplicationException(-1, s.msg, s.svc_name);
				//EDLog(1, 1, s.msg);
			}

			/*增加写入库队列 */
			bcls_stock_que.Tables[blk_name_wm].Rows.Add();
			int ii = bcls_stock_que.Tables[blk_name_wm].Rows.get_Count() - 1;
			bcls_stock_que.Tables[blk_name_wm].Rows[ii]["MAT_NO"] = tsmpe02["MAT_NO"];
			bcls_stock_que.Tables[blk_name_wm].Rows[ii]["TO_STOCK_NO"] = tsmpe02["STOCK_NO"];
			bcls_stock_que.Tables[blk_name_wm].Rows[ii]["STOCK_NO"] = tsmpe02["STOCK_NO"];
			bcls_stock_que.Tables[blk_name_wm].Rows[ii]["UNIT_CODE"] = " ";
			bcls_stock_que.Tables[blk_name_wm].Rows[ii]["OPER_FLAG"] = "1";
			bcls_stock_que.Tables[blk_name_wm].Rows[ii]["STOCK_OPER_ORDER"] = "1N";	// 

			bcls_rec->Tables[record_name].Rows[0]["mat_no"] = tsmpe12["MAT_NO"];
			bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "6";
			bcls_rec->Tables[record_name].Rows[0]["userid"] = c_userid;

			ret = 0;
			ret = f_sm00_record(bcls_rec, bcls_ret, conn);
			if (ret < 0)
			{
				Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			mat_no.push_back(tsmpe02["MAT_NO"].ToString());

			sqlstr = "DELETE FROM TSMPE12 ";
			if (tsmpe12.Delete("MAT_NO,STACKING_NO") == 0)
			{
				sprintf(s.msg, "删除码单材料记录出错，材料号[%s],计划号[%s]", (const char*)tsmpe02["MAT_NO"].ToString(), (const char*)tsmpe02["CONFM_PLAN_NO"].ToString());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			if (tsmpe12.QueryCount("STACKING_NO") == 0)
			{
				sqlstr = " DELETE FROM  TSMPE11 "
					" WHERE STACKING_NO = '" + tsmpe11["STACKING_NO"].ToString() + "' ";
				execute_sql.SetCommandText(sqlstr);
				execute_sql.ExecuteNonQuery();
			}
			else
			{
				sqlstr = " UPDATE tsmpe11 SET STACKING_NUM = STACKING_NUM -1 ,"
					" STACKING_WT = STACKING_WT -"+ tsmpe12["MAT_WT"].ToString() + " ,"
					" STACKING_GROSS_WT = STACKING_GROSS_WT -"+ tsmpe12["MAT_WT"].ToString() +
					" WHERE STACKING_NO = '"+ tsmpe11["STACKING_NO"].ToString() +"' ";
				execute_sql.SetCommandText(sqlstr);
				execute_sql.ExecuteNonQuery();
			}
		}
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
	}

	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.msg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
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

	Log::Debug("", __FUNCTION__, "s.sysmsg=[{0}]", s.sysmsg);

	//返回-1时事务将回滚，返回为0是事务将提交
	return doFlag;

}
