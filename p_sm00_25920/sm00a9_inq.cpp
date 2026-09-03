/* 程序对应表名    : tsm00a9
程序对应表中文名: 按仓库授权表
生成日期        : 2006-8-25 16:33:09
生成人          : 沈明琪  */
//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件

int f_sm00a9_inq(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
/*<remark>=========================================================
/// <summary>
/// 发货用户授权表查询
/// <para>
/// 1.根据传入的库区号和操作者，查询用户授权表信息。
/// </para>
/// <para>数据库表：TSMPEA9(发货用户授权表)         </para>
/// <para>主调用函数：前台SM00A9画面F2(用户查询)调用。   </para>
/// </summary>
/// <param name="stock_no">库区号    </param>
/// <param name="user_id">操作者               </param>
/// <returns>发货用户授权表信息</returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm00a9_inq)
/* -EP_SYSTEM_HEAD_END */
int f_sm00a9_inq(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);
	/* 程序用变量 */
	int doFlag = 0;
	int fetchRowCount = 0;
	/* 在SQL语句中使用的变量 */
	// EXEC SQL BEGIN DECLARE SECTION;
	CString stock_no = " ";
	CString user_id = " ";
	CString	factory_div;
	// EXEC SQL END DECLARE SECTION;
	//	压入数据名称
	//bcls_ret->Tables[0].Columns.Add(DT_STRING,"stock_no");				/*库区号*/
	//bcls_ret->Tables[0].Columns.Add(DT_STRING,"user_id");				/*操作者*/
	/* 使用的表结构变量 */
	// EXEC SQL INCLUDE tsmpea9.h;
	//// EXEC SQL INCLUDE tesuserinfo.h;
	/* 设置出错处理 */
	// EXEC SQL WHENEVER SQLERROR GOTO l_sqlerror;
	// EXEC SQL
	// ALTER SESSION SET NLS_DATE_FORMAT = 'YYYYMMDDhh24miss';
	CModel tsmpea9("TSMPEA9");
	CDbCommand cmd_inq(conn);
	CString sqlstr;

	try
	{
		/* 获得输入参数 */
		stock_no = bcls_rec->Tables[0].Rows[0]["stock_no"];
		user_id = bcls_rec->Tables[0].Rows[0]["user_id"];
		factory_div = bcls_rec->Tables[0].Rows[0]["factory_div"];

		stock_no = stock_no.TrimOrBlank();
		user_id = user_id.TrimOrBlank();
		Log::Info("", __FUNCTION__, "stock_no = [{0}]", stock_no);
		Log::Info("", __FUNCTION__, "user_id = [{0}]", user_id);

		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:


			sqlstr = CString("SELECT DISTINCT a.* "
				"			 FROM tsmpea9 a , vsmpea9 b "
				"			 WHERE 1=1 AND A.STOCK_NO = B.STOCK_NO ");
			if (stock_no.Trim() != "")
			{
				sqlstr += " AND a.stock_no=@stock_no";
				cmd_inq.Parameters.Set("stock_no", stock_no);
			}
			if (user_id.Trim() != "")
			{
				sqlstr += " AND a.user_id=@user_id";
				cmd_inq.Parameters.Set("user_id", user_id);
			}
			if (factory_div.Trim() != "")
			{
				sqlstr += " AND b.factory_div = @factory_div";
				cmd_inq.Parameters.Set("factory_div", factory_div);
			}
			sqlstr += " ORDER BY A.stock_no ASC, A.user_id ASC";
			break;
		}
		Log::Trace("", "", "salqtr={0}", sqlstr);
		cmd_inq.SetCommandText(sqlstr);
		fetchRowCount = cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);
		//cmd_inq.ExecuteReader();
		//// EXEC SQL OPEN tsmpea9_q;
		////for (fetchRowCount = 0; ;) {
		////  // EXEC SQL FETCH tsmpea9_q
		////    INTO :tsmpea9;
		////  if (sqlca.sqlcode == 1403) break;
		////  fetchRowCount ++;
		////  bcls_ret->SetColVal(1, fetchRowCount, (T_INFO *)&tsmpea9_info);
		////}
		//while (cmd_inq.Read())
		//{
		//	cmd_inq.Fetch(tsmpea9);
		//	fetchRowCount++;
		//	tsmpea9.MergeTo(bcls_ret->Tables[0], false);
		//}
		CFormattable arguments[] = { fetchRowCount };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000004"), arguments, 1);
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
	{
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
