/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_sm00_md_no
*  程序描述			: 码单号生成函数
*  备注说明			:
*  修改历史			:
*  		wuxin 2011-12-26			(ADD)程序建立
*			... ...
* **************************************************************************** */
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除


//名称空间引用




//外部函数声明

BM2_FUNCTION_EXPORT
 int f_sm00_md_no(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn) 
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/*程序用变量*/
	int doFlag = 0,i,blkNum,fetchRowCount,row_count=0;

	/* 业务变量 */
	CString	datetime("");
	CString	blkname("stacking_no");	/* 块名 */

	/* ***** 程序变量 ***** */
	CString c_user=s.userid,c_stock_no=" ",c_stacking_no=" ";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3(""),sqlstr4(""),sqlstr5(""),sqlstr6(""),sqlstr7(""),sqlstr8("");                    

	/* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn); 


	/* ***** 应用程序开始处理 ***** */
	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		/* ***** 获取传入的库区号  ***** */ 
		c_stock_no = bcls_rec->Tables[0].Rows[0]["stock_no"].ToString().TrimOrBlank();
		Log::Debug("" , __FUNCTION__ , "stock_no=[{0}]" , c_stock_no );
		if	(c_stock_no.Trim() == "" )
		{
			Log::Debug("" , __FUNCTION__ , "仓库代码空");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		// 定义返回块的内容
		/*if	(!bcls_ret->Tables[0].Columns.Contains("stacking_no"))
		{
			bcls_ret->Tables[0].Columns.Add(DT_STRING,"stacking_no");
			bcls_ret->Tables[0].Rows.Add();
		}*/
		/* ***** 查找没有材料的准发计划  ***** */ 
		sqlstr = "SELECT lpad(to_char(MD_NO.nextval),7,'0') STACKING_NO FROM DUAL";
		execute_sql.SetCommandText(sqlstr);
		execute_sql.ExecuteQuery(bcls_ret->Tables[0]);
		bcls_ret->Tables[0].Rows[0]["STACKING_NO"] = c_stock_no + bcls_ret->Tables[0].Rows[0]["STACKING_NO"].ToString();
		execute_sql.Close();



	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { _S("TSMPEA7"), ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000017")/*读取数据失败,表[{0}],sqlcode=[{1}]。请联系系统维护人员。*/, arguments, 2);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("" , __FUNCTION__ , "error=[{0}]", str );

		strncpy(s.sysmsg, (const char*)str, 399);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch(CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch(CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
