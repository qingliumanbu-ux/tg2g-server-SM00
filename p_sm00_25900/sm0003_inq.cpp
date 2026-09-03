/// <summary>
/// 功能说明：
/// <version>1.0.0.0</para>
/// <creator>13801 ShenMingQi</creator>
/// <history>2013-06-06 文件创建</history>
/// </summary>

/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2010
Author:   13801
Version:    3.0
Date:     2013-06-06
Description: 准发材料及准发红冲请求查询
**************************************************/

#include "stdafx.h"		// 框架头，不可删除



//名称空间引用




//外部函数声明
//int f_edsetcustominfo(EIClass * bcls_rec, EIClass * bcls_ret);

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


// Service 入口
BM2F_ENTERACE(sm0003_inq)



int f_sm0003_inq(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int ret = 0;
	int blknum = 0;
	int count = 0;
	int blkseq = 0;

	/* 业务变量 */
	CString	sqlstr("");              // 数据库SQL操作字符串
	CString	dateTime("");
	CString	blkname("");

	CString c_factory_div = " ";
	CString c_mat_kind = " ";
	CString c_order_no = " ";
	CString c_bill_of_lading_no = " ";
	CString c_mat_no = " ";
	CString c_red_flag = " ";
	CString c_userid = " ";
	CString c_plan_no = "";
	CString c_trnp_mode_code = " ";
	CString c_export_flag = "";
	CString c_stock_no = "";


	/* 实体类定义 */
	CModel tsmpe02("TSMPE02");

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	try
	{
		dateTime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		c_userid = s.userid;

		///* 自定义查询显示项读取 */
		//EIClass	in01;
		//in01.Tables[0].Columns.Add(DT_STRING,"function_id");	// 传入的功能名
		//in01.Tables[0].Rows.Add();   //新增空行
		//in01.Tables[0].Rows[0]["function_id"] = "SM0003_INQ";	// 在EDA2中定义
		//bcls_ret->blk_now = 1;										// 返回的块号
		//ret = f_edsetcustominfo( &in01, bcls_ret );			// 读取字段名
		//if	(ret != 0)
		//{
		//	sprintf	(s.sysmsg, "调用函数f_edsetcustominfo出错[%s]" , s.msg);
		//	Log::Error("" , __FUNCTION__ , s.sysmsg);
		//	throw	CApplicationException(-1, s.msg, log.Location);
		//}

		//取分区函数
		CString pathvar;
		pathvar = getenv("BM2_PART_NAME");
		Log::Info("", "", "pathvar = [{0}]", pathvar);

		//后台service获取分区号
		CString pathvar_1 = getenv("BM2_PART_NAME");
		CString pathvar_2 = getenv("BM2_BASE_DIR");
		int len = 0;
		Log::Debug("", __FUNCTION__, "pathvar_1={0}", pathvar_1);
		Log::Debug("", __FUNCTION__, "pathvar_2={0}", pathvar_2);

		/* 读取传入的参数 */
		c_order_no			= bcls_rec->Tables[0].Rows[0]["ORDER_NO"];	// 按字段名称读取
		c_bill_of_lading_no	= bcls_rec->Tables[0].Rows[0]["BILL_OF_LADING_NO"];
		c_mat_no			= bcls_rec->Tables[0].Rows[0]["MAT_NO"];
		c_plan_no			= bcls_rec->Tables[0].Rows[0]["PLAN_NO"];
		c_red_flag			= bcls_rec->Tables[0].Rows[0]["RED_FLAG"];
		c_factory_div       = bcls_rec->Tables[0].Rows[0]["FACTORY_DIV"];
		c_mat_kind          = bcls_rec->Tables[0].Rows[0]["MAT_KIND"];
		c_stock_no          = bcls_rec->Tables[0].Rows[0]["STOCK_NO"];

		/* 打印传入的参数 */
		Log::Info("" , __FUNCTION__ , "ORDER_NO = [{0}]"			, c_order_no);
		Log::Info("" , __FUNCTION__ , "BILL_OF_LADING_NO = [{0}]"	, c_bill_of_lading_no);
		Log::Info("" , __FUNCTION__ , "MAT_NO = [{0}]"				, c_mat_no);
		Log::Info("" , __FUNCTION__ , "PLAN_NO = [{0}]"				, c_plan_no);
		Log::Info("" , __FUNCTION__ , "RED_FLAG = [{0}]"			, c_red_flag);
		Log::Info("", __FUNCTION__, "FACTORY_DIV = [{0}]", c_factory_div);
		Log::Info("", __FUNCTION__, "MAT_KIND = [{0}]", c_mat_kind);

		/* 数据库操作 */
		CDbCommand cmd_inq(conn);
		CDbCommand cmd_inq1(conn);

		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr	=	" SELECT	*  "
						" FROM		tsmpe02  "
						" WHERE		ORDER_NO		LIKE	(@c_order_no) || '%' "
						" AND		MAT_NO			LIKE	(@c_mat_no ) || '%' "
						" AND		CONFM_PLAN_NO	LIKE	(@c_plan_no) || '%' "
						" AND		RED_FLAG		LIKE	(@c_red_flag) || '%' "
						" AND		BILL_OF_LADING_NO LIKE	(@c_bill_of_lading_no) || '%' "
						" AND		STOCK_NO LIKE	(@c_stock_no) || '%' ";
			break;
		}
		if (c_mat_kind.Trim() != "")
		{
			sqlstr += " AND  MAT_KIND = '" + c_mat_kind + "' ";
		}


		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("c_order_no", c_order_no.Trim());	// SQL语句中的变量赋值
		cmd_inq.Parameters.Set("c_mat_no", c_mat_no.Trim());
		cmd_inq.Parameters.Set("c_plan_no", c_plan_no.Trim());
		cmd_inq.Parameters.Set("c_red_flag", c_red_flag.Trim());
		cmd_inq.Parameters.Set("c_bill_of_lading_no", c_bill_of_lading_no.Trim());
		cmd_inq.Parameters.Set("c_stock_no", c_stock_no.Trim());
		cmd_inq.ExecuteQuery(bcls_ret->Tables[blknum]);
		if (!bcls_ret->Tables[blknum].Columns.Contains("STOCK_PLACE_NO"))
			bcls_ret->Tables[blknum].Columns.Add(DT_STRING, "STOCK_PLACE_NO");
		for (int i = 0; i < bcls_ret->Tables[blknum].Rows.get_Count(); i++)
		{
			tsmpe02.MergeFrom(bcls_ret->Tables[blknum].Rows[i]);
			if	(tsmpe02["MAT_KIND"].ToString().Trim() != "")
			{
				/* 读取物料表名 */
				#ifndef __MAT_KIND__
				#define __MAT_KIND__
				CString LS_MAT_KIND = "" ;
				CString table_name = "";
				#endif 

				if	(LS_MAT_KIND != tsmpe02["MAT_KIND"].ToString())
				{
					f_epep_get_tep0002( *bcls_rec , "M002" , (const char *)tsmpe02["MAT_KIND"], NULL, NULL, NULL, NULL, NULL);
					if	(bcls_rec->Tables["TEP0002"].Columns.Contains("CODE_DESC_2_CONTENT") )
					{
						table_name = bcls_rec->Tables["TEP0002"].Rows[0]["CODE_DESC_2_CONTENT"];
						LS_MAT_KIND = tsmpe02["MAT_KIND"].ToString() ;
					}
				}

				sqlstr = " SELECT STOCK_PLACE_NO FROM " + table_name + " WHERE MAT_NO = @tsmpe02.MAT_NO ";

				cmd_inq1.SetCommandText(sqlstr);
				cmd_inq1.Parameters.Set("tsmpe02.MAT_NO" , tsmpe02["MAT_NO"].ToString());
				cmd_inq1.ExecuteReader();
				if	(cmd_inq1.Read())
				{
					bcls_ret->Tables[blknum].Rows[i]["STOCK_PLACE_NO"] = cmd_inq1.GetString(1);
				}
				cmd_inq1.Close();
			}
		}

		//cmd_inq.ExecuteReader();
		//
		///* 循环从游标中读取记录，压回前台 */
		//count = 0;
		//while( cmd_inq.Read() )
		//{
		//	cmd_inq.Fetch(tsmpe02);//把数据都压在头文件里面
		//	// 将结果放入返回块
		//	tsmpe02.MergeTo(bcls_ret->Tables[blknum], false);   //true是以block的定义为准 ,false是以头文件结构覆盖block
		//	if	(tsmpe02["MAT_KIND"].ToString().Trim() != "")
		//	{
		//		/* 读取物料表名 */
		//		#ifndef __MAT_KIND__
		//		#define __MAT_KIND__
		//		CString LS_MAT_KIND = "" ;
		//		CString table_name = "";
		//		#endif 
		//
		//		if	(LS_MAT_KIND != tsmpe02["MAT_KIND"].ToString())
		//		{
		//			f_epep_get_tep0002( *bcls_rec , "M002" , (const char *)tsmpe02["MAT_KIND"], NULL, NULL, NULL, NULL, NULL);
		//			if	(bcls_rec->Tables["TEP0002"].Columns.Contains("CODE_DESC_2_CONTENT") )
		//			{
		//				table_name = bcls_rec->Tables["TEP0002"].Rows[0]["CODE_DESC_2_CONTENT"];
		//				LS_MAT_KIND = tsmpe02["MAT_KIND"].ToString() ;
		//			}
		//		}
		//
		//		sqlstr = " SELECT STOCK_PLACE_NO FROM " + table_name + " WHERE MAT_NO = @tsmpe02.MAT_NO ";
		//
		//		cmd_inq1.SetCommandText(sqlstr);
		//		cmd_inq1.Parameters.Set("tsmpe02.MAT_NO" , tsmpe02["MAT_NO"].ToString());
		//		cmd_inq1.ExecuteReader();
		//		if	(cmd_inq1.Read())
		//		{
		//			bcls_ret->Tables[blknum].Rows[count]["STOCK_PLACE_NO"] = cmd_inq1.GetString(1);
		//		}
		//		cmd_inq1.Close();
		//		count++ ;
		//	}
		//
		//}//while
		//cmd_inq.Close();	// 关闭游标


		Log::Trace("" , __FUNCTION__ , "query records. [{0}]", bcls_ret->Tables[blknum].Rows.get_Count() );	// 读取了多少条记录
		sprintf	(s.msg , "共读取到[%d]条记录" , count);
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
