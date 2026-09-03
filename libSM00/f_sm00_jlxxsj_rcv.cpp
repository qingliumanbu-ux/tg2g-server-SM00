/*
程序名称:		接收称重实绩
隶属子系统:		SM
产品名称:		BSM1
功能描述:		计量称重实绩处理，重量分摊
外部接口:		无
*/
/* C 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除
#include "epex.h"


//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

int	f_sm00_md_ok(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);	// 码单确认
int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection* conn);	/* 抛物料跟踪打包函数 */
int f_sm00_mm99(vector <CString> pack_num, int para_type, int event_id, CString msg, CDbConnection* conn);	/* 抛物料跟踪打包函数 */

/* -EP_CODE_VERSION 1
-EP_SYSTEM_HEAD_BEGIN
-此节代码请勿更改 */
// service入口
//BM2F_ENTERACE_TELE2(cm_pbm1s9_rcv, f_sm00_measure)

int	f_sm00_jlxxsj_rcv(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	int	ret = 0;
	int doFlag = 0;
	CString	sqlstr = "";
	//CString	block_name_master = "BODY";
	int	block_name_master = 0;

	CModel tsmpe11("TSMPE11");
	CModel tsmpe12("TSMPE12");
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_loop(conn);

	CString	PONDER_APP_NO = "";	// 过磅申请单号
	CString	PONDERING_NO = "";	// 磅单号
	CDecimal STACKING_WT_MAX = 0;   //码单重量允许最大值
	CDecimal STACKING_WT_MIN = 0;   //码单重量允许最小值
	CDecimal STACKING_GROSS_WT_MAX = 0;	// 码单毛重允许最大值
	CDecimal STACKING_GROSS_WT_MIN = 0;	// 码单毛重允许最小值
	CDecimal NET_WEIGHT = 0;    //过磅净重
	CDecimal DELIVY_TOL_RATE_MAX = 0.003;  // 3‰
	CDecimal DELIVY_TOL_RATE_MIN = 0.003;  // 3‰
	CDecimal WT_MAX = 0;	//	上限重量差值
	CDecimal WT_MIN = 0;	//	下限重量差值
	CDecimal STACKING_WT = 0;	// 理重
	CDecimal STACKING_GROSS_WT = 0;	// 毛重
	CString	TICKET_NO = "";	// 装车单
	CString	STACKING_STATUS = "";	// 装车单状态
	try
	{
		PONDER_APP_NO = bcls_rec->Tables[block_name_master].Rows[0]["PONDER_APP_NO"].ToString();	//过磅申请单号
		NET_WEIGHT = bcls_rec->Tables[block_name_master].Rows[0]["NET_WEIGHT"].ToDecimal();

		if (PONDER_APP_NO.Trim() == "")
		{
			sprintf(s.msg, "过磅申请单号不能为空");
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		if (NET_WEIGHT == 0)
		{
			sprintf(s.msg, "称重重量不能为0 ");
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		//去发货码单表内 根据过磅申请单号 获取装车单
		sqlstr = "SELECT TICKET_NO FROM TSMPE11 WHERE ponder_no = @PONDER_APP_NO ";
		Log::Debug("", __FUNCTION__, "sqlstr = {0}", sqlstr);
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("PONDER_APP_NO", PONDER_APP_NO);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			TICKET_NO = cmd_inq.GetString(1);
		}
		else
		{
			CString Remark_msg = "无此计量委托号【" + PONDER_APP_NO + "】";
			sprintf(s.msg, Remark_msg);
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}
		cmd_inq.Close();


		// 读取装车单上的重量和件数
		double	v_mat_wt = 0;	// 装车重量（理重）
		int		v_count = 0;	// 装车件数
		double	v_stacking_wt = 0;	// 实绩称重
		v_stacking_wt = NET_WEIGHT.ToDouble();

		sqlstr = " SELECT sum(STACKING_WT) , sum(STACKING_NUM) FROM tsmpe11 WHERE ponder_no = @PONDER_APP_NO ";
		Log::Debug("", __FUNCTION__, "sqlstr = {0}", sqlstr);
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("PONDER_APP_NO", PONDER_APP_NO);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			v_mat_wt = cmd_inq.GetDecimal(1).ToDouble();
			v_count = cmd_inq.GetInt32(2);
		}
		cmd_inq.Close();


		//然后根据装车号 获取材料明细 此车有多少卷
		int fetchRowCount = 0;
		sqlstr = "SELECT A.* FROM TSMPE12 A,TSMPE11 B WHERE A.TICKET_NO = B.TICKET_NO AND b.ponder_no = @PONDER_APP_NO ";
		Log::Debug("", __FUNCTION__, "sqlstr = {0}", sqlstr);
		cmd_inq_loop.SetCommandText(sqlstr);
		cmd_inq_loop.Parameters.Set("PONDER_APP_NO", PONDER_APP_NO);
		cmd_inq_loop.ExecuteReader();
		while (cmd_inq_loop.Read())
		{
			cmd_inq_loop.Fetch(tsmpe12);
			++fetchRowCount;
			// 计算磅差量=材料重量/总量 * 过磅重量
			if (fetchRowCount < v_count)
			{
				tsmpe12["MAT_DISCREP_WT"] = (tsmpe12["MAT_WT"].ToDecimal() / v_mat_wt * NET_WEIGHT) - tsmpe12["MAT_WT"].ToDecimal();	// 计算前几个的磅差量
				// 由于精度关系对计算出来的磅差进行四舍五入,保留3位小数
				tsmpe12["MAT_DISCREP_WT"] = tsmpe12["MAT_DISCREP_WT"].ToDecimal().Round(3);
				v_stacking_wt = v_stacking_wt - tsmpe12["MAT_WT"].ToDouble() - tsmpe12["MAT_DISCREP_WT"].ToDouble();	// 计算剩余量
			}
			if (fetchRowCount == v_count)	tsmpe12["MAT_DISCREP_WT"].ToDecimal() = v_stacking_wt - tsmpe12["MAT_WT"].ToDecimal();	// 计算最后一个材料的差量

			// 更新材料表上的磅差量
			sqlstr = " UPDATE	tsmpe12 "
				" SET	mat_discrep_wt	=	@tsmpe12.mat_discrep_wt "
				" , MAT_ACT_WT = MAT_WT + DECODE(@tsmpe12.WT_MODE ,'0',@tsmpe12.mat_discrep_wt,0) "
				" WHERE mat_no			=	@tsmpe12.mat_no and STACKING_NO = @STACKING_NO ";		// SQL语句定义
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("tsmpe12.mat_discrep_wt", tsmpe12["MAT_DISCREP_WT"].ToDecimal());	// SQL语句中的变量赋值
			cmd_inq.Parameters.Set("tsmpe12.mat_no", tsmpe12["MAT_NO"]);	// SQL语句中的变量赋值
			cmd_inq.Parameters.Set("STACKING_NO", tsmpe12["STACKING_NO"]);	// SQL语句中的变量赋值
			cmd_inq.Parameters.Set("tsmpe12.WT_MODE", tsmpe12["WT_MODE"]);	// SQL语句中的变量赋值
			if (cmd_inq.ExecuteNonQuery() == 0)		// 语句执行
			{
				CFormattable arguments[] = { tsmpe12["MAT_NO"].ToString(), 1403 };
				CMessageFormat::Format(s.msg, _RES("SM00S0001392")/*更新材料表上的磅差量出错，材料号=[{0}],sqlcode=[{1}]*/, arguments, 2);
				throw	CApplicationException(-1, s.msg, log.Location);
			}

			// 更新码单表上的磅差量
			sqlstr = " UPDATE	tsmpe11 "
				" SET	stacking_discrep_wt = (SELECT SUM(mat_discrep_wt) FROM tsmpe12 WHERE stacking_no =	@tsmpe12.stacking_no and WT_MODE = '0') "
				" WHERE stacking_no			=	@tsmpe12.stacking_no ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("tsmpe12.stacking_no", tsmpe12["STACKING_NO"].ToString());	// SQL语句中的变量赋值
			if (cmd_inq.ExecuteNonQuery() == 0)		// 语句执行
			{
				CFormattable arguments[] = { tsmpe12["STACKING_NO"].ToString(), 1403 };
				CMessageFormat::Format(s.msg, _RES("SM00S0001377")/*更新码单表上的磅差量出错，码单号=[{0}],sqlcode=[{1}]*/, arguments, 2);
				throw	CApplicationException(-1, s.msg, log.Location);
			}

			// 调用物料抛实重
			if (tsmpe12["WT_MODE"].ToString() == "0")
			{
				ret = f_sm00_mm99(tsmpe12["MAT_NO"].ToString(), 3, 10, s.msg, conn);
				if (ret != 0)
				{
					CFormattable	arguments[] = { s.msg };
					CMessageFormat::Format(s.msg, "调用物料模块封装函数出错:{0}", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
			}

		}
		cmd_inq_loop.Close();
		if (fetchRowCount == 0)
		{
			CFormattable arguments[] = { PONDER_APP_NO };
			CMessageFormat::Format(s.msg, "无此委托单[{0}]下的材料!", arguments, 1);
			throw	CApplicationException(-1, s.msg, log.Location);
		}



		// 调用码单确认函数
		CString blkname = "md_ok";
		EIClass bcls_rec_md;
		bcls_rec_md.Tables[0].set_TableName(blkname);
		bcls_rec_md.Tables[blkname].Columns.Add(DT_STRING, "TICKET_NO");



		// 按计量委托号读取装车单号		2021-10-17
		sqlstr = "select TICKET_NO from tsmpe11  WHERE ponder_no=@PONDER_APP_NO group by TICKET_NO ";
		cmd_inq_loop.SetCommandText(sqlstr);
		cmd_inq_loop.Parameters.Set("PONDER_APP_NO", PONDER_APP_NO);
		Log::Trace("", "", sqlstr);
		cmd_inq_loop.ExecuteReader();
		while (cmd_inq_loop.Read())
		{
			bcls_rec_md.Tables[blkname].Rows.Add();
			int ii = bcls_rec_md.Tables[blkname].Rows.get_Count() - 1;
			bcls_rec_md.Tables[blkname].Rows[ii]["TICKET_NO"] = cmd_inq_loop.GetString(1);
		}
		cmd_inq_loop.Close();




		/********************************************************************************
		*****	调用码单确认函数	*****
		********************************************************************************/
		bcls_rec->SetSYS(s);
		ret = 0;
		ret = f_sm00_md_ok(&bcls_rec_md, bcls_ret, conn);
		if (ret != 0)
		{
			/*CTransactionManager::Abort(0);
			CTransactionManager::Begin(0, 0);

			sqlstr = " UPDATE TSMPE11 SET DELIVY_REMARK = SUBSTR(@msg,1,200) "
			" WHERE PONDER_NO = '" + PONDER_APP_NO + "' ";
			Log::Debug("", __FUNCTION__, "sqlstr = {0}", sqlstr);

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("msg", s.msg);
			cmd_inq.Parameters.Set("PONDER_APP_NO", PONDER_APP_NO);
			cmd_inq.ExecuteNonQuery();

			CTransactionManager::Commit(0);
			CTransactionManager::Begin(0, 0);*/

			CFormattable	arguments[] = { s.msg };
			CMessageFormat::Format(s.msg, "调用f_sm00_md_ok函数出错:{0}", arguments, 1);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}




	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
	{
		CTransactionManager::Abort(0);
		CTransactionManager::Begin(0, 0);

		sqlstr = " UPDATE TSMPE11 SET DELIVY_REMARK = SUBSTR(@msg,1,200) "
			" WHERE PONDER_NO = '" + PONDER_APP_NO + "' ";
		Log::Debug("", __FUNCTION__, "sqlstr = {0}", sqlstr);

		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("msg", s.msg);
		cmd_inq.Parameters.Set("PONDER_APP_NO", PONDER_APP_NO);
		cmd_inq.ExecuteNonQuery();

		CTransactionManager::Commit(0);
		CTransactionManager::Begin(0, 0);

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

