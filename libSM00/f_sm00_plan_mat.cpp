/****************************************************
*	程序功能：	读取满足计划条件下的材料（按跺位）	*
*	编制日期：	2023-2-1							*
*	编制人员：	13801								*
*	传入参数：	计划号、跺位号						*
*	返回参数：	0	成功	-1	失败				*
*	出错描述：	s.msg								*
*****************************************************
*													*
****************************************************/
//框架公用头文件，勿删
#include "stdafx.h"

using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//程序用头文件
BM2_FUNCTION_EXPORT
int  f_sm00_plan_mat(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	int		doFlag  = 0;
	//CString	blkname = "SM00_MAT_CHECK";
	int blkname = 0;

	CString	table_name = "", c_bill_of_lading_no = "";
	CString c_stock_place_no = "";	// 材料库位号

	CDbCommand cmd_inq(conn);
	CDbCommand cmd_loop(conn);
	CDbCommand cmd_loop2(conn);
	CString sqlstr = "";
	CString sqlstr1 = "";

	try
	{
		// 读取传入的参数
		if (bcls_rec->Tables[blkname].Columns.Contains("BILL_OF_LADING_NO"))
		{
			c_bill_of_lading_no = bcls_rec->Tables[blkname].Rows[0]["BILL_OF_LADING_NO"];	//计划号
		}
		if (bcls_rec->Tables[blkname].Columns.Contains("STOCK_PLACE_NO"))
		{
			c_stock_place_no = bcls_rec->Tables[blkname].Rows[0]["STOCK_PLACE_NO"];
		}

		Log::Debug("", "", "bill_of_lading_no=[{0}]", c_bill_of_lading_no);
		Log::Debug("", "", "stock_place_no=[{0}]", c_stock_place_no);


		// 校验读取的参数
		if ( c_bill_of_lading_no.Trim() =="")
		{
			sprintf(s.msg, "计划号不能为空!");
			throw	CApplicationException(-1, s.msg, log.Location);
		}


		// 按计划号读取物料种类
		CString c_mat_kind = "";
		CString c_delivy_qty_flag = "";
		CString c_stock_no = "";

		sqlstr = " SELECT DISTINCT MAT_KIND,DELIVY_QTY_FLAG,STOCK_NO FROM TSMPE10 WHERE BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "'";
		cmd_inq.SetCommandText(sqlstr);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			c_mat_kind = cmd_inq.GetString(1);
			c_delivy_qty_flag = cmd_inq.GetString(2);
			c_stock_no = cmd_inq.GetString(3);

			table_name = "TMM" + c_mat_kind + "01";
			Log::Debug("", "", "table_name=【{0}】", table_name);
		}
		else
		{
			CFormattable arguments[] = { c_bill_of_lading_no };
			CMessageFormat::Format(s.msg, "没有读到计划号【{0}】", arguments, 1);
			throw	CApplicationException(-1, s.msg, s.svc_name);
		}
		cmd_inq.Close();


		sqlstr = " SELECT A.*,B.* FROM TSMPE10 C,TSMPE02 A," + table_name + " B "
			" WHERE C.BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' "
			" AND a.CONFM_STATUS	=	'6' "
			" AND	A.RED_FLAG		!=	'1' "
			" AND   A.BILL_OF_LADING_NO = C.BILL_OF_LADING_NO "
			" AND   A.MAT_NO        =   B.MAT_NO ";
		sqlstr += " ORDER BY B.STOCK_PLACE_NO , B.LAYERNO DESC ";


		if (c_delivy_qty_flag == "1" || c_delivy_qty_flag == "2" || c_delivy_qty_flag == "3")
		{
			// 按计划号读满足条件的材料库位
			if (c_mat_kind == "BW" && c_delivy_qty_flag == "1")
			{
				sqlstr1 = "SELECT C.BILL_OF_LADING_NO AS BILL_OF_LADING_NO_1 ,C.ORDER_NO AS ORDER_NO_1 ,A.* FROM TSMPE10 C,TSMPE02 A," + table_name + " B "
					" WHERE C.BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' "
					" AND a.CONFM_STATUS	=	'4' "
					" AND a.DELIVY_QTY_FLAG IN ('1','2') "
					//" AND	A.RED_FLAG		!=	'1' "
					" AND	a.MAT_KIND		= '" + c_mat_kind + "' "
					" AND   A.TRNP_MODE_CODE =  C.TRNP_MODE_CODE "	// 运输方式
					" AND	a.SG_SIGN		=	C.SG_SIGN "		// 钢牌号
					" AND	a.MAT_THICK		=	C.ORDER_THICK "	// 厚度
					" AND   A.FIX_FLAG      =	C.FIX_FLAG "	// 定尺标记
					" AND   A.PSC           =	C.PSC "			// 产品规范码
					" AND	a.MAT_NO		=	b.MAT_NO "
					" AND   A.MAT_WIDTH		=	DECODE (C.ORDER_WIDTH,0,A.MAT_WIDTH,C.ORDER_WIDTH) "
					" AND   A.MAT_LEN  BETWEEN  DECODE (C.ORDER_MIN_LEN,0,A.MAT_LEN,C.ORDER_MIN_LEN) "
					"                  AND      DECODE (C.ORDER_MAX_LEN,0,A.MAT_LEN,C.ORDER_MAX_LEN) "
					" AND   (A.STOCK_NO     IN  (SELECT STOCK_NO FROM TSI0021 WHERE STOCK_ADDR = '" + c_stock_no + "') "
					"                      OR  A.STOCK_NO = '" + c_stock_no + "' ) ";

				//liguangyuan update 20230818
// DM8 适配 CHANGE-266:查询。空值搜索 DECODE 改为标准 CASE。
// 改写原因：空值搜索 DECODE 改为标准 CASE,不依赖 NULL 相等匹配的未记载语义；依据 DM 官方文档,DM8 尚未实测。
// 本共用分支面向 DM8,其他 DB_KIND 标签也会执行此 SQL;参数、结果列、条件与排序保持不变。
// 原 SQL（完整保留）：
				// sqlstr = " SELECT DECODE(TRIM(A.BILL_OF_LADING_NO),'',A.BILL_OF_LADING_NO_1,A.BILL_OF_LADING_NO) AS BILL_OF_LADING_NO,"
					// " DECODE(A.ORDER_NO_1,NULL,B.ORDER_NO,A.ORDER_NO_1) AS ORDER_NO,"
					// " A.OLD_ORDER_NO,A.PROD_CNAME,A.RED_FLAG,"
					// " (CASE WHEN B.MAT_SHAPE_FLAG='7' THEN B.LAYERNO || B.ROWNO || B.COLUMN_NO ELSE B.LAYERNO || B.COLUMN_NO END) STOCK_PLACE_POSITION ,"
					// " B.* "
				// " FROM "+ table_name +" B ,(" + sqlstr1 + ") A WHERE B.MAT_NO = A.MAT_NO ";
// DM8 SQL：
				sqlstr = " SELECT CASE WHEN TRIM(A.BILL_OF_LADING_NO) IS NULL OR TRIM(A.BILL_OF_LADING_NO) = '' THEN A.BILL_OF_LADING_NO_1 ELSE A.BILL_OF_LADING_NO END AS BILL_OF_LADING_NO,"
					" CASE WHEN A.ORDER_NO_1 IS NULL THEN B.ORDER_NO ELSE A.ORDER_NO_1 END AS ORDER_NO,"
					" A.OLD_ORDER_NO,A.PROD_CNAME,A.RED_FLAG,"
					" (CASE WHEN B.MAT_SHAPE_FLAG='7' THEN B.LAYERNO || B.ROWNO || B.COLUMN_NO ELSE B.LAYERNO || B.COLUMN_NO END) STOCK_PLACE_POSITION ,"
					" B.* "
				" FROM "+ table_name +" B ,(" + sqlstr1 + ") A WHERE B.MAT_NO = A.MAT_NO ";
				if (c_stock_place_no.Trim() != "")
				{
					sqlstr += " AND B.STOCK_PLACE_NO = '" + c_stock_place_no.Trim() + "' ";
				}
				sqlstr += "ORDER BY B.STOCK_PLACE_NO ASC,B.LAYERNO DESC,B.ROWNO DESC,B.COLUMN_NO DESC ";
				//liguangyuan 20230818 注释
				//sqlstr = " SELECT A.BILL_OF_LADING_NO_1 AS BILL_OF_LADING_NO,DECODE(A.ORDER_NO_1,NULL,B.ORDER_NO,A.ORDER_NO_1) AS ORDER_NO "
				//	" ,A.OLD_ORDER_NO,A.PROD_CNAME,A.RED_FLAG,B.* ";
				//if (c_stock_place_no.Trim() != "")
				//{
				//	sqlstr += " FROM TMMBW01 B LEFT JOIN (" + sqlstr1 + ") A ON B.MAT_NO = A.MAT_NO WHERE 1=1 ";
				//	sqlstr += " AND B.STOCK_PLACE_NO = '" + c_stock_place_no.Trim() + "' ";
				//}
				//else
				//{
				//	sqlstr += " FROM TMMBW01 B ,(" + sqlstr1 + ") A WHERE B.MAT_NO = A.MAT_NO ";
				//}
				//sqlstr += " ORDER BY B.STOCK_PLACE_NO,B.LAYERNO DESC ";

			}

			//liguangyuan add 20230906
			// 按计划号读满足条件的材料库位
			if (c_mat_kind == "SM" && c_delivy_qty_flag == "1")
			{
				sqlstr1 = "SELECT C.BILL_OF_LADING_NO AS BILL_OF_LADING_NO_1 ,C.ORDER_NO AS ORDER_NO_1 ,A.* FROM TSMPE10 C,TSMPE02 A," + table_name + " B "
					" WHERE C.BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' "
					" AND a.CONFM_STATUS	=	'4' "
					" AND a.DELIVY_QTY_FLAG IN ('1','2') "
					//" AND	A.RED_FLAG		!=	'1' "
					" AND	a.MAT_KIND		= '" + c_mat_kind + "' "
					" AND   A.TRNP_MODE_CODE =  C.TRNP_MODE_CODE "	// 运输方式
					" AND	a.SG_SIGN		=	C.SG_SIGN "		// 钢牌号
					" AND	a.MAT_THICK		=	C.ORDER_THICK "	// 厚度
					//" AND   A.FIX_FLAG      =	C.FIX_FLAG "	// 定尺标记
					" AND   A.PSC           =	C.PSC "			// 产品规范码
					" AND	a.MAT_NO		=	b.MAT_NO "
					" AND   A.MAT_WIDTH		=	DECODE (C.ORDER_WIDTH,0,A.MAT_WIDTH,C.ORDER_WIDTH) "
					" AND   A.MAT_LEN  BETWEEN  DECODE (C.ORDER_MIN_LEN,0,A.MAT_LEN,C.ORDER_MIN_LEN) "
					"                  AND      DECODE (C.ORDER_MAX_LEN,0,A.MAT_LEN,C.ORDER_MAX_LEN) "
					" AND   (A.STOCK_NO     IN  (SELECT STOCK_NO FROM TSI0021 WHERE STOCK_ADDR = '" + c_stock_no + "') "
					"                      OR  A.STOCK_NO = '" + c_stock_no + "' ) ";

// DM8 适配 CHANGE-267:查询。空值搜索 DECODE 改为标准 CASE。
// 改写原因：空值搜索 DECODE 改为标准 CASE,不依赖 NULL 相等匹配的未记载语义；依据 DM 官方文档,DM8 尚未实测。
// 本共用分支面向 DM8,其他 DB_KIND 标签也会执行此 SQL;参数、结果列、条件与排序保持不变。
// 原 SQL（完整保留）：
				// sqlstr = " SELECT DECODE(TRIM(A.BILL_OF_LADING_NO),'',A.BILL_OF_LADING_NO_1,A.BILL_OF_LADING_NO) AS BILL_OF_LADING_NO,"
					// " DECODE(A.ORDER_NO_1,NULL,B.ORDER_NO,A.ORDER_NO_1) AS ORDER_NO,"
					// " A.OLD_ORDER_NO,A.PROD_CNAME,A.RED_FLAG,"
					// " (CASE WHEN B.MAT_SHAPE_FLAG='7' THEN B.LAYERNO || B.ROWNO || B.COLUMN_NO ELSE B.LAYERNO || B.COLUMN_NO END) STOCK_PLACE_POSITION ,"
					// " B.* "
					// " FROM "+ table_name +" B ,(" + sqlstr1 + ") A WHERE B.MAT_NO = A.MAT_NO ";
// DM8 SQL：
				sqlstr = " SELECT CASE WHEN TRIM(A.BILL_OF_LADING_NO) IS NULL OR TRIM(A.BILL_OF_LADING_NO) = '' THEN A.BILL_OF_LADING_NO_1 ELSE A.BILL_OF_LADING_NO END AS BILL_OF_LADING_NO,"
					" CASE WHEN A.ORDER_NO_1 IS NULL THEN B.ORDER_NO ELSE A.ORDER_NO_1 END AS ORDER_NO,"
					" A.OLD_ORDER_NO,A.PROD_CNAME,A.RED_FLAG,"
					" (CASE WHEN B.MAT_SHAPE_FLAG='7' THEN B.LAYERNO || B.ROWNO || B.COLUMN_NO ELSE B.LAYERNO || B.COLUMN_NO END) STOCK_PLACE_POSITION ,"
					" B.* "
					" FROM "+ table_name +" B ,(" + sqlstr1 + ") A WHERE B.MAT_NO = A.MAT_NO ";
				if (c_stock_place_no.Trim() != "")
				{
					sqlstr += " AND B.STOCK_PLACE_NO = '" + c_stock_place_no.Trim() + "' ";
				}
				sqlstr += "ORDER BY B.STOCK_PLACE_NO ASC,B.LAYERNO DESC,B.ROWNO DESC,B.COLUMN_NO DESC ";

			}

			if (c_mat_kind == "HP" && (c_delivy_qty_flag == "2" || c_delivy_qty_flag == "3"))
			{
				sqlstr1 = "SELECT C.BILL_OF_LADING_NO AS BILL_OF_LADING_NO_1 ,C.ORDER_NO AS ORDER_NO_1,A.* FROM TSMPE10 C,TSMPE02 A," + table_name + " B "
					" WHERE C.BILL_OF_LADING_NO = '" + c_bill_of_lading_no + "' "
					" AND a.CONFM_STATUS	=	'4' "
					//" AND a.DELIVY_QTY_FLAG IN ('1','2') "
					//" AND	A.RED_FLAG		!=	'1' "
					" AND	a.MAT_KIND		= '" + c_mat_kind + "' "
					" AND   A.TRNP_MODE_CODE =  C.TRNP_MODE_CODE "	// 运输方式
					//" AND   (A.ORDER_NO      =   C.ORDER_NO   OR   A.CONTRACT_NO = C.CONTRACT_NO ) "
					" AND	a.MAT_NO		=	b.MAT_NO "
					" AND   (A.STOCK_NO     IN  (SELECT STOCK_NO FROM TSI0021 WHERE STOCK_ADDR = '" + c_stock_no + "') "
					"                      OR  A.STOCK_NO = '" + c_stock_no + "' ) ";
				if (c_delivy_qty_flag == "2")
				{
					sqlstr1 += " AND A.ORDER_NO      =   C.ORDER_NO ";
				}
				if (c_delivy_qty_flag == "3")
				{
					sqlstr1 += " A.CONTRACT_NO = C.CONTRACT_NO ";
				}


// DM8 适配 CHANGE-268:查询。空值搜索 DECODE 改为标准 CASE。
// 改写原因：空值搜索 DECODE 改为标准 CASE,不依赖 NULL 相等匹配的未记载语义；依据 DM 官方文档,DM8 尚未实测。
// 本共用分支面向 DM8,其他 DB_KIND 标签也会执行此 SQL;参数、结果列、条件与排序保持不变。
// 原 SQL（完整保留）：
				// sqlstr = " SELECT A.BILL_OF_LADING_NO_1 AS BILL_OF_LADING_NO,DECODE(A.ORDER_NO_1,NULL,B.ORDER_NO,A.ORDER_NO_1) AS ORDER_NO "
					// " ,A.OLD_ORDER_NO,A.PROD_CNAME,A.RED_FLAG,B.* ";
// DM8 SQL：
				sqlstr = " SELECT A.BILL_OF_LADING_NO_1 AS BILL_OF_LADING_NO,CASE WHEN A.ORDER_NO_1 IS NULL THEN B.ORDER_NO ELSE A.ORDER_NO_1 END AS ORDER_NO "
					" ,A.OLD_ORDER_NO,A.PROD_CNAME,A.RED_FLAG,B.* ";
				if (c_stock_place_no.Trim() != "")
				{
					sqlstr += " FROM TMMHP01 B LEFT JOIN (" + sqlstr1 + ") A ON B.MAT_NO = A.MAT_NO WHERE 1=1 ";
					sqlstr += " AND B.STOCK_PLACE_NO = '" + c_stock_place_no.Trim() + "' ";
				}
				else
				{
					sqlstr += " FROM TMMHP01 B ,(" + sqlstr1 + ") A WHERE B.MAT_NO = A.MAT_NO ";
				}
				sqlstr += " ORDER BY B.STOCK_PLACE_NO , B.LAYERNO DESC ";
			}
		}


		cmd_loop.SetCommandText(sqlstr);
		Log::Debug("", "", "sqlstr={0}", sqlstr);
		cmd_loop.ExecuteQuery(bcls_ret->Tables[0]);

		if (!bcls_ret->Tables[0].Columns.Contains("COLOR_MARK"))
		{
			bcls_ret->Tables[0].Columns.Add(DT_STRING, "COLOR_MARK");
		}

		CString v_bill_of_lading_no = "";
		CString v_red_flag = "";
		for (int i = 0; i < bcls_ret->Tables[0].Rows.get_Count(); i++)
		{
			v_bill_of_lading_no = bcls_ret->Tables[0].Rows[i]["BILL_OF_LADING_NO"].ToString().Trim();
			//Log::Info("", "", "v_bill_of_lading_no={0}", v_bill_of_lading_no);
			if (v_bill_of_lading_no == c_bill_of_lading_no)
			{
				bcls_ret->Tables[0].Rows[i]["COLOR_MARK"] = "1";	// 可发货
			}
			else if (v_bill_of_lading_no == "")
			{
				bcls_ret->Tables[0].Rows[i]["COLOR_MARK"] = "0";	// 不可发货
			}
			else
			{
				bcls_ret->Tables[0].Rows[i]["COLOR_MARK"] = "2";	// 在其他计划配货中
			}
			v_red_flag = bcls_ret->Tables[0].Rows[i]["RED_FLAG"].ToString().Trim();
			if (v_red_flag == "1")
			{
				bcls_ret->Tables[0].Rows[i]["COLOR_MARK"] = "3";	// 准发红冲请求中
			}
		}

		sprintf	(s.msg , "处理成功");
	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg)-1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		Log::Error("", __FUNCTION__, "error=[{0}]", s.sysmsg);
		//__AG_DB_EXCEPTION_;			//使用EAppDef.h中宏定义
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
	if (doFlag < 0)
	{
		//CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0}：{1}", arguments, 2);
	}

	return doFlag;

}
