/*************************************************
Copyright:   Baosight Software LTD.co Copyright (c) 2010
Author:      179297
Version:     3.0
Date:        2021-2-8 12:18:02
Description: 铁运上道信息
**************************************************/

//框架公用头文件，勿删
#include "stdafx.h"
#include "epex.h"

//程序用头文件



//外部函数声明

// service入口

int f_paxx01_rcv(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{

	CTracer log(__FUNCTION__);

	/* 程序内部变量 */
	int   doFlag = 0;
	int   fetchRowCount = 0;
	int   outBlockRow = 0;
	int   ren = 0;
	int   v_count = 0;
	int   blkNum = 0;
	int   v_flag = 0;

	/* 业务变量 */
	CString  datetime("");
	CString	OPER_FLAG = "";

	/* 实体类定义 */
	CModel tsm00b4("TSM00B4");
	CModel hsm00b4("HSM00B4");


	// 数据库SQL操作字符串
	CString  sqlstr("");

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_loop(conn);
	CDbCommand cmd_upd(conn);


	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		//获得传入的数据行数

		tsm00b4.MergeFrom(bcls_rec->Tables[0].Rows[0]);
		tsm00b4["VEHICLE_NUM"] = bcls_rec->Tables[0].Rows[0]["VEHICLE_NUM"];	// 车皮数

		Log::Debug("", "", "VEHICLE_NUM =[{0}]", tsm00b4["VEHICLE_NUM"].ToDecimal());

		if (tsm00b4["VEHICLE_NUM"].ToDecimal().ToInt32() == 0)
		{
			sprintf(s.msg, "车皮数不能为0");
			throw	CApplicationException(-1, s.msg, log.Location);
		}
		int count = tsm00b4["VEHICLE_NUM"].ToDecimal().ToInt32();
		Log::Debug("", "", "count=[{0}]", count);

		CString c_return_mark = "", c_return_remark = "";
		for ( int i = 0; i < count; i++ )
		{
			OPER_FLAG = bcls_rec->Tables[0].Rows[0]["OPERATION_FLAG"];	// A--上道，L--离道
			tsm00b4["LANE_NO"] = bcls_rec->Tables[0].Rows[0]["RAILWAY_NO"];	// 股道代码
			tsm00b4["ARRIVAL_TIME"] = bcls_rec->Tables[0].Rows[0]["RAILWAY_TIME"];	// 上离道时刻
			tsm00b4["VEHICLE_NUM"] = bcls_rec->Tables[0].Rows[0]["VEHICLE_NUM"];	// 车皮数
			tsm00b4["SEQ_NO"] = bcls_rec->Tables[0].Rows[i]["VEHICLE_SEQ"];	// 顺位
			tsm00b4["VEHICLE_TYPE"] = bcls_rec->Tables[0].Rows[i]["VEHICLE_TYPE"];	// 车型
			tsm00b4["VEHICLE_NO"] = bcls_rec->Tables[0].Rows[i]["VEHICLE_NO"];	// 车皮号
			tsm00b4["VEHICLE_ID"] = bcls_rec->Tables[0].Rows[i]["VEHICLE_ID"];	// 车皮ID
			c_return_mark = bcls_rec->Tables[0].Rows[i]["RETURN_MARK"];	// 返厂标记
			c_return_remark = bcls_rec->Tables[0].Rows[i]["RETURN_REMARK"];	// 返厂原因
			tsm00b4["TICKET_NO"] = bcls_rec->Tables[0].Rows[i]["LAOD_NO"];	// 装车单号
			tsm00b4["NO_LOAD_FLAG"] = bcls_rec->Tables[0].Rows[i]["EMPTY_FLAG"];	// 空重标记

			//tsm00b4["LOAD_WEIGHT"] = bcls_rec->Tables[0].Rows[i]["WAGON_ST_WG"];	// 标重
			tsm00b4["USE_MARK"] = "Y";	// 车辆状态(可用）
			tsm00b4["TRNP_MODE_CODE"] = "2";//运输方式


			if ( i == 0 )
			{
				if (OPER_FLAG != "A" && OPER_FLAG != "L" && OPER_FLAG != "D")
				{ 
					sprintf(s.msg, "操作标志出错,A--上道，L--离道！");
					throw	CApplicationException(-1, s.msg, log.Location);
				}

				if ( tsm00b4["LANE_NO"].ToString().Trim() == "" )
				{
					sprintf(s.msg, "股道代码不能为空");
					throw	CApplicationException(-1, s.msg, log.Location);
				}

			}

			/* 数据校验 */
			if ( tsm00b4["VEHICLE_NO"].ToString().Trim() == "" )
			{
				sprintf(s.msg, "车皮号不能为空");
				throw	CApplicationException(-1,s.msg,log.Location);
			}
			if ( tsm00b4["VEHICLE_ID"].ToString().Trim() == "" )
			{
				sprintf(s.msg, "车皮号ID不能为空");
				throw	CApplicationException(-1,s.msg,log.Location);
			}


			if ( tsm00b4["NO_LOAD_FLAG"].ToString() == "F" )	// E--空  F--重
			{
				tsm00b4["NO_LOAD_FLAG"] = "1";		// 重车
			}
			else
			{
				tsm00b4["NO_LOAD_FLAG"] = "0";		// 空车
			}


			if (tsm00b4["USE_MARK"].ToString() == "Y" || tsm00b4["USE_MARK"].ToString().Trim() == "")
			{
				tsm00b4["USE_MARK"] = "1";	// 1-可用 ， 0--不可用
			}
			else
			{
				tsm00b4["USE_MARK"] = "0";	// 1-可用 ， 0--不可用
			}


			// 车皮上道
			if (OPER_FLAG == "A")
			{
				sqlstr = "select TICKET_NO from TSM00B4 WHERE VEHICLE_NO = '" + tsm00b4["VEHICLE_NO"].ToString().Trim() + "' ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					if (cmd_inq.GetString(1).Trim() == "")		
					{
						tsm00b4.Delete("VEHICLE_NO");	
					}
					else
					{
						sprintf(s.msg, "车皮号[%s]已经存在!", (const char *)tsm00b4["VEHICLE_NO"].ToString());
						throw	CApplicationException(-1, s.msg, s.svc_name);
					}
				}
				cmd_inq.Close();


				// 根据股道来判定库区
				sqlstr = "select  CODE,CODE_DESC_2_CONTENT,CODE_DESC_3_CONTENT from tep0002 where code_class = 'SMAG01' AND CODE_DESC_1_CONTENT = @LANE_NO ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("LANE_NO", tsm00b4["LANE_NO"].ToString());
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					tsm00b4["STOCK_NO"] = cmd_inq.GetString(1);
					tsm00b4["STOCK_NO"] = cmd_inq.GetString(2);
					tsm00b4["MAT_KIND"] = cmd_inq.GetString(3);
				}
				else
				{
					sprintf(s.msg, "请在代码SMAG01上维护股道和仓库的对应关系");
					//throw	CApplicationException(-1, s.msg, s.svc_name);
				}
				cmd_inq.Close();

				if (tsm00b4["STOCK_NO"].ToString().Trim().GetLength() > 3)
				{
					tsm00b4["STOCK_NO"] = " ";
				}


				// 根据库区来判定物料种类、厂别
				sqlstr = " select mat_kind,factory_div from TWM01 where stock_no = @stock_no ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("stock_no", tsm00b4["STOCK_NO"].ToString());
				Log::Debug("", "", "sqlstr={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					tsm00b4["MAT_KIND"] = cmd_inq.GetString(1);
					tsm00b4["FACTORY_DIV"] = cmd_inq.GetString(2);
				}
				cmd_inq.Close();

				tsm00b4["REC_CREATE_TIME"] = datetime;
				tsm00b4["REC_CREATOR"] = s.username;
				tsm00b4["STATUS"] = "0";		// 接收

				tsm00b4.TrimOrBlank();
				tsm00b4.Print();
				sqlstr = " INSERT tsm00b4 ";
				tsm00b4.Insert();

			}

			// 车皮离道
			if (OPER_FLAG == "L")
			{
				/* 读取车辆信息 */
				if (!tsm00b4.Query("VEHICLE_NO,VEHICLE_ID"))
				{
					CFormattable arguments[] = { tsm00b4["VEHICLE_NO"].ToString(), tsm00b4["VEHICLE_ID"].ToString() };
					CMessageFormat::Format(s.msg, "没有此车号{0}，ID号{1}的车辆信息", arguments, 2);
					throw	CApplicationException(-1, s.msg, s.svc_name);
				}

				//if (tsm00b4["TICKET_NO"].ToString().Trim() == "")//空车归档
				{
					/* 写入历史档*/
					hsm00b4.CopyFrom(tsm00b4);
					hsm00b4["REC_REVISE_TIME"] = datetime;
					hsm00b4["REC_REVISOR"] = s.userid;
					//hsm00b4["STATUS"] = "9";
					sqlstr = "INSERT HSM00B4 ";
					hsm00b4.TrimOrBlank();
					hsm00b4.Insert();

					/* 删除此车辆记录 */
					sqlstr = "INSERT TSM00B4 ";
					tsm00b4.Delete("VEHICLE_NO,VEHICLE_ID");
				}


			}

			// 车皮删除
			if ( OPER_FLAG == "D" )
			{
				sqlstr = "SELECT VEHICLE_NO FROM TSM00B4 WHERE VEHICLE_NO = @tsm00b4.VEHICLE_NO ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("tsm00b4.VEHICLE_NO", tsm00b4["VEHICLE_NO"].ToString());
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					sqlstr = " DELETE FROM TSM00B4 WHERE VEHICLE_NO = @tsm00b4.VEHICLE_NO AND TICKET_NO = ' '";	// 2021-11-23 09:01:19
					cmd_upd.SetCommandText(sqlstr);
					cmd_upd.Parameters.Set("tsm00b4.VEHICLE_NO", tsm00b4["VEHICLE_NO"].ToString());
					Log::Debug("", "", "sqlstr={0}", sqlstr);
					Log::Debug("", "", "VEHICLE_NO={0}", tsm00b4["VEHICLE_NO"].ToString());
					if (cmd_upd.ExecuteNonQuery() == 0)
					{
						sprintf(s.msg, "此车皮[%s]不满足删除条件！", (const char *)tsm00b4["VEHICLE_NO"].ToString());
						throw	CApplicationException(-1, s.msg, s.svc_name);
					}
				}
				cmd_inq.Close();
				continue;
			}

		}


		strcpy(s.msg, "电文接收成功！");//处理成功。
		s.flag = 0;
		return 0;

	}
	catch ( CDbException& ex )  //捕获数据库操作异常
	{
		CFormattable arguments[] ={ ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("数据库操作失败{0}")/*信息读取失败。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1; //数据库异常时返回-1，事务将被回滚
	}
	catch ( const CApplicationException& ex )
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch ( CException& ex )
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return(doFlag);
}
