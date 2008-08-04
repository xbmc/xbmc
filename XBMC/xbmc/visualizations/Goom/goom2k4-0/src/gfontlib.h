#ifndef _GFONTLIB_H
#define _GFONTLIB_H

#include "goom_graphic.h"

void gfont_load (void);
void goom_draw_text (Pixel * buf,int resolx,int resoly, int x, int y,
		const char *str, float chspace, int center);

#endif
