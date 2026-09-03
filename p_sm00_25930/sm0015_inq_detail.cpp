/************************************************
*	程序名称：产成品发货处理——装车材料查询	*
*	编制日期：2023-2-21   	                    *
*	编 制 人：13801					            *
*************************************************/

//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件

// service入口
BM2F_ENTERACE(sm0015_inq_detail)

//自定义的函数
int f_sm0015_inq_detail(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int	 doFlag = 0;				// 调用本函数的返回值
	int	 fetchRowCount = 0;		// 调用本函数的返回值
	int	 i = 0;
	int	 blkNum = 0;	// 块号
	int ret;

	CString ticket_no("");
	CString c_stacking_no("");
	CString sqlstr("");              // 数据库SQL操作字符串
	CString sqlstr1("");              // 数据库SQL操作字符串
	CModel tsmpe12("TSMPE12");
	//CTOM01 tom01(conn);

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);

	try
	{
		CTracer log(__FUNCTION__);
		fetchRowCount = bcls_rec->Tables[blkNum].Rows.get_Count();
		if (fetchRowCount == 0)
		{
			sprintf(s.msg, "没有传入参数");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		for (i = 0; i < fetchRowCount; i++)
		{
			tsmpe12["TICKET_NO"] = bcls_rec->Tables[0].Rows[i]["TICKET_NO"];
			if (i == 0)
			{
				ticket_no = "'" + tsmpe12["TICKET_NO"].ToString().Trim() + "'";
			}
			else
			{
				ticket_no = ticket_no + ",'" + tsmpe12["TICKET_NO"].ToString().Trim() + "'";
			}
		}


		sqlstr = "SELECT T.* FROM tsmpe12 T, tsmpe11 A  WHERE T.ticket_no IN ( ";
		sqlstr += ticket_no;
		sqlstr += " ) AND T.STACKING_NO = A.STACKING_NO ORDER BY T.ticket_no";
		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);		//整体压块传出



		//sqlstr = "SELECT T.*,A.STEEL_SERVICE_ID,A.COVER_SERVICE_ID,A.MAT_SERVICE_ID,B.ID_PURCHSE,B.COVER_FLAG AS ORDER_COVER_FLAG FROM tsmpe12 T LEFT JOIN TOM01 B ON T.ORDER_NO = B.ORDER_NO, tsmpe11 A  WHERE 1=1";
		//for (i = 0; i < fetchRowCount; i++)
		//{
		//	tsmpe12["TICKET_NO"] = bcls_rec->Tables[0].Rows[i]["TICKET_NO"];
		//	if (i == 0)
		//	{
		//		sqlstr += " AND (T.ticket_no IN ('" + tsmpe12["TICKET_NO"].ToString().Trim() + "') ";
		//	}
		//	else
		//	{
		//		sqlstr += " OR T.ticket_no IN ('" + tsmpe12["TICKET_NO"].ToString().Trim() + "') ";
		//	}
		//}
		//sqlstr += " ) AND T.STACKING_NO = A.STACKING_NO ORDER BY T.ticket_no";
		//cmd_inq.SetCommandText(sqlstr);
		//Log::Debug("", __FUNCTION__, "sqlstr={0}", sqlstr);
		//cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);		//整体压块传出

		CString table_name = "",table_name_h= "";
		if (!bcls_ret->Tables[0].Columns.Contains("MAT_WT_FT"))	// 磅差分摊
		{
			bcls_ret->Tables[0].Columns.Add(DT_DECIMAL, "MAT_WT_FT");
		}

		for (int i = 0; i < bcls_ret->Tables[0].Rows.get_Count(); i++)
		{
			tsmpe12.MergeFrom(bcls_ret->Tables[0].Rows[i]);
			bcls_ret->Tables[0].Rows[i]["MAT_WT_FT"] = bcls_ret->Tables[0].Rows[i]["MAT_WT"].ToDecimal() + bcls_ret->Tables[0].Rows[i]["MAT_DISCREP_WT"].ToDecimal();

			table_name = "TMM" + tsmpe12["MAT_KIND"].ToString() + "01";
			table_name_h = "HMM" + tsmpe12["MAT_KIND"].ToString() + "01";

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
		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
	}

	//返回-1时事务将回滚，返回为0是事务将提交
	return doFlag;

}
