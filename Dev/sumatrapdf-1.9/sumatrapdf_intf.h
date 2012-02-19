#pragma once

class PdfObj
{
public:
	virtual ~PdfObj(){}
};

class SumatraPdfIntf
{
public:
	PdfObj* (*ExtraPdfObjects)(INT& nObj); //返回的指针要用DeletePdfObjects释放
	void (*DeletePdfObjects)(PdfObj* pdfObjs); //delete PdfObj数组
public:
	SumatraPdfIntf()
	{
		ExtraPdfObjects = NULL;
		DeletePdfObjects = NULL;
	}
	virtual void AfterDrawPage(HDC hdc,INT x,INT y,INT cx,INT cy){};
	virtual void OnPageLoad(INT pageNo){}
};