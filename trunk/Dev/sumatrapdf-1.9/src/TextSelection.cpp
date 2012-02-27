/* Copyright 2006-2011 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "TextSelection.h"
#include "Vec.h"

/*MyCode*/
#include "..\..\..\..\biggod.dev\Ctrl\Utility.hpp"
extern "C"{
#include "mupdf.h"
};
//////////////////////////////////////////////////////////////////////////

TextSelection::TextSelection(BaseEngine *engine) : engine(engine)
{
    int count = engine->PageCount();
    coords = SAZA(RectI *, count);
	chInf = SAZA(char_inf *, count); //MyCode
    text = SAZA(TCHAR *, count);
    lens = SAZA(int, count);

    result.len = 0;
    result.pages = NULL;
    result.rects = NULL;

    startPage = -1;
    endPage = -1;
    startGlyph = -1;
    endGlyph = -1;
}

TextSelection::~TextSelection()
{
    Reset();

    for (int i = 0; i < engine->PageCount(); i++) {
        delete[] coords[i];
        coords[i] = NULL;
        str::ReplacePtr(&text[i], NULL);

		/*MyCode*/
		delete[] chInf[i];
		chInf[i] = NULL;
    }

    free(coords);
	free(chInf); //MyCode
    free(text);
    free(lens);
}

void TextSelection::Reset()
{
    result.len = 0;
    free(result.pages);
    result.pages = NULL;
    free(result.rects);
    result.rects = NULL;
}

// returns the index of the glyph closest to the right of the given coordinates
// (i.e. when over the right half of a glyph, the returned index will be for the
// glyph following it, which will be the first glyph (not) to be selected)
int TextSelection::FindClosestGlyph(int pageNo, double x, double y)
{
    assert(1 <= pageNo && pageNo <= engine->PageCount());
    if (!text[pageNo - 1]) {
        text[pageNo - 1] = engine->ExtractPageText(pageNo, _T("\n"), &coords[pageNo - 1], Target_View, &chInf[pageNo - 1]);

		/*MyCode*/
		if(chInf[pageNo - 1])
		{
			assert(chInf[pageNo - 1][0].node);
		}

        if (!text[pageNo - 1]) {
            text[pageNo - 1] = str::Dup(_T(""));
            lens[pageNo - 1] = 0;
            return 0;
        }
        lens[pageNo - 1] = (int)str::Len(text[pageNo - 1]);
    }

    double maxDist = -1;
    int result = -1;
    RectI *_coords = coords[pageNo - 1];

    for (int i = 0; i < lens[pageNo - 1]; i++) {
        if (!_coords[i].x && !_coords[i].dx)
            continue;

        double dist = _hypot(x - _coords[i].x - 0.5 * _coords[i].dx,
                             y - _coords[i].y - 0.5 * _coords[i].dy);
        if (maxDist < 0 || dist < maxDist) {
            result = i;
            maxDist = dist;
        }
    }

    if (-1 == result)
        return 0;
    assert(0 <= result && result < lens[pageNo - 1]);

    // the result indexes the first glyph to be selected in a forward selection
    RectD bbox = engine->Transform(_coords[result].Convert<double>(), pageNo, 1.0, 0);
    PointD pt = engine->Transform(PointD(x, y), pageNo, 1.0, 0);
    if (pt.x > bbox.x + 0.5 * bbox.dx)
        result++;

    return result;
}

void TextSelection::FillResultRects(int pageNo, int glyph, int length, StrVec *lines)
{
    RectI mediabox = engine->PageMediabox(pageNo).Round();
    RectI *c = &coords[pageNo - 1][glyph], *end = c + length;
    for (; c < end; c++) {
        // skip line breaks
        if (!c->x && !c->dx)
            continue;

        RectI c0 = *c, *c0p = c;
        for (; c < end && (c->x || c->dx); c++);
        c--;
        RectI c1 = *c;
        RectI bbox = c0.Union(c1).Intersect(mediabox);
        // skip text that's completely outside a page's mediabox
        if (bbox.IsEmpty())
            continue;

        if (lines) {
            lines->Push(str::DupN(text[pageNo - 1] + (c0p - coords[pageNo - 1]), c - c0p + 1));
            continue;
        }

        // cut the right edge, if it overlaps the next character
        if ((c[1].x || c[1].dx) && bbox.x < c[1].x && bbox.x + bbox.dx > c[1].x)
            bbox.dx = c[1].x - bbox.x;

        result.len++;
        result.pages = (int *)realloc(result.pages, sizeof(int) * result.len);
        result.pages[result.len - 1] = pageNo;
        result.rects = (RectI *)realloc(result.rects, sizeof(RectI) * result.len);
        result.rects[result.len - 1] = bbox;
    }
}

bool TextSelection::IsOverGlyph(int pageNo, double x, double y)
{
    int glyphIx = FindClosestGlyph(pageNo, x, y);
    PointI pt = PointD(x, y).Convert<int>();
    RectI *_coords = coords[pageNo - 1];
    // when over the right half of a glyph, FindClosestGlyph returns the
    // index of the next glyph, in which case glyphIx must be decremented
    if (glyphIx == lens[pageNo - 1] || !_coords[glyphIx].Inside(pt))
        glyphIx--;
    if (-1 == glyphIx)
        return false;
    return _coords[glyphIx].Inside(pt);
}

void TextSelection::StartAt(int pageNo, int glyphIx)
{
    startPage = pageNo;
    startGlyph = glyphIx;
    if (glyphIx < 0) {
        FindClosestGlyph(pageNo, 0, 0);
        startGlyph = lens[pageNo - 1] + glyphIx + 1;
    }
}

void TextSelection::SelectUpTo(int pageNo, int glyphIx)
{
    if (startPage == -1 || startGlyph == -1)
        return;

    endPage = pageNo;
    endGlyph = glyphIx;
    if (glyphIx < 0) {
        FindClosestGlyph(pageNo, 0, 0);
        endGlyph = lens[pageNo - 1] + glyphIx + 1;
    }

    result.len = 0;
    int fromPage = min(startPage, endPage), toPage = max(startPage, endPage);
    int fromGlyph = (fromPage == endPage ? endGlyph : startGlyph);
    int toGlyph = (fromPage == endPage ? startGlyph : endGlyph);
    if (fromPage == toPage && fromGlyph > toGlyph)
        swap(fromGlyph, toGlyph);

    for (int page = fromPage; page <= toPage; page++) {
        // make sure that glyph coordinates and page text have been cached
        if (!coords[page - 1])
            FindClosestGlyph(page, 0, 0);

        int glyph = page == fromPage ? fromGlyph : 0;
        int length = (page == toPage ? toGlyph : lens[page - 1]) - glyph;
        if (length > 0)
            FillResultRects(page, glyph, length);
    }
}

void TextSelection::SelectWordAt(int pageNo, double x, double y)
{
    int ix = FindClosestGlyph(pageNo, x, y);

    for (; ix > 0; ix--)
        if (!iswordchar(text[pageNo - 1][ix - 1]))
            break;
    StartAt(pageNo, ix);

    for (; ix < lens[pageNo - 1]; ix++)
        if (!iswordchar(text[pageNo - 1][ix]))
            break;
    SelectUpTo(pageNo, ix);
}

void TextSelection::CopySelection(TextSelection *orig)
{
    Reset();
    StartAt(orig->startPage, orig->startGlyph);
    SelectUpTo(orig->endPage, orig->endGlyph);
}

TCHAR *TextSelection::ExtractText(TCHAR *lineSep)
{
    StrVec lines;

    int fromPage = min(startPage, endPage), toPage = max(startPage, endPage);
    int fromGlyph = (fromPage == endPage ? endGlyph : startGlyph);
    int toGlyph = (fromPage == endPage ? startGlyph : endGlyph);
    if (fromPage == toPage && fromGlyph > toGlyph)
        swap(fromGlyph, toGlyph);

    for (int page = fromPage; page <= toPage; page++) {
        int glyph = page == fromPage ? fromGlyph : 0;
        int length = (page == toPage ? toGlyph : lens[page - 1]) - glyph;
        if (length > 0)
            FillResultRects(page, glyph, length, &lines);
    }

    return lines.Join(lineSep);
}

/*MyCode*/
//chPos：光标所在的字符位置
INT TextSelection::GetObjLineText(int pageNo, HXOBJ hObj, const PointD* pt, RectD* rtText, DOUBLE* xCursor, INT* chPos)
{
	INT lineTextPos = -1;

	if(text[pageNo - 1] && chInf[pageNo - 1])
	{
		WCHAR* pageText = text[pageNo - 1];
		RectI* pageCoords = coords[pageNo - 1];
		char_inf* pageChInf = chInf[pageNo - 1];

		int textLen = str::Len(pageText);
		if(textLen > 0)
		{
			int iPos = 0;
			while(iPos < textLen)
			{
				if(lineTextPos != -1)
					break;

				int iObjFirstCh = -1;
				for(int i = iPos;i < textLen;i++)
				{
					const char_inf& ci = pageChInf[i];

					if(ci.node != hObj || ci.iItem==-1)
						continue;

					iObjFirstCh = i;
					break;
				}

				if(iObjFirstCh == -1)
					break;

				int iFirstGoodCh = -1;
				for(int i = iObjFirstCh;i < textLen;i++)
				{
					const char_inf& ci = pageChInf[i];

					if(ci.node != hObj || ci.iItem==-1)
					{
						iPos = i;
						break;
					}

					if(pt->y >= pageCoords[i].y)
					{
						iFirstGoodCh = i;
						break;
					}
				}

				if(iFirstGoodCh != -1)
				{
					double left = 0.0;
					double top = 0.0;
					double right = 0.0;
					double bottom = 0.0;

					DOUBLE cursorDist = 10000.0;

					for(int i = iFirstGoodCh;i < textLen;i++)
					{
						const char_inf& ci = pageChInf[i];

						if(ci.node != hObj || pageText[i]=='\n')
						{
							iPos = i;
							break;
						}

						//计算行矩形范围
						if(i==iFirstGoodCh)
						{
							lineTextPos = i;

							left = pageCoords[i].x;
							top = pageCoords[i].y;
							right = pageCoords[i].x + pageCoords[i].dx;
							bottom = pageCoords[i].y + pageCoords[i].dy;									
						}
						else
						{
							if(pageCoords[i].x < left)
								left = pageCoords[i].x;
							if(pageCoords[i].y < top)
								top = pageCoords[i].y;
							if(pageCoords[i].x + pageCoords[i].dx > right)
								right = pageCoords[i].x + pageCoords[i].dx;
							if(pageCoords[i].y + pageCoords[i].dy > bottom)
								bottom = pageCoords[i].y + pageCoords[i].dy;									
						}

						//计算光标位置
						if(xCursor)
						{
							DOUBLE dist = fabs(pageCoords[i].x - pt->x);
							if(dist < cursorDist)
							{
								*xCursor = pageCoords[i].x;
								cursorDist = dist;

								if(chPos)
									*chPos = i;
							}

							dist = fabs((pageCoords[i].x + pageCoords[i].dx) - pt->x);
							if(dist < cursorDist)
							{
								if(i + 1 < textLen && pageText[i + 1]!='\n')
								{
									*xCursor = pageCoords[i + 1].x;

									if(chPos)
										*chPos = i + 1;
								}
								else
								{
									*xCursor = pageCoords[i].x + pageCoords[i].dx;

									if(chPos)
										*chPos = i;
								}

								cursorDist = dist;
							}
						}
					}

					if(rtText)
					{
						rtText->x = left;
						rtText->y = top;
						rtText->dx = right - left;
						rtText->dy = bottom - top;
					}
				}
			}
		}
	}

	return lineTextPos;
}
TCHAR* TextSelection::ExtractObjText(int pageNo, HXOBJ hObj, const PointD* pt, RectD* rtText, DOUBLE* xCursor)
{
	assert(1 <= pageNo && pageNo <= engine->PageCount());
	if (!coords[pageNo - 1])
		FindClosestGlyph(pageNo, 0, 0);

	INT lineTextPos = GetObjLineText(pageNo,hObj,pt,rtText,xCursor);
	if(lineTextPos==-1)
		return NULL;

	WCHAR* pageText = text[pageNo - 1];
	return &pageText[lineTextPos];
}
BOOL TextSelection::DeleteCharByPos(int pageNo, HXOBJ hObj, const PointD& pt, BOOL bBackspace, DOUBLE* xCursor)
{
	assert(1 <= pageNo && pageNo <= engine->PageCount());
	if (!coords[pageNo - 1])
		FindClosestGlyph(pageNo, 0, 0);

	INT chPos = -1;
	DOUBLE xCursor1 = 0.0;
	INT lineTextPos = GetObjLineText(pageNo,hObj,&pt,NULL,&xCursor1,&chPos);
	if(lineTextPos==-1 || chPos==-1)
		return FALSE;

	WCHAR* pageText = text[pageNo - 1];
	char_inf* pageChInf = chInf[pageNo - 1];

	INT iPosDel = chPos - 1;
	if(iPosDel < lineTextPos)
		return FALSE;

	char_inf& ci = pageChInf[iPosDel];
	if(ci.iItem==-1)
	{
		if(pageText[iPosDel] != ' ' || ci.node != hObj)
			return FALSE;
	}

	assert(ci.node == hObj);

	if(ci.iItem != -1)
	{
		assert(ci.iItem >= 0 && ci.iItem < ci.node->item.text->len);
	}

#if 0
	ci.node->item.text->items[ci.iItem].gid = 2;
	ci.node->item.text->items[ci.iItem].ucs = 'C';
	pageText[iPosDel] = 'C';
#else
	INT pageTextLen = lens[pageNo - 1];

	RectI* pageCoords = coords[pageNo - 1];
	INT widthDelta = -pageCoords[iPosDel].dx;
	if(iPosDel + 1 < pageTextLen)
	{
		widthDelta = -(pageCoords[iPosDel + 1].x - pageCoords[iPosDel].x);
	}

	INT indexChanged = 0;
	if(ci.iItem != -1)
	{
		assert(ci.node->item.text->items[ci.iItem].ucs==pageText[iPosDel]);
		ArrayDeleteElements(ci.node->item.text->items,ci.node->item.text->len,ci.iItem,1);

		indexChanged = -1;
	}
	else
	{
		assert(pageText[iPosDel]==' ');
	}

	pageTextLen = lens[pageNo - 1];
	ArrayDeleteElements(pageText,pageTextLen,iPosDel,1);
	pageText[pageTextLen] = _T('\0');

	pageTextLen = lens[pageNo - 1];
	ArrayDeleteElements(pageChInf,pageTextLen,iPosDel,1);

	pageTextLen = lens[pageNo - 1];
	ArrayDeleteElements(pageCoords,pageTextLen,iPosDel,1);

	lens[pageNo - 1] = pageTextLen;
	assert(str::Len(pageText)==pageTextLen);	

	for(INT i = iPosDel;i < pageTextLen;i++)
	{
		if(pageText[i]=='\n')
			widthDelta = 0;

		char_inf& ci = pageChInf[i];
		if(ci.iItem==-1)
		{
			if(widthDelta)
				pageCoords[i].x += widthDelta;

			continue;
		}

		if(ci.node==hObj)
		{
			if(indexChanged)
				ci.iItem += indexChanged;

			if(widthDelta)
			{
				ci.node->item.text->items[ci.iItem].x += widthDelta;
				pageCoords[i].x += widthDelta;
			}
		}
	}

	if(xCursor)
	{
		if(iPosDel < pageTextLen)
			*xCursor = pageCoords[iPosDel].x;
		else
		{
			if(pageTextLen > 0)
				*xCursor = pageCoords[pageTextLen - 1].x + pageCoords[pageTextLen - 1].dx;
		}
	}
#endif

	return TRUE;
}

//从res_text.c拷贝过来
static void
fz_grow_text(fz_text *text, int n)
{
	if (text->len + n < text->cap)
		return;
	while (text->len + n > text->cap)
		text->cap = text->cap + 36;
	text->items = (fz_text_item*)fz_realloc(text->items, text->cap, sizeof(fz_text_item));
}

static void
my_pdf_show_char(my_pdf_gstate *gstate,int cid,fz_matrix& tm)
{
	//pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;
	fz_matrix tsm, trm;
	float w0, w1, tx, ty;
	pdf_hmtx h;
	pdf_vmtx v;
	int gid;
	int ucsbuf[8];
	int ucslen;
	//int i;

	tsm.a = gstate->size * gstate->scale;
	tsm.b = 0;
	tsm.c = 0;
	tsm.d = gstate->size;
	tsm.e = 0;
	tsm.f = gstate->rise;

	ucslen = 0;
	if (fontdesc->to_unicode)
		ucslen = pdf_lookup_cmap_full(fontdesc->to_unicode, cid, ucsbuf);
	if (ucslen == 0 && cid < fontdesc->cid_to_ucs_len)
	{
		ucsbuf[0] = fontdesc->cid_to_ucs[cid];
		ucslen = 1;
	}
	if (ucslen == 0 || (ucslen == 1 && ucsbuf[0] == 0))
	{
		ucsbuf[0] = '?';
		ucslen = 1;
	}

	gid = pdf_font_cid_to_gid(fontdesc, cid);

	/* cf. http://code.google.com/p/sumatrapdf/issues/detail?id=1149 */
	if (fontdesc->wmode == 1 && fontdesc->font->ft_face)
		gid = pdf_ft_get_vgid(fontdesc, gid);

	if (fontdesc->wmode == 1)
	{
		v = pdf_get_vmtx(fontdesc, cid);
		tsm.e -= v.x * gstate->size * 0.001f;
		tsm.f -= v.y * gstate->size * 0.001f;
	}

	trm = fz_concat(tsm, tm);

#if 0
	/* flush buffered text if face or matrix or rendermode has changed */
	if (!csi->text ||
		fontdesc->font != csi->text->font ||
		fontdesc->wmode != csi->text->wmode ||
		fabsf(trm.a - csi->text->trm.a) > FLT_EPSILON ||
		fabsf(trm.b - csi->text->trm.b) > FLT_EPSILON ||
		fabsf(trm.c - csi->text->trm.c) > FLT_EPSILON ||
		fabsf(trm.d - csi->text->trm.d) > FLT_EPSILON ||
		gstate->render != csi->text_mode)
	{
		pdf_flush_text(csi);

		csi->text = fz_new_text(fontdesc->font, trm, fontdesc->wmode);
		csi->text->trm.e = 0;
		csi->text->trm.f = 0;
		csi->text_mode = gstate->render;
	}

	/* add glyph to textobject */
	fz_add_text(csi->text, gid, ucsbuf[0], trm.e, trm.f);

	/* add filler glyphs for one-to-many unicode mapping */
	for (i = 1; i < ucslen; i++)
		fz_add_text(csi->text, -1, ucsbuf[i], trm.e, trm.f);
#endif

	if (fontdesc->wmode == 0)
	{
		h = pdf_get_hmtx(fontdesc, cid);
		w0 = h.w * 0.001f;
		tx = (w0 * gstate->size + gstate->char_space) * gstate->scale;
		tm = fz_concat(fz_translate(tx, 0), tm);
	}

	if (fontdesc->wmode == 1)
	{
		w1 = v.w * 0.001f;
		ty = w1 * gstate->size + gstate->char_space;
		tm = fz_concat(fz_translate(0, ty), tm);
	}
}

static unsigned char* ansii_to_cid(pdf_font_desc *fontdesc,unsigned char* buf,int& cid)
{
	int cpt;

	buf = pdf_decode_cmap(fontdesc->encoding, buf, &cpt);
	cid = pdf_lookup_cmap(fontdesc->encoding, cpt);
	if (cid < 0)
	{
		fz_warn("cannot encode character with code point %#x", cpt);
		return NULL;
	}
//	if (cpt == 32)
//			pdf_show_space(csi, gstate->word_space);

	return buf;
}

BOOL TextSelection::InsertCharByPos(int pageNo, HXOBJ hObj, const PointD& pt, WCHAR chIns, DOUBLE* xCursor)
{
	assert(1 <= pageNo && pageNo <= engine->PageCount());
	if (!coords[pageNo - 1])
		FindClosestGlyph(pageNo, 0, 0);

	INT chPos = -1;
	DOUBLE xCursor1 = 0.0;
	INT lineTextPos = GetObjLineText(pageNo,hObj,&pt,NULL,&xCursor1,&chPos);
	if(lineTextPos==-1 || chPos==-1)
		return FALSE;

	WCHAR* pageText = text[pageNo - 1];
	char_inf* pageChInf = chInf[pageNo - 1];

	INT iPosIns = chPos;

	char_inf& ci = pageChInf[iPosIns];
	if(ci.iItem==-1)
	{
		if(pageText[iPosIns] != ' ' || ci.node != hObj)
			return FALSE;
	}

	assert(ci.node == hObj);

	if(ci.iItem != -1)
	{
		assert(ci.iItem >= 0 && ci.iItem < ci.node->item.text->len);
	}

	INT pageTextLen = lens[pageNo - 1];
	RectI* pageCoords = coords[pageNo - 1];

	INT widthDelta = 7;
	
	INT indexChanged = 0;
	if(ci.iItem != -1)
	{
		fz_grow_text(ci.node->item.text,1);

		fz_text_item txtItem;
		txtItem = ci.node->item.text->items[ci.iItem];
		//txtItem.x = txtItem.x + pageCoords[iPosIns].dx;
		widthDelta = pageCoords[iPosIns + 1].x - pageCoords[iPosIns].x;

		int cid = 0;
		unsigned char buf[] = {txtItem.ucs,0};
		ansii_to_cid(ci.node->item.text->gstate.font,buf,cid);
		fz_matrix tm = ci.node->item.text->trm;
		my_pdf_show_char(&ci.node->item.text->gstate,cid,tm);
		widthDelta = (int)ceilf(tm.e - 0.001f);

		chIns = txtItem.ucs;

		ArrayInsertElements(ci.node->item.text->items,ci.node->item.text->len,ci.iItem,&txtItem,1);

		indexChanged = 1;
	}
	else
	{
		assert(pageText[iPosIns]==' ');
	}

	{
		pageTextLen = lens[pageNo - 1];
		
		INT bufSize = pageTextLen + 1;
		EnsureBufSize(&pageText,bufSize,bufSize,bufSize + 1,EBS_CAlloc);
		text[pageNo - 1] = pageText;

		ArrayInsertElements(pageText,pageTextLen,iPosIns,&chIns,1);
		pageText[pageTextLen] = _T('\0');
	}

	{
		pageTextLen = lens[pageNo - 1];

		INT bufSize = pageTextLen + 1;
		EnsureBufSize(&pageChInf,bufSize,bufSize,bufSize + 1,NULL);
		chInf[pageNo - 1] = pageChInf;

		char_inf newCI = pageChInf[iPosIns];
		ArrayInsertElements(pageChInf,pageTextLen,iPosIns,&newCI,1);
	}

	{
		pageTextLen = lens[pageNo - 1];

		INT bufSize = pageTextLen + 1;
		EnsureBufSize(&pageCoords,bufSize,bufSize,bufSize + 1,NULL);
		coords[pageNo - 1] = pageCoords;

		RectI newRT = pageCoords[iPosIns];
		ArrayInsertElements(pageCoords,pageTextLen,iPosIns,&newRT,1);
	}

	lens[pageNo - 1] = pageTextLen;
	assert(str::Len(pageText)==pageTextLen);

	for(INT i = iPosIns + 1;i < pageTextLen;i++)
	{
		if(pageText[i]=='\n')
			widthDelta = 0;

		char_inf& ci = pageChInf[i];
		if(ci.iItem==-1)
		{
			if(widthDelta)
				pageCoords[i].x += widthDelta;

			continue;
		}

		if(ci.node==hObj)
		{
			if(indexChanged)
				ci.iItem += indexChanged;

			if(widthDelta)
			{
				ci.node->item.text->items[ci.iItem].x += widthDelta;
				pageCoords[i].x += widthDelta;
			}
		}
	}

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////