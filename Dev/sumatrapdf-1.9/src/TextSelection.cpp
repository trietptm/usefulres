/* Copyright 2006-2011 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "TextSelection.h"
#include "Vec.h"

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
TCHAR* TextSelection::ExtractObjText(int pageNo, HXOBJ hObj, const PointD* pt, RectD* rtText, DOUBLE* xCursor)
{
	assert(1 <= pageNo && pageNo <= engine->PageCount());
	if (!coords[pageNo - 1])
		FindClosestGlyph(pageNo, 0, 0);

	WCHAR* pLineText = NULL;

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
				if(pLineText)
					break;

				int iObjFirstCh = -1;
				for(int i = iPos;i < textLen;i++)
				{
					const char_inf& ci = pageChInf[i];

					if(ci.node != hObj)
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

					if(ci.node != hObj)
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
							pLineText = &pageText[i];

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
							}

							dist = fabs((pageCoords[i].x + pageCoords[i].dx) - pt->x);
							if(dist < cursorDist)
							{
								if(i + 1 < textLen && pageText[i + 1]!='\n')
									*xCursor = pageCoords[i + 1].x;
								else
									*xCursor = pageCoords[i].x + pageCoords[i].dx;

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

	return pLineText;
}
//////////////////////////////////////////////////////////////////////////