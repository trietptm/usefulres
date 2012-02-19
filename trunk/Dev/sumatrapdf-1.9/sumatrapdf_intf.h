#pragma once

class SumatraPdfIntf
{
public:
	virtual void AfterDrawPage(HDC hdc,INT x,INT y,INT cx,INT cy){};
	virtual void OnPageLoad(INT pageNo){}
};