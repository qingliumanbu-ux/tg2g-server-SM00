/*
程序名称:		cm_0000sd_rcv
隶属子系统:		SM00
产品名称:		PES
功能描述:		更新发货材料表上合同号
外部接口:		无
相关数据库表:
无
主要逻辑说明:	根据材料号更新材料表上的合同号
备注:
修改历史:
修改人			修改日期		内容
BM2IDE	2014-12-3		当前程序被创建。
*/
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除
#include "epex.h"


//名称空间引用




/*  函数申明  */

BM2F_ENTERACE_TELE( cm_0000sd_rcv )

int f_cm_0000sd_rcv( EIClass * bcls_rec , EIClass * bcls_ret , CDbConnection * conn )
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义
	/*定义函数名*/

	/*程序用变量*/
	int		i = 0;
	int		doFlag = 0;

	CModel tsmpe02("TSMPE02");

	/*在SQL语句中使用的变量*/
	CString c_userid= "";
	CString	sqlstr("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql( conn );

	try
	{

		/*获得传入参数*/
		c_userid= s.userid;

		//	读取传入的参数
		int	rows = bcls_rec->Tables[0].Rows.get_Count();
		for	( i = 0 ; i < rows ; i++)
		{
			Log::Trace("" , __FUNCTION__ , "第[{0}]条记录，共[{1}]条记录", i+1 , rows);
			if	(bcls_rec->Tables[0].Columns.Contains("MAT_NO"))
			{
				tsmpe02["MAT_NO"] = bcls_rec->Tables[0].Rows[i]["MAT_NO"] ;
			}
			else
			{
				sprintf	(s.msg , "没有传入材料号参数");
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			if	(bcls_rec->Tables[0].Columns.Contains("ORDER_NO"))
			{
				tsmpe02["ORDER_NO"] = bcls_rec->Tables[0].Rows[i]["ORDER_NO"];
			}
			else
			{
				sprintf	(s.msg , "没有传入合同号参数");
				throw	CApplicationException(-1, s.msg, s.svc_name);
			}

			Log::Trace( "" , __FUNCTION__ , "材料号=[{0}] , 合同号[{1}]" , tsmpe02["MAT_NO"], tsmpe02["ORDER_NO"]);

			/* ***** 检查输入参数合法性 ***** */
			tsmpe02.TrimOrBlank();
			if (tsmpe02["MAT_NO"].ToString().Trim() =="")
			{
				strcpy(s.msg,"材料号不能为空");
				strcpy(s.sysmsg,"材料号不能为空!");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			sqlstr = " UPDATE TSMPE02 SET ORDER_NO = @tsmpe02.ORDER_NO WHERE MAT_NO ='"+ tsmpe02["MAT_NO"].ToString()+"' ";
			if	(tsmpe02.Update("ORDER_NO","MAT_NO") == 0)
			{
				sprintf	(s.msg , "更新发货材料表记录出错，无此材料号[%s]" , (const char*)tsmpe02["MAT_NO"].ToString());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			sprintf( s.msg , "处理成功！" );
		}
	}

	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.msg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
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


	//返回-1时事务将回滚，返回为0是事务将提交
	return doFlag;

}
