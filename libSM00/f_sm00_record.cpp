/****************************************************
*	程序功能：写履历记录							*
*	编制日期：2009-06-09							*
*	编制人员：t013801								*
*	传入参数：材料号、履历类型						*
*	返回参数：0	成功	-1	失败					*
*	出错描述：s.msg									*
*****************************************************
*	履历类型说明：									*
*	0：	准发材料接收								*
*	1：	准发计划吊销								*
*	2：	准发红冲请求								*
*	3：	准发红冲确认								*
*	4：	准发确认									*
*	5：	出厂确认									*
*	6:	码单红冲									*
*	7:	发货计划接收								*
*	8:	发货计划删除								*
****************************************************/
#include "stdafx.h"		// 框架头，不可删除






//名称空间引用




//外部函数声明

BM2_FUNCTION_EXPORT
 int  f_sm00_record(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int	zlfh	=	1;		//重量符号
	int blkseq = 0;
	int	i	=	0;
	int	rows = 0;

	/* 业务变量 */
	CString	sqlstr("") , sqlstr1("");              // 数据库SQL操作字符串
	CString	dateTime("");
	CString	blkname("sm00_record");
	CString	str("");
	CString	mat_no("");
	CString	mark("");
	CString v_userid("");
	CString LS_MAT_KIND = "" ;
	CString table_name = "";

	/* 实体类定义 */
	CModel tsmpe02("TSMPE02");
	CModel tsmpe12("TSMPE12");
	CModel tsmpe00("TSMPE00");
	CModel tsmpea1("TSMPEA1");

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);

	try
	{
		dateTime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		CDateTime currTime = CDateTime::Now();
		tsmpea1["TRACK_SEQ_NO"] = dateTime + CString::Format( "%04ld",currTime.Millisecond()).SubstringNE(0,4);

		/* 判块是否存在 */
		blkseq = bcls_rec->Tables.IndexOf(blkname); 		//读取块名所在的块号
		if	(blkseq < 0)	bcls_rec->Tables[0].set_TableName(blkname);

		v_userid = s.userid;
		rows = bcls_rec->Tables[blkname].Rows.get_Count();
		for (i = 0 ; i < rows  ; i++ )
		{
			mat_no	= bcls_rec->Tables[blkname].Rows[i]["mat_no"].ToString().TrimOrBlank();	// 材料号
			mark	= bcls_rec->Tables[blkname].Rows[i]["event_mark"].ToString().TrimOrBlank();
			if	(bcls_rec->Tables[blkname].Columns.Contains("userid"))	v_userid= bcls_rec->Tables[blkname].Rows[i]["userid"];

			//传入的参数打印
			Log::Info("" , __FUNCTION__ ,"第[{0}]条记录，共[{1}]条记录",i+1,rows);
			Log::Info("" , __FUNCTION__ ,"材料号=[{0}]"		, mat_no);
			Log::Info("" , __FUNCTION__ ,"履历类型=[{0}]"	, mark  );
			Log::Info("" , __FUNCTION__ ,"v_userid=[{0}]"	, v_userid  );

			if	( mark == "6" || mark == "3")
			{
				zlfh = -1;
			}

			/* 数据库操作 */
			switch(conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr = " SELECT * FROM tsmpe12 "
					" WHERE mat_no = @mat_no "
					" AND	rec_create_time in ( select max(a.rec_create_time) from tsmpe12 a where a.mat_no	= @mat_no )";		// SQL语句定义

				sqlstr1 = " SELECT * FROM tsmpe02 "
					" WHERE mat_no = @mat_no ";

				break;
			}

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("mat_no", mat_no);	// SQL语句中的变量赋值
			cmd_inq.ExecuteReader();
			if	(cmd_inq.Read() )
			{
				cmd_inq.Fetch(tsmpe12);
				tsmpe02.CopyFrom(tsmpe12);
			}
			else
			{
				cmd_inq.Close();
				cmd_inq.SetCommandText(sqlstr1);
				cmd_inq.Parameters.Set("mat_no", mat_no);	// SQL语句中的变量赋值
				cmd_inq.ExecuteReader();
				if	(cmd_inq.Read() )
				{
					cmd_inq.Fetch(tsmpe02);
				}
				else
				{
					CFormattable arguments[] = { mat_no , 1403 };// 定义参数列表的数组
					CMessageFormat::Format(s.msg , _RES("SM00S0000036")/*读取准发材料表出错，材料号=[{0}],sqlcode=[{1}]*/,arguments,  2);//格式化字符串
					throw CApplicationException(-1 , s.msg , log.Location);
				}
			}
			cmd_inq.Close();
			tsmpea1.CopyFrom(tsmpe02);

			/* 读取事件名称 */
			tsmpea1["EVENT_NAME"] = " ";
			sqlstr = " SELECT code_desc_1_content FROM TEP0002"
				" WHERE	CODE_CLASS = 'SM03' "
				" AND	CODE = @mark ";

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("mark" , mark);
			cmd_inq.ExecuteReader();
			if	(cmd_inq.Read())
			{
				tsmpea1["EVENT_NAME"] = cmd_inq.GetString(1);
			}
			cmd_inq.Close();

			/* 按准发单据号读取合同信息 */
			sqlstr = " SELECT * FROM tsmpe00"
				" WHERE	READY_BILL_NO	= @ready_bill_no "
				;

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("ready_bill_no" , tsmpe02["READY_BILL_NO"].ToString());
			cmd_inq.ExecuteReader();
			if	(cmd_inq.Read())
			{
				cmd_inq.Fetch(tsmpe00);
			}
			//else
			//{
			//	CFormattable arguments[] = { tsmpe02.READY_BILL_NO , 1403 };// 定义参数列表的数组
			//	CMessageFormat::Format(s.msg , _RES("SM00S0000255")/*读取准发表出错，准发单据号=[{0}]，sqlcode=[{1}]*/,arguments,  2);//格式化字符串
			//	throw CApplicationException(-1 , s.msg , log.Location);
			//}
			cmd_inq.Close();


			/* 读取物料表名 */
			if	(LS_MAT_KIND != tsmpe02["MAT_KIND"].ToString())
			{
				sqlstr = "SELECT CODE_DESC_2_CONTENT FROM TEP0002 WHERE CODE_CLASS = 'M002' AND CODE = @MAT_KIND ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("MAT_KIND" , tsmpe02["MAT_KIND"].ToString());
				cmd_inq.ExecuteReader();
				if	(cmd_inq.Read())
				{
					table_name = cmd_inq.GetString(1);
					LS_MAT_KIND = tsmpe02["MAT_KIND"].ToString() ;
				}
				cmd_inq.Close();
			}

			//if	(LS_MAT_KIND != tsmpe02["MAT_KIND"].ToString())
			//{
			//	f_epep_get_tep0002( *bcls_rec , "M002" , (const char *)tsmpe02.MAT_KIND , NULL, NULL, NULL, NULL, NULL);
			//	if	(bcls_rec->Tables["TEP0002"].Columns.Contains("CODE_DESC_2_CONTENT") )
			//	{
			//		table_name = bcls_rec->Tables["TEP0002"].Rows[0]["CODE_DESC_2_CONTENT"];
			//		LS_MAT_KIND = tsmpe02["MAT_KIND"].ToString() ;
			//	}
			//}

			/* 按材料号读取物料表上的库位信息 */
			tsmpea1["KEYVALUE_1"] = " ";
			sqlstr = " SELECT stock_place_no FROM " + table_name + " where MAT_NO = @tsmpe02.mat_no ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("tsmpe02.mat_no" , tsmpe02["MAT_NO"].ToString());
			cmd_inq.ExecuteReader();
			if	(cmd_inq.Read())
			{
				tsmpea1["KEYVALUE_1"] = cmd_inq.GetString(1);
			}
			cmd_inq.Close();

			CString confm_shift= tsmpe02["CONFM_SHIFT"].ToString();
			CString confm_group = tsmpe02["CONFM_GROUP"].ToString();
			// 调用函数生成班次、班组
			f_epep_get_shift_group("DEFAULT",dateTime, confm_shift, confm_group, conn);
			//if(f_epep_get_shift_group("DEFAULT",dateTime,tsmpe02["CONFM_SHIFT"].ToString(),tsmpe02.CONFM_GROUP ,conn) < 0)
			//{
			//	sprintf(s.msg ,"f_epep_get_shift_group函数调用出错.");
			//	throw CApplicationException(-1, s.msg, s.svc_name);
			//}

			//	数据
			tsmpea1["REC_CREATOR"]				=	v_userid					;	/* 记录创建责任者 */
			tsmpea1["REC_CREATE_TIME"]			=	dateTime					;	/* 记录创建时刻 */
			//		tsmpea1.REC_REVISOR				=	s.username					;	/* 记录修改责任者 */
			//		tsmpea1.REC_REVISE_TIME			=	dateTime					;	/* 记录修改时刻 */
			//		tsmpea1.TRACK_SEQ_NO				=		;	/* 事件跟踪序列号 */
			tsmpea1["EVENT_ID"]				=	mark						;	/* 事件标识 */
			//		tsmpea1.EVENT_NAME				=          ;/* 电文或事件名称 */
			tsmpea1["EVENT_DATE"]				=	dateTime.Substring(0,8)		;	/* 事件日期 */
			tsmpea1["SHIFT_NO"]				=	    tsmpe02["CONFM_SHIFT"]			;	/* 班次号 */
			tsmpea1["SHIFT_GROUP"]				=	tsmpe02["CONFM_GROUP"]			;	/* 班组 */
			tsmpea1["EVENT_MAKER"]				=	v_userid					;	/* 事件责任者 */
			tsmpea1["FACTORY_DIV"]				=	tsmpe02["FACTORY_DIV"]			;	/* 厂别区分 */
			tsmpea1["MAT_NO"]					=	tsmpe02["MAT_NO"]			;	/* 定制材料号 */
			tsmpea1["CONFM_PLAN_NO"]			=	tsmpe02["CONFM_PLAN_NO"]		;	/* 准发计划号 */
			tsmpea1["READY_BILL_NO"]			=	tsmpe02["READY_BILL_NO"]		;	/* 准发单据号 */
			tsmpea1["BILL_OF_LADING_NO"]		=	tsmpe02["BILL_OF_LADING_NO"]	;	/* 提货单号 */
			tsmpea1["STACKING_NO"]				=	tsmpe02["STACKING_NO"]			;	/* 码单号 */
			tsmpea1["LEAVE_FACTORY_CARD"]		=	tsmpe02["LEAVE_FACTORY_CARD"]	;	/* 出厂证号 */
			tsmpea1["ORDER_NO"]				=	    tsmpe02["ORDER_NO"]			;	/* 合同号 */
			tsmpea1["ORDER_DEST"]				=	tsmpe00["ORDER_DEST"]			;	/* 合同去向 */
			tsmpea1["EXPORT_FLAG"]				=	tsmpe00["EXPORT_FLAG"]			;	/* 出口标记 */
			tsmpea1["ORDER_TYPE_CODE"]			=	tsmpe00["ORDER_TYPE_CODE"]		;	/* 合同性质代码 */
			tsmpea1["PROD_CODE"]				=	tsmpe02["PROD_CODE"]			;	/* 品名代码 */
			tsmpea1["PROD_CNAME"]				=	tsmpe02["PROD_CNAME"]			;	/* 品名中文 */
			tsmpea1["WT_MODE"]					=	tsmpe02["WT_MODE"]				;	/* 计算重量方式 */
			tsmpea1["PACK_TYPE_CODE"]			=	tsmpe00["PACK_TYPE_CODE"]		;	/* 包装类型代码 */
			tsmpea1["COMPLEX_DECIDE_CODE"]		=	tsmpe02["COMPLEX_DECIDE_CODE"]	;	/* 综合判定代码 */
			tsmpea1["STOCK_NO"]				=	    tsmpe02["STOCK_NO"]			;	/* 库号 */
			//		tsmpea1["FIELDNO"].ToString()[2];                       /* 区号 */
			//		tsmpea1["ROWNO"].ToString()[4];                         /* 行号 */
			//		tsmpea1["COLUMN_NO"].ToString()[4];                     /* 列号 */
			//		tsmpea1["LAYERNO"].ToString()[4];                       /* 层号 */
			tsmpea1["MAT_WT"]					=	tsmpe02["MAT_WT"].ToDecimal()	* zlfh		;	/* 材料重量 */
			tsmpea1["MAT_THICK"]				=	tsmpe02["MAT_THICK"]			;	/* 材料厚度 */
			tsmpea1["MAT_WIDTH"]				=	tsmpe02["MAT_WIDTH"]			;	/* 材料宽度 */
			tsmpea1["MAT_LEN"]					=	tsmpe02["MAT_LEN"]				;	/* 材料长度 */
			tsmpea1["MAT_TUBE"]				=	tsmpe02["MAT_TUBE"].ToDecimal() * zlfh		;	/* 材料根数 */
			//		tsmpea1.KEYVALUE_1				=	tmmcr01.STOCK_PLACE_NO		;	/* 关键字串1 */
			//		tsmpea1.KEYVALUE_2[51];                   /* 关键字串2 */
			//		tsmpea1.KEYVALUE_3[51];                   /* 关键字串3 */
			tsmpea1["VEHICLE_NO"]				=	tsmpe02["VEHICLE_NO"]			;	/* 车船号 */
			tsmpea1["TM_TYPE"]					=	tsmpe02["TM_TYPE"]				;	/* 终端类型 */

			tsmpea1.TrimOrBlank();
			Log::Trace("" , __FUNCTION__ , "材料号=[{0}]",tsmpea1["MAT_NO"]);

			//	写履历档信息
			sqlstr = " INSERT INTO tsmpea1 " ;
			tsmpea1.Insert();

			sprintf	(s.msg , _RES("GCRSS0000002")/*处理成功。*/);
		}
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = {  ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("" , __FUNCTION__ , "error=[{0}]", str );

		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
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
