/****************************************************
*	程序功能：	读取满足计划条件下材料的跺位		*
*	编制日期：	2023-2-1							*
*	编制人员：	13801								*
*	传入参数：	计划号								*
*	返回参数：	0	成功	-1	失败				*
*	出错描述：	s.msg								*
*****************************************************
*													*
****************************************************/
//框架公用头文件，勿删
#include "stdafx.h"

using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//程序用头文件
BM2_FUNCTION_EXPORT
int  f_sm00_plan_place(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int		doFlag  = 0;
	//CString	blkname = "SM00_MAT_CHECK";
	int blkname = 0;

	CString	table_name = "", c_bill_of_lading_no = "";

	CDbCommand cmd_inq(conn);
	CDbCommand cmd_loop(conn);
	CDbCommand cmd_loop2(conn);
	CString sqlstr = "";

	try
	{
		// 读取传入的参数
		if (bcls_rec->Tables[blkname].Columns.Contains("BILL_OF_LADING_NO"))
		{
			c_bill_of_lading_no = bcls_rec->Tables[blkname].Rows[0]["BILL_OF_LADING_NO"];	//计划号
		}
		Log::Debug("", "", "bill_of_lading_no=[{0}]", c_bill_of_lading_no);


		// 校验读取的参数
		if ( c_bill_of_lading_no.Trim() =="")
		{
			sprintf(s.msg, "计划号不能为空!");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		// 按计划号读取物料种类
		CString c_mat_kind = "";
		CString c_delivy_qty_flag = "";
		CString c_stock_no = "";

		sqlstr = " SELECT DISTINCT MAT_KIND,DELIVY_QTY_FLAG,STOCK_NO FROM TSMPE10 WHERE BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "'";
		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			c_mat_kind = cmd_inq.GetString(1);
			c_delivy_qty_flag = cmd_inq.GetString(2);
			c_stock_no = cmd_inq.GetString(3);

			table_name = "TMM" + c_mat_kind + "01";
			Log::Debug("", "", "table_name=【{0}】", table_name);
		}
		else
		{
			CFormattable arguments[] = { c_bill_of_lading_no };
			CMessageFormat::Format(s.msg, "没有读到计划号【{0}】", arguments, 1);
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}
		cmd_inq.Close();


		sqlstr = " SELECT DISTINCT B.STOCK_PLACE_NO FROM TSMPE10 C,TSMPE02 A," + table_name + " B "
			" WHERE C.BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' "
			" AND a.CONFM_STATUS	=	'6' "
			" AND	A.RED_FLAG		!=	'1' "
			" AND   A.BILL_OF_LADING_NO = C.BILL_OF_LADING_NO "
			" AND   A.MAT_NO        =   B.MAT_NO ";


		if (c_delivy_qty_flag == "1" || c_delivy_qty_flag == "2" || c_delivy_qty_flag == "3")
		{
			// 按计划号读满足条件的材料库位
			if (c_mat_kind == "BW" || c_delivy_qty_flag == "1" )
			{
				sqlstr = "SELECT DISTINCT B.STOCK_PLACE_NO FROM TSMPE10 C,TSMPE02 A," + table_name + " B "
					" WHERE C.BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' "
					" AND a.CONFM_STATUS	=	'4' "
					" AND a.DELIVY_QTY_FLAG IN ('1','2') "
					" AND	A.RED_FLAG		!=	'1' "
					" AND	a.MAT_KIND		= '" + c_mat_kind + "' "
					" AND   A.TRNP_MODE_CODE =  C.TRNP_MODE_CODE "	// 运输方式
					" AND	a.SG_SIGN		=	C.SG_SIGN "		// 钢牌号
					" AND	a.MAT_THICK		=	C.ORDER_THICK "	// 厚度
					" AND   A.FIX_FLAG      =	C.FIX_FLAG "	// 定尺标记
					" AND   A.PSC           =	C.PSC "			// 产品规范码
					" AND	a.MAT_NO		=	b.MAT_NO "
					" AND   A.MAT_WIDTH     =   DECODE (C.ORDER_WIDTH,0,A.MAT_WIDTH,C.ORDER_WIDTH) "
					" AND   A.MAT_LEN  BETWEEN  DECODE (C.ORDER_MIN_LEN,0,A.MAT_LEN,C.ORDER_MIN_LEN) "
					"                  AND      DECODE (C.ORDER_MAX_LEN,0,A.MAT_LEN,C.ORDER_MAX_LEN) "
					" AND   (A.STOCK_NO     IN  (SELECT STOCK_NO FROM TWM01 WHERE STOCK_ADDR = '" + c_stock_no + "') "
					"                      OR  A.STOCK_NO = '" + c_stock_no + "' ) ";
			}

			// 按计划号读满足条件的材料库位
			if (c_mat_kind == "SM" || c_delivy_qty_flag == "1")
			{
				sqlstr = "SELECT DISTINCT B.STOCK_PLACE_NO FROM TSMPE10 C,TSMPE02 A," + table_name + " B "
					" WHERE C.BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' "
					" AND a.CONFM_STATUS	=	'4' "
					" AND a.DELIVY_QTY_FLAG IN ('1','2') "
					" AND	A.RED_FLAG		!=	'1' "
					" AND	a.MAT_KIND		= '" + c_mat_kind + "' "
					" AND   A.TRNP_MODE_CODE =  C.TRNP_MODE_CODE "	// 运输方式
					" AND	a.SG_SIGN		=	C.SG_SIGN "		// 钢牌号
					" AND	a.MAT_THICK		=	C.ORDER_THICK "	// 厚度
					//" AND   A.FIX_FLAG      =	C.FIX_FLAG "	// 定尺标记
					" AND   A.PSC           =	C.PSC "			// 产品规范码
					" AND	a.MAT_NO		=	b.MAT_NO "
					" AND   A.MAT_WIDTH     =   DECODE (C.ORDER_WIDTH,0,A.MAT_WIDTH,C.ORDER_WIDTH) "
					" AND   A.MAT_LEN  BETWEEN  DECODE (C.ORDER_MIN_LEN,0,A.MAT_LEN,C.ORDER_MIN_LEN) "
					"                  AND      DECODE (C.ORDER_MAX_LEN,0,A.MAT_LEN,C.ORDER_MAX_LEN) "
					" AND   (A.STOCK_NO     IN  (SELECT STOCK_NO FROM TWM01 WHERE STOCK_ADDR = '" + c_stock_no + "') "
					"                      OR  A.STOCK_NO = '" + c_stock_no + "' ) ";
			}


			if (c_mat_kind == "HP" && (c_delivy_qty_flag == "2" || c_delivy_qty_flag == "3"))
			{
				sqlstr = "SELECT DISTINCT B.STOCK_PLACE_NO FROM TSMPE10 C,TSMPE02 A," + table_name + " B "
					" WHERE C.BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' "
					" AND a.CONFM_STATUS	=	'4' "
					//" AND a.DELIVY_QTY_FLAG IN ('1','2') "
					" AND	A.RED_FLAG		!=	'1' "
					" AND	a.MAT_KIND		= '" + c_mat_kind + "' "
					" AND   A.TRNP_MODE_CODE =  C.TRNP_MODE_CODE "	// 运输方式
					//" AND   (A.ORDER_NO      =   C.ORDER_NO   OR   A.CONTRACT_NO = C.CONTRACT_NO ) "
					" AND	a.MAT_NO		=	b.MAT_NO "
					" AND  (A.STOCK_NO     IN  (SELECT STOCK_NO FROM TWM01 WHERE STOCK_ADDR = '" + c_stock_no + "') "
					"                      OR  A.STOCK_NO = '" + c_stock_no + "' ) ";
				if (c_delivy_qty_flag == "2")
				{
					sqlstr += " AND A.ORDER_NO      =   C.ORDER_NO ";
				}
				if (c_delivy_qty_flag == "3")
				{
					sqlstr += " A.CONTRACT_NO = C.CONTRACT_NO ";
				}

			}
		}


		cmd_loop.SetCommandText(sqlstr);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_loop.ExecuteQuery(bcls_ret->Tables[0]);

		sprintf	(s.msg , "处理成功");
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		Log::Error("", __FUNCTION__, "error=[{0}]", s.sysmsg);
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
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
	if (doFlag < 0)
	{
		//CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
	}

	return doFlag;

}
