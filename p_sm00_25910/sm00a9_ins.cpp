/* 程序对应表名    : tsm00a9
   程序对应表中文名: 按仓库授权表
   生成日期        : 2006-8-28 PM 14:10:11
   生成人          : 沈明琪   */
//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件

int f_sm00a9_ins(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);
/*<remark>=========================================================
/// <summary>
/// 发货用户授权表新增
/// <para>
/// 1.根据传入的新增记录，把该记录存到表里面。
/// 2.新增条件：主键不能为空，不能重复
/// </para>
/// <para>数据库表：TSMPEA9(发货用户授权表)         </para>
/// <para>主调用函数：前台SM00A9画面F3(用户新增)调用。   </para>
/// </summary>
/// <param> </param>
/// <returns> </returns>
===========================================================</remark>*/

// service入口
BM2F_ENTERACE(sm00a9_ins)
/* -EP_SYSTEM_HEAD_END */
int f_sm00a9_ins(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	/* 程序用变量 */
	int doFlag = 0;
	int fetchRowCount;
	int i;
	CModel tsmpea9("TSMPEA9");
	CDbCommand cmd_inq(conn);
	CString sqlstr;

	CTracer log(__FUNCTION__);
	try
	{
		/* 对输入信息循环处理 */
		for (i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++) {
			/* 取得单行传入信息 */

			tsmpea9["REC_CREATOR"] = s.userid;
			tsmpea9["REC_CREATE_TIME"] = CDateTime::Now().ToString("yyyyMMddHHmmss");
			tsmpea9["STOCK_NO"] = bcls_rec->Tables[0].Rows[i]["STOCK_NO"];
			tsmpea9["USER_ID"] = bcls_rec->Tables[0].Rows[i]["USER_ID"];
			tsmpea9["USER_NAME"] = bcls_rec->Tables[0].Rows[i]["USER_NAME"];
			tsmpea9["STOCK_DESC"] = bcls_rec->Tables[0].Rows[i]["STOCK_DESC"];
			tsmpea9["USER_NAME"] = tsmpea9["USER_NAME"].ToString().SubstringNE(0, 30).TrimOrBlank();
			int count = tsmpea9.QueryCount("STOCK_NO,USER_ID");
			if (count>0)
			{
				strcpy(s.msg, "库区号" + tsmpea9["STOCK_NO"].ToString() + "和操作者" + tsmpea9["USER_ID"].ToString() + "已存在");
				s.flag = -1;
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			sqlstr = "insert into tsmpea9";
			tsmpea9.TrimOrBlank();

			if (!tsmpea9.Insert())
			{
				strcpy(s.msg, "新增失败");
				s.flag = -1;
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}

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
