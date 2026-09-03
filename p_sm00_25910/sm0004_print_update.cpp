/************************************************
*	程序名称：	码单打印计数					*
*   操作表名：	tsmpe11                         *
*   中文表名：	码单表    		                *
*   任务说明：出厂计划生成                 		*
*	编制日期：	2015-11-19	                    *
*	编 制 人：	沈明琪				            *
**************************************************/
/*************************************************
Copyright:宝信MES
Author:黄薇
Date:2011-12-12
Description:
**************************************************/


//框架公用头文件，勿删
#include "stdafx.h"
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//程序用头文件
#include "tsmpe11.h"

BM2F_ENTERACE(sm0004_print_update)

int f_sm0004_print_update(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{

	CTracer log(__FUNCTION__);

	/* 程序用变量 */
	int	    doFlag	=	0;				// 调用本函数的返回值
	int		fetchRowCount = 0;
	int		i;
	int		ret   =  0;
	int		temp  =  0;
	/* 在SQL语句中使用的变量 */

	CDbCommand cmd_upd(conn);
	CString sqlstr;

	CTSMPE11 tsmpe11(conn);

	try
	{

		int rows = bcls_rec->Tables[0].Rows.get_Count();
		for (i = 0 ; i < rows; i++ )
		{
			//  获取输入条件
			tsmpe11.STACKING_NO = bcls_rec->Tables[0].Rows[i]["STACKING_NO"].ToString().Trim();
			Log::Info("", __FUNCTION__, "STACKING_NO=[{0}]", tsmpe11.STACKING_NO);

			switch(conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr=CString("UPDATE TSMPE11 "
					" SET STACKING_PRINTS =  STACKING_PRINTS + 1 "
					" WHERE STACKING_NO = @tsmpe11.STACKING_NO");
				cmd_upd.SetCommandText(sqlstr);
				cmd_upd.Parameters.Set("tsmpe11.STACKING_NO", tsmpe11.STACKING_NO);
			break;
			}
			cmd_upd.ExecuteNonQuery();
		}
		sprintf	(s.msg , "更新打印记录成功" );
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{

		CFormattable arguments[] = { _S("TSMPE11"), ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000017")/*读取数据失败,表[{0}],sqlcode=[{1}]。请联系系统维护人员。*/, arguments, 2);

		CString str = sqlstr + "\r\n" + ex.GetMsg();

		Log::Error("" , __FUNCTION__ , "error=[{0}]", str );

		strncpy(s.sysmsg, (const char*)str, 399);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                  //数据库异常时返回-1，事务将被回滚
	}
	catch(const CApplicationException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1;
	}

	catch(const CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1;
	}

	//返回-1时事务将回滚，返回为0是事务将提交
	return doFlag;

}
