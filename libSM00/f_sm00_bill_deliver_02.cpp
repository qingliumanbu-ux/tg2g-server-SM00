/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_sm00_bill_deliver_02
*  程序描述			: 材料处理函数
*  备注说明			:
*  修改历史			:
*  		wuxin 2011-12-26			(ADD)程序建立
*			... ...
* **************************************************************************** */
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除
//#include "tsmpe11.h"
//#include "tsmpe02.h"

//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明
int f_sm00_mm99(CString pack_num , 
BM2_FUNCTION_IMPORT
 int para_type , int event_id ,CString msg,CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int  f_sm00_mat_check(EIClass * bcls_rec , EIClass * bcls_ret , CDbConnection * conn);

BM2_FUNCTION_EXPORT
 int f_sm00_bill_deliver_02(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn) 
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/*程序用变量*/
	int doFlag = 0,ret = 0;

	/* 业务变量 */
	CString	datetime("");
	CString	blkname = "SM00_MAT_CHECK";

	 /* ***** 程序变量 ***** */
	CString c_user=s.userid,c_stock_no=" ",c_stacking_no=" ",c_rowid=" ",c_mat_no= "";
	CString	c_mat_kind = "";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2("");                    
	
	/* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn);

/* ***** 应用程序开始处理 ***** */
	try
	{
		if	(bcls_rec->Tables.IndexOf(blkname) < 0)	
		{
			bcls_rec->Tables.Add(blkname); 
			bcls_rec->Tables[blkname].Columns.Add(DT_STRING , "MAT_NO");
			bcls_rec->Tables[blkname].Columns.Add(DT_STRING , "MAT_KIND");
			bcls_rec->Tables[blkname].Rows.Add();
		}

		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss"); 
		/* ***** 获取传入的库区号  ***** */ 
		c_stacking_no = bcls_rec->Tables[0].Rows[0]["stacking_no"].ToString().TrimOrBlank();

		/* ***** 查找没有材料的准发计划  ***** */ 
		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
		/* ***** tsmpea7 该表不需要建立索引，一个主键已经足够 ***** */
		sqlstr1 = CString(    
				   " SELECT * FROM tsmpe11 where stacking_no = @stacking_no "
										);
										
		sqlstr2 = CString(    
				   " SELECT mat_no , MAT_KIND FROM tsmpe02 where stacking_no = @stacking_no "  
										);	 

		break; 
		}  

       	sqlstr = sqlstr2;
		execute_sql.Parameters.Set( "stacking_no", c_stacking_no); 
	    execute_sql.SetCommandText( sqlstr ); 
	    execute_sql.ExecuteReader(); 

		while(execute_sql.Read())
		{   
			c_mat_no          = execute_sql.GetString(1);
			c_mat_kind        = execute_sql.GetString(2);

			// 调用材料检查函数看材料是否封锁 2014-07-08
			bcls_rec->Tables[blkname].Rows[0]["MAT_NO"]		= c_mat_no ;
			bcls_rec->Tables[blkname].Rows[0]["MAT_KIND"]	= c_mat_kind ;
			ret = 0;
			ret = f_sm00_mat_check(bcls_rec,bcls_ret, conn);
			if (ret < 0)
			{
				Log::Debug("" , __FUNCTION__ , "f_sm00_mat_check函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			} 

		    if(f_sm00_mm99(c_mat_no , 3, 3, s.msg,conn) != 0)
			{
				doFlag = -1; 
				throw CApplicationException(-1, s.msg, s.svc_name);
		    } 
		}
	 

	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { _S("TSMPE02"), ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000017")/*读取数据失败,表[{0}],sqlcode=[{1}]。请联系系统维护人员。*/, arguments, 2);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		//EDLog(1,1, "error=[%s]", (const char*)str );

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
