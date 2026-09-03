/* **************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_sm00_ready_incept
*  程序描述			: 准发接收（不编计划，产出直接确认
*  备注说明			:
*  修改历史			:
*  013801		 2015-8-7			(ADD)程序建立
*			... ...
* **************************************************************************** */
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除
#include "tsmpe02.h"
#include "tsmpe00.h"
#include "tsmpe01.h"
#ifdef _LINE_BW
#include "tmmbw01.h"
#endif

#ifdef _LINE_HP
#include "tmmhp01.h"
#endif

//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明

BM2_FUNCTION_IMPORT
int f_sm00_record(EIClass *bcls_rec , EIClass *bcls_ret , CDbConnection * conn);


BM2_FUNCTION_EXPORT
int f_sm00_ready_incept(EIClass *bcls_rec , EIClass *bcls_ret , CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/*程序用变量*/
	int doFlag = 0 , ret = 0;
	int count_sum = 0;
	int i_count = 0;

	/* 业务变量 */
	CString	datetime("");
	CString	record_name = "sm00_record";
	CString blk_name("READY_INCEPT");
	CString c_ready_bill_no;
	CString c_confm_plan_no;

	/* ***** 程序变量 ***** */
	CString c_user = s.userid;

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	CTSMPE02 tsmpe02(conn);
	CTSMPE00 tsmpe00(conn);
	CTSMPE01 tsmpe01(conn);
#ifdef _LINE_BW
	CTMMBW01 tmmbw01(conn);
#endif
#ifdef _LINE_HP
	CTMMHP01 tmmhp01(conn);
#endif

	/* ***** 应用程序开始处理 ***** */
	try
	{
		if ( bcls_rec->Tables.IndexOf(record_name) < 0 )
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING , "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING , "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING , "userid");
//			bcls_rec->Tables[record_name].Rows.Add();
		}

		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		int	rows = bcls_rec->Tables[blk_name].Rows.get_Count();
		for ( int i = 0; i < rows; i++ )
		{
			// 读取传人的参数
			tsmpe02.MAT_KIND = bcls_rec->Tables[blk_name].Rows[i]["MAT_KIND"];	// 按字段名称读取
			tsmpe02.MAT_NO = bcls_rec->Tables[blk_name].Rows[i]["MAT_NO"];

			Log::Trace("" , __FUNCTION__ , "材料号[{0}],物料种类[{1}]" , tsmpe02.MAT_NO , tsmpe02.MAT_KIND);

			// 根据材料号到材料表上读取记录，判是否有记录，有报错或跳过
			if ( tsmpe02.QueryCount("MAT_NO")>0 )
			{
				continue;
			}
#ifdef _LINE_BW
			// 按物料种类到物料表上读取材料信息
			if ( tsmpe02.MAT_KIND == "BW" )
			{
				tmmbw01.MAT_NO = tsmpe02.MAT_NO;
				if ( !tmmbw01.Query("MAT_NO"))
				{
					CFormattable arguments[] = { tmmbw01.MAT_NO };
					CMessageFormat::Format(s.msg , "读取物料表出错材料号[{0}]" , arguments , 1);
				}

				// 按合同号到TSMPE02表上读取准发计划号、准发单据号
				// 有记录时更新准发计划、准发单据上的重量、件数
				// 无记录时新增准发计划、准发单据记录
				sqlstr = "SELECT MAX(CONFM_PLAN_NO) , MAX(READY_BILL_NO) FROM TSMPE02 "
					" WHERE ORDER_NO = @order_no ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("order_no" , tmmbw01.ORDER_NO);	// SQL语句中的变量赋值
				cmd_inq.ExecuteReader();
				if ( cmd_inq.Read() )
				{
					c_confm_plan_no = cmd_inq.GetString(1);
					c_ready_bill_no = cmd_inq.GetString(2);
				}
				else
				{
					c_ready_bill_no = EPGetNextSeq("SM_READY_BILL_NO" , conn);
					c_confm_plan_no = c_ready_bill_no;
				}
				tsmpe02.READY_BILL_NO = c_ready_bill_no;
				tsmpe02.CONFM_PLAN_NO = c_confm_plan_no;

				// 根据准发计划号到准发计划表读取记录无新增，有修改计划量、件
				tsmpe01.CONFM_PLAN_NO = tsmpe02.CONFM_PLAN_NO;
				if ( tsmpe01.Query( ))
				{
					tsmpe01.PLAN_WT = tsmpe01.PLAN_WT + tmmbw01.MAT_ACT_WT;
					tsmpe01.PLAN_NUM = tsmpe01.PLAN_NUM + 1;
					tsmpe01.PLAN_TUBE = tsmpe01.PLAN_TUBE + tmmbw01.MAT_TUBE;
					tsmpe01.REC_REVISE_TIME = datetime;
					tsmpe01.REC_REVISOR = c_user;
					tsmpe01.TOTAL_MAT_NUM = tsmpe01.PLAN_NUM + 1;	// 合计材料个数
					tsmpe01.TOTAL_MAT_WT = tsmpe01.TOTAL_MAT_WT + tmmbw01.MAT_ACT_WT;	// 合计材料重量
					tsmpe01.TOTAL_TUBE = tsmpe01.PLAN_TUBE + tmmbw01.MAT_TUBE;	// 总根数
					tsmpe01.Update("PLAN_WT , PLAN_NUM , PLAN_TUBE , REC_REVISE_TIME , REC_REVISOR , TOTAL_MAT_NUM , TOTAL_MAT_WT , TOTAL_TUBE ");
				}
				else
				{
					tsmpe01.CopyFrom(tmmbw01);
					tsmpe01.CONFM_PLAN_NO = c_confm_plan_no;
					tsmpe01.PLAN_WT = tmmbw01.MAT_ACT_WT;
					tsmpe01.PLAN_NUM = 1;
					tsmpe01.PLAN_TUBE = tmmbw01.MAT_TUBE;
					tsmpe01.REC_REVISE_TIME = datetime;
					tsmpe01.REC_REVISOR = c_user;
					tsmpe01.REC_CREATE_TIME = datetime;
					tsmpe01.REC_CREATOR = c_user;
					tsmpe01.CONFM_STATUS = "4";	// 准发计划状态
					tsmpe01.PLAN_MAKER = c_user;	// 计划责任者
					tsmpe01.PRG_SEND_TIME = datetime;	// 计划下达时间
					tsmpe01.RESER_START_TIME = datetime;	// 预定开始执行时刻
					tsmpe01.RESER_END_TIME = datetime;	// 预定执行结束时刻
					tsmpe01.TOTAL_MAT_NUM = 1;	// 合计材料个数
					tsmpe01.TOTAL_MAT_WT = tmmbw01.MAT_ACT_WT;	// 合计材料重量
					tsmpe01.TOTAL_TUBE = tmmbw01.MAT_TUBE;	// 总根数
					tsmpe01.Insert();
				}

				// 根据准发单据号到准发单据表上读取记录无新增，有更新计划量、件
				tsmpe00.READY_BILL_NO = c_ready_bill_no;
				if ( tsmpe00.Query( ))
				{
					tsmpe00.PLAN_WT = tsmpe00.PLAN_WT + tmmbw01.MAT_ACT_WT;
					tsmpe00.PLAN_NUM = tsmpe00.PLAN_NUM + 1;
					tsmpe00.PLAN_TUBE = tsmpe00.PLAN_TUBE + tmmbw01.MAT_TUBE;
					tsmpe00.REC_REVISE_TIME = datetime;
					tsmpe00.REC_REVISOR = c_user;
					tsmpe00.TOTAL_MAT_NUM = tsmpe00.PLAN_NUM + 1;	// 合计材料个数
					tsmpe00.TOTAL_MAT_WT = tsmpe00.TOTAL_MAT_WT + tmmbw01.MAT_ACT_WT;	// 合计材料重量
					tsmpe00.TOTAL_TUBE = tsmpe00.PLAN_TUBE + tmmbw01.MAT_TUBE;	// 总根数
					tsmpe00.Update("PLAN_WT , PLAN_NUM , PLAN_TUBE , REC_REVISE_TIME , REC_REVISOR , TOTAL_MAT_NUM , TOTAL_MAT_WT , TOTAL_TUBE");
				}
				else
				{
					tsmpe00.CopyFrom(tmmbw01);
					tsmpe00.CONFM_PLAN_NO = c_confm_plan_no;
					tsmpe00.READY_BILL_NO = c_ready_bill_no;
					tsmpe00.PLAN_WT = tmmbw01.MAT_ACT_WT;
					tsmpe00.PLAN_NUM = 1;
					tsmpe00.PLAN_TUBE = tmmbw01.MAT_TUBE;
					tsmpe00.REC_REVISE_TIME = datetime;
					tsmpe00.REC_REVISOR = c_user;
					tsmpe00.REC_CREATE_TIME = datetime;
					tsmpe00.REC_CREATOR = c_user;
					tsmpe00.CONFM_STATUS = "4";		//准发计划状态
					tsmpe00.TOTAL_MAT_NUM = 1;	// 合计材料个数
					tsmpe00.TOTAL_MAT_WT = tmmbw01.MAT_ACT_WT;	// 合计材料重量
					tsmpe00.TOTAL_TUBE = tmmbw01.MAT_TUBE;	// 总根数
					tsmpe00.Insert();
				}

				// 新增准发材料记录
				tsmpe02.CopyFrom(tmmbw01);
				tsmpe02.CONFM_PLAN_NO = c_confm_plan_no;
				tsmpe02.READY_BILL_NO = c_ready_bill_no;
				tsmpe02.REC_REVISE_TIME = datetime;
				tsmpe02.REC_REVISOR = c_user;
				tsmpe02.REC_CREATE_TIME = datetime;
				tsmpe02.REC_CREATOR = c_user;
				tsmpe02.CONFM_STATUS = "4";		//准发计划状态
				tsmpe02.MAT_WT = tmmbw01.MAT_ACT_WT;	// 材料重量
				tsmpe02.WT_MODE = tmmbw01.MEASURE_WT_FLAG;	// 计重方式
				tsmpe02.DELIVY_QTY_FLAG = "1";	// 按量发货标记
				tsmpe02.Insert();

				// 压发货履历数据
				bcls_rec->Tables[record_name].Rows.Add();
				bcls_rec->Tables[record_name].Rows[i_count]["mat_no"] = tsmpe02.MAT_NO;
				bcls_rec->Tables[record_name].Rows[i_count]["event_mark"] = "4";
				bcls_rec->Tables[record_name].Rows[i_count]["userid"] = c_user;
				i_count++;
			}
#endif
#ifdef _LINE_HP
			if ( tsmpe02.MAT_KIND == "HP" )
			{
				tmmhp01.MAT_NO = tsmpe02.MAT_NO;
				if ( !tmmhp01.Query("MAT_NO") )
				{
					CFormattable arguments[] = { tmmhp01.MAT_NO };
					CMessageFormat::Format(s.msg , "读取物料表出错材料号[{0}]" , arguments , 1);
				}

				// 按合同号到TSMPE02表上读取准发计划号、准发单据号
				// 有记录时更新准发计划、准发单据上的重量、件数
				// 无记录时新增准发计划、准发单据记录
				sqlstr = "SELECT MAX(CONFM_PLAN_NO) , MAX(READY_BILL_NO) FROM TSMPE02 "
					" WHERE ORDER_NO = @order_no ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("order_no" , tmmhp01.ORDER_NO);	// SQL语句中的变量赋值
				cmd_inq.ExecuteReader();
				if ( cmd_inq.Read() )
				{
					c_confm_plan_no = cmd_inq.GetString(1);
					c_ready_bill_no = cmd_inq.GetString(2);
				}
				else
				{
					c_ready_bill_no = EPGetNextSeq("SM_READY_BILL_NO" , conn);
					c_confm_plan_no = c_ready_bill_no;
				}
				tsmpe02.READY_BILL_NO = c_ready_bill_no;
				tsmpe02.CONFM_PLAN_NO = c_confm_plan_no;

				// 根据准发计划号到准发计划表读取记录无新增，有修改计划量、件
				tsmpe01.CONFM_PLAN_NO = tsmpe02.CONFM_PLAN_NO;
				if ( tsmpe01.Query() )
				{
					tsmpe01.PLAN_WT = tsmpe01.PLAN_WT + tmmhp01.MAT_ACT_WT;
					tsmpe01.PLAN_NUM = tsmpe01.PLAN_NUM + 1;
					tsmpe01.PLAN_TUBE = tsmpe01.PLAN_TUBE + tmmhp01.MAT_NUM;
					tsmpe01.REC_REVISE_TIME = datetime;
					tsmpe01.REC_REVISOR = c_user;
					tsmpe01.TOTAL_MAT_NUM = tsmpe01.PLAN_NUM + 1;	// 合计材料个数
					tsmpe01.TOTAL_MAT_WT = tsmpe01.TOTAL_MAT_WT + tmmhp01.MAT_ACT_WT;	// 合计材料重量
					tsmpe01.TOTAL_TUBE = tsmpe01.PLAN_TUBE + tmmhp01.MAT_NUM;	// 总根数
					tsmpe01.Update("PLAN_WT , PLAN_NUM , PLAN_TUBE , REC_REVISE_TIME , REC_REVISOR , TOTAL_MAT_NUM , TOTAL_MAT_WT , TOTAL_TUBE");
				}
				else
				{
					tsmpe01.CopyFrom(tmmhp01);
					tsmpe01.CONFM_PLAN_NO = c_confm_plan_no;
					tsmpe01.PLAN_WT = tmmhp01.MAT_ACT_WT;
					tsmpe01.PLAN_NUM = 1;
					tsmpe01.PLAN_TUBE = tmmhp01.MAT_NUM;
					tsmpe01.REC_REVISE_TIME = datetime;
					tsmpe01.REC_REVISOR = c_user;
					tsmpe01.REC_CREATE_TIME = datetime;
					tsmpe01.REC_CREATOR = c_user;
					tsmpe01.CONFM_STATUS = "4";	// 准发计划状态
					tsmpe01.PLAN_MAKER = c_user;	// 计划责任者
					tsmpe01.PRG_SEND_TIME = datetime;	// 计划下达时间
					tsmpe01.RESER_START_TIME = datetime;	// 预定开始执行时刻
					tsmpe01.RESER_END_TIME = datetime;	// 预定执行结束时刻
					tsmpe01.TOTAL_MAT_NUM = 1;	// 合计材料个数
					tsmpe01.TOTAL_MAT_WT = tmmhp01.MAT_ACT_WT;	// 合计材料重量
					tsmpe01.TOTAL_TUBE = tmmhp01.MAT_NUM;	// 总根数
					tsmpe01.Insert();
				}

				// 根据准发单据号到准发单据表上读取记录无新增，有更新计划量、件
				tsmpe00.READY_BILL_NO = c_ready_bill_no;
				if ( tsmpe00.Query() )
				{
					tsmpe00.PLAN_WT = tsmpe00.PLAN_WT + tmmhp01.MAT_ACT_WT;
					tsmpe00.PLAN_NUM = tsmpe00.PLAN_NUM + 1;
					tsmpe00.PLAN_TUBE = tsmpe00.PLAN_TUBE + tmmhp01.MAT_NUM;
					tsmpe00.REC_REVISE_TIME = datetime;
					tsmpe00.REC_REVISOR = c_user;
					tsmpe00.TOTAL_MAT_NUM = tsmpe00.PLAN_NUM + 1;	// 合计材料个数
					tsmpe00.TOTAL_MAT_WT = tsmpe00.TOTAL_MAT_WT + tmmhp01.MAT_ACT_WT;	// 合计材料重量
					tsmpe00.TOTAL_TUBE = tsmpe00.PLAN_TUBE + tmmhp01.MAT_NUM;	// 总根数
					tsmpe00.Update("PLAN_WT , PLAN_NUM , PLAN_TUBE , REC_REVISE_TIME , REC_REVISOR , TOTAL_MAT_NUM , TOTAL_MAT_WT , TOTAL_TUBE");
				}
				else
				{
					tsmpe00.CopyFrom(tmmhp01);
					tsmpe00.CONFM_PLAN_NO = c_confm_plan_no;
					tsmpe00.READY_BILL_NO = c_ready_bill_no;
					tsmpe00.PLAN_WT = tmmhp01.MAT_ACT_WT;
					tsmpe00.PLAN_NUM = 1;
					tsmpe00.PLAN_TUBE = tmmhp01.MAT_NUM;
					tsmpe00.REC_REVISE_TIME = datetime;
					tsmpe00.REC_REVISOR = c_user;
					tsmpe00.REC_CREATE_TIME = datetime;
					tsmpe00.REC_CREATOR = c_user;
					tsmpe00.CONFM_STATUS = "4";		//准发计划状态
					tsmpe00.TOTAL_MAT_NUM = 1;	// 合计材料个数
					tsmpe00.TOTAL_MAT_WT = tmmhp01.MAT_ACT_WT;	// 合计材料重量
					tsmpe00.TOTAL_TUBE = tmmhp01.MAT_NUM;	// 总根数
					tsmpe00.Insert();
				}

				// 新增准发材料记录
				tsmpe02.CopyFrom(tmmhp01);
				tsmpe02.CONFM_PLAN_NO = c_confm_plan_no;
				tsmpe02.READY_BILL_NO = c_ready_bill_no;
				tsmpe02.REC_REVISE_TIME = datetime;
				tsmpe02.REC_REVISOR = c_user;
				tsmpe02.REC_CREATE_TIME = datetime;
				tsmpe02.REC_CREATOR = c_user;
				tsmpe02.CONFM_STATUS = "4";		//准发计划状态
				tsmpe02.MAT_WT = tmmhp01.MAT_ACT_WT;	// 材料重量
				tsmpe02.MAT_TUBE = tmmhp01.MAT_NUM;
				tsmpe02.WT_MODE = tmmhp01.MEASURE_WT_FLAG;	// 计重方式
				tsmpe02.DELIVY_QTY_FLAG = "1";	// 按量发货标记
				tsmpe02.Insert();

				// 压发货履历数据
				bcls_rec->Tables[record_name].Rows.Add();
				bcls_rec->Tables[record_name].Rows[i_count]["mat_no"] = tsmpe02.MAT_NO;
				bcls_rec->Tables[record_name].Rows[i_count]["event_mark"] = "4";
				bcls_rec->Tables[record_name].Rows[i_count]["userid"] = c_user;
				i_count++;
			}
#endif
			// 写发货履历记录
			if ( bcls_rec->Tables[record_name].Rows.get_Count() > 0 )
			{
				ret = 0;
				ret = f_sm00_record(bcls_rec, bcls_ret, conn);
				if ( ret < 0 )
				{
					Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
			}
		}
	}
	catch ( CDbException& ex )  //捕获数据库操作异常
	{
		CFormattable arguments[] = { _S("TSMPEA7") , ex.GetCode() };
		CMessageFormat::Format(s.msg , _RES("GCRSS0000017")/*读取数据失败,表[{0}],sqlcode=[{1}]。请联系系统维护人员。*/ , arguments , 2);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("" , __FUNCTION__ , "error=[{0}]" , str);

		strncpy(s.sysmsg , (const char*)str , 399);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch ( CApplicationException& ex )  //捕获应用错误
	{
		strncpy(s.msg , (const char*)ex.GetMsg() , 399);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch ( CException& ex )
	{
		strncpy(s.msg , (const char*)ex.GetMsg() , 399);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
