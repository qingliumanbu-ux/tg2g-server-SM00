/// <summary>
/// 功能说明：
/// <version>1.0.0.0</para>
/// <creator>13801 ShenMingQi</creator>
/// <history>2015-7-17 文件创建</history>
/// </summary>

/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2010
Author:   13801
Version:    3.0
Date:     2015-7-17
Description: 磅差后码单传MMS
**************************************************/

#include "stdafx.h"		// 框架头，不可删除

#include "tsmpe12.h"

//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明

/*<remark>=========================================================
/// <summary>
/// 准发材料及准发红冲请求查询
/// <para>
/// 到ED54表上读取显示的配置项；
/// 1.根据传入的合同号、发货计划号、材料号、准发计划号、运输方式、红冲标志查询出符合条件的材料。
/// 2.根据查询的材料到物料表上读取当前的库位信息。
/// </para>
/// </summary>
/// <returns>指定连铸机下的炉次制造命令信息</returns>
===========================================================</remark>*/

int	f_smbw_md_ok(EIClass * bcls_rec , EIClass * bcls_ret , CDbConnection * conn);		//棒线码单确认
int	f_smhp_md_ok(EIClass * bcls_rec , EIClass * bcls_ret , CDbConnection * conn);		//厚板码单确认

// Service 入口
BM2F_ENTERACE(sm0006_trace)

int f_sm0006_trace(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int ret = 0;
	int i=0;

	/* 业务变量 */
	CString	sqlstr("");              // 数据库SQL操作字符串
	CString	dateTime("");
	CString	blk_name = "MD_OK";			/* 定义传入的块名 */

	CString c_stacking_no = " ";
	CString c_mat_kind;

	/* 实体类定义 */
	CTSMPE12	tsmpe12(conn);

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	try
	{
		dateTime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		if ( !bcls_rec->Tables.Contains(blk_name) )
		{
			bcls_rec->Tables.Add(blk_name);
			bcls_rec->Tables[blk_name].Columns.Add(DT_STRING , "stacking_no");
		}

		c_stacking_no = bcls_rec->Tables[0].Rows[0]["stacking_no"];
		c_mat_kind	= bcls_rec->Tables[0].Rows[0]["mat_kind"];

		bcls_rec->Tables[blk_name].Rows.Add();
		bcls_rec->Tables[blk_name].Rows[0]["stacking_no"] = c_stacking_no;

		if	(c_mat_kind == "BW")
		{
#ifdef _LINE_BW
			ret = f_smbw_md_ok(bcls_rec , bcls_ret , conn);
			if ( ret != 0 )
			{
				throw CApplicationException(-1 , s.msg , s.svc_name);
			}
#endif
		}
		else if	(c_mat_kind == "HP")
		{
#ifdef _LINE_HP
			ret = f_smhp_md_ok(bcls_rec , bcls_ret , conn);
			if ( ret != 0 )
			{
				throw CApplicationException(-1 , s.msg , s.svc_name);
			}
#endif
		}
		else
		{
			sprintf(s.msg , "此物料不能执行码单上传");
			throw CApplicationException(-1 , s.msg , s.svc_name);
		}
		sprintf	(s.msg , "上传成功" );
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = {  ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("" , __FUNCTION__ , "error=[{0}]", str);

		strncpy(s.sysmsg, (const char*)str, sizeof(s.msg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch(CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg)-1);
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
