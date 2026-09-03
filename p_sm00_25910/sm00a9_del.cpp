/* 程序对应表名    : tsm00a9
   程序对应表中文名: 按仓库授权表
   生成日期        : 2006-8-28 PM 14:10:11
   生成人          : 沈明琪   */
//框架公用头文件，勿删
#include "stdafx.h"




//程序用头文件

int f_sm00a9_del(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);
/*<remark>=========================================================
/// <summary>
/// 发货用户授权表删除
/// <para>
/// 1.根据传入的记录，把该记录从表中删除掉。
/// </para>
/// <para>数据库表：TSMPEA9(发货用户授权表)         </para>
/// <para>主调用函数：前台SM00A9画面F5(用户删除)调用。   </para>
/// </summary>
/// <returns> </returns>
===========================================================</remark>*/
// service入口

BM2F_ENTERACE(sm00a9_del)
/* -EP_SYSTEM_HEAD_END */
int f_sm00a9_del(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
  /* 程序用变量 */
  int doFlag = 0;
  int fetchRowCount;
  int i;
  /* 在SQL语句中使用的变量 */
  // EXEC SQL BEGIN DECLARE SECTION;
  CString stock_no;
  CString ename;
  // EXEC SQL END DECLARE SECTION;
  /* 使用的表结构变量 */
  // EXEC SQL INCLUDE tsmpea9.h;
  //	读取输入参数
	bcls_rec-> GetSYS(&s);
 		//获取一些系统信息
  /* 设置出错处理 */
  // EXEC SQL WHENEVER SQLERROR GOTO l_sqlerror;
  // EXEC SQL
    // ALTER SESSION SET NLS_DATE_FORMAT = 'YYYYMMDDhh24miss';
	CModel tsmpea9("TSMPEA9");
CDbCommand cmd_inq(conn);
CString sqlstr;

CTracer log(__FUNCTION__);
try
{
  /* 对输入信息循环处理 */
  for (i = 1; i <= bcls_rec->Tables[0].Rows.get_Count(); i++ ) {
    /* 取得单行传入信息 */
    //tsmpea9.MergeFrom(bcls_rec->Tables[0].Rows[i-1]);
		/*tsmpea9["REC_CREATOR"] = s.userid;
		tsmpea9["REC_CREATE_TIME"]=CDateTime::Now().ToString("yyyyMMddhhmmss");*/
		tsmpea9["STOCK_NO"] = bcls_rec->Tables[0].Rows[i-1]["STOCK_NO"];
		tsmpea9["USER_ID"] = bcls_rec->Tables[0].Rows[i-1]["USER_ID"];
		int rowAffected = tsmpea9.Delete("STOCK_NO,USER_ID");
		if (rowAffected < 0)
		{
			strcpy(s.msg,"删除失败");
			s.flag = -1;
			throw CApplicationException(-1,s.msg,s.svc_name);
		}
  }

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
