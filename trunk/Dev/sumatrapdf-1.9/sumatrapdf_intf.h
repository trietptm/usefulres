#pragma once

class PdfObj
{
public:
	virtual ~PdfObj(){}
};

class SumatraPdfIntf
{
public:
	PdfObj* (*ExtraPdfObjects)(INT& nObj); //���ص�ָ��Ҫ��DeletePdfObjects�ͷ�
	void (*DeletePdfObjects)(PdfObj* pdfObjs); //delete PdfObj����
public:
	SumatraPdfIntf()
	{
		ExtraPdfObjects = NULL;
		DeletePdfObjects = NULL;
	}
	virtual void AfterDrawPage(HDC hdc,INT x,INT y,INT cx,INT cy){};
	virtual void OnPageLoad(INT pageNo){}
};