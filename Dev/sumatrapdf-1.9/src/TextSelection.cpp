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
    coords = SAZA(RectD *, count);
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
        text[pageNo - 1] = engine->ExtractPageTextD(pageNo, _T("\n"), &coords[pageNo - 1], Target_View, &chInf[pageNo - 1]);

		/*MyCode*/
		if(text[pageNo - 1] && text[pageNo - 1][0] && text[pageNo - 1][0] != '\n' && chInf[pageNo - 1])
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
    RectD *_coords = coords[pageNo - 1];

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
    RectD mediabox = engine->PageMediabox(pageNo);//@.Round();
    RectD *c = &coords[pageNo - 1][glyph], *end = c + length;
    for (; c < end; c++) {
        // skip line breaks
        if (!c->x && !c->dx)
            continue;

        RectD c0 = *c, *c0p = c;
        for (; c < end && (c->x || c->dx); c++);
        c--;
        RectD c1 = *c;
        RectD bbox = c0.Union(c1).Intersect(mediabox);
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
        result.rects[result.len - 1] = bbox.Round(); //Round():MyCode
    }
}

bool TextSelection::IsOverGlyph(int pageNo, double x, double y)
{
    int glyphIx = FindClosestGlyph(pageNo, x, y);
    PointD pt = PointD(x, y);//@.Convert<int>();
    RectD *_coords = coords[pageNo - 1];
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
INT TextSelection::GetObjLineText(int pageNo, HPDFOBJ hObj, const PointD* pt, FRect* rtText, DOUBLE* xCursor, INT* chPos, INT* endLinePos)
{
	INT lineTextPos = -1;

	if(text[pageNo - 1] && chInf[pageNo - 1])
	{
		WCHAR* pageText = text[pageNo - 1];
		RectD* pageCoords = coords[pageNo - 1];
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

					if(ci.node != (fz_display_node*)hObj || ci.iItem==-1 || pageText[i]=='\n')
						continue;

					iObjFirstCh = i;
					break;
				}

				if(iObjFirstCh == -1)
					break;

#if 0
				int iFirstGoodCh = -1;
				for(int i = iObjFirstCh;i < textLen;i++)
				{
					const char_inf& ci = pageChInf[i];

					if(ci.node != (fz_display_node*)hObj || ci.iItem==-1)
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
#else
				int iFirstGoodCh = iObjFirstCh;
#endif

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

						if(ci.node != (fz_display_node*)hObj || pageText[i]=='\n')
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

							if(endLinePos)
								*endLinePos = i + 1;

							bottom = pageCoords[i].y + pageCoords[i].dy;									
						}
						else
						{
							if(pageCoords[i].x < left)
								left = pageCoords[i].x;
							if(pageCoords[i].y < top)
								top = pageCoords[i].y;
							if(pageCoords[i].x + pageCoords[i].dx > right)
							{
								right = pageCoords[i].x + pageCoords[i].dx;

								if(endLinePos)
									*endLinePos = i + 1;
							}
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

#if 0
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
#else
							if(i == textLen - 1 || pageText[i + 1]=='\n')
							{
								dist = fabs((pageCoords[i].x + pageCoords[i].dx) - pt->x);
								if(dist < cursorDist)
								{
									*xCursor = pageCoords[i].x + pageCoords[i].dx;

									if(chPos)
										*chPos = i + 1;
								}
							}
#endif
						}
					}

					if(rtText)
					{
						rtText->x0 = left;
						rtText->y0 = top;
						rtText->x1 = right;
						rtText->y1 = bottom;
					}
				}
			}
		}
	}

	return lineTextPos;
}
TCHAR* TextSelection::ExtractObjLineText(int pageNo, HPDFOBJ hObj, const PointD* pt, SumatraPdfIntf::LineTextResult& ltr)
{
	assert(1 <= pageNo && pageNo <= engine->PageCount());
	if (!coords[pageNo - 1])
		FindClosestGlyph(pageNo, 0, 0);

	INT endLinePos = 0;
	INT lineTextPos = GetObjLineText(pageNo,hObj,pt,ltr.rtText,ltr.xCursor,NULL,&endLinePos);
	if(lineTextPos==-1)
		return NULL;

	if(ltr.textPos)
		*ltr.textPos = lineTextPos;

	INT tLen = endLinePos - lineTextPos;
	if(ltr.textLen)
		*ltr.textLen = tLen;

	if(ltr.ppRawText)
	{
		if(tLen > 0)
		{
			WCHAR* pageText = text[pageNo - 1];
			char_inf* pageChInf = chInf[pageNo - 1];
			INT pageTextLen = lens[pageNo - 1];

			INT bufSize = 0;
			INT len = 0;
			WCHAR* buf = NULL;
			EnsureBufSize(&buf,bufSize,len,tLen + 1,EBS_CAlloc | EBS_ZeroInit);

			WCHAR ofsBuf[MAX_PATH] = {0};

			for(INT i = lineTextPos;i < endLinePos;i++)
			{
				assert(i >= 0 && i < pageTextLen);

				char_inf& ci = pageChInf[i];
				if(ci.iItem==-1 || ci.node != (fz_display_node*)hObj)
					continue;

				assert(ci.node->item.text->items[ci.iItem].ucs==pageText[i]);

				EnsureBufSize(&buf,bufSize,len,len + 1 + 1,EBS_CAlloc | EBS_ZeroInit);
				buf[len] = pageText[i];
				buf[len + 1] = 0;
				len++;

				if(ci.node->item.text->items[ci.iItem].offset==0.0)
					continue;

				_snwprintf(ofsBuf,MAX_PATH - 1,L"[%.2f]",ci.node->item.text->items[ci.iItem].offset);
				INT ofsBufLen = wcslen(ofsBuf);
				EnsureBufSize(&buf,bufSize,len,len + 1 + ofsBufLen,EBS_CAlloc | EBS_ZeroInit);
				memcpy(&buf[len],ofsBuf,(ofsBufLen + 1) * sizeof(WCHAR));
				len += ofsBufLen;
			}

			if(buf)
				*ltr.ppRawText = buf;
		}
	}
	
	WCHAR* pageText = text[pageNo - 1];
	return &pageText[lineTextPos];
}
BOOL TextSelection::DeleteCharByPos(int pageNo, HPDFOBJ hObj, const PointD& pt, BOOL bBackspace, DOUBLE* xCursor, RectD* updateRect)
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
	INT pageTextLen = lens[pageNo - 1];
	RectD* pageCoords = coords[pageNo - 1];

	INT iPosDel = chPos;
	if(bBackspace)
		iPosDel--;

	if(iPosDel < lineTextPos || iPosDel >= pageTextLen)
		return FALSE;

	if(pageText[iPosDel]=='\n')
		return FALSE;

	double rawPosX = pageCoords[iPosDel].x;

	char_inf& ci = pageChInf[iPosDel];
	if(ci.iItem==-1)
	{
		if(pageText[iPosDel] != ' ' || ci.node != (fz_display_node*)hObj)
			return FALSE;
	}

	assert(ci.node == (fz_display_node*)hObj);

	if(ci.iItem != -1)
	{
		assert(ci.iItem >= 0 && ci.iItem < ci.node->item.text->len);
	}

#if 0
	ci.node->item.text->items[ci.iItem].gid = 2;
	ci.node->item.text->items[ci.iItem].ucs = 'C';
	pageText[iPosDel] = 'C';
#else
	pageTextLen = lens[pageNo - 1];
	
	double widthDelta = -pageCoords[iPosDel].dx;
	double heightDelta = -pageCoords[iPosDel].dx;
	if(iPosDel + 1 < pageTextLen)
	{
		widthDelta = -(pageCoords[iPosDel + 1].x - pageCoords[iPosDel].x);
		heightDelta = -(pageCoords[iPosDel + 1].y - pageCoords[iPosDel].y);
	}

	if(updateRect)
	{
#if 0
		if(iPosDel < pageTextLen)
		{
			RectD mediabox = engine->PageMediabox(pageNo);

			updateRect->x = pageCoords[iPosDel].x;
			updateRect->y = pageCoords[iPosDel].y;
			updateRect->dx = (mediabox.x + mediabox.dx) - updateRect->x;
			updateRect->dy = pageCoords[iPosDel].dy;
		}
#else
		assert(iPosDel < pageTextLen);
		{
			PointD pt;
			pt.x = pageCoords[iPosDel].x +  pageCoords[iPosDel].dx / 2.0;
			pt.y = pageCoords[iPosDel].y +  pageCoords[iPosDel].dy / 2.0;

			DOUBLE xCursor;
			FRect rtTextNew;
			GetObjLineText(pageNo,hObj,&pt,&rtTextNew,&xCursor);

			updateRect->x = rtTextNew.x0;
			updateRect->y = rtTextNew.y0;
			updateRect->dx = rtTextNew.x1 - rtTextNew.x0;
			updateRect->dy = rtTextNew.y1 - rtTextNew.y0;
		}

		{
			/* add some fuzz at the edges, as especially glyph rects
			* are sometimes not actually completely bounding the glyph */
			updateRect->x -= 20; updateRect->y -= 20;
			updateRect->dx += 2*20; updateRect->dy += 2*20;
		}
#endif
	}

	INT indexChanged = 0;
	if(ci.iItem != -1)
	{
		assert(ci.node->item.text->items[ci.iItem].ucs==pageText[iPosDel]);
		ArrayDeleteElements(ci.node->item.text->items,ci.node->item.text->len,ci.iItem,1);

		if(ci.node->last && ci.node->last->is_dup)
		{
			assert(ci.node->last->item.text->items[ci.iItem].ucs==pageText[iPosDel]);
			ArrayDeleteElements(ci.node->last->item.text->items,ci.node->last->item.text->len,ci.iItem,1);
		}

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
		{
			widthDelta = 0;
			heightDelta = 0;
		}

		char_inf& ci = pageChInf[i];
		if(ci.iItem==-1)
		{
			if(widthDelta)
			{
				pageCoords[i].x += widthDelta;
				pageCoords[i].y += heightDelta;
			}

			continue;
		}

		if(ci.node==(fz_display_node*)hObj)
		{
			if(indexChanged)
				ci.iItem += indexChanged;

			if(widthDelta)
			{
				ci.node->item.text->items[ci.iItem].x += (float)widthDelta;
				ci.node->item.text->items[ci.iItem].y += (float)heightDelta;

				if(ci.node->last && ci.node->last->is_dup)
				{
					ci.node->last->item.text->items[ci.iItem].x += (float)widthDelta;
					ci.node->last->item.text->items[ci.iItem].y += (float)heightDelta;
				}

				pageCoords[i].x += (float)widthDelta;
				pageCoords[i].y += (float)heightDelta;
			}
		}
	}

	if(bBackspace && xCursor)
	{
		if(iPosDel < pageTextLen)
		{
			if(pageText[iPosDel] == '\n')
			{
				//assert(iPosDel > 0);
				*xCursor = rawPosX;//pageCoords[iPosDel - 1].x + pageCoords[iPosDel - 1].dx;
			}
			else
				*xCursor = pageCoords[iPosDel].x;
		}
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

void my_pdf_show_char(my_pdf_gstate *gstate,int cid,fz_matrix& tm)
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

unsigned char* ansii_to_cid(pdf_font_desc *fontdesc,unsigned char* buf,int& cid,int* o_cpt)
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

	if(o_cpt)
		*o_cpt = cpt;

	return buf;
}

BOOL TextSelection::InsertCharByPos(int pageNo, HPDFOBJ hObj, const PointD& pt, WCHAR chIns, DOUBLE* xCursor, RectD* updateRect)
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
		if(pageText[iPosIns] != ' ' || ci.node != (fz_display_node*)hObj)
			return FALSE;
	}

	assert(ci.node == (fz_display_node*)hObj);

	if(ci.iItem != -1)
	{
		assert(ci.iItem >= 0 && ci.iItem < ci.node->item.text->len);
	}

	fz_display_node* node = ci.node;

	INT pageTextLen = lens[pageNo - 1];
	RectD* pageCoords = coords[pageNo - 1];

	if(updateRect)
	{
#if 0
		if(iPosIns < pageTextLen)
		{
			RectD mediabox = engine->PageMediabox(pageNo);

			updateRect->x = pageCoords[iPosIns].x;
			updateRect->y = pageCoords[iPosIns].y;
			updateRect->dx = (mediabox.x + mediabox.dx) - updateRect->x;
			updateRect->dy = pageCoords[iPosIns].dy;
		}
#endif
	}

	float widthDelta = 7;
	float heightDelta = 7;
	
	INT indexChanged = 0;

	INT iTextItem = ci.iItem;
	float xInsPos = 0.0;
	if(iTextItem==-1)
	{
		xInsPos = (float)pageCoords[iPosIns].x;

		for(INT i = iPosIns + 1;i < pageTextLen;i++)
		{
			char_inf& ci = pageChInf[i];
			
			if(ci.node==(fz_display_node*)hObj)
			{
				if(ci.iItem != -1)
				{
					iTextItem = ci.iItem;
					break;
				}
			}
		}
	}
	else
	{
		xInsPos = ci.node->item.text->items[iTextItem].x;
	}
	assert(iTextItem != -1);
	if(iTextItem==-1)
		return FALSE;

	//if(ci.iItem != -1)
	{
		fz_grow_text(ci.node->item.text,1);

		fz_text_item txtItem;
		txtItem = ci.node->item.text->items[iTextItem];
		txtItem.ucs = chIns;
		txtItem.x = xInsPos;
		txtItem.offset = 0.0;
		
		WCHAR wbuf[] = {txtItem.ucs,0};
		unsigned char buf[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,wbuf,-1,(LPSTR)buf,sizeof(buf),NULL,NULL);

		int cid = 0;
		int cpt = 0;
		if(!ansii_to_cid(ci.node->item.text->gstate.font,buf,cid,&cpt))
			return FALSE;

		txtItem.gid = pdf_font_cid_to_gid(ci.node->item.text->gstate.font, cid);

		fz_matrix tm = ci.node->item.text->gstate.tm;
		tm.e = 0.0;
		tm.f = 0.0;
		my_pdf_show_char(&ci.node->item.text->gstate,cid,tm);
		widthDelta = tm.e;
		heightDelta = tm.f;

		if(cpt == 32)
		{
			float e = 0, f = 0;
			my_pdf_show_space(&node->item.text->gstate, node->item.text->gstate.word_space, &e, &f);
			widthDelta += e;
			heightDelta += f;
		}

		ArrayInsertElements(ci.node->item.text->items,ci.node->item.text->len,iTextItem,&txtItem,1);

		if(ci.node->last && ci.node->last->is_dup)
		{
			fz_grow_text(ci.node->last->item.text,1);
			ArrayInsertElements(ci.node->last->item.text->items,ci.node->last->item.text->len,iTextItem,&txtItem,1);
		}

		indexChanged = 1;
	}
// 	else
// 	{
// 		assert(pageText[iPosIns]==' ');
// 	}

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

		if(newCI.iItem==-1)
		{
			assert(iPosIns > 0);
			assert(pageChInf[iPosIns - 1].iItem != -1);
			
			newCI.iItem = pageChInf[iPosIns - 1].iItem + 1;
		}
		assert(newCI.iItem != -1);

		ArrayInsertElements(pageChInf,pageTextLen,iPosIns,&newCI,1);
	}

	{
		pageTextLen = lens[pageNo - 1];

		INT bufSize = pageTextLen + 1;
		EnsureBufSize(&pageCoords,bufSize,bufSize,bufSize + 1,NULL);
		coords[pageNo - 1] = pageCoords;

		RectD newRT = pageCoords[iPosIns];
		ArrayInsertElements(pageCoords,pageTextLen,iPosIns,&newRT,1);
	}

	lens[pageNo - 1] = pageTextLen;
	assert(str::Len(pageText)==pageTextLen);

	float newObjRight = node->rect.x1;

	for(INT i = iPosIns + 1;i < pageTextLen;i++)
	{
		if(pageText[i]=='\n')
		{
			widthDelta = 0;
			heightDelta = 0;
		}

		char_inf& ci = pageChInf[i];
		if(ci.iItem==-1)
		{
			if(widthDelta)
			{
				pageCoords[i].x += widthDelta;			
				pageCoords[i].y += heightDelta;
			}

			if(newObjRight < pageCoords[i].x + pageCoords[i].dx)
			{
				newObjRight = (float)(pageCoords[i].x + pageCoords[i].dx);
			}
			continue;
		}

		if(ci.node==(fz_display_node*)hObj)
		{
			if(indexChanged)
				ci.iItem += indexChanged;

			if(widthDelta)
			{
				ci.node->item.text->items[ci.iItem].x += widthDelta;
				ci.node->item.text->items[ci.iItem].y += heightDelta;

				if(ci.node->last && ci.node->last->is_dup)
				{
					ci.node->last->item.text->items[ci.iItem].x += widthDelta;
					ci.node->last->item.text->items[ci.iItem].y += heightDelta;
				}

				pageCoords[i].x += widthDelta;
				pageCoords[i].y += heightDelta;

				if(newObjRight < pageCoords[i].x + pageCoords[i].dx)
				{
					newObjRight = (float)(pageCoords[i].x + pageCoords[i].dx);
				}
			}
		}
	}

	if(node->rect.x1 < newObjRight)
	{
		node->rect.x1 = newObjRight;
	}

	if(node->last && node->last->is_dup)
	{
		if(node->last->rect.x1 < newObjRight)
		{
			node->last->rect.x1 = newObjRight;
		}
	}

	if(xCursor)
	{
		if(iPosIns + 1 < pageTextLen)
			*xCursor = pageCoords[iPosIns + 1].x;
		else
		{
			if(pageTextLen > 0)
				*xCursor = pageCoords[pageTextLen - 1].x + pageCoords[pageTextLen - 1].dx;
		}
	}

	if(updateRect)
	{
		assert(iPosIns < pageTextLen);
		{
			PointD pt;
			pt.x = pageCoords[iPosIns].x +  pageCoords[iPosIns].dx / 2.0;
			pt.y = pageCoords[iPosIns].y +  pageCoords[iPosIns].dy / 2.0;

			DOUBLE xCursor;
			FRect rtTextNew;
			GetObjLineText(pageNo,(HPDFOBJ)node,&pt,&rtTextNew,&xCursor);

			updateRect->x = rtTextNew.x0;
			updateRect->y = rtTextNew.y0;
			updateRect->dx = rtTextNew.x1 - rtTextNew.x0;
			updateRect->dy = rtTextNew.y1 - rtTextNew.y0;
		}

		{
			/* add some fuzz at the edges, as especially glyph rects
			* are sometimes not actually completely bounding the glyph */
			updateRect->x -= 20; updateRect->y -= 20;
			updateRect->dx += 2*20; updateRect->dy += 2*20;
		}

		if(updateRect->x < node->rect.x0)
			node->rect.x0 = (float)updateRect->x;
		if(updateRect->y < node->rect.y0)
			node->rect.y0 = (float)updateRect->y;
		if(updateRect->x + updateRect->dx > node->rect.x1)
			node->rect.x1 = (float)(updateRect->x + updateRect->dx);
		if(updateRect->y + updateRect->dy > node->rect.y1)
			node->rect.y1 = (float)(updateRect->y + updateRect->dy);
	}

	return TRUE;
}

BOOL TextSelection::MoveObject(int pageNo, HPDFOBJ hObj, const FPoint& relMove)
{
	assert(1 <= pageNo && pageNo <= engine->PageCount());
	if (!coords[pageNo - 1])
		FindClosestGlyph(pageNo, 0, 0);

	WCHAR* pageText = text[pageNo - 1];
	char_inf* pageChInf = chInf[pageNo - 1];
	RectD* pageCoords = coords[pageNo - 1];
	INT pageTextLen = lens[pageNo - 1];

	for(INT i = 0;i < pageTextLen;i++)
	{
		char_inf& ci = pageChInf[i];
		if(ci.node!=(fz_display_node*)hObj)
			continue;

		pageCoords[i].Offset((float)relMove.x,(float)relMove.y);

		if(ci.iItem != -1)
		{
			assert(ci.iItem >= 0 && ci.iItem < ci.node->item.text->len);
			ci.node->item.text->items[ci.iItem].x += (float)relMove.x;
			ci.node->item.text->items[ci.iItem].y += (float)relMove.y;

			if(ci.node->last && ci.node->last->is_dup)
			{
				assert(ci.iItem >= 0 && ci.iItem < ci.node->last->item.text->len);
				ci.node->last->item.text->items[ci.iItem].x += (float)relMove.x;
				ci.node->last->item.text->items[ci.iItem].y += (float)relMove.y;
			}
		}
	}

	fz_display_node* node = (fz_display_node*)hObj;
	node->rect.x0 += (float)relMove.x;
	node->rect.y0 += (float)relMove.y;
	node->rect.x1 += (float)relMove.x;
	node->rect.y1 += (float)relMove.y;

	if(node->last && node->last->is_dup)
	{
		node->last->rect.x0 += (float)relMove.x;
		node->last->rect.y0 += (float)relMove.y;
		node->last->rect.x1 += (float)relMove.x;
		node->last->rect.y1 += (float)relMove.y;
	}

	return TRUE;
}

BOOL TextSelection::MoveCursor(int pageNo, HPDFOBJ hObj, const PointD& pt, INT nMove, DOUBLE* xCursor)
{
	if(nMove==0)
		return FALSE;

	assert(1 <= pageNo && pageNo <= engine->PageCount());
	if (!coords[pageNo - 1])
		FindClosestGlyph(pageNo, 0, 0);

	INT chPos = -1;
	DOUBLE xCursor1 = 0.0;
	INT lineTextPos = GetObjLineText(pageNo,hObj,&pt,NULL,&xCursor1,&chPos);
	if(lineTextPos==-1 || chPos==-1)
		return FALSE;

	INT pageTextLen = lens[pageNo - 1];

	INT iPosCur = chPos;
	if(iPosCur <= 0 && nMove < 0)
		return FALSE;
	if(iPosCur >= pageTextLen && nMove > 0)
		return FALSE;

	WCHAR* pageText = text[pageNo - 1];
	RectD* pageCoords = coords[pageNo - 1];
	char_inf* pageChInf = chInf[pageNo - 1];

	if(nMove < 0)
	{
		if(iPosCur > 1)
		{
			if(pageText[iPosCur - 1]=='\n')
				return FALSE;
		}
	}
	else if(nMove > 0)
	{
		if(pageText[iPosCur]=='\n')
			return FALSE;
	}
	else
	{
		assert(0);
		return FALSE;
	}

	INT iPosNew = iPosCur + nMove;
	if(iPosNew > pageTextLen)
		iPosNew = pageTextLen;
	if(iPosNew < 0)
		iPosNew = 0;

	if(xCursor)
	{
		if(iPosNew < pageTextLen)
		{
			if(pageText[iPosNew] == '\n')
			{
				assert(iPosNew > 0);
				*xCursor = pageCoords[iPosNew - 1].x + pageCoords[iPosNew - 1].dx;
			}
			else
				*xCursor = pageCoords[iPosNew].x;
		}
		else
		{
			if(pageTextLen > 0)
				*xCursor = pageCoords[pageTextLen - 1].x + pageCoords[pageTextLen - 1].dx;
		}
	}	

	return TRUE;
}

BOOL TextSelection::UpdateTextPos(int pageNo, fz_display_node* node, FRect* rtText)
{
	assert(1 <= pageNo && pageNo <= engine->PageCount());
	if (!coords[pageNo - 1])
		FindClosestGlyph(pageNo, 0, 0);

	if(node->item.text->len <= 0)
		return FALSE;

	if(!node->item.text || !node->item.text->font)
		return FALSE;

	pdf_font_desc *fontdesc = node->item.text->gstate.font;
	if(!fontdesc)
		return FALSE;

	float newLineHeight = node->rect.y1 - node->rect.y0;
	float newObjRight = node->rect.x1;

	fz_matrix tm = node->item.text->gstate.tm;
	tm.e = 0.0;
	tm.f = 0.0;

	for(INT i = 0;i < node->item.text->len;i++)
	{
		WCHAR wbuf[] = {node->item.text->items[i].ucs,0};
		unsigned char buf[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,wbuf,-1,(LPSTR)buf,sizeof(buf),NULL,NULL);

		int cid = 0;
		int cpt = 0;
		if(!ansii_to_cid(fontdesc,buf,cid,&cpt))
			continue;

		node->item.text->items[i].x = node->item.text->items[0].x + tm.e;
		node->item.text->items[i].y = node->item.text->items[0].y + tm.f;

		if(node->last && node->last->is_dup)
		{
			assert(i >= 0 && i < node->last->item.text->len);
			node->last->item.text->items[i].x = node->last->item.text->items[0].x + tm.e;
			node->last->item.text->items[i].y = node->last->item.text->items[0].y + tm.f;
		}

		my_pdf_show_char(&node->item.text->gstate,cid,tm);

		if(cpt == 32)
		{
			float e = 0, f = 0;
			my_pdf_show_space(&node->item.text->gstate, node->item.text->gstate.word_space, &e, &f);
			tm.e += e;
			tm.f += f;
		}

		if(node->item.text->items[i].offset != 0.0)
		{
			float e = 0, f = 0;
			my_pdf_show_space(&node->item.text->gstate, 
				-node->item.text->items[i].offset * node->item.text->gstate.size * 0.001f, &e, &f);
			tm.e += e;
			tm.f += f;
		}
				
		newObjRight = node->item.text->items[0].x + tm.e;
		
		float chWidth, lineHeight;
		if(fz_get_char_width_line_height(node->item.text,i,&chWidth,&lineHeight))
		{
			if(i == 0)
			{
				newLineHeight = lineHeight;
			}
		}
	}

	node->rect.x1 = newObjRight;

	float oldLineHeight = node->rect.y1 - node->rect.y0;
	float oldBtmSpace = node->item.text->items[0].y - node->rect.y0;
	if(oldBtmSpace < 0)
		oldBtmSpace = 0;

	float heightRate = newLineHeight / oldLineHeight;
	float newBtmSpace = oldBtmSpace * heightRate;

#if 0
	node->rect.y0 = node->item.text->items[0].y - newBtmSpace;
	node->rect.y1 = node->rect.y0 + newLineHeight;

	if(node->last && node->last->is_dup)
	{
		node->last->rect = node->rect;
	}
#endif

	WCHAR* pageText = text[pageNo - 1];
	RectD* pageCoords = coords[pageNo - 1];
	char_inf* pageChInf = chInf[pageNo - 1];
	INT pageTextLen = lens[pageNo - 1];

	INT iFirstGoodCh = -1;
	for(INT i = 0;i < pageTextLen;i++)
	{
		char_inf& ci = pageChInf[i];
		if(ci.node!=node)
			continue;

		if(pageText[i]=='\n')
			break;

		if(ci.iItem != -1)
		{
			iFirstGoodCh = i;

			assert(ci.iItem >= 0 && ci.iItem < ci.node->item.text->len);
			pageCoords[i].x = ci.node->item.text->items[ci.iItem].x;
			pageCoords[i].y = ci.node->item.text->items[ci.iItem].y;

			float chWidth, lineHeight;
			if(fz_get_char_width_line_height(node->item.text,i,&chWidth,&lineHeight))
			{
				pageCoords[i].dx = chWidth;
				pageCoords[i].dy = lineHeight;
			}
		}
	}

	if(/*rtText && */iFirstGoodCh != -1)
	{
		PointD pt;
		pt.x = pageCoords[iFirstGoodCh].x +  pageCoords[iFirstGoodCh].dx / 2.0;
		pt.y = pageCoords[iFirstGoodCh].y +  pageCoords[iFirstGoodCh].dy / 2.0;

		DOUBLE xCursor;
		FRect rtNewText;
		GetObjLineText(pageNo,(HPDFOBJ)node,&pt,&rtNewText,&xCursor);

		node->rect.x0 = (float)rtNewText.x0;
		node->rect.y0 = (float)rtNewText.y0;
		node->rect.x1 = (float)rtNewText.x1;
		node->rect.y1 = (float)rtNewText.y1;		

		if(node->last && node->last->is_dup)
		{
			node->last->rect = node->rect;
		}

		if(rtText)
			*rtText = rtNewText;
	}

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////