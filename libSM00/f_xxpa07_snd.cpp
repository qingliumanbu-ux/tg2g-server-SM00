/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2010
Author:   13801
Version:    3.0
Date:     2018-5-11
Description: 铁运车皮空车确认电文发送
**************************************************/
/* C/C++ 的标准头文件部分 */

#include "stdafx.h"		// 框架头，不可删除

    //车辆接收表
#include "epex.h"

//名称空间引用




//外部函数声明

BM2_FUNCTION_EXPORT
int f_xxpa07_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int i;
	int fetchRowCount = 0;

	/* 业务变量 */
	CString	datetime("");
	CString tc_no = "XXPA07";	// 电文号

	CString	oper_flag = "I";		// I:Insert;U:Update;D:Delete

	/* 实体类定义 */
	CModel tsm00b4("TSM00B4");

	CString		sqlstr("");              // 数据库SQL操作字符串
	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);

	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		/*获得传入参数*/
		int row = bcls_rec->Tables[0].Rows.get_Count();
		if ( row == 0 )
		{
			sprintf(s.msg, "传入的记录数为0");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		tsm00b4.MergeFrom(bcls_rec->Tables[0].Rows[0]);
		tsm00b4.Query("VEHICLE_NO");
		if (tsm00b4["DIS_SOURCE"].ToString().Trim() != "")
		{
			return 0;
		}
		tc_no = tsm00b4["REC_CREATOR"].ToString().SubstringNE(2, 2) + tsm00b4["REC_CREATOR"].ToString().SubstringNE(0, 2) + "07";
		Log::Debug("", "", "tc_no={0}", tc_no);


		Log::Debug("", "", "s.svcname=[{0}]",s.svc_name);
		CString svc_name =  s.svc_name;
		if ( svc_name == "sm0012_cancel" )
		{
			oper_flag = "D";
		}
		if ( svc_name == "sm0012_cfm" )
		{
			oper_flag = "U";
		}

		// 生成电文发送对象
		EPEX epex(&s);

		// 初始化电文格式
		if ( epex.Initialize(tc_no) < 0 )   //电文号
		{
			CFormattable arguments[] ={ epex.GetMsg() };// 定义参数列表的数组
			CMessageFormat::Format(s.msg, "初始化电文时失败！ 原因描述：[ {0}]", arguments, 1);//格式化字符串
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		fetchRowCount = 0;
		for ( i = 0; i < row; i++ )
		{
			/* 读取传人参数 */
			tsm00b4.MergeFrom(bcls_rec->Tables[0].Rows[i]);
			tsm00b4.Query("VEHICLE_NO");
			/* 显示读取的参数 */
			tsm00b4.Print();

			if ( fetchRowCount == 0 )
			{
				/* 数据压电文 */
				if (epex.SetValue("OPERATION_FLAG", fetchRowCount, oper_flag) < 0	// 操作标记
					|| epex.SetValue("RAILWAY_NO", fetchRowCount, tsm00b4["LANE_NO"].ToString()) < 0	// 股道
					|| epex.SetValue("AFFIRM_BY", fetchRowCount, s.userid) < 0	// 确认者
					|| epex.SetValue("AFFIRM_TIME", fetchRowCount, datetime) < 0	// 确认时刻
					|| epex.SetValue("RAILWAY_NUM", fetchRowCount, row) < 0	// 车皮数
					)
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, "发送电文时失败! 原因描述： [{0}]", arguments, 1);//格式化字符串
					throw CApplicationException(-1, s.msg, log.Location);
				}

			}

			if (epex.SetValue("SEQ_NO", fetchRowCount, tsm00b4["SEQ_NO"].ToDecimal()) < 0	// 顺位
				|| epex.SetValue("VEHICLE_CODE", fetchRowCount, tsm00b4["VEHICLE_TYPE"].ToString()) < 0	// 车辆类型
				|| epex.SetValue("VEHICLE_NO", fetchRowCount, tsm00b4["VEHICLE_NO"].ToString()) < 0	// 车皮号
				|| epex.SetValue("VEHICLE_ID", fetchRowCount, tsm00b4["VEHICLE_ID"].ToString()) < 0	// 车皮ID
				)
			{
				CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, "发送电文时失败! 原因描述： [{0}]", arguments, 1);//格式化字符串
				throw CApplicationException(-1, s.msg, log.Location);
			}


			if (tsm00b4["STATUS"].ToString() == "2" && tsm00b4["USE_MARK"].ToString() == "1")
			{
				if (epex.SetValue("USE_YN", fetchRowCount, "Y") < 0
					//|| epex.SetValue("REMARK", fetchRowCount, " ") < 0
					)
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, "发送电文时失败! 原因描述： [{0}]", arguments, 1);//格式化字符串
					throw CApplicationException(-1, s.msg, log.Location);
				}
			}
			else
			{
				if (epex.SetValue("USE_YN", fetchRowCount, "N") < 0
					//|| epex.SetValue("REMARK", fetchRowCount, tsm00b4["REASON_CODE"].ToString()) < 0
					)
				{
					CFormattable arguments[] = { epex.GetMsg() };// 定义参数列表的数组
					CMessageFormat::Format(s.msg, "发送电文时失败! 原因描述： [{0}]", arguments, 1);//格式化字符串
					throw CApplicationException(-1, s.msg, log.Location);
				}
			}

			fetchRowCount ++;
		}


		// 发送电文
		if ( epex.SendTele() < 0 )
		{
			{
				CFormattable arguments[] ={ epex.GetMsg() };// 定义参数列表的数组
				CMessageFormat::Format(s.msg, _RES("SM00S0000055")/*发送电文时失败! 原因描述： [{0}]*/, arguments, 1);//格式化字符串
			}
			//sprintf(s.msg,"发送电文时失败! 原因描述: %s", epex.GetMsg());//转换前
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		// 释放
		epex.Uninitialize();

	}
	catch ( CDbException& ex )  //捕获数据库操作异常
	{
		CFormattable arguments[] ={ ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);

		strncpy(s.sysmsg, (const char*)str, sizeof(s.msg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch ( CApplicationException& ex )  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch ( CException& ex )
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
