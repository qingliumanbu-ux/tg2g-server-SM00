/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************	
*  程序名称			: sm0000_stock 隶属:SM00
*  程序描述			: 发货库号查询
*  备注说明			: 9000
*  修改历史			: 		
*  		2011-12-15 	吴新			(ADD)程序建立
*			... ...
* ***********************************/
#include "stdafx.h"
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;   
//#include "tsmpe01.h"


int f_sm0000_stock(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);

/*<remark>=========================================================
/// <summary>
/// 发货库号查询
/// <para>
/// 1.根据前台传过来的用户id，从数据库查询出来库区号
/// </para>
/// <para>数据库表：TSMPEA9(发货用户授权表)         </para>
/// <para>主调用函数：前台SM0001，SM0002，SM0004，SM00A8，SM00A9画面调用。   </para>
/// </summary>
/// <param name="c_user">用户id    </param>
/// <returns>库区号和库区描述</returns>
===========================================================</remark>*/

BM2F_ENTERACE(sm0000_stock)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_sm0000_stock(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0;

	/* ***** 程序变量 ***** */
	CString c_user=" ",c_mat_kind=" ",c_tc_no=" ";
	CString	c_factory_div = " ";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3("");    

	/* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn);

	/* ***** 给返回块定义列名 ***** */
	//bcls_ret->Tables[0].Columns.Add( DT_STRING, "STOCK_NO");
	//bcls_ret->Tables[0].Columns.Add( DT_STRING, "STOCK_DESC");


	/* ***** 应用程序开始处理 ***** */
	try
	{
		/* ***** 获取前台参数  ***** */
		if (bcls_rec->Tables[0].Columns.Contains("user_id"))	c_user = bcls_rec->Tables[0].Rows[0]["USER_ID"].ToString().TrimOrBlank();
		if (bcls_rec->Tables[0].Columns.Contains("factory_div"))	c_factory_div = bcls_rec->Tables[0].Rows[0]["factory_div"].ToString().TrimOrBlank();
		if (bcls_rec->Tables[0].Columns.Contains("mat_kind"))	c_mat_kind = bcls_rec->Tables[0].Rows[0]["mat_kind"].ToString().TrimOrBlank();

		CString c_part = "";
		if (bcls_rec->Tables[0].Columns.Contains("PART"))	c_part = bcls_rec->Tables[0].Rows[0]["PART"].ToString().TrimOrBlank();
		Log::Debug("", "", "part=[{0}]", c_part);
		Log::Debug("", "", "factory_div=[{0}]", c_factory_div);
		Log::Debug("", "", "c_user=[{0}]", c_user);

		if (c_mat_kind.Trim() == "")	c_mat_kind = "%";


		CString code = "";
		sqlstr = "select CODE_DESC_1_CONTENT  from tep0002 where code_class = 'SM99' "
			" AND CODE = '" + c_part + "' ";
		execute_sql.SetCommandText(sqlstr);
		execute_sql.ExecuteReader();
		if (execute_sql.Read())
		{
			code = execute_sql.GetString(1);
		}
		execute_sql.Close();
		if (code != "1")
		{
			c_user = "admin";	// 测试期间，跳过权限
		}

		CString c_trnp_mode_code = "";
		if (bcls_rec->Tables[0].Columns.Contains("TRNP_MODE_CODE"))
		{
			c_trnp_mode_code = bcls_rec->Tables[0].Rows[0]["TRNP_MODE_CODE"].ToString().TrimOrBlank();
		}


		Log::Debug("", "", "user_id=[{0}],mat_kind=[{1}]", c_user, c_mat_kind);
		/* ***** 获取库区号  ***** */
		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
			sqlstr = CString(
				" select distinct stock_no,stock_desc from twm01 where user_id = decode( @user_id,'admin',user_id,@user_id ) "
				" AND  MAT_KIND LIKE @c_mat_kind "
				" order by stock_no "
				); 

			break; 
		}

		sqlstr = "select distinct stock_no,stock_desc from twm01 a where 1=1 ";

		if (c_user != "admin" && c_user.Find("admin") == -1)
		{
			sqlstr += " AND A.user_id = '" + c_user + "' ";
		}
		if (c_mat_kind.Trim() != "" && c_mat_kind.Trim() != "%")
		{
			sqlstr += " AND (A.MAT_LINE_TYPE = '" + c_mat_kind.Trim() + "'";
			sqlstr += " OR A.MAT_KIND = '" + c_mat_kind.Trim() + "') ";
		}
		if (c_factory_div.Trim() != "")
		{
			sqlstr += " AND A.FACTORY_DIV = '" + c_factory_div.Trim() + "'";
		}
		//if (c_part.Trim() != "")
		//{
		//	sqlstr += " AND EXISTS ( SELECT 1 FROM TSI0021 B WHERE A.STOCK_NO = B.STOCK_NO AND B.AREA_CODE IN (' ', @part ) )";
		//	execute_sql.Parameters.Set("part", c_part);
		//}
		if (c_trnp_mode_code == "2")
		{
			sqlstr += " AND EXISTS (SELECT 1 FROM TEP0002 B WHERE B.CODE_CLASS = 'SMAG01' AND  B.CODE_DESC_2_CONTENT LIKE '%' || A.STOCK_NO || '%') ";
		}
		sqlstr += " order by A.stock_no ";


			/* ***** 执行SQL   ***** */
		sqlstr = sqlstr;
		execute_sql.SetCommandText( sqlstr );
		execute_sql.Parameters.Clear(); 
		execute_sql.Parameters.Set("user_id",c_user);
		execute_sql.Parameters.Set("c_mat_kind", c_mat_kind);
		Log::Debug("", "", "sqlstr==={0}", sqlstr);
		Log::Debug("", "", "user_id=[{0}],mat_kind=[{1}]", c_user, c_mat_kind);

		execute_sql.ExecuteQuery(bcls_ret->Tables[0]);

	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{ 
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;  
	}
	catch(const CApplicationException& ex)
	{
		//	strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应 
		Log::Error("" , __FUNCTION__ , "error=[{0}]", s.msg );  
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1; 
	}

	catch(const CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1; 
	}

	return doFlag;
}
