// 程序对应表名    : tsmpe11
// 程序对应表中文名:　码单记录表
// 生成日期        : 2007-12-10 15:35:26
// 生成人          : 201387
//程序用途	：查询码单记录表
//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件

//-EP_CODE_VERSION 1
//-EP_SYSTEM_HEAD_BEGIN
//-此节代码请勿更改
// service入口
BM2F_ENTERACE(sm0004_rpt_inq)
//-EP_SYSTEM_HEAD_END
int f_sm0004_rpt_inq(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{

	CTracer log(__FUNCTION__);
	//程序用变量
	int doFlag = -1;
	int fetchRowCount = 0;
	CDecimal lowweight = 0;
	CDecimal highweight = 0;
	//在SQL语句中使用的变量
	// EXEC SQL BEGIN DECLARE SECTION;
	CString	datetime = "";
	CString	date = "";
	CString	time = "";
	CDecimal lowlimit = 0;
	CDecimal higelimit = 0;
	CString c_delivy_time_from;
	CString c_delivy_time_to;
	CString v_stacking_no_1 = "";
	CString v_rec_creator = "";//创建者
	int flag = 0;
	CString order_cust_cname = " ";
	CString v_out_stock_code = " ";
	CString	userid;
	// EXEC SQL END DECLARE SECTION;
	//使用的表结构变量
	// EXEC SQL INCLUDE tsmpe11.h;

	//设置出错处理
	CModel tsmpe11("TSMPE11");
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq1(conn);
	CDbCommand cmd_inq2(conn);
	CDbCommand cmd_inq3(conn);
	CString sqlstr;

	try
	{

		userid = s.userid;

		if (!bcls_ret->Tables[0].Columns.Contains("order_cust_cname"))
		{
			bcls_ret->Tables[0].Columns.Add(DT_STRING, "order_cust_cname");			// 订货用户名
		}

		if (!bcls_ret->Tables[0].Columns.Contains("STACKING_NO_1"))
		{
			bcls_ret->Tables[0].Columns.Add(DT_STRING, "STACKING_NO_1");			// 码单一维码图片
		}

		//获得输入参数
		tsmpe11["STACKING_NO"] = bcls_rec->Tables[0].Rows[0]["stacking_no"].ToString().TrimOrBlank();

		Log::Info("", __FUNCTION__, "stacking_no=[{0}]", tsmpe11["STACKING_NO"].ToString());
		//未打印
		try
		{
			switch (conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:


				sqlstr = " SELECT tsmpe11.* ,tsmpe11.STACKING_NO as STACKING_NO_1"
					" FROM tsmpe11  "
					" WHERE STACKING_NO = @tsmpe11.STACKING_NO "
					;

				sqlstr += " ORDER BY DELIVY_TIME DESC";

				break;
			}

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("tsmpe11.STACKING_NO", tsmpe11["STACKING_NO"].ToString().Trim());

			Log::Trace("", __FUNCTION__, "sqlstr11=[{0}]", sqlstr);
			cmd_inq.ExecuteReader();
			fetchRowCount = 0;
			while (cmd_inq.Read())
			{
				int k = 0;
				k = cmd_inq.Fetch(tsmpe11, 1);
				v_stacking_no_1 = cmd_inq.GetString(k);
				Log::Trace("", __FUNCTION__, "fetchRowCount= [{0}]", fetchRowCount + 1);
				switch (conn->DatabaseKind)
				{
				case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
				case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
				case DB_KIND_MSSQL:	        // MS SQL Server数据库
				case DB_KIND_ORACLE:	        // Oracle 数据库
				default:


					sqlstr = "SELECT nvl(ORDER_CUST_CNAME , ' ') FROM tsmpe10 WHERE ORDER_NO = @tsmpe11.ORDER_NO";
					break;
				}

				cmd_inq1.SetCommandText(sqlstr);
				cmd_inq1.Parameters.Set("tsmpe11.ORDER_NO", tsmpe11["ORDER_NO"].ToString());
				cmd_inq1.ExecuteReader();
				if (cmd_inq1.Read())
				{
					order_cust_cname = cmd_inq1.GetString(1);
				}
				else
				{
					order_cust_cname = " ";
				}
				cmd_inq1.Close();
				Log::Trace("", __FUNCTION__, "order_cust_cname=[{0}]", order_cust_cname);

				//登录用户名
				sqlstr = "SELECT CNAME FROM tesuserinfo WHERE  ENAME  = @tsmpe11.REC_CREATOR";

				Log::Trace("", __FUNCTION__, "sqlstr= [{0}]", sqlstr);
				Log::Trace("", __FUNCTION__, "REC_CREATOR = [{0}]", tsmpe11["REC_CREATOR"].ToString());
				cmd_inq1.SetCommandText(sqlstr);
				cmd_inq1.Parameters.Set("tsmpe11.REC_CREATOR", tsmpe11["REC_CREATOR"].ToString());
				cmd_inq1.ExecuteReader();
				if (cmd_inq1.Read())
				{
					v_rec_creator = cmd_inq1.GetString(1);
				}
				else
				{
					v_rec_creator = " ";
				}
				cmd_inq1.Close();

				tsmpe11.MergeTo(bcls_ret->Tables[0], false);
				bcls_ret->Tables[0].Rows[fetchRowCount]["order_cust_cname"] = order_cust_cname;
				bcls_ret->Tables[0].Rows[fetchRowCount]["STACKING_NO_1"] = v_stacking_no_1;
				bcls_ret->Tables[0].Rows[fetchRowCount]["REC_CREATOR"] = v_rec_creator;//制单人
				bcls_ret->Tables[0].Rows[fetchRowCount]["STOCK_NO"] = tsmpe11["STOCK_NO"].ToString();
				//bcls_ret->Tables[0].Rows[fetchRowCount]["DELIVY_TIME"] = tsmpe11["DELIVY_TIME"].ToString().Substring(0, 4) + "/" + tsmpe11["DELIVY_TIME"].ToString().Substring(4, 2) + "/" + tsmpe11["DELIVY_TIME"].ToString().Substring(6, 2) + " " + tsmpe11["DELIVY_TIME"].ToString().Substring(8, 2) + ":"+tsmpe11["DELIVY_TIME"].ToString().Substring(10, 2);
				bcls_ret->Tables[0].Rows[fetchRowCount]["DELIVY_TIME"] = tsmpe11["DELIVY_TIME"];
				fetchRowCount++;
				/*bcls_ret->SetColVal(1, fetchRowCount, (T_INFO *)&tsmpe11_info);
				bcls_ret-> SetColVal(1,fetchRowCount, "order_cust_cname"	,order_cust_cname);*/
			}
		}
		catch (const CException& ex)
		{
			{
				CFormattable arguments[] = { ex.GetCode() };
				CMessageFormat::Format(s.msg, _RES("SM00S0001273")/*读取码单记录出错,sqlcode=[{0}]*/, arguments, 1);
				CString str = sqlstr + "\r\n" + ex.GetMsg();
				Log::Error("", __FUNCTION__, "error=[{0}]", str);
			}
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		{CFormattable arguments[] = { fetchRowCount };
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
