/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: sm00a8_ins
*  程序描述			: 报表抬头信息新增
*  备注说明			:
*  修改历史			:
*  		2008-8-13 	BM2IDE			(ADD)程序建立
*			... ...
* **************************************************************************** */
//框架公用头文件，勿删
#include "stdafx.h"
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//程序用头文件
#include "tsmpea8.h"
int f_sm00a8_ins(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);
void f_epep_sqlerror_handler(void);

/*<remark>=========================================================
/// <summary>
/// 报表抬头信息新增
/// <para>
/// 1.根据输入的信息，把它存储到报表信息表里面。
/// 2.新增条件：主键不能为空，主键不能存在
/// </para>
/// <para>数据库表：TSMPEA8(发货报表定义表)         </para>
/// <para>主调用函数：前台SM00A8画面F3(报表新增)调用。   </para>
/// </summary>
/// <param> </param>
/// <returns> </returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm00a8_ins)
	/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm00a8_ins(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
CTracer log(__FUNCTION__);
	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int i = 0;
	/* ***** 程序表结构引用 ***** */
	// EXEC SQL INCLUDE tsmpea8.h;
	/* ***** 宿主变量定义 ***** */
	// EXEC SQL BEGIN DECLARE SECTION;
	CString c_factory_div = " ";
	CString c_userid = " ";
	CString c_datetime = " ";
	int i_count = 0;
	// EXEC SQL END DECLARE SECTION;
	/* ***** 创建电文处理对象 ***** */
	//EPEX epex(&s);
	/* ***** 设置出错处理 ***** */
	// EXEC SQL WHENEVER SQLERROR GOTO l_sqlerror;
	// EXEC SQL
		// ALTER SESSION SET NLS_DATE_FORMAT = 'YYYYMMDDhh24miss';
CTSMPEA8 tsmpea8(conn);
CDbCommand cmd_inq(conn);
CString sqlstr;

try
{

	/* ***** 获取输入参数 ***** */

	bcls_rec->GetSYS(&s);
	// c_factory_div = bcls_rec->Tables[0].Rows[0]["factory_div"];
	c_datetime=CDateTime::Now().ToString("yyyyMMddhhmmss");
	c_userid = s.userid;
	/* ***** 打印输入参数 ***** */
	Log::Info("" , __FUNCTION__ , "输入参数 c_factory_div=[{0}]", c_factory_div);
	Log::Info("" , __FUNCTION__ , "输入参数 c_userid=[{0}]", c_userid);
	//c_factory_div=c_factory_div.TrimOrBlank();
	/* ***** 设置返回块 ***** */
	/* ***** 检查输入参数合法性 ***** */
	/* ***** 程序处理 ***** */
	//循环获取输入参数
	for (i = 1; i <= bcls_rec->Tables[0].Rows.get_Count(); i++)
	{
		tsmpea8.MergeFrom(bcls_rec->Tables[0].Rows[i-1]);
		tsmpea8.TrimOrBlank();
		Log::Info("" , __FUNCTION__ , "输入参数 tsmpea8.STOCK_NO=[{0}]",tsmpea8.STOCK_NO);
		Log::Info("" , __FUNCTION__ , "输入参数 tsmpea8.REPORT_CODE=[{0}]",tsmpea8.REPORT_CODE);

		//新增记录
		tsmpea8.REC_CREATE_TIME = c_datetime;
		tsmpea8.REC_CREATOR = c_userid;
		tsmpea8.REC_REVISE_TIME = " ";
		tsmpea8.REC_REVISOR = " ";
		tsmpea8.REC_ERASE_TIME = " ";
		tsmpea8.REC_ERASOR = " ";
		tsmpea8.ARCHIVE_FLAG = " ";
		i_count = tsmpea8.QueryCount("STOCK_NO,REPORT_CODE");
		if (i_count > 0)
		{
			s.flag = -1;
			strcpy(s.msg,"主键已存在，请修改！");
			throw CApplicationException(-1,s.msg,s.svc_name);
		}
		if (!tsmpea8.Insert())
		{
			s.flag = -1;
			strcpy(s.msg,"新增失败！");
			throw CApplicationException(-1,s.msg,s.svc_name);
		}
		// EXEC SQL
			//INSERT INTO tsmpea8 VALUES (:tsmpea8);
	}
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch(CApplicationException& ex)  //捕获应用错误
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch(CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
	
}
