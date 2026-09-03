/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_sm00_count
*  程序描述			: 材料状态计算
*  备注说明			:
*  修改历史			:
*  		2008-8-11 	BM2IDE			(ADD)程序建立
*		2011-04-14	013801		程序精简，想去除表HSMPE00 , HSMPE01 , HSMPE02
*			... ...
* **************************************************************************** */
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除

//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明
BM2_FUNCTION_EXPORT
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection * conn) 
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/*程序用变量*/
	int doFlag = 0,i,blkNum,fetchRowCount;

	/* 业务变量 */
	CString	datetime("");
	CString	blkname("");	/* 块名 */

	 /* ***** 程序变量 ***** */
   CString c_user=" ",c_tc_no=" ";

   /* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3(""),sqlstr4(""),sqlstr5(""),sqlstr6(""),sqlstr7(""),sqlstr8("");                    
	
   /* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn);

/* ***** 应用程序开始处理 ***** */
	try
	{
	
	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	/* ***** 查找没有材料的准发计划  ***** */ 
	switch(conn->DatabaseKind)
	{
	case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
	case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
	case DB_KIND_MSSQL:	        // MS SQL Server数据库
	case DB_KIND_ORACLE:	        // Oracle 数据库
	default: 
	/* ***** 计划号 计划量的计算 ***** */
	sqlstr = CString(   /* 如果在材料档里没有某计划的材料，那么，该计划的状态应该置为 '9',计划量置为 0 */
			" update tsmpe01 a set confm_status = '9', "
								" rec_revise_time = @datetime, "
								" rec_revisor     = @c_user, "
								" plan_num        = 0, "
								" plan_wt         = 0 "
								" where not exists ( select 1 "
								"                       from  tsmpe02 b " 
								"                       where a.confm_plan_no = b.confm_plan_no) "
								" and a.confm_plan_no = @confm_plan_no "

									);	

	sqlstr1 = CString( /* 如果在发货材料档里没有某计划的材料，那么，该计划的发货量置为 0 */
			" update tsmpe01 a set "
								" delivy_num        = 0, "
								" delivy_wt         = 0 "
								" where not exists ( select 1 "
								"                       from  tsmpe12 b " 
								"                       where a.confm_plan_no = b.confm_plan_no) "
								" and a.confm_plan_no = @confm_plan_no "

									);	

	sqlstr2 = CString(
			" insert into hsmpe01 select * from tsmpe01 where confm_plan_no = @confm_plan_no and confm_status = '9' " 

									);	

	sqlstr3 = CString(
			" delete from tsmpe01 where confm_plan_no = @confm_plan_no and confm_status = '9' " 

									);	

	sqlstr4 = CString( /* 计算某计划下的计划量，发货量 */
		              "  update  tsmpe01 a set rec_revise_time = @datetime, "
					  "                        rec_revisor     = @c_user, "
					  " 											 (plan_num,plan_wt) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe02 b where a.confm_plan_no = b.confm_plan_no ), "
	                  "                (delivy_num,delivy_wt ) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe12 b where a.confm_plan_no = b.confm_plan_no ) "
					  "                    where confm_plan_no = @confm_plan_no "
		 
									); 

	/* ***** 单据号 计划量的计算 ***** */
	sqlstr5 = CString(/* 计算某计划下的所有单据的计划量及发货量 */
		"  update  tsmpe00 a set rec_revise_time = @datetime, rec_revisor     = @c_user, "
		"  (plan_num,plan_wt) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe02 b where a.confm_plan_no = @confm_plan_no AND a.ready_bill_no = b.ready_bill_no ), "
		"  (delivy_num,delivy_wt ) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe12 b where a.confm_plan_no = @confm_plan_no AND a.ready_bill_no = b.ready_bill_no ) "
		"  where confm_plan_no = @confm_plan_no "

		);

	sqlstr6 = CString(/* 如果某计划的计划量为0，那么，该计划状态为 '9',否则，依旧；计划总量 = 计划量 + 发货量  */
		              "  update  tsmpe00 a set confm_status = decode(plan_num,0,'9',confm_status),total_mat_num = plan_num + delivy_num,total_mat_wt = plan_wt + delivy_wt " 
					  "                    where confm_plan_no = @confm_plan_no "
		 
									); 
//	sqlstr9 = CString(/* 计算准发计划的状态 */
//		              "  update  tsmpe01 a set  "
//					  "                        CONFM_STATUS = '9',    "
//					  "                    where confm_plan_no = @confm_plan_no "
//					  "                      and not exists ( select 1  "
//					  "                                         from tsmpe02 b "
//					  "                                         where a.confm_plan_no = b.confm_plan_no ) "
//		 
//									); 
	
	sqlstr7 = CString(
			" insert into hsmpe00 select * from tsmpe00 where confm_plan_no = @confm_plan_no and confm_status = '9' " 

									);	

	sqlstr8 = CString(
			" delete from tsmpe00 where confm_plan_no = @confm_plan_no and confm_status = '9' " 

									);	

	break; 
	}  

	sqlstr = sqlstr;
	execute_sql.SetCommandText( sqlstr );
	execute_sql.Parameters.Clear();
	execute_sql.Parameters.Set( "datetime" , datetime );  
	execute_sql.Parameters.Set( "c_user" , p_userid );
	execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
	execute_sql.ExecuteNonQuery(); 

	sqlstr = sqlstr1;
	execute_sql.SetCommandText( sqlstr );
	execute_sql.Parameters.Clear();
	execute_sql.Parameters.Set( "datetime" , datetime );  
	execute_sql.Parameters.Set( "c_user" , p_userid );
	execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
	execute_sql.ExecuteNonQuery(); 

//	sqlstr = sqlstr2;
//	execute_sql.SetCommandText( sqlstr );
//	execute_sql.Parameters.Clear();
//	execute_sql.Parameters.Set( "datetime" , datetime );  
//	execute_sql.Parameters.Set( "c_user" , p_userid );
//	execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
//	execute_sql.ExecuteNonQuery(); 

 //   sqlstr = sqlstr3;
	//execute_sql.SetCommandText( sqlstr );
	//execute_sql.Parameters.Clear();
	//execute_sql.Parameters.Set( "datetime" , datetime );  
	//execute_sql.Parameters.Set( "c_user" , p_userid );
	//execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
	//execute_sql.ExecuteNonQuery(); 

    sqlstr = sqlstr4;
	execute_sql.SetCommandText( sqlstr );
	execute_sql.Parameters.Clear();
	execute_sql.Parameters.Set( "datetime" , datetime );  
	execute_sql.Parameters.Set( "c_user" , p_userid );
	execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
	execute_sql.ExecuteNonQuery(); 

	sqlstr = sqlstr5;
	execute_sql.SetCommandText( sqlstr );
	execute_sql.Parameters.Clear();
	execute_sql.Parameters.Set( "datetime" , datetime );  
	execute_sql.Parameters.Set( "c_user" , p_userid );
	execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
	execute_sql.ExecuteNonQuery(); 

	sqlstr = sqlstr6;
	execute_sql.SetCommandText( sqlstr );
	execute_sql.Parameters.Clear();
	execute_sql.Parameters.Set( "datetime" , datetime );  
	execute_sql.Parameters.Set( "c_user" , p_userid );
	execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
	execute_sql.ExecuteNonQuery(); 

//	sqlstr = sqlstr7;
//	execute_sql.SetCommandText( sqlstr );
//	execute_sql.Parameters.Clear();
//	execute_sql.Parameters.Set( "datetime" , datetime );  
//	execute_sql.Parameters.Set( "c_user" , p_userid );
//	execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
//	execute_sql.ExecuteNonQuery(); 

	//sqlstr = sqlstr8;
	//execute_sql.SetCommandText( sqlstr );
	//execute_sql.Parameters.Clear();
	//execute_sql.Parameters.Set( "datetime" , datetime );  
	//execute_sql.Parameters.Set( "c_user" , p_userid );
	//execute_sql.Parameters.Set( "confm_plan_no" , p_confm_plan_no );
	//execute_sql.ExecuteNonQuery();  

	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { _S("TPSSMD1"), ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000017")/*读取数据失败,表[{0}],sqlcode=[{1}]。请联系系统维护人员。*/, arguments, 2);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		//EDLog(1,1, "error=[%s]", (const char*)str );

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
