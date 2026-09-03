// 程序对应表名    : tsmpe12
// 程序对应表中文名:　码单记录表
// 生成日期        : 2007-12-10 15:35:26
// 生成人          : 201387
//程序用途	：查询材料记录表,被smsw64画面调用
//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件

//-EP_CODE_VERSION 1
//-EP_SYSTEM_HEAD_BEGIN
//-此节代码请勿更改
// service入口
BM2F_ENTERACE(sm0004_rpt_inq2)
//-EP_SYSTEM_HEAD_END
int f_sm0004_rpt_inq2(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);

	//程序用变量
	int doFlag = -1;
	int fetchRowCount = 0;//前台传入记录个数
	int fetchRowCount1 = 0;//后台传出记录个数
	//在SQL语句中使用的变量
	// EXEC SQL BEGIN DECLARE SECTION;
	CString	datetime = "";
	CString	date = "";
	CString	time = "";
	CDecimal lowlimit = 0;
	CDecimal higelimit = 0;
	CString	v_stacking_type;
	CDecimal D_MAT_NUM = 0;
	CString v_row_no = " ";
	// EXEC SQL END DECLARE SECTION;
	//使用的表结构变量
	// EXEC SQL INCLUDE tsmpe12.h;

	//设置出错处理
	CModel tsmpe12("TSMPE12");
	CDbCommand cmd_inq(conn);
	CString sqlstr;

	try
	{
		//  // EXEC SQL WHENEVER SQLERROR GOTO l_sqlerror;
		//gettime	(datetime);					// 取系统日期时间
		//EPCutStrZ(datetime , 1, 8 ,date );	// 取系统日期
		//EPCutStrZ(datetime , 9, 6 ,time );	// 取系统时间

		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");				// 取系统日期时间
		date = datetime.Substring(0, 8);
		time = datetime.Substring(8, 6);

		if (bcls_ret->Tables[0].Columns.IndexOf("ROW_NO") < 0)//序号
		{
			bcls_ret->Tables[0].Columns.Add(DT_STRING, "ROW_NO");
		}

		//循环读取记录
		fetchRowCount1 = 0;
		for (fetchRowCount = 1; fetchRowCount <= bcls_rec->Tables[0].Rows.get_Count(); fetchRowCount++)
		{
			tsmpe12["STACKING_NO"] = bcls_rec->Tables[0].Rows[fetchRowCount - 1]["stacking_no"];
			// v_stacking_type = bcls_rec->Tables[0].Rows[fetchRowCount-1]["stacking_type"];
			Log::Info("", __FUNCTION__, "码单号=[{0}]", tsmpe12["STACKING_NO"].ToString());

			switch (conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:

				sqlstr = "SELECT ROW_NUMBER() OVER() as ROW_NO,tsmpe12.* "
					"	FROM tsmpe12  "
					"    WHERE 1=1";
				if (tsmpe12["STACKING_NO"].ToString().Trim() != "")
				{
					sqlstr += " and STACKING_NO = @tsmpe12.STACKING_NO  ";
				}

				sqlstr += " ORDER BY mat_no";
				break;
			}
			Log::Trace("", __FUNCTION__, "sqlstr= [{0}]", sqlstr);
			Log::Trace("", __FUNCTION__, "STACKING_NO= [{0}]", tsmpe12["STACKING_NO"].ToString());
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("tsmpe12.STACKING_NO", tsmpe12["STACKING_NO"].ToString().Trim());
			//fetchRowCount1=cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);
			cmd_inq.ExecuteReader();
			while (cmd_inq.Read())
			{
				v_row_no = cmd_inq.GetString(1);
				int k = 2;
				cmd_inq.Fetch(tsmpe12, k);

				tsmpe12.MergeTo(bcls_ret->Tables[0], false);// 将结果放入返回块
				bcls_ret->Tables[0].Rows[fetchRowCount1]["ROW_NO"] = v_row_no;

				fetchRowCount1++;
			}
			cmd_inq.Close();

		}
		{
			CFormattable arguments[] = { fetchRowCount1 };
			CMessageFormat::Format(s.msg, _RES("GCRSS0000004")/*查询到[{0}]条记录。*/, arguments, 1);
		}
		doFlag = 0;


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
