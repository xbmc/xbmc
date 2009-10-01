/* 
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
		
    mac_wrdwindow.h
    Macintosh graphics driver for WRD
*/

#ifndef MAC_GRF_H
#define MAC_GRF_H

#include "png.h"
#include "wrd.h"

#if ENTITY
 #define EXTERN /*entitiy*/
#else
 #define EXTERN  extern 
#endif

static const RGBColor	black={0,0,0},white={0xFFFF,0xFFFF,0xFFFF};

#define BASE_X	8
#define	BASE_Y	16
#define LINES 25
#define COLS 80
#define SIZEX 640
#define SIZEY 400

// for graphics
EXTERN GWorldPtr	graphicWorld[8], dispWorld, charbufWorld;
#define	GACTIVE_PIX	(graphicWorld[activeGraphics]->portPixMap)
#define	GDISP_PIX	(graphicWorld[dispGraphics]->portPixMap)
#define	DISP_PIX	(dispWorld->portPixMap)
EXTERN int			gmode_mask, gmode_mask_gline, dev_gon_flag, dev_redrawflag;
#define				DEV_SET_GMODE(mask)  (gmode_mask=gmode_mask_gline=(mask))
EXTERN RGBColor		dev_palette[20][17];
EXTERN int			startpal, endpal; //for @FADE

//for text
EXTERN char			char_vram[25+1][80+2];
EXTERN int			wrd_ton;
#define				CHAR_VRAM(x,y)  (char_vram[y][x])
EXTERN char			char_color_vram[25+1][80+1];
#define				CHAR_COLOR_VRAM(x,y)  (char_color_vram[y][x])
EXTERN char			multi_byte_flag[25+1][80+1];
#define				MULTI_BYTE_FLAG(x,y)  (multi_byte_flag[y][x])   //...Umm
EXTERN int			activeGraphics, dispGraphics, gvram_bank_num;
#define WRD_LOCX(x)		(((x)-1)*BASE_X)
#define WRD_LOCY(y)		((y)*BASE_Y-3)
#define WRD_MOVE_COURSOR_TMP(x,y)	MoveTo( WRD_LOCX(x), WRD_LOCY(y) )
#define IS_MULTI_BYTE(c)	( ((c)&0x80) && ((0x1 <= ((c)&0x7F) && ((c)&0x7F) <= 0x1f) ||\
				 (0x60 <= ((c)&0x7F) && ((c)&0x7F) <= 0x7c)))
#define SYNC_DISP(rect)	(CopyBits((BitMap*)&DISP_PIX, &win.ref->portBits, \
			&(rect), &(rect), srcCopy,0)) //make visible Graphics
#define LOCK_ALL_PIXMAP() (LockPixels(GDISP_PIX),LockPixels(GACTIVE_PIX),LockPixels(DISP_PIX))
#define UNLOCK_ALL_PIXMAP() (UnlockPixels(GDISP_PIX),UnlockPixels(GACTIVE_PIX),UnlockPixels(DISP_PIX))

#define SET_G_COLOR(code,world)	((world)->fgColor=(code))
EXTERN int pallette_exist, fading;


EXTERN int		wrd_coursor_x,wrd_coursor_y;
EXTERN int		wrd_text_color_attr;
#define CATTR_LPART (1)
#define CATTR_16FONT (1<<1)
#define CATTR_COLORED (1<<2)
#define CATTR_BGCOLORED (1<<3) 
#define CATTR_TXTCOL_MASK_SHIFT 4
#define CATTR_TXTCOL_MASK (7<<CATTR_TXTCOL_MASK_SHIFT)
#define CATTR_INVAL (1<<31)
#define TCOLOR_INDEX_SHIFT	32
#define TCODE2INDEX(attr)	((((attr)&CATTR_TXTCOL_MASK)>>CATTR_TXTCOL_MASK_SHIFT)+TCOLOR_INDEX_SHIFT)
#define SET_T_COLOR(attr)	(wrd_text_color_attr=(attr))
#define SET_T_RGBFORECOLOR_TMP(attr)	(dispWorld->fgColor=TCODE2INDEX(attr))

void dev_init(int version);
void dev_set_height(int height);
void dev_redisp(Rect rect);
void dev_remake_disp(Rect rect);
void dev_draw_text_gmode(PixMapHandle pixmap, int x, int y, const char* s, int len,
		int pmask, int mode, int fgcolor, int bgcolor, int ton_mode);

void dev_change_palette(RGBColor pal[16]);
void dev_change_1_palette(int code, RGBColor color);
void dev_init_text_color();
void MyCopyBits(PixMapHandle srcPixmap, PixMapHandle dstPixmap,
		Rect srcRect, Rect dstRect, short mode, int trans, int pmask,
		int maskx, int masky, const uint8 maskdata[]);
void dev_gmove(int x1, int y1, int x2, int y2, int xd, int yd,
			GWorldPtr srcworld, GWorldPtr destworld, int sw, int trans, int mask,
			int maskx, int masky, const uint8 maskdata[]);
void dev_box(PixMapHandle pixmap, Rect rect, int color, int pmask);
void dev_line(int x1, int y1, int x2, int y2, int color, int style,
	int pmask, PixMapHandle pixmap );
void dev_gline(int x1, int y1, int x2, int y2, int p1, int sw, int p2, GWorldPtr world);
void mac_setfont(GWorldPtr world, Str255 fontname);
//#define WRD_FONTNAME "\pìôïùñæí©"
#define WRD_FONTNAME "\pOsakaÅ|ìôïù"

void sry_start();
void sry_end();
void sry_start();
void sry_update();

int mac_loadpng_pre( png_structp *png_ptrp, png_infop *info_ptrp, struct timidity_file * tf);
int mac_loadpng(png_structp png_ptr, png_infop info_ptr, GWorldPtr world, RGBColor pal[256] );
void mac_loadpng_post(png_structp png_ptr, png_infop info_ptr);

EXTERN Rect portRect
#if ENTITY
 ={0,0,480,640}
#endif
;

EXTERN RGBColor textcolor[8]
#if ENTITY
	={{0x0000,0x0000,0x0000},	//0: black
	{0xFFFF,0x0000,0x0000},	//1:red
	{0x0000,0xFFFF,0x0000},	//2:green
	{0xFFFF,0xFFFF,0x0000},	//3:yellow
	{0x0000,0x0000,0xFFFF},	//4:blue
	{0xFFFF,0x0000,0xFFFF},	//5:purpl
	{0x0000,0xFFFF,0xFFFF},	//6:mizuiro
	{0xFFFF,0xFFFF,0xFFFF} } //7:white
#endif
;

#endif //MAC_GRF_H


