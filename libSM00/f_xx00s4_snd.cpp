/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_sm_xx00s4_snd
*  程序描述			: 发货实绩电文发送
*  备注说明			:
*  修改历史			:
*  		2012-04-28 	BM2IDE			(ADD)程序建立
*			... ...
*	2015-9-2	13801	增加电文号读取配置信息
* **************************************************************************** */
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除 

	/* 发货码单表 */
	/* 码单材料表 */
#include "epex.h"

//名称空间引用




//外部函数声明

BM2_FUNCTION_EXPORT
int f_xx00s4_snd(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序用变量 */
	int	    doFlag = 0;				// 调用本函数的返回值
	int		fetchRowCount = 0;
	int		i = 0;
	int		ret = 0;

	/* 实体类定义 */
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");

	CDbCommand cmd_inq(conn);
	CString sqlstr;
	CString tc_no = "7000S4";

	try
	{
		/* ***** 创建电文处理对象 ***** */
		EPEX epex(&s);

		tc_no = bcls_rec->Tables["0"].Rows[0]["tc_no"].ToString().TrimOrBlank();	//电文号
		tc_no = tc_no.SubstringNE(0, 4) + "S4";

		int row = bcls_rec->Tables["0"].Rows.get_Count();
		for (i = 0; i < row; i++)
		{
			// 初始化电文格式
			if (epex.Initialize(tc_no) < 0)   //电文号
			{
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0001464")/*初始化电文时失败！ 原因描述：[ {0}]*/, arguments, 1);//格式化字符串
				}
				//sprintf(s.msg,"初始化电文时失败! 原因描述: %s", epex.GetMsg());//转换前
				throw	CApplicationException(-1, s.msg, log.Location);
			}

			/* ***** 获取单据数据块号  ***** */
			tsmpe11["STACKING_NO"] = bcls_rec->Tables["0"].Rows[i]["stacking_no"].ToString().TrimOrBlank();	//码单号

			/* ***** 打印输入参数 ***** */
			Log::Info("", __FUNCTION__, "码单号	stacking_no=[{0}]", tsmpe11["STACKING_NO"].ToString());

			//读取码单表信息
			switch (conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr = "SELECT	* \
					FROM	tsmpe11 \
					WHERE	stacking_no = @stacking_no";
				cmd_inq.Parameters.Set("stacking_no", tsmpe11["STACKING_NO"].ToString());
				break;
			}
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.ExecuteReader();
			while (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe11);
			}
			tsmpe11.TrimOrBlank();
			cmd_inq.Close();

			/* ***** 检查输入参数合法性 ***** */
			if (tsmpe11["STACKING_NO"].ToString() == " ")
			{
				sprintf(s.msg, "输入的码单号[%s]有误，请重新输入！", (const char*)tsmpe11["STACKING_NO"].ToString());
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			if (tsmpe11["BILL_OF_LADING_NO"].ToString() == " ")
			{
				sprintf(s.msg, "输入的提货单号[%s]有误，请重新输入！", (const char*)tsmpe11["BILL_OF_LADING_NO"].ToString());
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			if (tsmpe11["ORDER_NO"].ToString() == " ")
			{
				sprintf(s.msg, "输入的合同号[%s]有误，请重新输入！", (const char*)tsmpe11["ORDER_NO"].ToString());
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			if (0 >= tsmpe11["STACKING_WT"].ToDecimal())
			{
				sprintf(s.msg, "输入的码单重量[%f]有误，请重新输入！", tsmpe11["STACKING_WT"].ToDecimal().ToDouble());
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			if (0 >= tsmpe11["STACKING_NUM"].ToDecimal())
			{
				sprintf(s.msg, "输入的码单个数[%d]有误，请重新输入！", tsmpe11["STACKING_NUM"].ToDecimal().ToInt32());
				throw	CApplicationException(-1, s.msg, log.Location);
			}

			tsmpe11["STACKING_WT"] = tsmpe11["STACKING_DISCREP_WT"].ToDecimal() + tsmpe11["STACKING_WT"].ToDecimal();
			// 设置x7000s4_info的值
			if (epex.SetValue(0, tsmpe11) < 0)
			{
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000054")/*写入表头数据时出错! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
				}
				//sprintf(s.msg,"写入表头数据时出错! 原因描述: %s", epex.GetMsg());//转换前
				throw	CApplicationException(-1, s.msg, log.Location);
			}

			//读取码单材料表信息
			switch (conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr = "SELECT	* \
						FROM	tsmpe12 \
						WHERE	stacking_no = @stacking_no";
				break;
			}
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("stacking_no", tsmpe11["STACKING_NO"].ToString());
			cmd_inq.ExecuteReader();
			fetchRowCount = 0;
			Log::Trace("", __FUNCTION__, "tsmpe11[STACKING_NO] = [{0}]", tsmpe11["STACKING_NO"].ToString());
			Log::Trace("", __FUNCTION__, "sqlstr = [{0}]", sqlstr);
			while (cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe12);
				tsmpe12.TrimOrBlank();
				Log::Trace("", __FUNCTION__, "tsmpe12[MAT_NO] = [{0}]", tsmpe12["MAT_NO"].ToString());
				/************************
				*	码单下材料压入电文	*
				************************/
				if (epex.SetValue("MAT_NO", fetchRowCount, tsmpe12["MAT_NO"].ToString()) < 0)
				{
					{
						CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
						CMessageFormat::Format(s.msg, _RES("SM00S0000064")/*写入循环体数据时出错! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
					}
					throw	CApplicationException(-1, s.msg, log.Location);
				}
				if (epex.SetValue("MAT_WT", fetchRowCount, tsmpe12["MAT_WT"].ToDecimal() + tsmpe12["MAT_DISCREP_WT"].ToDecimal()) < 0)
				{
					{
						CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
						CMessageFormat::Format(s.msg, _RES("SM00S0000064")/*写入循环体数据时出错! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
					}
					throw	CApplicationException(-1, s.msg, log.Location);
				}
				fetchRowCount++;
			}
			cmd_inq.Close();

			// 发送电文
			if (epex.SendTele() < 0)
			{
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
				}
				//sprintf(s.msg,"发送电文时失败! 原因描述: %s", epex.GetMsg());//转换前
				throw	CApplicationException(-1, s.msg, log.Location);
			}
			// 释放
			epex.Uninitialize();
		}
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);

		strncpy(s.sysmsg, (const char*)str, sizeof(s.msg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
