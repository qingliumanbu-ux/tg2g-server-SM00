/****************************************************
*	程序功能：	读取计划车号下配车的材料			*
*	编制日期：	2023-2-2							*
*	编制人员：	13801								*
*	传入参数：	计划号、车号						*
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
int  f_sm00_plan_mat_aim(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int		doFlag  = 0;
	//CString	blkname = "SM00_MAT_CHECK";
	int blkname = 0;

	CString	table_name = "", c_bill_of_lading_no = "";
	CString c_vehicle_no = "";	// 车号
	CString c_vehicle_no_1 = "";	// 车号

	CDbCommand cmd_inq(conn);
	CDbCommand cmd_loop(conn);
	CDbCommand cmd_loop2(conn);
	CString sqlstr = "";
	CString sqlstr1 = "";

	try
	{
		// 读取传入的参数
		if (bcls_rec->Tables[blkname].Columns.Contains("BILL_OF_LADING_NO"))
		{
			c_bill_of_lading_no = bcls_rec->Tables[blkname].Rows[0]["BILL_OF_LADING_NO"];	//计划号
		}
		if (bcls_rec->Tables[blkname].Columns.Contains("VEHICLE_NO"))
		{
			c_vehicle_no = bcls_rec->Tables[blkname].Rows[0]["VEHICLE_NO"];
		}

		Log::Debug("", "", "bill_of_lading_no=[{0}]", c_bill_of_lading_no);
		Log::Debug("", "", "vehicle_no=[{0}]", c_vehicle_no);


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

		sqlstr = " SELECT DISTINCT MAT_KIND,VEHICLE_NO FROM TSMPE10 WHERE BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "'";
		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			c_mat_kind = cmd_inq.GetString(1);
			c_vehicle_no_1 = cmd_inq.GetString(2);

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


		// 输入的车号为空时到材料表上读取配车的车号
		if (c_vehicle_no_1.Trim() != "")
		{
			c_vehicle_no = c_vehicle_no_1;
		}

		if (c_vehicle_no.Trim() == "")
		{
			sqlstr = "select DISTINCT VEHICLE_NO from tsmpe02 where BILL_OF_LADING_NO = @BILL_OF_LADING_NO and VEHICLE_NO != ' ' ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("BILL_OF_LADING_NO", c_bill_of_lading_no);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				c_vehicle_no = cmd_inq.GetString(1);
			}
			cmd_inq.Close();
		}
		Log::Debug("", "", "VEHICLE_NO = [{0}]", c_vehicle_no);


		if (c_vehicle_no.Trim() != "")
		{
			// sql语句
			sqlstr = "SELECT T.* ,A.STOCK_PLACE_NO,A.LAYERNO,A.HEAT_NO FROM TSMPE02 T ," + table_name + " A "
				//" ,TSMPE10 B "
				" WHERE T.RED_FLAG != '1' "
				" AND T.MAT_NO = A.MAT_NO "
				//" AND T.BILL_OF_LADING_NO = B.BILL_OF_LADING_NO AND T.ORDER_NO = B.ORDER_NO "
				" AND T.VEHICLE_NO = '" + c_vehicle_no.Trim() + "' ";
			sqlstr += " ORDER BY A.STOCK_PLACE_NO,A.LAYERNO DESC";


			cmd_loop.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_loop.ExecuteQuery(bcls_ret->Tables[0]);
		}


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
