/****************************************************
*	程序功能：	检查材料是否封锁					*
*	编制日期：	2014-07-08							*
*	编制人员：	13801								*
*	传入参数：	材料号、物料种类					*
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
 int  f_sm00_mat_check(EIClass * bcls_rec , EIClass * bcls_ret , CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int		doFlag  = 0;
	CString	blkname = "SM00_MAT_CHECK";

	CString	c_mat_no = "" , c_mat_kind = "";
	CString	table_name = "" , c_hold_flag = "0";

	CDbCommand cmd_inq(conn);
	CString sqlstr="";

	try
	{
		if	(!bcls_rec->Tables.Contains(blkname))
		{
			sprintf	(s.msg , "没有传入块名[%s]" , (const char*)blkname);
			throw	CApplicationException(-1 , s.msg , log.Location);
		}
		c_mat_kind	= bcls_rec->Tables[blkname].Rows[0]["MAT_KIND"].ToString();
		c_mat_no	= bcls_rec->Tables[blkname].Rows[0]["MAT_NO"].ToString();
		// 根据物料种类读取物料表名
		sqlstr	= " SELECT	CODE_DESC_2_CONTENT FROM TEP0002 WHERE CODE_CLASS = 'M002' AND CODE = @MAT_KIND ";
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("MAT_KIND" , c_mat_kind);
		cmd_inq.ExecuteReader();
		if	(cmd_inq.Read())
		{
			table_name = cmd_inq.GetString(1).Trim();
		}
		cmd_inq.Close();

		// 如果表名为空，报错
		if	(table_name.Trim() == "")
		{
			sprintf	(s.msg , "物料表名为空，物料类型=[%s]" , (const char *)c_mat_kind);
			throw	CApplicationException(-1, s.msg, log.Location);
		}

		// 按材料号读取物料管理封锁标记
		sqlstr = " SELECT HOLD_FLAG FROM " + table_name.Trim() + " where MAT_NO =  @mat_no " ; 
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set( "mat_no" , c_mat_no.Trim() ); 
		cmd_inq.ExecuteReader();
		if	(cmd_inq.Read())
		{
			c_hold_flag = cmd_inq.GetString(1).Trim();
		}
		cmd_inq.Close();

		// 判管理封锁标记是否为0，不为0就是有封锁
		if	(c_hold_flag != "0")
		{
			sprintf	(s.msg , "材料号[%s]的封锁标记为[%s]" , (const char*) c_mat_no , (const char*) c_hold_flag);
			throw	CApplicationException(-1 , s.msg , log.Location);
		}
		sprintf	(s.msg , "处理成功");
		
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
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

	return doFlag;

}
