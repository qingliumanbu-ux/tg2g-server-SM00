/*
程序名称:		sm0015_md_ok
隶属子系统:		SMSW
产品名称:		BSM1
功能描述:		对汽运装车未确认的装车单进行确认
外部接口:		无
*/
//	2022-3-25	13801	增加炼钢实重交货的称重
/* C 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除
#include "epex.h"


//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

int	f_sm00_md_ok(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	// 码单确认
//int f_xxdn01_send(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	// 持出电文发送
//int f_sm00_xxjlwt_snd(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn);	// 计量委托

/* -EP_CODE_VERSION 1
-EP_SYSTEM_HEAD_BEGIN
-此节代码请勿更改 */
// service入口
//BM2F_ENTERACE_TELE2(cm_pbm1s9_rcv, f_sm00_measure)
BM2F_ENTERACE(sm0015_md_ok)

int	f_sm0015_md_ok(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	int	ret = 0;
	int doFlag = 0;
	CString	sqlstr = "";

	CDbCommand cmd_inq(conn);

	CString	ticket_no = "";	// 装车单
	CString	STACKING_STATUS = "";	// 装车单状态
	CString	TRNP_MODE_CODE = "";	// 运输方式
	CString	STOCK_NO = "";	// 库区
	CString	SYS_CODE = "";	// 系统别
	CString VEHICLE_NO = "";
	CString MAT_KIND = "";	// 物料种类
	CString WT_MODE = "";	// 计重方式，0--实重，1--理重
	CString	STOCK_TYPE_CODE = "";	// 厂内外区分  0--厂内库，1--厂外库
	CString PONDER_NO = "";	// 磅单号，计量委托号
	try
	{
		int v_count = bcls_rec->Tables[0].Rows.get_Count();
		if ( v_count == 0 )
		{
			sprintf(s.msg, "没有传入参数");
			throw	CApplicationException(-1,s.msg,log.Location);
		}


		EIClass	bcls_rec_jlwt;
		bcls_rec_jlwt.Tables[0].set_TableName("JLWT");
		bcls_rec_jlwt.Tables[0].Columns.Add(DT_STRING, "TICKET_NO");
		bcls_rec_jlwt.Tables[0].Columns.Add(DT_STRING, "OPER_FLAG");


		for ( int i = 0; i < v_count; i++ )
		{
			ticket_no = bcls_rec->Tables[0].Rows[i]["TICKET_NO"].ToString();	// 装车单号

			Log::Debug("", __FUNCTION__, " 第 [{0}] 条,共[{1}]条，装车单号[{2}]", i + 1, v_count, ticket_no);

			if (ticket_no.Trim() == "")
			{
				sprintf(s.msg, "装车单号不能为空");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}



			// 读取装车单状态
			STACKING_STATUS = " ", TRNP_MODE_CODE = " ";
			sqlstr = " select STACKING_STATUS,TRNP_MODE_CODE,STOCK_NO,VEHICLE_NO,MAT_KIND,WT_MODE,PONDER_NO "
				" FROM TSMPE11 WHERE ticket_no = '" + ticket_no + "' "
				" ORDER BY WT_MODE ";
			cmd_inq.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr=[{0}]", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				STACKING_STATUS = cmd_inq.GetString(1);
				TRNP_MODE_CODE = cmd_inq.GetString(2);
				STOCK_NO = cmd_inq.GetString(3);
				VEHICLE_NO = cmd_inq.GetString(4);
				MAT_KIND = cmd_inq.GetString(5);
				WT_MODE = cmd_inq.GetString(6);
				PONDER_NO = cmd_inq.GetString(7);
			}
			else
			{
				sprintf(s.msg, "无此%s装车单号", (const char *)ticket_no);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			cmd_inq.Close();

			if (STACKING_STATUS == "0")
			{
				sprintf(s.msg, "此%s装车单,已经确认，不能再次确认！", (const char *)ticket_no);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			if (TRNP_MODE_CODE == "22")
			{
				sprintf(s.msg, "此%s装车单,是铁运不能再此确认！", (const char *)ticket_no);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}


			// 车号长度是否为7位，否报错
			if (VEHICLE_NO.Trim().GetLength() < 7)
			{
				sprintf(s.msg, "此%s装车单,上车号位数不足！", (const char *)ticket_no);
				throw CApplicationException(-1, s.msg, s.svc_name);
			}



			// 是炼钢板坯在厂内库实重交货的材料需要发送计量委托	2022-3-25
			Log::Info("", "", "MAT_KIND={0},WT_MODE={1},PONDER_NO={2}", MAT_KIND, WT_MODE, PONDER_NO);
			if (MAT_KIND == "SM")	// 是炼钢板坯
			{
				if (WT_MODE == "0")	// 实重交货
				{
					sqlstr = " SELECT  STOCK_TYPE_CODE FROM TSI0021 WHERE STOCK_NO = '" + STOCK_NO + "' ";
					cmd_inq.SetCommandText(sqlstr);
					Log::Info("", "", "sqlstr={0}", sqlstr);
					cmd_inq.ExecuteReader();
					if (cmd_inq.Read())
					{
						STOCK_TYPE_CODE = cmd_inq.GetString(1);
					}
					cmd_inq.Close();
					if (STOCK_TYPE_CODE == "0")	// 是厂内库
					{
						if (PONDER_NO.Trim() == "")	// 磅单号为空，表示还没有发送计划委托
						{
							// 发送计量委托
							bcls_rec_jlwt.Tables[0].Rows.Add();
							int ii = bcls_rec_jlwt.Tables[0].Rows.get_Count() - 1;
							bcls_rec_jlwt.Tables[0].Rows[ii]["TICKET_NO"] = ticket_no;
							bcls_rec_jlwt.Tables[0].Rows[ii]["OPER_FLAG"] = "I";

							//ret = f_sm00_xxjlwt_snd(&bcls_rec_jlwt, bcls_ret, conn);
							if (ret < 0)
							{
								throw	CApplicationException(-1, s.msg, log.Location);
							}
							continue;
						}
						else
						{
							CFormattable arguments[] = { ticket_no };
							CMessageFormat::Format(s.msg, "装车单{0}已经发送了计量委托！", arguments, 1);
							throw	CApplicationException(-1, s.msg, s.svc_name);
						}
					}
				}
			}
			// 2022-3-25


			// 调用码单确认函数
			CString blkname = "md_ok";
			EIClass bcls_rec_md;
			bcls_rec_md.Tables[0].set_TableName(blkname);

			bcls_rec_md.Tables[blkname].Rows.Clear();
			bcls_rec_md.Tables[blkname].Rows.Add();
			bcls_rec_md.Tables[blkname].Columns.Add(DT_STRING, "TICKET_NO");
			bcls_rec_md.Tables[blkname].Rows[0]["TICKET_NO"] = ticket_no;
			/********************************************************************************
			*****	调用码单确认函数	*****
			********************************************************************************/
			//EDLog(1,1,"★★★★★调用函数 f_smbw_md_ok 开始★★★★★出厂★★★");
			//EDLog	(1,1,"装车单号=[%s]"	, (const char*)c_stacking_no );
			bcls_rec->SetSYS(s);
			ret = 0;
			ret = f_sm00_md_ok(&bcls_rec_md, bcls_ret, conn);
			if (ret != 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			sprintf(s.msg, "装车单确认正确，码单传销售物流！");
		}

		// 取系统别
		sqlstr = "SELECT SYS_CODE FROM TWM01 WHERE STOCK_NO = '" + STOCK_NO + "' ";
		cmd_inq.SetCommandText(sqlstr);
		Log::Trace("", "", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			SYS_CODE = cmd_inq.GetString(1);
		}
		cmd_inq.Close();


		//if (SYS_CODE == "P8" || SYS_CODE == "P9" || SYS_CODE == "P7" || SYS_CODE == "P5" || SYS_CODE == "P4" || SYS_CODE == "P3" || SYS_CODE == "P2" || SYS_CODE == "P1")
		{
			//ret = f_xxdn01_send(bcls_rec, bcls_ret, conn);
			if (ret != 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}

	}
	catch ( CDbException& ex )  //捕获数据库操作异常
	{
		CFormattable arguments[] ={ ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch ( CApplicationException& ex )  //捕获应用错误
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch ( CException& ex )
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}

