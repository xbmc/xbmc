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

    wrdt_mac.c

    Written by by T.Nogami	<t-nogami@happy.email.ne.jp>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include "wrd.h"
#include "VTparse.h"

//#include <Palettes.h>
#include "mac_main.h"
#include "mac_c.h"
#include "mac_wrdwindow.h"
#include "mac_mag.h"
#include "mac_wrd.h"
#include "aq.h"

static int wrd_argc;
//static int wrd_args[WRD_MAXPARAM];
static int inkey_flag;
//#define WRD_DEBUG(x)	ctl->cmsg x 
#define WRD_DEBUG(x)	/*nothing*/

static int wrdt_open(char *wrdt_opts);
static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[]);
static void wrdt_update_events(void);
static int wrdt_start(int wrdflag)
{
	if( wrdflag ){
		#ifdef ENABLE_SHERRY
		sry_start();
		#endif
		ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
			  "WRD START");
	}
	return 0;
}

static void wrdt_end(void)
{
	wrd_argc = 0;
	inkey_flag = 0;
	#ifdef ENABLE_SHERRY
	sry_end();
	#endif
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "WRD END");
}

static void wrdt_end(void);
static void wrdt_close(void);

#define wrdt mac_wrdt_mode
#define win mac_WrdWindow

WRDTracer wrdt =
{
    "mac WRD tracer", 'm',
    0,
    wrdt_open,
    wrdt_apply,
    #ifdef ENABLE_SHERRY
    sry_wrdt_apply,
    #else
    NULL,
    #endif
    wrdt_update_events,
    wrdt_start,
    wrdt_end,
    wrdt_close
};

// ***********************************************
// Low level output

void dev_set_height(int height)
{
	SizeWindow(win.ref, WRD_GSCR_WIDTH, height, false);
}

static void mac_RedrawControl(int flag)
{
	dev_redrawflag=flag;
}

void dev_redisp(Rect rect)
{
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	
	if( !dev_redrawflag ) return;
	
	//ActivatePalette(win.ref);
	LOCK_ALL_PIXMAP();
	GetGWorld(&oldGW, &oldGD);

		SetPortWindowPort(win.ref);
		RGBBackColor(&white);
		RGBForeColor(&black);
		CopyBits((BitMap*)&DISP_PIX, &win.ref->portBits,
				&rect, &rect, srcCopy,0);

	SetGWorld(oldGW, oldGD);
	UNLOCK_ALL_PIXMAP();
}

static void reverse_helper(int x, int y, int byte)
{
	Rect rect;

	rect.left=WRD_LOCX(x); //erase upper
	rect.top=WRD_LOCY(y-1)+3;
	rect.right=rect.left+BASE_X*byte;
	rect.bottom=rect.top+1;
	PaintRect(&rect);
	
	rect.left=WRD_LOCX(x)+7*byte; //erase right
	//rect.top=WRD_LOCY(y-1)+2;
	rect.right=rect.left+byte;
	rect.bottom=WRD_LOCY(y)+3;
	PaintRect(&rect);
}

static void dev_text_redraw(int locx1, int locy1, int locx2, int locy2)
{
	int x,y,startx, mode,color, len;
	GDHandle	oldGD;
	GWorldPtr	oldGW;

	if( !wrd_ton ) return;
	
	if( locx1<1 ) locx1=1;
	if( locx2>80 ) locx2=80;
	if( locy1<1 ) locy1=1;
	if( locy2>25 ) locy2=25;
	if( wrd_ton==2 ){
		//locx1-= (locx1-1)%4;
		locx1=1;
	}
	
	LOCK_ALL_PIXMAP();
	GetGWorld(&oldGW, &oldGD);

		SetGWorld(dispWorld,0);
		TextMode(srcOr);
		for( y=locy1; y<=locy2; y++){
			startx=locx1;
			if( startx-1>=1 && MULTI_BYTE_FLAG(startx-1,y) )
					startx--;
			for( x=startx; x<=locx2; ){
				if( CHAR_VRAM(x,y)==0 ){ x++; continue;}
				SET_T_RGBFORECOLOR_TMP(CHAR_COLOR_VRAM(x,y)&CATTR_TXTCOL_MASK);
				mode= (CHAR_COLOR_VRAM(x,y)&CATTR_BGCOLORED)? 2:1;
				color= TCODE2INDEX(CHAR_COLOR_VRAM(x,y));
				len= MULTI_BYTE_FLAG(x,y)? 2:1;
				if( wrd_ton==2 ){
					char *cp;
					if( MULTI_BYTE_FLAG(x-1,y) ){
						cp= &CHAR_VRAM(x-1,y); len=2;
						SET_T_RGBFORECOLOR_TMP(CHAR_COLOR_VRAM(x-1,y)&CATTR_TXTCOL_MASK);
						mode= (CHAR_COLOR_VRAM(x-1,y)&CATTR_BGCOLORED)? 2:1;
						color= TCODE2INDEX(CHAR_COLOR_VRAM(x-1,y));
					}else{
						cp= &CHAR_VRAM(x,y);
					}
					dev_draw_text_gmode( dispWorld->portPixMap, WRD_LOCX(x), WRD_LOCY(y-1)+3,
						cp, len, 0xFF, mode, color, color, wrd_ton );
					x+= len*2;
					continue;
				}
				(CHAR_COLOR_VRAM(x,y)&CATTR_BGCOLORED)?
					TextMode(notSrcOr) : TextMode(srcOr);
				WRD_MOVE_COURSOR_TMP(x,y);
				if( MULTI_BYTE_FLAG(x,y) ){
					DrawText(&CHAR_VRAM(x,y), 0, 2);
					if(CHAR_COLOR_VRAM(x,y)&CATTR_BGCOLORED) reverse_helper(x,y, 2);
					x+=2;
				}else{
					if( CHAR_VRAM(x,y)==' ' && (CHAR_COLOR_VRAM(x,y)&CATTR_BGCOLORED) ){
						Rect rect;              //speedy draw
						rect.top=WRD_LOCY(y-1)+3;
						rect.left=WRD_LOCX(x);
						rect.bottom=rect.top+BASE_Y;
						rect.right=rect.left+BASE_X;
						PaintRect(&rect); x++;
					}else{
						DrawText(&CHAR_VRAM(x,y), 0, 1);			
						if(CHAR_COLOR_VRAM(x,y)&CATTR_BGCOLORED) reverse_helper(x,y, 1); x++;
					}
				}
			}
		}

	SetGWorld(oldGW, oldGD);
	UNLOCK_ALL_PIXMAP();
}

static void dev_text_redraw_rect(Rect rect)
{
	dev_text_redraw(rect.left/BASE_X+1, rect.top/BASE_Y+1,
						rect.right/BASE_X+1, rect.bottom/BASE_Y+1);
}

void dev_remake_disp(Rect rect)
{						//copy gdisp -> disp, draw text on gdisp
	LOCK_ALL_PIXMAP();
		if( dev_gon_flag) MyCopyBits(GDISP_PIX, DISP_PIX,
						rect, rect, 0, 0, 0xFF, 0,0,0);
			else dev_box(DISP_PIX, rect, 0, 0xFF); //all pal=0 color
	UNLOCK_ALL_PIXMAP();
	
	dev_text_redraw_rect(rect);
}

static Rect loc2rect(int locx1, int locy1, int locx2, int locy2)
{
	Rect	rect;
	
	if( locx1 < 1 ) locx1=1;
	if( locx2 > COLS ) locx2=COLS;
	if( locy1 < 1 ) locy1=1;
	if( locy2 > LINES ) locy2=LINES;
	
	rect.top=WRD_LOCY(locy1-1)+3;
	rect.left=WRD_LOCX(locx1);
	rect.bottom=WRD_LOCY(locy2)+3;
	rect.right=WRD_LOCX(locx2+1);
	return rect;
}

static void dev_text_clear(int locx1, int locy1, int locx2, int locy2,
							int color, char ch, int need_update)
{									// clear (x1,y1) .... (x2,y1)
	int		y, startx,endx, width;
	
	if( COLS<locx2 ) locx2=COLS;
	if( locx1<0 || COLS<locx1  || locx2<0 ||
		locy1<0 || LINES<locy1 || locy2<0 || LINES<locy2 ) return;
	if( locx2 < locx1 ) return;
	
	if( ch==' ' && !(color & 0x08) ){ch=0;}
	width=locx2-locx1+1;
	for( y=locy1; y<=locy2; y++ ){
		startx= locx1-(MULTI_BYTE_FLAG(locx1-1,y)? 1:0);
		endx= locx2+(MULTI_BYTE_FLAG(locx2,y)? 1:0);
		width=endx-startx+1;
		memset(&CHAR_VRAM(startx,y), ch, width);
		memset(&MULTI_BYTE_FLAG(startx,y), 0, width);
		memset(&CHAR_COLOR_VRAM(startx,y), color, width);
	}
	if( need_update ){
		Rect rect=loc2rect(locx1-1, locy1, locx2+1, locy2); //take margin
		dev_remake_disp(rect);
		dev_redisp(rect);
	}
}

static void dev_text_clear_all()
{
	memset(&CHAR_VRAM(0,0), 0, sizeof(char_vram));
	memset(&MULTI_BYTE_FLAG(0,0), 0, sizeof(multi_byte_flag));
}

static void dev_text_output(const char* text, int n)
{	
	int i, startx=wrd_coursor_x, endx=wrd_coursor_x+n-1;
	GDHandle	oldGD;
	GWorldPtr	oldGW;

	if( wrd_coursor_x<=0 || 81<=wrd_coursor_x ||
		wrd_coursor_y<=0 || 26 <=wrd_coursor_y ) return;

	LOCK_ALL_PIXMAP();
	GetGWorld(&oldGW, &oldGD);
	
	dev_text_clear(startx, wrd_coursor_y, endx, wrd_coursor_y, 0, 0, false);
	
	SetGWorld(oldGW, oldGD);
	UNLOCK_ALL_PIXMAP();
	
	for( i=0; i<n; i++ ){
		if( wrd_coursor_x+i<=0 || 81<=wrd_coursor_x+i ||
			wrd_coursor_y<=0 || 26 <=wrd_coursor_y ) continue;
		CHAR_VRAM(wrd_coursor_x+i,wrd_coursor_y)=text[i];
		CHAR_COLOR_VRAM(wrd_coursor_x+i,wrd_coursor_y)= wrd_text_color_attr;
		if( IS_MULTI_BYTE(text[i]) ){
			MULTI_BYTE_FLAG(wrd_coursor_x+i,wrd_coursor_y)=1;
			if( i<n ){
				i++; CHAR_VRAM(wrd_coursor_x+i,wrd_coursor_y)=text[i];
				MULTI_BYTE_FLAG(wrd_coursor_x+i,wrd_coursor_y)=0;
			}
		}
	}
	wrd_coursor_x+=n;
	if( wrd_ton==2) endx+=2;

	dev_remake_disp(loc2rect(startx-1, wrd_coursor_y, endx+1, wrd_coursor_y));
	dev_redisp(loc2rect(startx-1, wrd_coursor_y, endx+1, wrd_coursor_y));
}

static void dev_text_scroll(int x1, int y1, int x2, int y2, int mode, int color, char ch, int num)
{
	int y,width;

	if( num<=0 ) return;
	switch(mode)
	{
	case 0: //scroll upper
		for( y=y1; y<=y2 && y<=LINES; y++ ){
			if( y-num <y1 ) continue;
			memcpy(&CHAR_VRAM(1,y-num),&CHAR_VRAM(1,y),COLS);
			memcpy(&CHAR_COLOR_VRAM(1,y-num),&CHAR_COLOR_VRAM(1,y),COLS);
			memcpy(&MULTI_BYTE_FLAG(1,y-num),&MULTI_BYTE_FLAG(1,y),COLS);
		}
		dev_text_clear(x1, y2-num+1, x2, y2, color, ch, false);
		break;
	case 1: //scroll down
		for( y=y2; y>=y1 && y>=1; y-- ){
			if( y+num> y2 ) continue;
			memcpy(&CHAR_VRAM(1,y+num),&CHAR_VRAM(1,y),COLS);
			memcpy(&CHAR_COLOR_VRAM(1,y+num),&CHAR_COLOR_VRAM(1,y),COLS);
			memcpy(&MULTI_BYTE_FLAG(1,y+num),&MULTI_BYTE_FLAG(1,y),COLS);
		}
		dev_text_clear(x1, y1, x2, y1+num-1, color, ch, false);
		break;
	case 2: //scroll right
	case 3: //scroll left
		if( mode==3 ) num*=-1;
		if( x1+num<1 ) x1=1-num;
		if( x2+num>COLS ) x2=COLS-num;
		width=x2-x1+1; if( width<=0 ) break;
		for( y=y1; y<=y2 && y<=LINES; y++ ){
			memmove(&CHAR_VRAM(x1+num,y),&CHAR_VRAM(x1,y),width);
			memmove(&CHAR_COLOR_VRAM(x1+num,y),&CHAR_COLOR_VRAM(x1,y),width);
			memmove(&MULTI_BYTE_FLAG(x1+num,y),&MULTI_BYTE_FLAG(x1,y),width);
		}
		if( mode==2 ) //right
			dev_text_clear(x1, y1, x1+num-1, y2, color, ch, false);
		else if( mode==3 )
			dev_text_clear(x2+num+1, y1, x2, y2, color, ch, false);			
		break;
	}
}

static void dev_move_coursor(int x, int y)
{
	wrd_coursor_x=x;
	wrd_coursor_y=y;
	if( wrd_coursor_x<1 ) wrd_coursor_x=1;
	else if( wrd_coursor_x>COLS ) wrd_coursor_x=COLS;
	if( wrd_coursor_y<1 ) wrd_coursor_y=1;
	else if( wrd_coursor_y>LINES ) wrd_coursor_y=LINES;
}

static void dev_newline()
{
	if( wrd_coursor_y>=25 ){
		dev_text_scroll(1, 1, 80, 25, 0, 0, 0, 1);
		dev_remake_disp(portRect);
		dev_redisp(portRect);
		dev_move_coursor(1, 25);
	}else{
		dev_move_coursor(1, wrd_coursor_y+1);
	}
}

static void dev_clear_graphics(int pmask)
{				//clear active bank only
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	
	GetGWorld(&oldGW, &oldGD);
	LOCK_ALL_PIXMAP();

	SetGWorld(graphicWorld[activeGraphics],0);	
		dev_box(GACTIVE_PIX, portRect, 0, pmask);
	UNLOCK_ALL_PIXMAP();
	SetGWorld(oldGW, oldGD);
	
	if( activeGraphics==dispGraphics ){
		dev_remake_disp(portRect);
		dev_redisp(portRect);
	}
}

#define  CHECK_RECT(rect) {            \
	short	tmp;                       \
	if( rect.left>rect.right ){ tmp=rect.left; rect.left=rect.right; rect.right=tmp;} \
	if( rect.top>rect.bottom ){ tmp=rect.top; rect.top=rect.bottom; rect.bottom=tmp;} \
}


void dev_gmove(int x1, int y1, int x2, int y2, int xd, int yd,
		GWorldPtr srcworld, GWorldPtr destworld, int sw, int trans, int pmask,
		int maskx, int masky, const uint8 maskdata[])
{
	static Rect	src,dest, rect;
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	
	if( srcworld==NULL || destworld==NULL ){
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Can't use gvram bank" );
		return;
	}
	
	LOCK_ALL_PIXMAP();
	GetGWorld(&oldGW, &oldGD);
	LockPixels(srcworld->portPixMap);
	LockPixels(destworld->portPixMap);
	
	SetRect(&src,  x1,y1,x2+1,y2+1);		CHECK_RECT(src);
	SetRect(&dest, xd,yd, xd+x2-x1+1, yd+y2-y1+1);	CHECK_RECT(dest);
	
	SetPortWindowPort(win.ref);
	RGBBackColor(&white);
	RGBForeColor(&black);
	if( sw==0 ){ //simple copy
		//CopyBits((BitMap*)&graphicWorld[vs]->portPixMap, (BitMap*)&graphicWorld[vd]->portPixMap,
		//		&src, &dest, srcCopy,0); //make offscreen Graphics
		MyCopyBits(srcworld->portPixMap, destworld->portPixMap,
				src, dest, 0, 0, gmode_mask,0,0,0); //make offscreen Graphics
	} else if(sw==1){ //exchange
		CopyBits((BitMap*)&srcworld->portPixMap, (BitMap*)&destworld->portPixMap,
				&src, &dest, srcXor,0);
		CopyBits((BitMap*)&destworld->portPixMap, (BitMap*)&srcworld->portPixMap,
				&dest, &src, srcXor,0);
		CopyBits((BitMap*)&srcworld->portPixMap, (BitMap*)&destworld->portPixMap,
				&src, &dest, srcXor,0);	//make offscreen Graphics
		
	} else if(sw==2){	//xor copy
		CopyBits((BitMap*)&srcworld->portPixMap, (BitMap*)&destworld->portPixMap,
				&src, &dest, srcXor,0); //make offscreen Graphics
	}else if( sw & 0x10 ){ //xcopy mode
		MyCopyBits(srcworld->portPixMap, destworld->portPixMap,
			src, dest, sw, trans, pmask, maskx, masky, maskdata); //make offscreen Graphics
	}
	
	SetGWorld(oldGW, oldGD);
	UNLOCK_ALL_PIXMAP();
	UnlockPixels(srcworld->portPixMap);
	UnlockPixels(destworld->portPixMap);

	if( graphicWorld[dispGraphics]==destworld ){
		dev_remake_disp(dest);
		dev_redisp(dest);
	}
	if( graphicWorld[dispGraphics]==srcworld && sw==1 ){  //exchange? update src
		dev_remake_disp(src);
		dev_redisp(src);
	}
}

static void dev_gscreen(int act, int dis)
{
	if( act!=0 && act!=1 ) return;
	if( dis!=0 && dis!=1 ) return;
	
	activeGraphics=act;
	if( dispGraphics!=dis ){
		dispGraphics=dis;
		dev_remake_disp(portRect);
		dev_redisp(portRect);
	}
}

static int dev_get_pixel(int x, int y)
{
	Ptr baseadr;
	int rowBytes;
	
	baseadr=GetPixBaseAddr(graphicWorld[activeGraphics]->portPixMap);
	rowBytes= (**graphicWorld[activeGraphics]->portPixMap).rowBytes & 0x1FFF;

	return baseadr[y*rowBytes+x];
}

void dev_gline(int x1, int y1, int x2, int y2, int p1, int sw, int p2, GWorldPtr world)
{
	Rect	rect;
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	
	GetGWorld(&oldGW, &oldGD);
	LockPixels(world->portPixMap);
	SetGWorld(world,0);
	
	rect.left=x1; rect.right=x2;
	rect.top=y1; rect.bottom=y2;
	CHECK_RECT(rect);
	
	switch(sw)
	{
	case 0: //line
		if( p2==0 || p2==WRD_NOARG ) p2= 0xFF;
		dev_line(x1, y1, x2, y2, p1,p2, gmode_mask_gline,
			world->portPixMap );
		break;
	case 1: //rect
		if( p2==0 || p2==WRD_NOARG ) p2= 0xFF;
		dev_line(x1, y1, x2, y1, p1,p2, gmode_mask_gline,world->portPixMap );
		dev_line(x1, y1, x1, y2, p1,p2, gmode_mask_gline,world->portPixMap );
		dev_line(x2, y1, x2, y2, p1,p2, gmode_mask_gline,world->portPixMap );
		dev_line(x1, y2, x2, y2, p1,p2, gmode_mask_gline,world->portPixMap );
		break;
	case 2:	//filled rect
	      	if( p2==WRD_NOARG ) p2= p1;
		rect.right++; rect.bottom++;
		dev_box(world->portPixMap, rect, p2, gmode_mask_gline);
		if( p1!=p2 ){
			dev_line(x1, y1, x2, y1, p1,0xFF, gmode_mask_gline,world->portPixMap );
			dev_line(x1, y1, x1, y2, p1,0xFF, gmode_mask_gline,world->portPixMap );
			dev_line(x2, y1, x2, y2, p1,0xFF, gmode_mask_gline,world->portPixMap );
			dev_line(x1, y2, x2, y2, p1,0xFF, gmode_mask_gline,world->portPixMap );
		}
		break;
	}
	SetGWorld(oldGW, oldGD);
	UnlockPixels(world->portPixMap);

	if( graphicWorld[dispGraphics]==world ){
		rect.right++; rect.bottom++;
		dev_remake_disp(rect);
		if( pallette_exist) dev_redisp(rect);
	}
}

static void dev_gcircle(int x, int y, int r, int p1, int sw, int p2)
{
	//int		onbit;
	Rect	rect;
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	
	GetGWorld(&oldGW, &oldGD);
	LockPixels(GACTIVE_PIX);
	SetGWorld(graphicWorld[activeGraphics],0);
	rect.left=x-r; rect.right=x+r;
	rect.top=y-r; rect.bottom=y+r;
	
	switch(sw)
	{
	case 0:
	case 1: //frame
		SET_G_COLOR(p1,graphicWorld[activeGraphics]);
		FrameOval(&rect);
		break;
	case 2:	//filled circle
		SET_G_COLOR(p2,graphicWorld[activeGraphics]);
		PaintOval(&rect);
		SET_G_COLOR(p1,graphicWorld[activeGraphics]);
		FrameOval(&rect);
		break;
	}
	SetGWorld(oldGW, oldGD);
	UnlockPixels(GACTIVE_PIX);

	if( activeGraphics==dispGraphics ){
		rect.right++; rect.bottom++;
		dev_remake_disp(rect);
		dev_redisp(rect);
	}
}

static void dev_set_text_attr(int esccode);
static int Parse(int c);

void dev_init(int version)
{
	int i;
	
	inkey_flag = 0;
	dev_gon_flag=1;
	dev_set_text_attr(37); //white
	dev_change_1_palette(0, black);
	dev_change_1_palette(16, black); //for gon(0)
	
    gmode_mask=0xF;
   	if( version<=0 || (380<=version && version<=399))
			gmode_mask_gline=0x7;  //change gline behavier
	else    gmode_mask_gline=0xF;

	for(i=0; i<gvram_bank_num; i++){
		dev_gscreen(i, dispGraphics); dev_clear_graphics(0xFF);
	}
    dev_gscreen(0, 0);

	dev_text_clear_all(); wrd_ton=1;
   	dev_remake_disp(portRect);
	dev_redisp(portRect);

	dev_move_coursor(1,1);
	startpal=endpal=0;
	pallette_exist=true;
	fading=false;
	Parse(-1); //initialize parser
	
	wrd_init_path();
}

static OSErr get_vsscreen()
{
	OSErr	err;
	Rect	rect=portRect;
	
	rect.right++; rect.bottom++;  //keep safty zone
	err=NewGWorld(&graphicWorld[gvram_bank_num], 8, &rect,
						0, 0,0);
	if( ! err ){
		ctl->cmsg(CMSG_INFO, VERB_NORMAL, "get gvram bank %d",gvram_bank_num );
		gvram_bank_num++;
	}else{
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Can't get gvram bank %d",gvram_bank_num );
	}
	return err;
}

static OSErr dev_vsget(int num)
{
	OSErr	err;
	int i;
	for( i=gvram_bank_num; i<2+num; i++ ){
		err=get_vsscreen();
		if( err ) return err;
	}
	return 0;
}

static OSErr dev_setup()
{
	static OSErr	err=0;
	int		i;
	Rect		destRect;
	
	if( err ) return err; // once errored, do not retry
	
	destRect.top=destRect.left=0;
	destRect.right=640;
	destRect.bottom=480;
	gvram_bank_num=0;
	
	err=NewGWorld(&dispWorld, 8, &destRect,0,0,0);
	if( err ) return err;
	
	{
	Rect charbufRect={0,0,16,32};
	err=NewGWorld(&charbufWorld, 8, &charbufRect,0,0,0);
	if( err ) return err;
	mac_setfont(charbufWorld, WRD_FONTNAME);
	}
	
	//wrd_palette= NewPalette( 33, 0, pmTolerant, 0x0000);
	//if( wrd_palette )  SetPalette(win.ref, wrd_palette, true);
	
	for( i=0; i<=1; i++){
		err=get_vsscreen();
		if( err ) return err;
	}
	
	dev_init_text_color();
	
	mac_setfont(dispWorld, WRD_FONTNAME);

	dev_init(-1);
	return 0; //noErr
}

// ***********************************************
#pragma mark -

/*VTParse Table externals*/
extern int groundtable[];
extern int csitable[];
extern int dectable[];
extern int eigtable[];
extern int esctable[];
extern int iestable[];
extern int igntable[];
extern int scrtable[];
extern int scstable[];
extern int mbcstable[];
extern int smbcstable[];
#define MAXPARAM 20
#define DEFAULT -1
#define TAB_SET 8
#define MBCS 1
#define CATTR_LPART (1)
#define CATTR_16FONT (1<<1)
#define CATTR_COLORED (1<<2)
#define CATTR_BGCOLORED (1<<3) 
#define CATTR_TXTCOL_MASK_SHIFT 4
#define CATTR_TXTCOL_MASK (7<<CATTR_TXTCOL_MASK_SHIFT)
#define CATTR_INVAL (1<<31)

static void DelChar(int /*x*/, int /*y*/){}
#define ClearLine(y) dev_text_clear(1, (y), 80, (y), 0, 0, false)
#define ClearRight()	dev_text_clear(wrd_coursor_x, wrd_coursor_y, 80, wrd_coursor_y, 0, 0, false)
#define ClearLeft()		dev_text_clear(1, wrd_coursor_y, wrd_coursor_x, wrd_coursor_y, 0, 0, false)

static void RedrawInject(int x1, int y1, int x2, int y2, Boolean)
{
	Rect rect;
	SetRect(&rect,x1,y1,x2,y2);
	dev_remake_disp(rect);
	dev_redisp(rect);
}

static void dev_set_text_attr(int esccode)
{
start:
	switch(esccode){
	default:
	  esccode=37; goto start;

	case 17: esccode=31; goto start;
	case 18: esccode=34; goto start;
	case 19: esccode=35; goto start;
	case 20: esccode=32; goto start;
	case 21: esccode=33; goto start;
	case 22: esccode=36; goto start;
	case 23: esccode=37; goto start;

	case 16:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	  wrd_text_color_attr&=~CATTR_TXTCOL_MASK;
	  wrd_text_color_attr|=(
			  ((esccode>=30)?(esccode-30):(esccode-16))<<
			  CATTR_TXTCOL_MASK_SHIFT);
	  wrd_text_color_attr|=CATTR_COLORED;
	  wrd_text_color_attr&=~CATTR_BGCOLORED;
	  break;
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	  wrd_text_color_attr&=~CATTR_TXTCOL_MASK;
	  wrd_text_color_attr|=(esccode-40)<<CATTR_TXTCOL_MASK_SHIFT;
	  wrd_text_color_attr|=CATTR_BGCOLORED;
	  break;
	}
}

static int Parse(int c)
{
  static int *prstbl=groundtable;
  static char mbcs;
  static int params[MAXPARAM],nparam=0;
  static int hankaku=0;
  static int savcol,savline;
  static long savattr;

  if(c==-1) {
    prstbl=groundtable;
    mbcs=0;
    nparam=0;
    hankaku=0;
    savcol=savline=0;
    return 0;
  }

  if(mbcs&&
     prstbl !=mbcstable&&
     prstbl !=scstable&&
     prstbl !=scstable){
    mbcs=0;
  }
  switch(prstbl[c]){
  case CASE_IGNORE_STATE:
    prstbl=igntable;
    break;
  case CASE_IGNORE_ESC:
    prstbl=iestable;
    break;
  case CASE_ESC:
    prstbl=esctable;
    break;
  case CASE_ESC_IGNORE:
    prstbl=eigtable;
    break;
  case CASE_ESC_DIGIT:
    if(nparam<MAXPARAM){
      if(params[nparam]==DEFAULT){
	params[nparam]=0;
      }
      if( c==' ' ){
      	c='0';
      }
      params[nparam]*=10;
      params[nparam]+=c-'0';
    }
    break;
  case CASE_ESC_SEMI:
    nparam++;
    params[nparam]=DEFAULT;
    break;
  case CASE_TAB:
    wrd_coursor_x+=TAB_SET;
    wrd_coursor_x&=~(TAB_SET-1);
    break;
  case CASE_BS:
    if(wrd_coursor_x > 0)
      wrd_coursor_x--;
#if 0 /* ^H maybe work backward character in MIMPI's screen */
    DelChar(wrd_coursor_y,wrd_coursor_x);
    mywin.scrnbuf[wrd_coursor_y][wrd_coursor_x].c=0;
    mywin.scrnbuf[wrd_coursor_y][wrd_coursor_x].attr=0;
#endif
    break;
  case CASE_CSI_STATE:
    nparam=0;
    params[0]=DEFAULT;
    prstbl=csitable;
    break;
  case CASE_SCR_STATE:
    prstbl=scrtable;
    mbcs=0;
    break;
  case CASE_MBCS:
    hankaku=0;
    prstbl=mbcstable;
    mbcs=MBCS;
    break;
  case CASE_SCS_STATE:
    if(mbcs)
      prstbl=smbcstable;
    else
      prstbl=scstable;
    break;
  case CASE_GSETS:
    wrd_text_color_attr=(mbcs)?(wrd_text_color_attr|CATTR_16FONT):
      (wrd_text_color_attr&~(CATTR_16FONT));
    if(!mbcs){
      hankaku=(c=='I')?1:0;
    }
    prstbl=groundtable;
    break;
  case CASE_DEC_STATE:
    prstbl =dectable;
    break;
  case CASE_SS2:
  case CASE_SS3:
    /*These are ignored because this will not accept SS2 SS3 charset*/
  case CASE_GROUND_STATE:
    prstbl=groundtable;
    break;
  case CASE_CR:
    wrd_coursor_x=1;
    prstbl=groundtable;
    break;
  case CASE_IND:
  case CASE_VMOT:
    wrd_coursor_y++;
    wrd_coursor_x=1;
    prstbl=groundtable;
    break;
  case CASE_CUP:
    wrd_coursor_y=(params[0]<1)?0:params[0];
    if(nparam>=1)
      wrd_coursor_x=(params[1]<1)?0:params[1];
    else
      wrd_coursor_x=0;
    prstbl=groundtable;
    break;
  case CASE_PRINT:
    if(wrd_text_color_attr&CATTR_16FONT){
      if(!(wrd_text_color_attr&CATTR_LPART)&&(wrd_coursor_x==COLS)){
	wrd_coursor_x++;
	return 1;
      }
      wrd_text_color_attr^=CATTR_LPART;
    }
    else
      wrd_text_color_attr&=~CATTR_LPART;
    DelChar(wrd_coursor_y,wrd_coursor_x);
    if(hankaku==1)
      c|=0x80;
    //mywin.scrnbuf[wrd_coursor_y][wrd_coursor_x].attr=wrd_text_color_attr;
    //mywin.scrnbuf[wrd_coursor_y][wrd_coursor_x].c=c;
    wrd_coursor_x++;
    break;
  case CASE_CUU:
    wrd_coursor_y-=((params[0]<1)?1:params[0]);
    prstbl=groundtable;
    break;
  case CASE_CUD:
    wrd_coursor_y+=((params[0]<1)?1:params[0]);
    prstbl=groundtable;
    break;
  case CASE_CUF:
    wrd_coursor_x+=((params[0]<1)?1:params[0]);
    prstbl=groundtable;
    break;
  case CASE_CUB:
    wrd_coursor_x-=((params[0]<1)?1:params[0]);
  	if( wrd_coursor_x<1 ) wrd_coursor_x=1;
    prstbl=groundtable;
    break;
  case CASE_ED:
    switch(params[0]){
    case DEFAULT:
    case 0:
      {
	int j;
	  ClearRight();
	for(j=wrd_coursor_y+1;j<=LINES;j++)
	  ClearLine(j);	
      }
      break;
    case 1:
      {
	int j;
	  ClearLeft();
	for(j=1;j<wrd_coursor_y;j++)
	  ClearLine(j);	
      }
      break;
    case 2:
      {
	//int j;
	//for(j=0;j<LINES;j++){
	//  free(mywin.scrnbuf[j]);
	//  mywin.scrnbuf[j]=NULL;
	//}
	dev_text_clear_all();
	wrd_coursor_y=1;
	wrd_coursor_x=1;
	break;
      }
    }
    RedrawInject(0,0,SIZEX,SIZEY,false);
    prstbl=groundtable;
    break;
  case CASE_DECSC:
    savcol=wrd_coursor_x;
    savline=wrd_coursor_y;
    savattr=wrd_text_color_attr;
    prstbl=groundtable;
  case CASE_DECRC:
    wrd_coursor_x=savcol;
    wrd_coursor_y=savline;
    wrd_text_color_attr=savattr;
    prstbl=groundtable;
    break;
  case CASE_SGR:
    {
      int i;
      for(i=0;i<nparam+1;i++)
		dev_set_text_attr(params[i]);
    }
    prstbl=groundtable;
    break;
  case CASE_EL:
    switch(params[0]){
    case DEFAULT:
    case 0:
      ClearRight();
      break;
    case 1:
      ClearLeft();
      break;
    case 2:
      ClearLine(wrd_coursor_y);
      break;
    }
    RedrawInject(0,0,SIZEX,SIZEY,false);
    prstbl=groundtable;
    break;
  case CASE_NEL:
    wrd_coursor_y++;
    wrd_coursor_x=1;
    wrd_coursor_y=(wrd_coursor_y<LINES)?wrd_coursor_y:LINES;
    break;
/*Graphic Commands*/
  case CASE_MY_GRAPHIC_CMD:
    //GrphCMD(params,nparam);
    prstbl=groundtable;
    break;
  case CASE_DL:
	dev_text_scroll(1, wrd_coursor_y+params[0], COLS, LINES, 0, 0, 0, params[0]);
    RedrawInject(0,0,SIZEX,SIZEY,false);
	prstbl=groundtable;
	break;
/*Unimpremented Command*/
  case CASE_ICH:
  case CASE_IL:
  case CASE_DCH:
  case CASE_DECID:
  case CASE_DECKPAM:
  case CASE_DECKPNM:
  //case CASE_IND:
  case CASE_HP_BUGGY_LL:
  case CASE_HTS:
  case CASE_RI:
  case CASE_DA1:
  case CASE_CPR:
  case CASE_DECSET:
  case CASE_RST:
  case CASE_DECSTBM:
  case CASE_DECREQTPARM:
  case CASE_OSC:
  case CASE_RIS:
  case CASE_HP_MEM_LOCK:
  case CASE_HP_MEM_UNLOCK:
  case CASE_LS2:
  case CASE_LS3:
  case CASE_LS3R:
  case CASE_LS2R:
  case CASE_LS1R:
    ctl->cmsg(CMSG_INFO,VERB_VERBOSE,"NOT IMPREMENTED:%d\n",prstbl[c]);
    prstbl=groundtable;
    break;
  case CASE_BELL:
  case CASE_IGNORE:
  default:
    break;
  }
  if( prstbl==groundtable ) return 1;
  return 0;
}

// ***********************************************
#pragma mark -
#pragma mark ================== Wrd Window

static int open_WrdWin();
static void click_WrdWin(Point local, short modifiers);
static void update_WrdWin();
static int	message_WrdWin(int message, long param);

//#define win mac_WrdWindow  //already defined
MacWindow win={
	0,	//WindowRef
	open_WrdWin,
	click_WrdWin,
	update_WrdWin,
	goaway_default,
	close_default,
	message_WrdWin,
	0, 50,70
};

static int open_WrdWin()
		/*success-> return 0;*/
{
	open_window(&win, kWrdWinID);
	position_window(&win);
	
	return 0;
}

static void click_WrdWin(Point /*local*/, short /*modifiers*/)
{
}

static void update_WrdWin()
{
	dev_redisp(portRect);
}

static int	message_WrdWin(int message, long /*param*/)
{
	Rect rect;
	
	switch(message){
	case MW_GROW:
		rect=win.ref->portRect;
		//rect.right-=15; rect.bottom-=15;
		rect = win.ref->portRect;
		rect.right  -= 15;
		//rect.bottom -= 15;
		return 0;
	
	}

	return -1;  //not supported
}

// ***********************************************
#pragma mark -

/*ARGSUSED*/
static int wrdt_open(char * /*wrdt_opts*/)
{	//  success -> return 0
	OSErr	err;
	
	err=dev_setup();
	if( err ) return err;
    wrdt.opened = 1;
    wrd_argc = 0;
    inkey_flag = 0;
	ctl->cmsg(CMSG_INFO,VERB_VERBOSE,"open macintosh wrd player\n");
    return 0;
}

static void wrdt_update_events(void)
{
}

static void wrdt_close(void)
{
    wrdt.opened = 0;
    inkey_flag = 0;
}

static char *wrd_event2string(int id)
{
    char *name;

    name = event2string(id);
    if(name != NULL)
	return name + 1;
    return "";
}

// **************************************************
#pragma mark -
static void mac_wrd_pal(int pnum, int wrd_args[])
{
	int code;
	RGBColor color;
	
	for( code=0; code<16; code++ ){
		color.red=((wrd_args[code] >> 8) & 0x000F) * 0x1111;
		color.green=((wrd_args[code] >> 4) & 0x000F) * 0x1111;
		color.blue=(wrd_args[code] & 0x000F) * 0x1111;
		dev_palette[pnum][code]=color;
		if( pnum==0 ){
			dev_change_1_palette(code, color);
		}
	}

	if( pnum==0 ){
		//dev_remake_disp(portRect);
		dev_redisp(portRect);
	}
}

static void wrd_fadestep(int nowstep, int maxstep)
{
	RGBColor	pal[16];
	int code;
	//static unsigned long	lasttick=0;
	static int	skip_num;
	
	//if( nowstep!=1 && nowstep!=maxstep /*&& (nowstep%4)==0*/ && lasttick==TickCount() ){
	//	return;  //too fast fade. skip fading.
	//}
	
	if( nowstep==1 ){
		skip_num=0;
	}
	
	if( nowstep!=maxstep && !mac_flushing_flag){	//consider skipping
		const int	skip_threshold[11]={99,99,6,5,4, 2,1,0,0,0,0};
		int	threshold= skip_threshold[ (int)(aq_filled_ratio()*10) ];
		if( skip_num<threshold ){
			skip_num++;
			return;     // system is busy
		}
	}
	
	skip_num=0;
	for( code=0; code<16; code++ ){
		pal[code].red=
			(dev_palette[startpal][code].red*(maxstep-nowstep) +
				dev_palette[endpal][code].red*nowstep)/maxstep;
		pal[code].green=
			(dev_palette[startpal][code].green*(maxstep-nowstep) +
				dev_palette[endpal][code].green*nowstep)/maxstep;
		pal[code].blue=
			(dev_palette[startpal][code].blue*(maxstep-nowstep) +
				dev_palette[endpal][code].blue*nowstep)/maxstep;
	}
	dev_change_palette(pal);
	dev_redisp(portRect);
	if( nowstep==maxstep ) fading=false;
	//lasttick=TickCount();
}

static void mac_wrd_fade(int p1, int p2, int speed)
{
	startpal=p1; endpal=p2;
	if( fading ){	//double fade command
		wrd_fadestep(1, 1);
	}
	if(speed==0){
		dev_change_palette( dev_palette[p2]);
		//dev_remake_disp(portRect);
		dev_redisp(portRect);
	}else{
		fading=true;
	}
}

static void dev_gon(int gon)
{
	dev_gon_flag=gon;
	dev_remake_disp(portRect);
	dev_redisp(portRect);
}

static void dev_palrev(int paln )
{
	int code;
	for( code=0; code<16; code++ ){
		dev_palette[paln][code].red   ^= 0xFFFF;
		dev_palette[paln][code].green ^= 0xFFFF;
		dev_palette[paln][code].blue  ^= 0xFFFF;
	}
	if( paln==0 ){
		dev_change_palette(dev_palette[0]);
		dev_redisp(portRect);
	}
}

static int wrd_mag(char* filename, int x, int y, int /*s*/, int p)
{
	int		err;
	Rect	rect;
	//char	fullpath[255];
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	
	GetGWorld(&oldGW, &oldGD);
	LockPixels(GACTIVE_PIX);
		SetGWorld(graphicWorld[activeGraphics],0);
		err= mac_mag_load(filename,  x,y, GACTIVE_PIX, p ,&rect);
		SetGWorld(oldGW, oldGD);
	UnlockPixels(GACTIVE_PIX);
	
	if( err ) return err;
	
	if( p==0 || p==2 ){
		dev_change_palette(dev_palette[17]);
		rect=portRect;	//update all
	}
	if( activeGraphics==dispGraphics || p==0 || p==2 ){
		dev_remake_disp(rect);
		dev_redisp(rect);
	}
	memcpy(&dev_palette[18+ (activeGraphics==0? 0:1)]
					, &dev_palette[17], 16*sizeof(RGBColor));
	return 0; //no error
}

static int wrd_pho(char* filename)
{
	//char	fullpath[255];
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	
	GetGWorld(&oldGW, &oldGD);
	LockPixels(GACTIVE_PIX);
	SetGWorld(graphicWorld[activeGraphics],0);
	mac_pho_load(filename, GACTIVE_PIX);

	SetGWorld(oldGW, oldGD);
	UnlockPixels(GACTIVE_PIX);
	
	if( activeGraphics==dispGraphics ){
		dev_remake_disp(portRect);
		dev_redisp(portRect);
	}
	return 0; //no error
}

static void wrd_load_default_image()
{
	char	filename[256], *p;
	
	strncpy(filename, current_file_info->filename, sizeof(filename));
	p= strrchr( filename, '.' );
	if( p==0 ) return;
	strncpy( p, ".mag", sizeof(filename) - (p - filename) - 1 );
	filename[sizeof(filename) - 1] = '\0';
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@DEFAULT_LOAD_MAG(%s)", filename);

	if( wrd_mag(filename, WRD_NOARG, WRD_NOARG, 1,0)==0 ) //no err
		return;
	
		//retry pho file
	strncpy(filename, current_file_info->filename sizeof(filename));
	p= strrchr( filename, '.' );
	if( p==0 ) return;
	strncpy( p, ".pho", sizeof(filename) - (p - filename) - 1 );
	filename[sizeof(filename) - 1] = '\0';
	wrd_pho(filename);
}

// **************************************************
#pragma mark -
static void mac_wrd_color(int c)
{
	dev_set_text_attr(c);
}


static void mac_wrd_DrawText(const char* str, int len)
{
	int i;
	
	for( i=0; i<=len; ){
		if( str[i]==0 || i==len ){
			dev_text_output(str, i);
			break;
		}else if( wrd_coursor_x+i>80 ){
			dev_text_output(str, i);
			dev_newline();
			//i++;
			str+=i; len-=i; i=0;		
		}else if( str[i]=='\x1b' ){ //esc sequence
			if( i ){
				dev_text_output(str, i);
				str+=i; len-=i; i=0;
			}
			for(;;i++){
				if( Parse(str[i]) ){
					break; //esc sequence ended
				}
			}
			i++;
			str+=i; len-=i; i=0;			
		}else if (str[i]=='\t' ){ //tab space
			int newx;
			dev_text_output(str, i);
			newx=((wrd_coursor_x-1)|7)+2;
			dev_text_clear(wrd_coursor_x, wrd_coursor_y, newx-1, wrd_coursor_y, 0, 0, true);
			dev_move_coursor(newx,wrd_coursor_y);
			i++;
			str+=i; len-=i; i=0;
		}else{
			i++;
		}
	}
}

static void mac_wrd_doESC(const char* code )
{
	char	str[20]="\33[";
	strncat(str, code, sizeof(str) - strlen(str) - 1);
        str[sizeof(str)-1] = '\0';
	mac_wrd_DrawText(str, strlen(str));
}

static void mac_wrd_event_esc(int esc)
{	
	mac_wrd_doESC(event2string(esc)+1);
}

static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[])
{
    char *p;
    char *text;
    int i, len;
	
	if( ! win.show ) return;

    //wrd_args[wrd_argc++] = arg;
    if(cmd == WRD_ARG)
	return;
    if(cmd == WRD_MAGPRELOAD){
	//char *p = wrd_event2string(arg);
	 /* Load MAG file */
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@WRD_MAGPRELOAD"));
    wrd_argc = 0;
	return;
	}
    if(inkey_flag)
	printf("* ");
    switch(cmd)
    {
    case WRD_LYRIC:
    case WRD_NL:
		if(cmd == WRD_NL)
			text = "\n";
		else{
			p = wrd_event2string(wrd_args[0]);
			len = strlen(p);
			text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));
			code_convert(p, text, SAFE_CONVERT_LENGTH(len), NULL, NULL);
		}
		len = strlen(text);
		if( len ){
		mac_wrd_DrawText(text, text[len-1]=='\n'? len-1:len);
		if( text[len-1]=='\n' ){
			dev_newline();
		}
		ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "%s", text);
		}
		reuse_mblock(&tmpbuffer);
		break;
      case WRD_COLOR:
		mac_wrd_color(wrd_args[0]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE, "@COLOR(%d)", wrd_args[0]));
	break;
      case WRD_END: /* Never call */
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,"@END"));
	break;
      case WRD_ESC:
      	mac_wrd_event_esc(wrd_args[0]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@ESC(%s)", wrd_event2string(wrd_args[0])));
	break;
      case WRD_EXEC:
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@EXEC(%s)", wrd_event2string(wrd_args[0])));
	break;
      case WRD_FADE:
	mac_wrd_fade(wrd_args[0], wrd_args[1], wrd_args[2]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@FADE(%d,%d,%d)", wrd_args[0], wrd_args[1], wrd_args[2]));
	break;
      case WRD_FADESTEP:
	wrd_fadestep(wrd_args[0], WRD_MAXFADESTEP);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@FADESTEP(%d/%d)", wrd_args[0], WRD_MAXFADESTEP));
	break;
      case WRD_GCIRCLE:
	dev_gcircle(wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@GCIRCLE(%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]));
	break;
      case WRD_GCLS:
	dev_clear_graphics(wrd_args[0]? wrd_args[0]:0xFF);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@GCLS(%d)", wrd_args[0]));
	break;
      case WRD_GINIT:
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE, "@GINIT()"));
	break;
      case WRD_GLINE:
	dev_gline(wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6],graphicWorld[activeGraphics]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@GLINE(%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6]));
	break;
      case WRD_GMODE:
	DEV_SET_GMODE(wrd_args[0]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@GMODE(%d)", wrd_args[0]));
	break;
      case WRD_GMOVE:
	wrd_args[0] &= ~0x7;  wrd_args[4] &= ~0x7;  
	wrd_args[2] |= 0x7;
	dev_gmove(wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], graphicWorld[wrd_args[6]], graphicWorld[wrd_args[7]],
	       wrd_args[8], 0, gmode_mask, 0,0,0);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@GMOVE(%d,%d, %d,%d, %d,%d, %d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6], wrd_args[7], wrd_args[8]));
	break;
      case WRD_GON:
	dev_gon(wrd_args[0]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@GON(%d)", wrd_args[0]));
	break;
      case WRD_GSCREEN:
	dev_gscreen(wrd_args[0], wrd_args[1]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@GSCREEN(%d,%d)", wrd_args[0], wrd_args[1]));
	break;
      case WRD_INKEY:
	inkey_flag = 1;
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE, "@INKEY - begin"));
	break;
      case WRD_OUTKEY:
	inkey_flag = 0;
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE, "@INKEY - end"));
	break;
      case WRD_LOCATE:
		dev_move_coursor(wrd_args[0], wrd_args[1]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@LOCATE(%d,%d)", wrd_args[0], wrd_args[1]));
	break;
      case WRD_LOOP: /* Never call */
	break;
    case WRD_MAG:
   	wrd_mag(wrd_event2string(wrd_args[0]), wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4]);
/* 	p = (char *)new_segment(&tmpbuffer, MIN_MBLOCK_SIZE); */
/* 	strcpy(p, "@MAG("); */
/* 	strcat(p, wrd_event2string(wrd_args[0])); */
/* 	strcat(p, ","); */
/* 	for(i = 1; i < 3; i++) */
/* 	{ */
/* 	    if(wrd_args[i] == WRD_NOARG) */
/* 		strcat(p, "*,"); */
/* 	    else */
/* 		sprintf(p + strlen(p), "%d,", wrd_args[i]); */
/* 	} */
/* 	sprintf(p + strlen(p), "%d,%d)", wrd_args[3], wrd_args[4]); */
/* 	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE, "%s", p)); */
/* 	reuse_mblock(&tmpbuffer); */
	break;
      case WRD_MIDI: /* Never call */
	break;
      case WRD_OFFSET: /* Never call */
	break;
      case WRD_PAL:
/*       	mac_wrd_pal( wrd_args[0], &wrd_args[1]); */
/* 	p = (char *)new_segment(&tmpbuffer, MIN_MBLOCK_SIZE); */
/* 	sprintf(p, "@PAL(%03x", wrd_args[0]); */
/* 	for(i = 1; i < 17; i++) */
/* 	    sprintf(p + strlen(p), ",%03x", wrd_args[i]); */
/* 	strcat(p, ")"); */
/* 	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE, "%s", p)); */
/* 	reuse_mblock(&tmpbuffer); */
	break;
      case WRD_PALCHG:
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@PALCHG(%s)", wrd_event2string(wrd_args[0])));
	break;
      case WRD_PALREV:
	dev_palrev(wrd_args[0]);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@PALREV(%d)", wrd_args[0]));
	break;
      case WRD_PATH:
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@PATH(%s)", wrd_event2string(wrd_args[0])));
	break;
      case WRD_PLOAD:
   	wrd_pho(wrd_event2string(wrd_args[0]));
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@PLOAD(%s)", wrd_event2string(wrd_args[0])));
	break;
      case WRD_REM:
	p = wrd_event2string(wrd_args[0]);
	len = strlen(p);
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));
	code_convert(p, text, SAFE_CONVERT_LENGTH(len), NULL, NULL);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE, "@REM %s", text));
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_REMARK:
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@REMARK(%s)", wrd_event2string(wrd_args[0])));
	break;
      case WRD_REST: /* Never call */
	break;
      case WRD_SCREEN: /* Not supported */
	break;
      case WRD_SCROLL:
	dev_text_scroll(wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
					wrd_args[4], wrd_args[5], wrd_args[6], 1);
	dev_remake_disp(portRect);
	dev_redisp(portRect);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@SCROLL(%d,%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5], wrd_args[6]));
	break;
      case WRD_STARTUP:
	dev_init(wrd_args[0]);
	dev_init_text_color();
	wrd_load_default_image();
	inkey_flag = 0;
	dev_set_height(400);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@STARTUP(%d)", wrd_args[0]));
	break;
      case WRD_STOP: /* Never call */
	break;
      case WRD_TCLS:
	dev_text_clear(wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],wrd_args[4],wrd_args[5], true);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@TCLS(%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]));
	break;
      case WRD_TON:
	wrd_ton=wrd_args[0];
	dev_remake_disp(portRect);
	dev_redisp(portRect);
	WRD_DEBUG((CMSG_INFO, VERB_VERBOSE,
		  "@TON(%d)", wrd_args[0]));
	break;
      case WRD_WAIT: /* Never call */
	break;
      case WRD_WMODE: /* Never call */
	break;

	/* Ensyutsukun */
      case WRD_eFONTM:
	print_ecmd("FONTM", wrd_args, 1);
	break;
      case WRD_eFONTP:
	print_ecmd("FONTP", wrd_args, 4);
	break;
      case WRD_eFONTR:
	print_ecmd("FONTR", wrd_args, 17);
	break;
      case WRD_eGSC:
	print_ecmd("GSC", wrd_args, 1);
	break;
      case WRD_eLINE:
	print_ecmd("LINE", wrd_args, 1);
	break;
      case WRD_ePAL:
	print_ecmd("PAL", wrd_args, 2);
	break;
      case WRD_eREGSAVE:
	print_ecmd("REGSAVE", wrd_args, 17);
	break;
      case WRD_eSCROLL:
	print_ecmd("SCROLL",wrd_args, 2);
	break;
      case WRD_eTEXTDOT:
	print_ecmd("TEXTDOT", wrd_args, 1);
	break;
      case WRD_eTMODE:
	print_ecmd("TMODE", wrd_args, 1);
	break;
      case WRD_eTSCRL:
	print_ecmd("TSCRL", wrd_args, 0);
	break;
      case WRD_eVCOPY:
	wrd_args[0] &= ~0x7;  wrd_args[4] &= ~0x7;  
	wrd_args[2] |= 0x7;
	dev_gmove(wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		wrd_args[4],wrd_args[5],
	       graphicWorld[wrd_args[6]+(wrd_args[8]? 2:0)],
	       graphicWorld[wrd_args[7]+ (wrd_args[8]? 0:2)], 0/*normal copy*/,0,gmode_mask,
	       0,0,0 );
			//ignore mode in this version, always EMS->GVRAM
	print_ecmd("VCOPY", wrd_args, 9);
	break;
      case WRD_eVSGET:
	dev_vsget(wrd_args[0]);
	print_ecmd("VSGE", wrd_args, 4);
	break;
      case WRD_eVSRES:
	print_ecmd("VSRES", wrd_args, 0);
	break;
      case WRD_eXCOPY:
	dev_gmove(wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	     		wrd_args[5], graphicWorld[wrd_args[6]], graphicWorld[wrd_args[7]],
	       		wrd_args[8]+0x10, 0/*trans*/, gmode_mask, 0,0,0 );	
	print_ecmd("XCOPY", wrd_args, 14);
	break;

	/* Extensionals */
      case WRD_START_SKIP:
	    mac_RedrawControl(0);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "WRD START SKIP");
	break;
      case WRD_END_SKIP:
	    mac_RedrawControl(1);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "WRD END SKIP");
	break;
#ifdef ENABLE_SHERRY
      case WRD_SHERRY_UPDATE:
	sry_update();
	break;
#endif
    }
    wrd_argc = 0;
}
