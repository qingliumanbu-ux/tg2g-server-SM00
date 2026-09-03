/****************************************************
*	程序功能：	按量发货装车材料重量校验			*
*	编制日期：	2023-2-1							*
*	编制人员：	13801								*
*	传入参数：	车号、计划号					*
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
 int  f_sm00_load_wt_check(EIClass * bcls_rec , EIClass * bcls_ret , CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int		doFlag  = 0;
	//CString	blkname = "SM00_MAT_CHECK";
	int blkname = 0;

	CString	c_vehicle_no = "", c_bill_of_lading_no = "", c_delivy_qty_flag = "";

	CDbCommand cmd_inq(conn);
	CDbCommand cmd_loop(conn);
	CDbCommand cmd_loop2(conn);
	CString sqlstr = "";

	try
	{
		// 读取传入的参数
		if (bcls_rec->Tables[blkname].Columns.Contains("bill_of_lading_no"))
		{
			c_bill_of_lading_no = bcls_rec->Tables[blkname].Rows[0]["bill_of_lading_no"];	//计划号
		}
		if (bcls_rec->Tables[blkname].Columns.Contains("vehicle_no"))
		{
			c_vehicle_no = bcls_rec->Tables[blkname].Rows[0]["vehicle_no"];	//车号
		}
		Log::Debug("", "", "vehicle_no=[{0}]", c_vehicle_no);
		Log::Debug("", "", "bill_of_lading_no=[{0}]", c_bill_of_lading_no);


		// 校验读取的参数
		if (c_vehicle_no.Trim() == "" && c_bill_of_lading_no.Trim() =="")
		{
			sprintf(s.msg, "车号和计划号不能同时为空!");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		// 按计划号、车号读取发货材料表上的重量、计划号、合同号
		CString c_order_no = "";
		CDecimal d_mat_wt = 0;
		CDecimal d_mat_wt_sum = 0;
		CString c_mat_kind = "";
		int k = 0;

		CString c_trnp_mode_code = "";
		CDecimal d_plan_wt = 0;
		CDecimal d_plan_wt_d = 0;

		CString c_vehicle_type = "";

		sqlstr = " SELECT BILL_OF_LADING_NO,ORDER_NO,sum(MAT_WT),MAX(MAT_KIND),MAX(DELIVY_QTY_FLAG) FROM TSMPE02 WHERE 1=1 ";
		if (c_vehicle_no.Trim() != "")
		{
			sqlstr += " AND VEHICLE_NO = '" + c_vehicle_no.Trim() + "' ";
		}
		if (c_bill_of_lading_no.Trim() != "")
		{
			sqlstr += " AND BILL_OF_LADING_NO = '" + c_bill_of_lading_no.Trim() + "' ";
		}
		sqlstr += " GROUP BY BILL_OF_LADING_NO,ORDER_NO ";

		cmd_loop.SetCommandText(sqlstr);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_loop.ExecuteReader();
		while (cmd_loop.Read())
		{
			k++;
			c_bill_of_lading_no = cmd_loop.GetString(1);
			c_order_no = cmd_loop.GetString(2);
			d_mat_wt = cmd_loop.GetDecimal(3);
			c_mat_kind = cmd_loop.GetString(4);
			c_delivy_qty_flag = cmd_loop.GetString(5);//20230922 liguangyuan
			Log::Debug("", "", "c_bill_of_lading_no={0}", c_bill_of_lading_no);
			Log::Debug("", "", "c_order_no={0}", c_order_no);
			Log::Debug("", "", "d_mat_wt={0}", d_mat_wt);
			Log::Debug("", "", "c_mat_kind={0}", c_mat_kind);

			d_mat_wt_sum = d_mat_wt_sum + d_mat_wt;

			// 按车号、计划号、合同号到发货计划表上读取计划量、运输方式
			int k1 = 0;
			sqlstr = "SELECT TRNP_MODE_CODE,PLAN_WT,PLAN_WT_D FROM TSMPE10 "
				" WHERE BILL_OF_LADING_NO = '" + c_bill_of_lading_no.Trim() + "' ";
			//且按量标记不为0按件。按件tsmpe10没有合同号20230922liguangyuan
			if (c_order_no.Trim() != "" && c_delivy_qty_flag != "0")
			{
				sqlstr += " AND ORDER_NO ='" + c_order_no.Trim() + "' ";
			}

			cmd_loop2.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			cmd_loop2.ExecuteReader();
			while (cmd_loop2.Read())
			{
				k1++;
				c_trnp_mode_code = cmd_loop2.GetString(1);
				d_plan_wt = cmd_loop2.GetDecimal(2);
				d_plan_wt_d = cmd_loop2.GetDecimal(3);
				Log::Debug("", "", "c_trnp_mode_code={0}", c_trnp_mode_code);
				Log::Debug("", "", "d_plan_wt={0}", d_plan_wt);
				Log::Debug("", "", "d_plan_wt_d={0}", d_plan_wt_d);


				// 运输方式是铁运时到车皮信息表TSM00B4上读取车辆类型VEHICLE_TYPE,根据车辆类型确定装载量
				if (c_trnp_mode_code.SubstringNE(1,1) == "2")	// 铁运时
				{
					sqlstr = "SELECT VEHICLE_TYPE FROM TSM00B4 WHERE VEHICLE_NO ='" + c_vehicle_no + "' ";
					cmd_inq.SetCommandText(sqlstr);
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					cmd_inq.ExecuteReader();
					if (cmd_inq.Read())
					{
						c_vehicle_type = cmd_inq.GetString(1);
						if (c_vehicle_type.Trim() == "C70")
						{
							d_plan_wt = 70;
							d_plan_wt_d = 70;
						}
						else
						{
							d_plan_wt = 60;
							d_plan_wt_d = 60;
						}
					}
					cmd_inq.Close();
				}
				else  // 汽运时
				{
					if (d_mat_wt > d_plan_wt_d )
					{
						CFormattable arguments[] = { c_bill_of_lading_no,c_order_no,d_mat_wt,d_plan_wt_d };
						CMessageFormat::Format(s.msg, "计划号【{0}】，合同号【{1}】，装车重量【{2}】超出计划重量【{3}】", arguments, 4);
						throw	CApplicationException(-1, s.msg, s.svc_name);
					}

				}
			}
			cmd_loop2.Close();
			if (k1==0)
			{
				sprintf(s.msg, "没有读取到计划重量！");
				throw	CApplicationException(-1, s.msg, log.Location);
			}
		}
		cmd_loop.Close();
		if ( k== 0)
		{
			sprintf(s.msg, "没有读取到装车材料重量！");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		// 铁运时判整车是否超重
		if (d_mat_wt_sum > d_plan_wt_d && c_trnp_mode_code.SubstringNE(1, 1) == "2")
		{
			CFormattable arguments[] = { c_vehicle_no,c_vehicle_type, d_mat_wt_sum, d_plan_wt_d };
			CMessageFormat::Format(s.msg, "车号【{0}】，车辆类型【{1}】，装车重量【{2}】超出车辆限重【{3}】", arguments, 4);
			throw	CApplicationException(-1, s.msg, s.svc_name);
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
