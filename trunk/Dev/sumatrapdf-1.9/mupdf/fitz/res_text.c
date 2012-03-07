#include "fitz.h"

/*MyCode*/
#include "mupdf.h"
//////////////////////////////////////////////////////////////////////////

fz_text *
fz_new_text(fz_font *font, fz_matrix trm, int wmode, my_pdf_gstate* gstate)
{
	fz_text *text;

	text = fz_malloc(sizeof(fz_text));
	text->font = fz_keep_font(font);
	text->trm = trm;
	text->wmode = wmode;
	text->len = 0;
	text->cap = 0;
	text->items = NULL;

	/*MyCode*/
	if(gstate)
	{
// 		text->gstate.char_space = gstate->char_space;
// 		text->gstate.font = pdf_keep_font(gstate->font);
// 		text->gstate.rise = gstate->rise;
// 		text->gstate.scale = gstate->scale;
// 		text->gstate.size = gstate->size;
// 		text->gstate.tm = gstate->tm;

		memcpy(&text->gstate,gstate,sizeof(text->gstate));
		text->gstate.font = pdf_keep_font(gstate->font);
	}
	else
	{
// 		text->gstate.char_space = 0;
// 		text->gstate.font = NULL;
// 		text->gstate.rise = 0;
// 		text->gstate.scale = 0;
// 		text->gstate.size = 0;

		memset(&text->gstate,0,sizeof(text->gstate));
	}
	//////////////////////////////////////////////////////////////////////////

	return text;
}

void
fz_free_text(fz_text *text)
{
	/*MyCode*/
	if(text->gstate.font)
		pdf_drop_font(text->gstate.font);
	//////////////////////////////////////////////////////////////////////////

	fz_drop_font(text->font);
	fz_free(text->items);
	fz_free(text);
}

fz_text *
fz_clone_text(fz_text *old)
{
	fz_text *text;

	text = fz_malloc(sizeof(fz_text));
	text->font = fz_keep_font(old->font);
	text->trm = old->trm;
	text->wmode = old->wmode;
	text->len = old->len;
	text->cap = text->len;
	text->items = fz_calloc(text->len, sizeof(fz_text_item));
	memcpy(text->items, old->items, text->len * sizeof(fz_text_item));

	/*MyCode*/
	if(old->gstate.font)
	{
// 		text->gstate.char_space = old->gstate.char_space;
// 		text->gstate.font = pdf_keep_font(old->gstate.font);
// 		text->gstate.rise = old->gstate.rise;
// 		text->gstate.scale = old->gstate.scale;
// 		text->gstate.size = old->gstate.size;
// 		text->gstate.tm = old->gstate.tm;

		memcpy(&text->gstate,&old->gstate,sizeof(text->gstate));
		text->gstate.font = pdf_keep_font(old->gstate.font);
	}
	else
		memset(&text->gstate,0,sizeof(text->gstate));
	//////////////////////////////////////////////////////////////////////////	

	return text;
}

fz_rect
fz_bound_text(fz_text *text, fz_matrix ctm)
{
	fz_matrix trm;
	fz_rect bbox;
	fz_rect fbox;
	int i;

	if (text->len == 0)
		return fz_empty_rect;

	/* find bbox of glyph origins in ctm space */

	bbox.x0 = bbox.x1 = text->items[0].x;
	bbox.y0 = bbox.y1 = text->items[0].y;

	for (i = 1; i < text->len; i++)
	{
		bbox.x0 = MIN(bbox.x0, text->items[i].x);
		bbox.y0 = MIN(bbox.y0, text->items[i].y);
		bbox.x1 = MAX(bbox.x1, text->items[i].x);
		bbox.y1 = MAX(bbox.y1, text->items[i].y);
	}

	bbox = fz_transform_rect(ctm, bbox);

	/* find bbox of font in trm * ctm space */

	trm = fz_concat(text->trm, ctm);
	trm.e = 0;
	trm.f = 0;

	fbox.x0 = text->font->bbox.x0 * 0.001f;
	fbox.y0 = text->font->bbox.y0 * 0.001f;
	fbox.x1 = text->font->bbox.x1 * 0.001f;
	fbox.y1 = text->font->bbox.y1 * 0.001f;

	fbox = fz_transform_rect(trm, fbox);

	/* expand glyph origin bbox by font bbox */

	bbox.x0 += fbox.x0;
	bbox.y0 += fbox.y0;
	bbox.x1 += fbox.x1;
	bbox.y1 += fbox.y1;

	return bbox;
}

static void
fz_grow_text(fz_text *text, int n)
{
	if (text->len + n < text->cap)
		return;
	while (text->len + n > text->cap)
		text->cap = text->cap + 36;
	text->items = fz_realloc(text->items, text->cap, sizeof(fz_text_item));
}

void
fz_add_text(fz_text *text, int gid, int ucs, float x, float y)
{
	fz_grow_text(text, 1);
	text->items[text->len].ucs = ucs;
	text->items[text->len].gid = gid;
	text->items[text->len].x = x;
	text->items[text->len].y = y;

	text->items[text->len].offset = 0.0; //MyCode

	text->len++;
}

static int isxmlmeta(int c)
{
	return c < 32 || c >= 128 || c == '&' || c == '<' || c == '>' || c == '\'' || c == '"';
}

void fz_debug_text(fz_text *text, int indent)
{
	int i, n;
	for (i = 0; i < text->len; i++)
	{
		for (n = 0; n < indent; n++)
			putchar(' ');
		if (!isxmlmeta(text->items[i].ucs))
			printf("<g ucs=\"%c\" gid=\"%d\" x=\"%g\" y=\"%g\" />\n",
				text->items[i].ucs, text->items[i].gid, text->items[i].x, text->items[i].y);
		else
			printf("<g ucs=\"U+%04X\" gid=\"%d\" x=\"%g\" y=\"%g\" />\n",
				text->items[i].ucs, text->items[i].gid, text->items[i].x, text->items[i].y);
	}
}