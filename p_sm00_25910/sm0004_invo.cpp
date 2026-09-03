/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: sm0004_invo
*  程序描述			: 码单报表查询
*  备注说明			:
*  修改历史			:
*  		2008-11-10 	BM2IDE			(ADD)程序建立
*			... ...
* **************************************************************************** */
#include "stdafx.h"		// 框架头，不可删除




//名称空间引用




//外部函数声明
int f_xxxxxx(EIClass *bcls_rec,  EIClass *bcls_ret , CDbConnection * conn);
int f_edsetcustominfo(EIClass * bcls_rec, EIClass * bcls_ret);


// service入口
BM2F_ENTERACE(sm0004_invo)
	/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0004_invo(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int count = 0;

	/* 业务变量 */
	CString	sqlstr("");              // 数据库SQL操作字符串
	CString	dateTime("");
	CString	blkname("");
	CString	c_stacking_no("");

	/* 实体类定义 */
	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	try
	{
		dateTime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		/* 读取传入的参数 */
		c_stacking_no = bcls_rec->Tables[0].Rows[0]["stacking_no"];	// 按字段名称读取
		Log::Info("" , __FUNCTION__ , "输入参数 c_stacking_no[{0}]", c_stacking_no);
		bcls_ret->Tables.Add();

		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr = " SELECT * FROM tsmpe11 "
				" WHERE stacking_no = @c_stacking_no ";		// SQL语句定义
			break;
		}

		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("c_stacking_no", c_stacking_no);	// SQL语句中的变量赋值
		cmd_inq.ExecuteReader();

		/* 循环从游标中读取记录，压回前台 */
		while( cmd_inq.Read() )
		{
			int k = cmd_inq.Fetch(tsmpe11 , 1 );//把数据都压在头文件里面
			// 将结果放入返回块
			tsmpe11.MergeTo(bcls_ret->Tables[0], false);   //true是以block的定义为准 ,false是以头文件结构覆盖block
			count++;
		}//while
		cmd_inq.Close();	// 关闭游标
		if	(count == 0)
		{
			CFormattable arguments[] = { c_stacking_no , 1403 };
			CMessageFormat::Format (s.msg , _RES("SM00S0001336")/*读取码单表出错，码单号=[{0}],sqlcode=[{1}]*/ , arguments , 2);
			throw	CApplicationException(-1 , s.msg , log.Location);
		}
		
		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr = " SELECT * FROM tsmpe12 "
				" WHERE stacking_no = @c_stacking_no ";		// SQL语句定义
			break;
		}

		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("c_stacking_no", c_stacking_no);	// SQL语句中的变量赋值
		cmd_inq.ExecuteReader();
		/* 循环从游标中读取记录，压回前台 */
		count = 0;
		while( cmd_inq.Read() )
		{
			int k = cmd_inq.Fetch(tsmpe12 , 1 );//把数据都压在头文件里面
			// 将结果放入返回块
			tsmpe12.MergeTo(bcls_ret->Tables[1], false);   //true是以block的定义为准 ,false是以头文件结构覆盖block
			count++;
		}//while
		cmd_inq.Close();	// 关闭游标

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
