/****************************************************
*	程序功能：调用物料函数							*
*	编制日期：2011-01-05							*
*	编制人员：wuxin								*
*	传入参数：材料号、参数类型						*
*	返回参数：0	成功	-1	失败					*
*	出错描述：s.msg									*
*****************************************************
*	参数类型说明：									*
*	1：	提单号										*
*	2：	码单号										*
*	3：	材料号										*
*	事件标识说明：									*
*	1：	提单编制		SM04						*
*	2：	提单释放									*
*	3：	码单确认		SM01						*
*	4： 准发计划接收	PM03						*
*	5： 准发确认		PM05						*
*	-1:	提单编制撤销	SM05						*
*	-2:	提单释放撤销								*
*	-3:	码单红冲		SM02						*
*	-4: 准发吊销		PM04						*
*	-5：准发红冲确认	PM13						*
*****************************************************
*	2013-10-30	13801	调用函数判 MAT_KIND			*
****************************************************/
//框架公用头文件，勿删
#include "stdafx.h"





//程序用头文件
//
BM2_FUNCTION_IMPORT
 int f_mm0099(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn);

BM2_FUNCTION_IMPORT
 int f_mmhr99(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn);

BM2_FUNCTION_IMPORT
 int f_mmsm99(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn);

BM2_FUNCTION_IMPORT
 int f_mmcr99(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn);

BM2_FUNCTION_IMPORT
 int f_mmhp99(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn);

BM2_FUNCTION_IMPORT
 int f_mmbw99(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn);
BM2_FUNCTION_EXPORT
int  f_sm00_mm99(CString pack_num , int para_type , int event_id , CString msg, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	Log::Info("" , __FUNCTION__ , "keyvalue=[{0}]" , pack_num);
	Log::Info("" , __FUNCTION__ , "参数类型=[{0}] , 事件标识=[{1}]" , para_type ,  event_id );
	int		doFlag  = 0;
	int		i	    =	0;
	CString	blkname = "";
	int		blkSeq  = 0;
	int		ret     = 0;
	int		temp	=	0;

	CModel tsmpe02("TSMPE02");
	CString	v_pack_num="",v_mat_no="",v_mat_kind="",v_mat_line_type="",v_event_id="",v_bill_of_lading_no="";
	CString	v_system_id = "SM00";

	EIClass	bcls_rec , bcls_ret;
	CDbCommand execute_sql(conn);
	CString sqlstr="",sqlstr1="",sqlstr2="";

	try
	{
		//新增一个"MM0099"的块用以传输数据
		blkname = "MM0099";
		if(!bcls_rec.Tables.Contains(blkname))//判断是否有块，如果没有指定块的话，则加上
		{
			bcls_rec.Tables.Add(blkname);
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "mat_no");
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "event_id");
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "func_id");
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "bill_of_lading_no");
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "order_no");
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "confm_plan_no");
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "confm_reject_cause");
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "mat_kind");
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "SYSTEM_ID");			// 系统标识4位，到二级模块
			bcls_rec.Tables[blkname].Columns.Add( DT_STRING, "EVENT_LINE_TYPE");	// 事件产线类型，全产线/00,其他产线/SM,HR,CR等
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "WHOLE_BACKLOG_CODE");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "WHOLE_BACKLOG_SEQ");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "NEXT_WHOLE_BACKLOG_CODE");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "NEXT_WHOLE_BACKLOG_SEQ");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "STOCK_NO");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "TRUCK_NO");	// 车号
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "HOLD_FLAG");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "HOLD_TIME");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "HOLD_MAKER");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "MNG_HOLD_MAKER");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "MNG_HOLD_TIME");
			bcls_rec.Tables[blkname].Rows.Add();
		}

		//新增一个"MM0099"的传输列

		//判断输入参数的合法性
		if 		(event_id == 1)	{ v_event_id = "SM04"; v_system_id = "SM";}  //提单接受
		else if	(event_id == -1){ v_event_id = "SM05"; v_system_id = "SM";}
		else if	(event_id == 3)	{ v_event_id = "SM01"; v_system_id = "SM";}//发货实绩
		else if	(event_id == -3){ v_event_id = "SM02"; v_system_id = "SM";}
		else if	(event_id == 4)	{ v_event_id = "PM03"; v_system_id = "SM";}//准发计划下发
		else if	(event_id == -4){ v_event_id = "PM04"; v_system_id = "SM";}//准发吊销,红冲请求
		else if	(event_id == 5)	{ v_event_id = "PM05"; v_system_id = "SM";}//准发确认
		else if	(event_id == -5){ v_event_id = "PM13"; v_system_id = "SM";}//红冲确认
		else
		{
			{
				CFormattable arguments[] = { event_id}; // 定义参数列表的数组
				CMessageFormat::Format (s.msg , _RES("SM00S0000026")/*未明确的 event_id 值, 传入的 event_id ={0}*/ , arguments , 1 );
			}
			throw CApplicationException(-1, s.msg, log.Location);
		}

		/* ***** format sql ****** */
		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr1 = CString(
				"   SELECT	mat_no ,  bill_of_lading_no , order_no , confm_plan_no , red_cause_desc,mat_kind,FACTORY_DIV,STOCK_NO,VEHICLE_NO "
				" ,REC_CREATE_TIME "
				" 	FROM	tsmpe02                                                                         "
				"    WHERE	mat_no 		=	@mat_no                                                         "
				"     union                                                                                   "
				"   SELECT	mat_no ,  bill_of_lading_no , order_no , confm_plan_no , ' '  red_cause_desc,mat_kind,FACTORY_DIV,STOCK_NO,VEHICLE_NO "
				" , REC_CREATE_TIME "
				"     FROM	tsmpe12                                                                          "
				"    WHERE	mat_no 		=	@mat_no                                                          "
				"      AND	3	        =	@event_id	"
				"   ORDER BY REC_CREATE_TIME DESC "
				);



			break;
		}
		 
		/* ***** 执行SQL   ***** */
		sqlstr = sqlstr1;
		execute_sql.SetCommandText( sqlstr );
		execute_sql.Parameters.Set( "mat_no" , pack_num );
		execute_sql.Parameters.Set( "event_id" , event_id  );
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		execute_sql.ExecuteReader();

		//tsmpe02["MAT_NO"]             = " ";
		if ( execute_sql.Read() )
		{
			Log::Debug("" , __FUNCTION__ , "bbbbbbbbbbbbbbbbbbb" );
			tsmpe02.Reset();
			tsmpe02["MAT_NO"]             = execute_sql.GetString(1);
			tsmpe02["BILL_OF_LADING_NO"]  = execute_sql.GetString(2);
			tsmpe02["ORDER_NO"]           = execute_sql.GetString(3);
			tsmpe02["CONFM_PLAN_NO"]      = execute_sql.GetString(4);
			tsmpe02["RED_CAUSE_DESC"]     = execute_sql.GetString(5);
			tsmpe02["MAT_KIND"]           = execute_sql.GetString(6);
			tsmpe02["FACTORY_DIV"]        = execute_sql.GetString(7);
			tsmpe02["STOCK_NO"]			  = execute_sql.GetString(8);
			tsmpe02["VEHICLE_NO"]		  = execute_sql.GetString(9);
		}
		execute_sql.Close();


		Log::Trace("" , __FUNCTION__ , "tsmpe02[MAT_NO] =[{0}]" , tsmpe02["MAT_NO"].ToString());
		if ( 0 == tsmpe02["MAT_NO"].ToString().Compare(" "))
		{
			throw CApplicationException(-1, s.msg, log.Location);
		}

		bcls_rec.Tables[blkname].Rows.Clear();
		bcls_rec.Tables[blkname].Rows.Add();
		bcls_rec.Tables[blkname].Rows[0]["mat_no"] = tsmpe02["MAT_NO"];
		bcls_rec.Tables[blkname].Rows[0]["event_id"] = v_event_id;
		bcls_rec.Tables[blkname].Rows[0]["func_id"] = "f_sm00_mm99";
		bcls_rec.Tables[blkname].Rows[0]["bill_of_lading_no"] = tsmpe02["BILL_OF_LADING_NO"];
		bcls_rec.Tables[blkname].Rows[0]["order_no"] = tsmpe02["ORDER_NO"];
		bcls_rec.Tables[blkname].Rows[0]["confm_plan_no"] = tsmpe02["CONFM_PLAN_NO"];
		bcls_rec.Tables[blkname].Rows[0]["confm_reject_cause"] = tsmpe02["RED_CAUSE_DESC"];
		bcls_rec.Tables[blkname].Rows[0]["mat_kind"] = tsmpe02["MAT_KIND"];
		bcls_rec.Tables[blkname].Rows[0]["SYSTEM_ID"] = v_system_id + tsmpe02["MAT_KIND"].ToString();
		bcls_rec.Tables[blkname].Rows[0]["EVENT_LINE_TYPE"] = "00";
		bcls_rec.Tables[blkname].Rows[0]["STOCK_NO"] = tsmpe02["STOCK_NO"].ToString();
		bcls_rec.Tables[blkname].Rows[0]["TRUCK_NO"] = tsmpe02["VEHICLE_NO"].ToString();

		if (v_event_id == "PM13")
		{
			bcls_rec.Tables[blkname].Rows[0]["HOLD_FLAG"] = "2";
			bcls_rec.Tables[blkname].Rows[0]["HOLD_TIME"] = s.datetime;
			bcls_rec.Tables[blkname].Rows[0]["HOLD_MAKER"] = s.username;
			bcls_rec.Tables[blkname].Rows[0]["MNG_HOLD_MAKER"] = s.datetime;
			bcls_rec.Tables[blkname].Rows[0]["MNG_HOLD_TIME"] = s.username;

		}

		CString	table_name;
		CDbCommand cmd_inq(conn);

		table_name = "TMM" + tsmpe02["MAT_KIND"].ToString() + "01";


		/* 暂时屏蔽 2023-1-18 */
		if (tsmpe02["MAT_KIND"].ToString() == "HP")
		{

		}
		else
		{
			sqlstr = "select WHOLE_BACKLOG_CODE ,WHOLE_BACKLOG_SEQ,NEXT_WHOLE_BACKLOG_CODE,NEXT_WHOLE_BACKLOG_SEQ "
				" ,NVL(SUBSTR(WHOLE_BACKLOG,(WHOLE_BACKLOG_SEQ-1)*2-1,2),' ') "
				" from " + table_name + " where mat_no = @MAT_NO ";
			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Set("MAT_NO", tsmpe02["MAT_NO"].ToString());
			Log::Debug("", "", "sqlstr ={0}", sqlstr);
			cmd_inq.ExecuteReader();
			if (cmd_inq.Read())
			{
				bcls_rec.Tables[blkname].Rows[0]["WHOLE_BACKLOG_CODE"] = cmd_inq.GetString(1);
				bcls_rec.Tables[blkname].Rows[0]["WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(2);
				bcls_rec.Tables[blkname].Rows[0]["NEXT_WHOLE_BACKLOG_CODE"] = cmd_inq.GetString(3);;
				bcls_rec.Tables[blkname].Rows[0]["NEXT_WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(4);
				if (v_event_id == "PM05")
				{
					bcls_rec.Tables[blkname].Rows[0]["WHOLE_BACKLOG_CODE"] = "9A";
					bcls_rec.Tables[blkname].Rows[0]["WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(2) + 1;
					bcls_rec.Tables[blkname].Rows[0]["NEXT_WHOLE_BACKLOG_CODE"] = "9B";
					bcls_rec.Tables[blkname].Rows[0]["NEXT_WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(4) + 1;
				}
				if (v_event_id == "PM13" && cmd_inq.GetString(1) == "9A")
				{
					bcls_rec.Tables[blkname].Rows[0]["WHOLE_BACKLOG_CODE"] = cmd_inq.GetString(5);
					bcls_rec.Tables[blkname].Rows[0]["WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(2) - 1;
					bcls_rec.Tables[blkname].Rows[0]["NEXT_WHOLE_BACKLOG_CODE"] = "9A";
					bcls_rec.Tables[blkname].Rows[0]["NEXT_WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(4) - 1;
				}
			}
			cmd_inq.Close();
		}

		//// 根据 bcls_rec 含有的块，调用对应的函数
		//if	(f_mm0099(&bcls_rec , &bcls_ret, conn) != 0)
		//{
		//	throw CApplicationException(-1, s.msg, log.Location);
		//}
		Log::Trace("" , __FUNCTION__ , "tsmpe02[FACTORY_DIV] =[{0}]" , tsmpe02["FACTORY_DIV"].ToString());
		Log::Trace("" , __FUNCTION__ , "tsmpe0[MAT_KIND] =[{0}]" , tsmpe02["MAT_KIND"].ToString());

#ifdef _LINE_HR
		//热轧产线
		if(tsmpe02["MAT_KIND"].ToString() == "HR")
		{
			if	(f_mmhr99(&bcls_rec , &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		//炼钢产线
#ifdef _LINE_SM
		if(tsmpe02["MAT_KIND"].ToString() == "SM")
		{
			if	(f_mmsm99(&bcls_rec , &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		//厚板产线
#ifdef _LINE_HP
		if(tsmpe02["MAT_KIND"].ToString() == "HP")
		{
			if	(f_mmhp99(&bcls_rec , &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		//冷轧产线
#ifdef _LINE_CR
		if(tsmpe02["MAT_KIND"].ToString() == "CR")
		{
			if	(f_mmcr99(&bcls_rec , &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		//棒线产线
#ifdef _LINE_BW
		if(tsmpe02["MAT_KIND"].ToString() == "BW")
		{
			if	(f_mmbw99(&bcls_rec , &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif
		
		msg = _RES("GCRSS0000002")/*处理成功。*/;
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
int  f_sm00_mm99(vector <CString> v_mat_no, int para_type, int event_id, CString msg, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);
	CString pack_num;

	Log::Info("", __FUNCTION__, "keyvalue=[{0}]", pack_num);
	Log::Info("", __FUNCTION__, "参数类型=[{0}] , 事件标识=[{1}]", para_type, event_id);
	int		doFlag = 0;
	int		i = 0;
	CString	blkname = "";
	int		blkSeq = 0;
	int		ret = 0;
	int		temp = 0;

	CModel tsmpe02("TSMPE02");
	CString	v_pack_num = "", v_mat_kind = "", v_mat_line_type = "", v_event_id = "", v_bill_of_lading_no = "";
	CString	v_system_id = "SM00";

	EIClass	bcls_rec, bcls_ret;
	CDbCommand execute_sql(conn);
	CString sqlstr = "", sqlstr1 = "", sqlstr2 = "";

	try
	{
		//新增一个"MM0099"的块用以传输数据
		blkname = "MM0099";
		if (!bcls_rec.Tables.Contains(blkname))//判断是否有块，如果没有指定块的话，则加上
		{
			bcls_rec.Tables.Add(blkname);
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "mat_no");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "event_id");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "func_id");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "bill_of_lading_no");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "order_no");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "confm_plan_no");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "confm_reject_cause");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "mat_kind");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "SYSTEM_ID");			// 系统标识4位，到二级模块
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "EVENT_LINE_TYPE");	// 事件产线类型，全产线/00,其他产线/SM,HR,CR等
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "WHOLE_BACKLOG_CODE");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "WHOLE_BACKLOG_SEQ");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "NEXT_WHOLE_BACKLOG_CODE");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "NEXT_WHOLE_BACKLOG_SEQ");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "STOCK_NO");
			bcls_rec.Tables[blkname].Columns.Add(DT_STRING, "TRUCK_NO");	// 车号
			//bcls_rec.Tables[blkname].Rows.Add();
		}

		//新增一个"MM0099"的传输列

		//判断输入参数的合法性
		if (event_id == 1)	{ v_event_id = "SM04"; v_system_id = "SM"; }  //提单接受
		else if (event_id == -1){ v_event_id = "SM05"; v_system_id = "SM"; }
		else if (event_id == 3)	{ v_event_id = "SM01"; v_system_id = "SM"; }//发货实绩
		else if (event_id == -3){ v_event_id = "SM02"; v_system_id = "SM"; }
		else if (event_id == 4)	{ v_event_id = "PM03"; v_system_id = "SM"; }//准发计划下发
		else if (event_id == -4){ v_event_id = "PM04"; v_system_id = "SM"; }//准发吊销,红冲请求
		else if (event_id == 5)	{ v_event_id = "PM05"; v_system_id = "SM"; }//准发确认
		else if (event_id == -5){ v_event_id = "PM13"; v_system_id = "SM"; }//红冲确认
		else
		{
			{
				CFormattable arguments[] = { event_id }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, _RES("SM00S0000026")/*未明确的 event_id 值, 传入的 event_id ={0}*/, arguments, 1);
			}
			throw CApplicationException(-1, s.msg, log.Location);
		}

		/* ***** format sql ****** */
		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr1 = CString(
				"   SELECT	mat_no ,  bill_of_lading_no , order_no , confm_plan_no , red_cause_desc,mat_kind,FACTORY_DIV,STOCK_NO,VEHICLE_NO "
				" , REC_CREATE_TIME "
				" 	FROM	tsmpe02                                                                         "
				"    WHERE	mat_no 		=	@mat_no                                                         "
				"     union                                                                                   "
				"   SELECT	mat_no ,  bill_of_lading_no , order_no , confm_plan_no , ' '  red_cause_desc,mat_kind,FACTORY_DIV,STOCK_NO,VEHICLE_NO "
				" , REC_CREATE_TIME "
				"     FROM	tsmpe12                                                                          "
				"    WHERE	mat_no 		=	@mat_no                                                          "
				"      AND	3	        =	@event_id	                                                             "
				"   ORDER BY rec_create_time DESC "
				);



			break;
		}

		for (int i = 0; i < v_mat_no.size(); i++)
		{
			pack_num = v_mat_no[i];

			/* ***** 执行SQL   ***** */
			sqlstr = sqlstr1;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("mat_no", pack_num);
			execute_sql.Parameters.Set("event_id", event_id);
			Log::Debug("", "", "sqlstr={0}", sqlstr);

			execute_sql.ExecuteReader();

			//tsmpe02["MAT_NO"]             = " ";
			if (execute_sql.Read())
			{
				Log::Debug("", __FUNCTION__, "bbbbbbbbbbbbbbbbbbb");
				tsmpe02.Reset();
				tsmpe02["MAT_NO"] = execute_sql.GetString(1);
				tsmpe02["BILL_OF_LADING_NO"] = execute_sql.GetString(2);
				tsmpe02["ORDER_NO"] = execute_sql.GetString(3);
				tsmpe02["CONFM_PLAN_NO"] = execute_sql.GetString(4);
				tsmpe02["RED_CAUSE_DESC"] = execute_sql.GetString(5);
				tsmpe02["MAT_KIND"] = execute_sql.GetString(6);
				tsmpe02["FACTORY_DIV"] = execute_sql.GetString(7);
				tsmpe02["STOCK_NO"] = execute_sql.GetString(8);
				tsmpe02["VEHICLE_NO"] = execute_sql.GetString(9);
			}
			execute_sql.Close();


			Log::Trace("", __FUNCTION__, "tsmpe02[MAT_NO] =[{0}]", tsmpe02["MAT_NO"].ToString());
			if (0 == tsmpe02["MAT_NO"].ToString().Compare(" "))
			{
				CFormattable arguments[] = { pack_num };
				CMessageFormat::Format(s.msg, "没有读取到材料号[{0}]的信息！", arguments, 1);
				throw CApplicationException(-1, s.msg, log.Location);
			}

			//bcls_rec.Tables[blkname].Rows.Clear();
			bcls_rec.Tables[blkname].Rows.Add();
			int ii = bcls_rec.Tables[blkname].Rows.get_Count() - 1;
			bcls_rec.Tables[blkname].Rows[ii]["mat_no"] = tsmpe02["MAT_NO"];
			bcls_rec.Tables[blkname].Rows[ii]["event_id"] = v_event_id;
			bcls_rec.Tables[blkname].Rows[ii]["func_id"] = "f_sm00_mm99";
			bcls_rec.Tables[blkname].Rows[ii]["bill_of_lading_no"] = tsmpe02["BILL_OF_LADING_NO"];
			bcls_rec.Tables[blkname].Rows[ii]["order_no"] = tsmpe02["ORDER_NO"];
			bcls_rec.Tables[blkname].Rows[ii]["confm_plan_no"] = tsmpe02["CONFM_PLAN_NO"];
			bcls_rec.Tables[blkname].Rows[ii]["confm_reject_cause"] = tsmpe02["RED_CAUSE_DESC"];
			bcls_rec.Tables[blkname].Rows[ii]["mat_kind"] = tsmpe02["MAT_KIND"];
			bcls_rec.Tables[blkname].Rows[ii]["SYSTEM_ID"] = v_system_id + tsmpe02["MAT_KIND"].ToString();
			bcls_rec.Tables[blkname].Rows[ii]["EVENT_LINE_TYPE"] = "00";
			bcls_rec.Tables[blkname].Rows[ii]["STOCK_NO"] = tsmpe02["STOCK_NO"].ToString();
			bcls_rec.Tables[blkname].Rows[ii]["TRUCK_NO"] = tsmpe02["VEHICLE_NO"].ToString();

			CString	table_name;
			CDbCommand cmd_inq(conn);

			table_name = "TMM" + tsmpe02["MAT_KIND"].ToString() + "01";

			/* 暂时屏蔽 2023-1-18 */
			if (tsmpe02["MAT_KIND"].ToString() == "HP")
			{

			}
			else
			{
				sqlstr = "select WHOLE_BACKLOG_CODE ,WHOLE_BACKLOG_SEQ,NEXT_WHOLE_BACKLOG_CODE,NEXT_WHOLE_BACKLOG_SEQ "
					" ,NVL(SUBSTR(WHOLE_BACKLOG,(WHOLE_BACKLOG_SEQ-1)*2-1,2),' ') "
					" from " + table_name + " where mat_no = @MAT_NO ";
				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("MAT_NO", tsmpe02["MAT_NO"].ToString());
				Log::Debug("", "", "sqlstr ={0}", sqlstr);
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					bcls_rec.Tables[blkname].Rows[ii]["WHOLE_BACKLOG_CODE"] = cmd_inq.GetString(1);
					bcls_rec.Tables[blkname].Rows[ii]["WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(2);
					bcls_rec.Tables[blkname].Rows[ii]["NEXT_WHOLE_BACKLOG_CODE"] = cmd_inq.GetString(3);;
					bcls_rec.Tables[blkname].Rows[ii]["NEXT_WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(4);
					if (v_event_id == "PM05")
					{
						bcls_rec.Tables[blkname].Rows[ii]["WHOLE_BACKLOG_CODE"] = "9A";
						bcls_rec.Tables[blkname].Rows[ii]["WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(2) + 1;
						bcls_rec.Tables[blkname].Rows[ii]["NEXT_WHOLE_BACKLOG_CODE"] = "9B";
						bcls_rec.Tables[blkname].Rows[ii]["NEXT_WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(4) + 1;
					}
					if (v_event_id == "PM13" && cmd_inq.GetString(1) == "9A")
					{
						bcls_rec.Tables[blkname].Rows[ii]["WHOLE_BACKLOG_CODE"] = cmd_inq.GetString(5);
						bcls_rec.Tables[blkname].Rows[ii]["WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(2) - 1;
						bcls_rec.Tables[blkname].Rows[ii]["NEXT_WHOLE_BACKLOG_CODE"] = "9A";
						bcls_rec.Tables[blkname].Rows[ii]["NEXT_WHOLE_BACKLOG_SEQ"] = cmd_inq.GetDecimal(4) - 1;
					}
				}
				cmd_inq.Close();
			}
		}
		//// 根据 bcls_rec 含有的块，调用对应的函数
		//if	(f_mm0099(&bcls_rec , &bcls_ret, conn) != 0)
		//{
		//	throw CApplicationException(-1, s.msg, log.Location);
		//}
		Log::Trace("", __FUNCTION__, "tsmpe02[FACTORY_DIV] =[{0}]", tsmpe02["FACTORY_DIV"].ToString());
		Log::Trace("", __FUNCTION__, "tsmpe0[MAT_KIND] =[{0}]", tsmpe02["MAT_KIND"].ToString());

#ifdef _LINE_HR
		//热轧产线
		if (tsmpe02["MAT_KIND"].ToString() == "HR")
		{
			if (f_mmhr99(&bcls_rec, &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		//炼钢产线
#ifdef _LINE_SM
		if (tsmpe02["MAT_KIND"].ToString() == "SM")
		{
			if (f_mmsm99(&bcls_rec, &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		//厚板产线
#ifdef _LINE_HP
		if (tsmpe02["MAT_KIND"].ToString() == "HP")
		{
			if (f_mmhp99(&bcls_rec, &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		//冷轧产线
#ifdef _LINE_CR
		if (tsmpe02["MAT_KIND"].ToString() == "CR")
		{
			if (f_mmcr99(&bcls_rec, &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		//棒线产线
#ifdef _LINE_BW
		if (tsmpe02["MAT_KIND"].ToString() == "BW")
		{
			if (f_mmbw99(&bcls_rec, &bcls_ret, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}
#endif

		msg = _RES("GCRSS0000002")/*处理成功。*/;
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



