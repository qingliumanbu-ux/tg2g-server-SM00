/// <summary>
/// 功能说明: 通用查询功能信息查询
/// </summary>
/// Copyright: Baosight Software LTD.co Copyright (c) 2010
/// Company: 上海宝信软件股份有限公司
/// Author:   项目组
/// Version:  1.0
/// History:  2016-04-08 张金金 [创建]
///           2016-04-08 项目组 修改
///	

#include "stdafx.h"
#include "Be2UserModel/SI/CFormDevConfig.h"

// Service 入口
BM2F_ENTERACE(sm00_save)
int f_sm00_save(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	int doFlag = 0;

	try
	{
		doFlag = BE2::CFormDevConfig::SaveUtility(bcls_rec, bcls_ret, conn);
	}
	catch (CException& ex)
	{
		strcpy(s.msg, ex.GetMsg());
		s.flag = -1;
		doFlag = -1;
	}
	return doFlag;
}