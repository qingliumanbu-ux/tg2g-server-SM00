/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_cm_3000s4_snd
*  程序描述			: 热轧发货实绩电文发送
*  备注说明			:
*  修改历史			:
*  		wuxin 2011-12-29			(ADD)程序建立
*			... ...
* **************************************************************************** */
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除
#include "epex.h"



//名称空间引用




//外部函数声明
BM2_FUNCTION_EXPORT
 int f_sm00_bill_deliver_01(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/*程序用变量*/
	int doFlag = 0,i,blkNum,fetchRowCount,row_count=0,ret=0,j=0;

	/* 业务变量 */
	CString	datetime("");
	CString	blkname("");	/* 块名 */

	/* ***** 程序变量 ***** */
	CString c_user=s.userid,c_stock_no=" ",c_stacking_no=" ",c_rowid=" ",c_mat_no= "",c_factory_div = "";
	CString c_mat_kind = "";
		
	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr0(""),sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3(""),sqlstr4(""),sqlstr5(""),sqlstr6(""),sqlstr7(""),sqlstr8("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	CModel tsmpe11("TSMPE11");
	CModel tsmpe02("TSMPE02");
	/* ***** 创建电文处理对象 ***** */
	EPEX epex(&s);
	CString c_tc_no="";

	/* ***** 应用程序开始处理 ***** */
	try
	{

		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:

			sqlstr0 = CString(
				" SELECT MESSAGEID from messageconfig where memo LIKE '%发货实绩%' AND direction = 'S'  "
				);


			sqlstr1 = CString(
				" SELECT * FROM tsmpe11 where stacking_no = @stacking_no "
				);

			sqlstr2 = CString(
				" SELECT * FROM tsmpe02 where stacking_no = @stacking_no "
				);



			break;
		}

		//        /* *****获取电文号   ***** */
		//		sqlstr = sqlstr0;
		//		execute_sql.SetCommandText( sqlstr );
		//		execute_sql.ExecuteReader();
		//
		//		if	(execute_sql.Read())
		//		{
		//		 c_tc_no = execute_sql.GetString(1);
		//		}
		//		execute_sql.Close();

		//根据厂别设置电文号
		c_factory_div = bcls_rec->Tables[0].Rows[0]["factory_div"].ToString().TrimOrBlank();
		c_mat_kind = bcls_rec->Tables[0].Rows[0]["mat_kind"].ToString().TrimOrBlank();
		if(c_mat_kind == "CR")
		{
			c_tc_no = "4000S4";
		}
		if(c_mat_kind == "SM")
		{
			c_tc_no = "2000S4";
		}
		if(c_mat_kind == "HR")
		{
			c_tc_no = "3000S4";
		}
		if(c_mat_kind == "HP")
		{
			c_tc_no = "5000S4";
		}

		// 根据配置读取电文号 13801 2015-9-2
		sqlstr = "SELECT CODE_DESC_2_CONTENT FROM TEP0002 "
			" WHERE CODE_CLASS = 'SMPE00' AND CODE_DESC_5_CONTENT = '1' AND CODE = @tc_no ";
		cmd_inq.Parameters.Set("tc_no" , c_tc_no);
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.ExecuteReader();
		if ( cmd_inq.Read() )
		{
			c_tc_no = cmd_inq.GetString(1);
		}
		cmd_inq.Close();

		/* ***** 电文初始化  ***** */
		Log::Trace("" , __FUNCTION__ , "c_tc_no=[{0}]", c_tc_no );
		Log::Trace("" , __FUNCTION__ , "c_factory_div[{0}]" , c_factory_div); 
		Log::Trace("" , __FUNCTION__ , "c_mat_kind[{0}]" , c_mat_kind);
		ret = 0;
		ret = epex.Initialize(c_tc_no);
		if	(ret != 0)
		{
			sprintf	( s.msg , epex.GetMsg());
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			Log::Trace("" , __FUNCTION__ , "rows=[{0}]" , bcls_rec->Tables[0].Rows.get_Count() );

			c_stacking_no = bcls_rec->Tables[0].Rows[i]["stacking_no"].ToString().TrimOrBlank();

			Log::Trace("" , __FUNCTION__ , "c_stacking_no=[{0}]", c_stacking_no );

			if ( c_stacking_no == " " )
			{
				break;
			}

			/* *****获取码单信息   ***** */
			sqlstr = sqlstr1;
			execute_sql.Parameters.Set( "stacking_no", c_stacking_no);
			execute_sql.SetCommandText( sqlstr );
			execute_sql.ExecuteReader();

			while(execute_sql.Read())
			{
				execute_sql.Fetch(tsmpe11);
			}
			execute_sql.Close();

			/* ***** 电文赋值  ***** */
			tsmpe11["STACKING_WT"] = tsmpe11["STACKING_DISCREP_WT"].ToDecimal() + tsmpe11["STACKING_WT"].ToDecimal();
			ret = 0;
			ret = epex.SetValue(0 , tsmpe11);
			if	(ret != 0)
			{
				sprintf	( s.msg , epex.GetMsg());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			/* *****获取材料信息   ***** */
			sqlstr = sqlstr2;
			execute_sql.Parameters.Set( "stacking_no", c_stacking_no);
			execute_sql.SetCommandText( sqlstr );
			execute_sql.ExecuteReader();

			j = 0;
			while(execute_sql.Read())
			{
				execute_sql.Fetch(tsmpe02);
				/* ***** 组织材料信息  ***** */
				if(epex.SetValue("MAT_NO",j,tsmpe02["MAT_NO"].ToString())<0)
				{
					sprintf	( s.msg , epex.GetMsg());
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				if(epex.SetValue("MAT_WT",j,tsmpe02["MAT_WT"].ToDecimal())<0)
				{
					sprintf	( s.msg , epex.GetMsg());
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				j++;

			}
			execute_sql.Close();

			/* ***** 电文发送  ***** */
			Log::Debug("" , __FUNCTION__ , "电文发送前epex.SendTele");
			ret = 0;
			ret = epex.SendTele();
			if	(ret != 0)
			{
				sprintf	( s.msg , epex.GetMsg());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			Log::Debug("" , __FUNCTION__ , "电文发送后epex.SendTele");

		}



	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { _S("TSMPE02"), ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000017")/*读取数据失败,表[{0}],sqlcode=[{1}]。请联系系统维护人员。*/, arguments, 2);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Debug("" , __FUNCTION__ , "error=[{0}]", str );

		strncpy(s.sysmsg, (const char*)str, 399);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch(CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch(CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}
