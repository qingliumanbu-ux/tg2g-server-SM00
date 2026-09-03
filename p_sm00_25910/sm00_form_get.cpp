/// <summary>
/// 功能说明: 画面布局配置获取
/// </summary>
/// Copyright: Baosight Software LTD.co Copyright (c) 2010
/// Company: 上海宝信软件股份有限公司
/// Author:   项目组
/// Version:  1.0
/// History:  2015-12-2 kimmy [创建]
///          
///	

#include "stdafx.h"
#include "Be2UserModel/SI/CFormDevConfig.h"

// Service 入口
BM2F_ENTERACE(sm00_form_get)
int f_sm00_form_get(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	int doFlag = 0;

	try
	{
		EDLog(1, 1, "==================== xxxx_form_get Begin ====================");

		doFlag = BE2::CFormDevConfig::GetFormDevConfig(bcls_rec, bcls_ret, conn);

		if (doFlag != 0)
		{
			s.flag = -1;
			return -1;
		}

		EDLog(1, 1, "==================== xxxx_form_get End ====================");

	}
	catch (CException& ex)
	{
		strcpy(s.msg, ex.GetMsg());
		EDLog(1, 1, "s.msg = [%s]", s.msg);
		s.flag = -1;
		doFlag = -1;
	}
	return doFlag;
}