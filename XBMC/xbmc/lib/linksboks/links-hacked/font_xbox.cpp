/* Xbox XFONT (and freetype) backend */

extern "C" {
#include "links.h"
}

#ifdef __XBOX__

#if defined(XBOX_USE_XFONT)

#include "../linksboks.h"

extern LinksBoksExtFont *(*g_LinksBoksGetExtFontFunction) (unsigned char *fontname, int fonttype);

extern "C" {

int xfont_get_char_metric(struct font *font, int code, int *x, int *y)
{
	WCHAR str[2];
	_snwprintf(str, 1, L"%c", (wint_t)code);

    if(!font)
        return 0;

	XFONT *xf =(XFONT *)(font->data);

	if(FAILED(xf->GetTextExtent(str, 1, (unsigned int *)x)))
		return 0;
	*y = xf->GetTextHeight();

	if(!*x || !*y)
		return 0;

	return 1;
}


int xfont_get_char(struct font *font, int code, unsigned char **dest, int *x, int *y)
{
	WCHAR str[2];
	_snwprintf(str, 1, L"%c", (wint_t)code);

    if(!font)
        return 0;

	XFONT *xf =(XFONT *)(font->data);
	if(!xf)
		return 0;

	if(FAILED(xf->GetTextExtent(str, 1, (unsigned int *)x)))
		return 0;
	*y = xf->GetTextHeight();

	if(!*x || !*y)
		return 0;

	*dest = (unsigned char *)mem_calloc(*x*(*y));

	if(FAILED(xf->TextOutToMemory(*dest, *x, *x, *y, D3DFMT_LIN_A8, str, 1, 0, 0)))
		return 0;

	return 1;
}

void xfont_free_font(struct font *font)
{
}

void xfont_index_font(struct font *font)
{
	int nbletters = 0;
	int i;
	WCHAR str[2];
	XFONT *xf = (XFONT *)font->data;

	if(!xf)
		return;

	font->letter = (struct letter *)mem_alloc(sizeof(struct letter));

	int refx, refy;
	if(!xfont_get_char_metric(font, 8, &refx, &refy))
		return;
	unsigned char *reference = new unsigned char[refx*refy];
	unsigned char *dest = new unsigned char[refx*refy];
	memset(reference, 0, refx*refy);
	_snwprintf(str, 1, L"%c", (wint_t)8);

	if(FAILED(xf->TextOutToMemory(reference, refx, refx, refy, D3DFMT_LIN_A8, str, 1, 0, 0)))
		return;

	for(i = 32; i < 65536; i++){ /* Lower Unicode only */
		int xw,yw;

		if(xfont_get_char_metric(font, i, &xw, &yw))
		{
			if((xw == refx) && (yw == refy) && (i != 32))
			{
				/* If same size as reference and is not space, we check if we can discard it */
				memset(dest, 0, xw*yw);
				_snwprintf(str, 1, L"%c", (wint_t)i);
				if(FAILED(xf->TextOutToMemory(dest, xw, xw, yw, D3DFMT_LIN_A8, str, 1, 0, 0)))
					continue;
				if(!memcmp((const void *)dest, (const void *)reference, xw*yw))
					continue;
			}

			nbletters++;
			/* Inefficient, but can't think of anything better for now */
			font->letter = (struct letter *)mem_realloc(font->letter, sizeof(struct letter)*nbletters);
			font->letter[nbletters-1].xsize = xw;
			font->letter[nbletters-1].ysize = yw;
			font->letter[nbletters-1].code = i;
			font->letter[nbletters-1].list = NULL;

		}

	}

	font->n_letters = nbletters;

	delete reference;
	delete dest;
}

struct font *create_xfont_font(unsigned char *filename)
{
	struct font *font;
	unsigned cell_height, descent;
	LinksBoksExtFont *lxf;

	if(!filename) return NULL;

	if(!g_LinksBoksGetExtFontFunction)
		return NULL;

	lxf = g_LinksBoksGetExtFontFunction(filename, LINKSBOKS_EXTFONT_TYPE_XFONT);
	if(!lxf || !lxf->fontdata)
		return NULL;

	font = create_font(lxf->family, lxf->weight, lxf->slant, lxf->adstyl, lxf->spacing);

	font->data = lxf->fontdata;

	((XFONT *)font->data)->GetFontMetrics(&cell_height, &descent);

	font->baseline = cell_height - descent;

    font->get_char = xfont_get_char;
    font->get_char_metric = xfont_get_char_metric;
    font->free_font = xfont_free_font;
    font->index_font = xfont_index_font;
    font->font_type = FONT_TYPE_XFONT;
    font->filename = stracpy(filename);

	return font;
}

void init_xfont()
{
}

void finalize_xfont()
{
}

} /* extern "C" */

#endif /* XBOX_USE_XFONT */

#endif /* __XBOX__ */