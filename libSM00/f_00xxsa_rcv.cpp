/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_0070sa_rcv
*  程序描述			: 准发红冲确认电文处理
*  备注说明			: 
*  修改历史			:
*  		2023-2-10	BM2IDE			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"
#include "epex.h"


  


  

int f_sm00_record(EIClass *bcls_rec,EIClass *bcls_ret,CDbConnection * conn);
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection * conn);
int f_sm00_mm99(CString pack_num , int para_type , int event_id ,CString msg,CDbConnection * conn);	/* 抛物料跟踪打包函数 */

//// service入口
//BM2F_ENTERACE_TELE(cm_0070sa_rcv)
	/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_00xxsa_rcv(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn)
{
	CTracer log(__FUNCTION__);
	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, i = 0,ret=0;
	CString	record_name = "sm00_record";

	CString lpsz_user_id,c_datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	CString	lpsz_out_div;
	EIClass sm_bcls_rec;  

	CModel tsmpe02("TSMPE02");
	CModel tsmpe01("TSMPE01");
	CModel tsmpe00("TSMPE00");

   /* ***** 电文变量定义 ***** */ 
	CString    c_mat_no ;
	CString    c_ready_bill_no ;
	CString    c_order_no ;
	CString    c_red_cause_desc ;
	CString    c_rec_revisor ;
	CString    c_rec_revise_time ;
	CString    c_red_flag ;	// 0--红冲请求，1--红冲确认，2--红冲请求取消


   /* ***** 程序变量 ***** */
   CString c_user=" ",c_tc_no=" ";

   /* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3(""),sqlstr4(""),sqlstr5("");   
	
   /* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn);

   /* ***** 应用程序开始处理 ***** */
	try
	{

		/* ***** 获取电文号 ***** */
		c_tc_no = s.username;
		c_user = c_tc_no;

		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}

		/* ***** 解析电文 ***** */
		c_mat_no = bcls_rec->Tables[0].Rows[0]["mat_no"].ToString().TrimOrBlank();
		c_ready_bill_no = bcls_rec->Tables[0].Rows[0]["ready_bill_no"].ToString().TrimOrBlank();
		c_order_no = bcls_rec->Tables[0].Rows[0]["order_no"].ToString().TrimOrBlank();
		c_red_cause_desc = bcls_rec->Tables[0].Rows[0]["red_cause_desc"].ToString().TrimOrBlank();
		c_rec_revisor = bcls_rec->Tables[0].Rows[0]["rec_revisor"].ToString().TrimOrBlank();
		c_rec_revise_time = bcls_rec->Tables[0].Rows[0]["rec_revise_time"].ToString().TrimOrBlank();
		c_red_flag = bcls_rec->Tables[0].Rows[0]["red_flag"].ToString().TrimOrBlank();


		if (c_mat_no.Compare(" ") == 0)
		{
			doFlag = -1;
			strcpy(s.msg, _RES("SM00S0000760")/*接收材料号为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (c_red_flag.Trim() != "0" && c_red_flag.Trim() != "1" && c_red_flag.Trim() != "2")
		{
			doFlag = -1;
			sprintf(s.msg, "红冲标记出错[%s]，不为0，1，2",(const char *)c_red_flag);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr = CString(
				" select count(1) from tsmpe02 where mat_no = @mat_no "
				);

			sqlstr1 = CString(
				" select * from tsmpe02 where mat_no = @mat_no "
				);

			sqlstr2 = CString(
				" delete from tsmpe02 where mat_no = @mat_no  "
				);

			sqlstr3 = CString(
				" UPDATE	tsmpe02 "
				"				SET	    rec_revise_time = to_char(sysdate,'yyyymmddhh24miss')  "
				"				   ,	rec_revisor		= @c_user  "
				"						,	red_flag		= ' '   "
				"						,	red_cause_code	= ' '   "
				"						,	red_cause_desc	= ' '   "
				"						,	dept_code		= ' '   "
				"						,	dept_code_cname	= ' '   "
				"						,	red_maker		= ' '   "
				"					WHERE	mat_no = @mat_no        "
				);


			break;
		}

		/**************** 判断材料号是否合法 **********************/
		sqlstr = sqlstr;
		execute_sql.SetCommandText(sqlstr);
		execute_sql.Parameters.Clear();
		execute_sql.Parameters.Set("mat_no", c_mat_no);
		row_count = execute_sql.ExecuteScalar().ToInt32();

		if (0 == row_count)
		{
			doFlag = -1;
			strcpy(s.msg, _RES("SM00S0000762")/*接收红冲确认电文材料号在准发材料表中不存在.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		sqlstr = sqlstr1;
		execute_sql.SetCommandText(sqlstr);
		execute_sql.Parameters.Clear();
		execute_sql.Parameters.Set("mat_no", c_mat_no);
		execute_sql.ExecuteReader();
		while (execute_sql.Read())
		{
			execute_sql.Fetch(tsmpe02);
		}
		execute_sql.Close();

		if (tsmpe02["CONFM_STATUS"].ToString().Compare("4") != 0)
		{
			doFlag = -1;
			strcpy(s.msg, _RES("SM00S0000763")/*要红冲的材料状态不对.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		/**************** 红冲逻辑开始编写  **********************/
		// 红冲请求确认
		if (c_red_flag.Compare("1") == 0)
		{
			/**************** 调用物料封装  函数*********************/
			if (f_sm00_mm99(c_mat_no, 3, -5, s.msg, conn) != 0)
			{
				doFlag = -1;
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			/**************** 调用物料封装  函数结束  **********************/

			bcls_rec->Tables[record_name].Rows[0]["mat_no"] = c_mat_no;
			bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "3";
			bcls_rec->Tables[record_name].Rows[0]["userid"] = c_rec_revisor;

			ret = 0;
			ret = f_sm00_record(bcls_rec, bcls_ret, conn);
			if (ret < 0)
			{
				Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			/* *****	删除材料信息*********************************************** */
			sqlstr = sqlstr2;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("mat_no", c_mat_no);
			execute_sql.ExecuteNonQuery();
			//更新准发计划表,准发单据表计划重量 材料总重量
			doFlag = f_sm00_count(tsmpe02["CONFM_PLAN_NO"].ToString(), c_user, conn);
			if (doFlag < 0)
			{

				doFlag = -1;
				sprintf(s.msg, "f_sm00_count函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}

		// 红冲请求删除
		if (c_red_flag.Compare("2") == 0)
		{

			/* *****	修改信息*********************************************** */
			sqlstr = sqlstr3;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("mat_no", c_mat_no);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.ExecuteNonQuery();
		}

		// 红冲请求
		if (c_red_flag.Trim() == "0")
		{
			sqlstr = "UPDATE TSMPE02 SET RED_FLAG = '1' ,red_cause_desc = '" + c_red_cause_desc + "' "
				" WHERE MAT_NO = '" + c_mat_no + "' ";
			execute_sql.SetCommandText(sqlstr);
			Log::Debug("", "", "sqlstr={0}", sqlstr);
			if (execute_sql.ExecuteNonQuery() == 1)
			{
				bcls_rec->Tables[record_name].Rows[0]["mat_no"] = c_mat_no;
				bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "2";
				bcls_rec->Tables[record_name].Rows[0]["userid"] = c_rec_revisor;

				ret = 0;
				ret = f_sm00_record(bcls_rec, bcls_ret, conn);
				if (ret < 0)
				{
					Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
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
	catch(const CApplicationException& ex)
	{
	//	strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		Log::Error("" , __FUNCTION__ , "error=[{0}]", s.msg );  
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
