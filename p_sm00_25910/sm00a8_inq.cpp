/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: sm00a8_inq
*  程序描述			: 报表抬头信息查询
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
int f_sm00a8_inq(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);
void f_epep_sqlerror_handler(void);
/*<remark>=========================================================
/// <summary>
/// 报表抬头信息查询
/// <para>
/// 1.根据传入的库区号和报表代码，查询报表信息。
/// </para>
/// <para>数据库表：TSMPEA8(发货报表定义表)         </para>
/// <para>主调用函数：前台SM00A8画面F2(查询)调用。   </para>
/// </summary>
/// <param name="c_stock_no">库区号    </param>
/// <param name="c_report_code">报表代码               </param>
/// <returns>报表信息</returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm00a8_inq)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm00a8_inq(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	/* ***** 程序表结构引用 ***** */
	// EXEC SQL INCLUDE tsmpea8.h;
	/* ***** 宿主变量定义 ***** */
	// EXEC SQL BEGIN DECLARE SECTION;
	CString c_factory_div = " ";
	CString c_stock_no = " ";
	CString c_report_code = " ";
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

	CTracer log(__FUNCTION__);
	try
	{

		/* ***** 获取输入参数 ***** */
		// c_factory_div = bcls_rec->Tables[0].Rows[0]["factory_div"];
		c_stock_no = bcls_rec->Tables[0].Rows[0]["stock_no"];
		c_report_code = bcls_rec->Tables[0].Rows[0]["report_code"];
		/* ***** 打印输入参数 ***** */
		Log::Info("" , __FUNCTION__ , "输入参数 c_stock_no=[{0}]", c_stock_no);
		Log::Info("" , __FUNCTION__ , "输入参数 c_report_code=[{0}]", c_report_code);
		//c_factory_div=c_factory_div.TrimOrBlank();
		c_stock_no=c_stock_no.TrimOrBlank();
		c_report_code=c_report_code.TrimOrBlank();

		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:


			sqlstr = " SELECT t1.*  "
				" FROM tsmpea8 t1  "
				" WHERE t1.report_code LIKE TRIM(@c_report_code) || '%%' ";
			if (c_stock_no.Trim()!="")
			{
				sqlstr += " AND t1.stock_no = @c_stock_no ";
			}

			sqlstr += " ORDER BY t1.stock_no, t1.report_code ";
			break;
		}
		//..HYF 20130408 下面是什么语句。。。
		
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("c_report_code",c_report_code);
		if (c_stock_no.Trim()!="")
		{
			cmd_inq.Parameters.Set("c_stock_no",c_stock_no);
		}

		cmd_inq.ExecuteReader();
		while (cmd_inq.Read())
		{
			cmd_inq.Fetch(tsmpea8);
			fetchRowCount++;
			tsmpea8.MergeTo(bcls_ret->Tables[0],false);
		}
		// EXEC SQL CLOSE cur_smpea8_q;
		cmd_inq.Close();
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
