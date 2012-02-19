#pragma once

class FRect
{
public:
	float x0, y0;
	float x1, y1;
public:
	FRect()
	{
		x0 = y0 = 0.0;
		x1 = y1 = 0.0;
	}
};

class PdfObj
{
public:
	FRect rect;
public:
	virtual ~PdfObj(){}
};

class SumatraPdfIntf
{
public:
	PdfObj* (*ExtraPdfObjects)(INT pageNo,INT& nObj); //返回的指针要用DeletePdfObjects释放
	void (*DeletePdfObjects)(PdfObj* pdfObjs); //delete PdfObj数组
public:
	SumatraPdfIntf()
	{
		ExtraPdfObjects = NULL;
		DeletePdfObjects = NULL;
	}
	virtual void AfterDrawPage(HDC hdc,const RECT& pageOnScreen){};
	virtual void OnPageLoad(INT pageNo){}
};