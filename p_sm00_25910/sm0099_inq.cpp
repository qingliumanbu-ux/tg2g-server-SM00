/*************************************************
 *	Copyright: Baosight Software LTD.co Copyright (c) 2010
 *	Author:   13801
 *	Version:    3.0
 *	Date:     2013-09-12
 *	Description: 发货履历查询
 **************************************************/

#include "stdafx.h"		// 框架头，不可删除
	// 发货履历表

//名称空间引用




//外部函数声明
//int f_edsetcustominfo(EIClass * bcls_rec, EIClass * bcls_ret);

// Service 入口
BM2F_ENTERACE(sm0099_inq)

int f_sm0099_inq(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int ret = 0;
	int blkseq = 0;

	/* 业务变量 */
	CString	sqlstr("");              // 数据库SQL操作字符串
	CString	dateTime("");
	CString	blkname("");

	/* 实体类定义 */
	CModel tsmpea1("TSMPEA1");

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);

	try
	{
		dateTime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		/* ***** 获取输入参数 ***** */
		CString c_factory_div 		= bcls_rec->Tables[blkseq].Rows[0]["factory_div"];	// 厂别
		CString c_event_date_from 	= bcls_rec->Tables[blkseq].Rows[0]["event_date_from"];	// 开始日期
		CString c_event_date_to 	= bcls_rec->Tables[blkseq].Rows[0]["event_date_to"];	// 结束日期
		CString c_stock_no 			= bcls_rec->Tables[blkseq].Rows[0]["stock_no"];	// 仓库代码
		CString c_order_no 			= bcls_rec->Tables[blkseq].Rows[0]["order_no"];	// 合同号
		CString c_mat_no 			= bcls_rec->Tables[blkseq].Rows[0]["mat_no"];	// 材料号
		CString c_ready_bill_no 	= bcls_rec->Tables[blkseq].Rows[0]["ready_bill_no"];	// 准发单据号
		CString c_bill_of_lading_no = bcls_rec->Tables[blkseq].Rows[0]["bill_of_lading_no"];	// 提货单号
		CString c_stacking_no 		= bcls_rec->Tables[blkseq].Rows[0]["stacking_no"];	// 码单号
		CString c_prod_code 		= bcls_rec->Tables[blkseq].Rows[0]["prod_code"];	// 品名代码
		CString c_event_id 			= bcls_rec->Tables[blkseq].Rows[0]["event_id"];	// 事件

		if	(c_event_date_to.Trim() == "00010101")	c_event_date_to = "99999999";

		/* ***** 打印输入参数 ***** */
		Log::Info("" , __FUNCTION__ , "c_factory_div=[{0}]"			, c_factory_div );
		Log::Info("" , __FUNCTION__ , "c_event_date_from=[{0}]"		, c_event_date_from );
		Log::Info("" , __FUNCTION__ , "c_event_date_to=[{0}]"		, c_event_date_to );
		Log::Info("" , __FUNCTION__ , "c_stock_no=[{0}]"			, c_stock_no );
		Log::Info("" , __FUNCTION__ , "c_order_no=[{0}]"			, c_order_no );
		Log::Info("" , __FUNCTION__ , "c_mat_no=[{0}]"				, c_mat_no );
		Log::Info("" , __FUNCTION__ , "c_ready_bill_no=[{0}]"		, c_ready_bill_no );
		Log::Info("" , __FUNCTION__ , "c_bill_of_lading_no=[{0}]"	, c_bill_of_lading_no );
		Log::Info("" , __FUNCTION__ , "c_stacking_no=[{0}]"			, c_stacking_no );
		Log::Info("" , __FUNCTION__ , "c_prod_code=[{0}]"			, c_prod_code );
		Log::Info("" , __FUNCTION__ , "c_event_id=[{0}]"			, c_event_id );

		///* ***** 设置返回块 ***** */
		///* ***** 动态显示定义字段 ***** */
		//EIClass	in01;
		//in01.Tables[0].Columns.Add(DT_STRING,"function_id");	// 传入的功能名
		//in01.Tables[0].Rows.Add();   //新增空行
		//in01.Tables[0].Rows[0]["function_id"] = "SM0099_INQ";	// 在EDA2中定义
		//bcls_ret->blk_now = 1;										// 返回的块号
		//ret = f_edsetcustominfo( &in01, bcls_ret );			// 读取字段名
		//if	(ret != 0)
		//{
		//	sprintf	(s.sysmsg, "调用函数f_edsetcustominfo出错[%s]" , (const char *)s.msg);
		//	Log::Error("" , __FUNCTION__ , s.sysmsg);
		//	throw	CApplicationException(-1, s.msg, log.Location);
		//}

		/* 数据库操作 */
		CDbCommand cmd_inq(conn);

		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr = " SELECT * FROM tsmpea1 A "
				" WHERE rec_create_time BETWEEN @c_event_date_from AND @c_event_date_to "
				" and(a.EVENT_ID in(select code from tep0002 where code_class = 'SM03' and code_desc_5_content = '1') "
				"  or(select count(1) from tep0002 where code_class = 'SM03' and code_desc_5_content = '1') = 0) "
				" AND	stock_no			like @c_stock_no || '%' "
				" AND	order_no			LIKE @c_order_no || '%' "
				" AND	mat_no				LIKE @c_mat_no || '%' "
				" AND	ready_bill_no		LIKE @c_ready_bill_no || '%' "
				" AND	bill_of_lading_no	LIKE @c_bill_of_lading_no || '%' "
				" AND	stacking_no			LIKE @c_stacking_no || '%' "
				" AND	prod_code			LIKE @c_prod_code || '%' "
				" AND	EVENT_ID			LIKE @c_event_id || '%' "
				" ORDER BY rec_create_time DESC ";
			break;
		}

		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("c_event_date_from"		, c_event_date_from);	// SQL语句中的变量赋值
		cmd_inq.Parameters.Set("c_event_date_to"		, c_event_date_to);
		cmd_inq.Parameters.Set("c_stock_no"				, c_stock_no.Trim());
		cmd_inq.Parameters.Set("c_order_no"				, c_order_no.Trim());
		cmd_inq.Parameters.Set("c_mat_no"				, c_mat_no.Trim());
		cmd_inq.Parameters.Set("c_ready_bill_no"		, c_ready_bill_no.Trim());
		cmd_inq.Parameters.Set("c_bill_of_lading_no"	, c_bill_of_lading_no.Trim());
		cmd_inq.Parameters.Set("c_stacking_no"			, c_stacking_no.Trim());
		cmd_inq.Parameters.Set("c_prod_code"			, c_prod_code.Trim());
		cmd_inq.Parameters.Set("c_event_id"				, c_event_id.Trim());
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);
		////cmd_inq.ExecuteReader();

		/////* 循环从游标中读取记录，压回前台 */
		////while( cmd_inq.Read() )
		////{
		////	cmd_inq.Fetch(tsmpea1);//把数据都压在头文件里面
		////	// 将结果放入返回块
		////	tsmpea1.MergeTo(bcls_ret->Tables[0], true);   //true是以block的定义为准 ,false是以头文件结构覆盖block
		////}//while
		////cmd_inq.Close();	// 关闭游标

		sprintf	(s.msg , "共读取[%d]条记录" , bcls_ret->Tables[0].Rows.get_Count());	// 读取了多少条记录

	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = {  ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("" , __FUNCTION__ , "error=[{0}]", str );

		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch(CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1);
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
