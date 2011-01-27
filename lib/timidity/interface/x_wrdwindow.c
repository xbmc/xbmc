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

    x_wrdwindow.c - MIMPI WRD for X Window written by Takanori Watanabe.
                  - Modified by Masanao Izumo.
*/

/*
 * PC98 Screen Emulator
 * By Takanori Watanabe.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "timidity.h"
#include "common.h"
#include "x_mag.h"
#include "x_wrdwindow.h"
#include "VTparse.h"
#include "wrd.h"
#include "controls.h"
#include "aq.h"

#ifndef XSHM_SUPPORT
#if defined(HAVE_XSHMCREATEPIXMAP) && \
    defined(HAVE_X11_EXTENSIONS_XSHM_H) && \
    defined(HAVE_SYS_IPC_H) && \
    defined(HAVE_SYS_SHM_H)
#define XSHM_SUPPORT 1
#else
#define XSHM_SUPPORT 0
#endif
#endif /* XSHM_SUPPORT */

#if XSHM_SUPPORT
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#define SIZEX WRD_GSCR_WIDTH
#define SIZEY WRD_GSCR_HEIGHT
#define MBCS 1
#define DEFAULT -1
#define JISX0201 "-*-fixed-*-r-normal--16-*-*-*-*-*-jisx0201.1976-*"
#define JISX0208 "-*-fixed-*-r-normal--16-*-*-*-*-*-jisx0208.1983-*"
#define TXTCOLORS 8
#define LINES WRD_TSCR_HEIGHT
#define COLS WRD_TSCR_WIDTH
#define TAB_SET 8
#define CSIZEX ((SIZEX)/(COLS))
#define CSIZEY ((SIZEY)/(LINES))
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
typedef struct{
  long attr;/*if attr<0 this attr is invalid*/
#define CATTR_LPART (1)
#define CATTR_16FONT (1<<1)
#define CATTR_COLORED (1<<2)
#define CATTR_BGCOLORED (1<<3) 
#define CATTR_TXTCOL_MASK_SHIFT 4
#define CATTR_TXTCOL_MASK (7<<CATTR_TXTCOL_MASK_SHIFT)
#define CATTR_INVAL (1<<31)
  char c;
}Linbuf;
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

typedef struct _ImagePixmap
{
  Pixmap pm;
  XImage *im; /* Shared pixmap image (NULL for non-support) */
#if XSHM_SUPPORT
  XShmSegmentInfo	shminfo;
#endif /* XSHM_SUPPORT */
} ImagePixmap;

static struct MyWin{
  Display *d;
  Window w;
  XFontStruct *f8;/*8bit Font*/
  XFontStruct *f16;/*16bit Font*/

  GC gc,	/* For any screens (nomask) */
     gcgr,	/* For Graphic screens (masked by @gmode) */
     gcbmp;	/* For bitmap (depth=1) */

#define NUMVSCREEN 2
  Pixmap screens[NUMVSCREEN];	/* Graphics screen buffers (PseudoColor) */
  Pixmap active_screen, disp_screen; /* screens[0] or screens[1] */
  Pixmap offscr;		/* Double buffer */
  Pixmap workbmp;		/* Tempolary usage (bitmap) */
/* active_screen: Graphics current draw screen.
 * disp_screen:   Graphics visiable screen.
 * The color class of MIMPI graphics screen is 4 bit pseudo color.
 * The plane bits is masked by (basepix | pmask[0..3])
 *
 * offscr: Background pixmap of the main window.
 * This screen is also used for double buffer.
 * Redraw(): (disp_screen + text) ----> offscr ----> Window
 * In TrueColor, it is needed to convert PseudoColor to TrueColor, here.
 */

  /* For PC98 screen emulation.
   * Many text escape sequences are supported.
   */
  Linbuf **scrnbuf;
  int curline;
  int curcol;
  long curattr;

#define NUMPLANE 4
#define NUMPXL 16
#define MAXPAL 20
  XColor txtcolor[TXTCOLORS],curcoltab[NUMPXL],
  gcolor[MAXPAL][NUMPXL],gcolorbase[NUMPXL];
  unsigned long basepix, pmask[NUMPLANE], gscreen_plane_mask;
  int ton;
  int gon;
  int gmode;
#define TON_NOSHOW 0
  Colormap cmap;
#define COLOR_BG 0
#define COLOR_DEFAULT 7
  int redrawflag;
}mywin;

static struct VGVRAM{
  Pixmap *vpix;
  int num;
}vgvram;

const static char *TXTCOLORNAME[8]={
    WRD_TEXT_COLOR0,
    WRD_TEXT_COLOR1,
    WRD_TEXT_COLOR2,
    WRD_TEXT_COLOR3,
    WRD_TEXT_COLOR4,
    WRD_TEXT_COLOR5,
    WRD_TEXT_COLOR6,
    WRD_TEXT_COLOR7
};
const static int colval12[16]={
  0x000,0xf00,0x0f0,0xff0,0x00f,0xf0f,0x0ff,0xfff,
  0x444,0xc44,0x4c4,0xcc4,0x44c,0xc4c,0x4cc,0xccc
};

static char *image_buffer; /* Used for @MAG or @PLOAD */
Visual *theVisual;
int theScreen, theDepth;
int bytes_per_image_pixel;

static int Parse(int);
static void Redraw(int,int,int,int);
static int RedrawText(Drawable,int,int,int,int); /* Redraw only text */
static void RedrawInject(int x,int y,int width,int height,int flag);

/**** ImagePixmap interfaces ****/
static ImagePixmap *create_shm_image_pixmap(int width, int height);
static void free_image_pixmap(ImagePixmap *ip);

/* for truecolor */
static Bool truecolor;
static ImagePixmap *shm_screen;	/* for TrueColor conversion */
static unsigned long truecolor_palette[NUMPXL];
static int shm_format;

/*12bit color value to color member of XColor structure */
static void col12toXColor(int val,XColor *set)
{
  set->red=((val>>8)&0xf)*0x1111;
  set->green=((val>>4)&0xf)*0x1111;
  set->blue=(0xf&val)*0x1111;
}

static int highbit(unsigned long ul)
{
    int i;  unsigned long hb;
    hb = 0x80000000UL;
    for(i = 31; ((ul & hb) == 0) && i >= 0;  i--, ul<<=1)
	;
    return i;
}

static unsigned long trueColorPixel(unsigned long r, /* 0..255 */
				    unsigned long g, /* 0..255 */
				    unsigned long b) /* 0..255 */
{
    static int rs, gs, bs;

    if(r == 0xffffffff) /* for initialize */
    {
	rs = 15 - highbit(theVisual->red_mask);
	gs = 15 - highbit(theVisual->green_mask);
	bs = 15 - highbit(theVisual->blue_mask);
	return 0;
    }

    r *= 257; /* 0..65535 */
    g *= 257; /* 0..65535 */
    b *= 257; /* 0..65535 */
    if(rs < 0)	r <<= -rs;
    else	r >>=  rs;
    if(gs < 0)	g <<= -gs;
    else	g >>=  gs;
    if(bs < 0)	b <<= -bs;
    else	b >>=  bs;
    r &= theVisual->red_mask;
    g &= theVisual->green_mask;
    b &= theVisual->blue_mask;

    return r | g | b;
}

static void store_palette(void)
{
  if(truecolor) {
    int i;
    for(i = 0; i < NUMPXL; i++)
      truecolor_palette[i] = trueColorPixel(mywin.curcoltab[i].red / 257,
					    mywin.curcoltab[i].green / 257,
					    mywin.curcoltab[i].blue / 257);
    Redraw(0, 0, SIZEX, SIZEY);
  }
  else
    XStoreColors(mywin.d, mywin.cmap, mywin.curcoltab, NUMPXL);
}

static int InitColor(Colormap cmap, Bool allocate)
{
  int i,j;
  XColor dummy;
  if(allocate) {
    for(i=0;i<TXTCOLORS;i++)
      XAllocNamedColor(mywin.d,cmap,TXTCOLORNAME[i],&mywin.txtcolor[i],&dummy);
    if (!truecolor) {
      if(!XAllocColorCells(mywin.d,cmap,True,mywin.pmask,NUMPLANE,&mywin.basepix,1))
	return 1;
    } else {
      trueColorPixel(0xffffffff, 0, 0);
      mywin.basepix = 0;
      for(i = 0; i < NUMPLANE; i++)
	mywin.pmask[i] = (1u<<i); /* 1,2,4,8 */
    }
    mywin.gscreen_plane_mask = 0;
    for(i = 0; i < NUMPLANE; i++)
      mywin.gscreen_plane_mask |= mywin.pmask[i];
    mywin.gscreen_plane_mask |= mywin.basepix;
  }

  for(i=0;i<NUMPXL;i++){
    int k;
    unsigned long pvalue=mywin.basepix;
    k=i;
    for(j=0;j<NUMPLANE;j++){
      pvalue|=(((k&1)==1)?mywin.pmask[j]:0);
      k=k>>1;
    }
    col12toXColor(colval12[i],&mywin.curcoltab[i]);
    mywin.curcoltab[i].pixel=pvalue;
    mywin.curcoltab[i].flags=DoRed|DoGreen|DoBlue;
  }
  if(!truecolor)
    XStoreColors(mywin.d, mywin.cmap, mywin.curcoltab, NUMPXL);
  else {
    int i;
    for(i = 0; i < NUMPXL; i++)
      truecolor_palette[i] = trueColorPixel(mywin.curcoltab[i].red / 257,
					    mywin.curcoltab[i].green / 257,
					    mywin.curcoltab[i].blue / 257);
  }
    
  for(i=0;i<MAXPAL;i++)
    memcpy(mywin.gcolor[i],mywin.curcoltab,sizeof(mywin.curcoltab));
  memcpy(mywin.gcolorbase,mywin.curcoltab,sizeof(mywin.curcoltab));
  return 0;
}

/*Initialize Window subsystem*/
/* return:
 * -1: error
 *  0: success
 *  1: already initialized
 */
static int InitWin(char *opt)
{
  XSizeHints *sh;
  XGCValues gcv;
  int i;
  static int init_flag = 0;

  if(init_flag)
      return init_flag;

  ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Initialize WRD window");

  /*Initialize Charactor buffer and attr */
  mywin.curline=0;
  mywin.curcol=0;
  mywin.ton=1;
  mywin.gon=1;
  mywin.gmode=-1;
  mywin.curattr=0;/*Attribute Ground state*/
  mywin.scrnbuf=(Linbuf **)calloc(LINES,sizeof(Linbuf *));
  mywin.redrawflag=1;
  if((mywin.d=XOpenDisplay(NULL))==NULL){
    ctl->cmsg(CMSG_ERROR,VERB_NORMAL,"WRD: Can't Open Display");
    init_flag = -1;
    return -1;
  }

  if(strchr(opt, 'd')) {
      /* For debug */
      fprintf(stderr,"Entering -Wx Debug mode\n");
      XSynchronize(mywin.d, True);
  }

  theScreen = DefaultScreen(mywin.d);
  theDepth = DefaultDepth(mywin.d, theScreen);
  theVisual = DefaultVisual(mywin.d, theScreen);

  /* check truecolor */
  if(theVisual->class == TrueColor || theVisual->class == StaticColor)
    truecolor=True;
  else
    truecolor=False;

  if((mywin.f8=XLoadQueryFont(mywin.d,JISX0201))==NULL){
    ctl->cmsg(CMSG_ERROR,VERB_NORMAL,"%s: Can't load font",JISX0201);
    /* Can't load font JISX0201 */
    XCloseDisplay(mywin.d);
    mywin.d=NULL;
    init_flag = -1;
    return -1;
  }
  if((mywin.f16=XLoadQueryFont(mywin.d,JISX0208))==NULL){
    ctl->cmsg(CMSG_ERROR,VERB_NORMAL,"%s: Can't load font",JISX0208);
    XCloseDisplay(mywin.d);
    mywin.d=NULL;
    init_flag = -1;
    return -1;
  }

  mywin.w=XCreateSimpleWindow(mywin.d,DefaultRootWindow(mywin.d)
			      ,0,0,SIZEX,SIZEY
			      ,10,
			      BlackPixel(mywin.d, theScreen),
			      WhitePixel(mywin.d, theScreen));
  mywin.cmap=DefaultColormap(mywin.d, theScreen);

  if(truecolor) {
#if XSHM_SUPPORT
    shm_format = XShmPixmapFormat(mywin.d);
    if(shm_format == ZPixmap)
      shm_screen = create_shm_image_pixmap(SIZEX, SIZEY);
    else
      shm_screen = NULL; /* No-support other format */
    if(!shm_screen)
      ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "X SHM Extention is off");
#else
    shm_screen = NULL;
#endif
  }

  /*This block initialize Colormap*/
  if(InitColor(mywin.cmap, True)!=0){
    mywin.cmap=XCopyColormapAndFree(mywin.d,mywin.cmap);
    if(InitColor(mywin.cmap, True)!=0){
      ctl->cmsg(CMSG_ERROR,VERB_NORMAL,"WRD: Can't initialize colormap");
      XCloseDisplay(mywin.d);
      mywin.d=NULL;
      init_flag = -1;
      return -1;
    }
    XSetWindowColormap(mywin.d,mywin.w,mywin.cmap);
  }
  /*Tell Window manager not to allow resizing
   * And set Application name
   */
  sh=XAllocSizeHints();
  sh->flags=PMinSize|PMaxSize;
  sh->min_width=SIZEX;
  sh->min_height=SIZEY;
  sh->max_width=SIZEX;
  sh->max_height=SIZEY;
  XSetWMNormalHints(mywin.d,mywin.w,sh);
  XStoreName(mywin.d,mywin.w,"timidity");
  XSetIconName(mywin.d,mywin.w,"TiMidity");
  XFree(sh);

  /* Alloc background pixmap(Graphic plane)*/
  for(i=0;i<NUMVSCREEN;i++)
    mywin.screens[i]=XCreatePixmap(mywin.d,mywin.w,SIZEX,SIZEY,theDepth);
  mywin.offscr=XCreatePixmap(mywin.d,mywin.w,SIZEX,SIZEY,theDepth);
  mywin.workbmp=XCreatePixmap(mywin.d,mywin.w,SIZEX,CSIZEY,1);
  mywin.active_screen=mywin.screens[0];
  mywin.disp_screen=mywin.screens[0];
  XSetWindowBackgroundPixmap(mywin.d, mywin.w, mywin.offscr);

  gcv.graphics_exposures = False;
  mywin.gc=XCreateGC(mywin.d,mywin.w,GCGraphicsExposures, &gcv);
  mywin.gcgr=XCreateGC(mywin.d,mywin.w,GCGraphicsExposures, &gcv);
  mywin.gcbmp=XCreateGC(mywin.d,mywin.workbmp,0,0);

  XSetForeground(mywin.d,mywin.gcbmp,0);
  XSetBackground(mywin.d,mywin.gcbmp,1);

  /*This Initialize background pixmap(Graphic plane)*/
  XSetForeground(mywin.d,mywin.gcgr,mywin.curcoltab[0].pixel);
  XFillRectangle(mywin.d,mywin.active_screen,mywin.gcgr,0,0,SIZEX,SIZEY);
  for(i=0;i<NUMVSCREEN;i++)
      XFillRectangle(mywin.d, mywin.screens[i], mywin.gcgr,
		     0, 0, SIZEX, SIZEY);
  XFillRectangle(mywin.d, mywin.offscr, mywin.gcgr, 0, 0, SIZEX, SIZEY);

  XSetForeground(mywin.d,mywin.gc,mywin.txtcolor[COLOR_DEFAULT].pixel);
  XSelectInput(mywin.d,mywin.w,ButtonPressMask);

  { /* calc bytes_per_image_pixel */
    XImage *im;
    im = XCreateImage(mywin.d, theVisual, theDepth, ZPixmap,
		      0, NULL, 1, 1, 8, 0);
    bytes_per_image_pixel = im->bits_per_pixel/8;
    XDestroyImage(im);
  }
  image_buffer=(char *)safe_malloc(SIZEX*SIZEY*bytes_per_image_pixel);
  init_flag = 1;
  return 0;
}

/***************************************************
 Redraw Routine 
  This redraw Text screen.
  Graphic Screen is treated as background bitmap
 ***************************************************/
static int DrawReverseString16(Display *disp,Drawable d,GC gc,int x,int y,
		  XChar2b *string,int length)
{
  int lbearing,ascent;
  lbearing=mywin.f16->min_bounds.lbearing;
  ascent=mywin.f16->max_bounds.ascent;
  XSetFont(disp,mywin.gcbmp,mywin.f16->fid);
  XDrawImageString16(disp,mywin.workbmp,mywin.gcbmp,-lbearing,ascent,
		     string,length);
  XSetClipMask(disp,gc,mywin.workbmp);
  XSetClipOrigin(disp,gc,x+lbearing,y-ascent);
  XFillRectangle(disp,d,gc,x+lbearing,y-ascent,CSIZEX*length*2,CSIZEY);
  XSetClipMask(disp,gc,None);
  return 0;
}

static int DrawReverseString(Display *disp,Drawable d,GC gc,int x,int y,
		  char *string,int length)
{
  int lbearing,ascent;
  lbearing=mywin.f16->min_bounds.lbearing;
  ascent=mywin.f16->max_bounds.ascent;
  XSetFont(disp,mywin.gcbmp,mywin.f8->fid);
  XDrawImageString(disp,mywin.workbmp,mywin.gcbmp,-lbearing,ascent,
		   string,length);
  XSetClipMask(disp,gc,mywin.workbmp);
  XSetClipOrigin(disp,gc,x+lbearing,y-ascent);
  XFillRectangle(disp,d,gc,x+lbearing,y-ascent,CSIZEX*length,CSIZEY);
  XSetClipMask(disp,gc,None);
  return 0;
}

static int RedrawText(Drawable drawable, int x,int y,int width,int height)
{
  int i,yfrom,yto,xfrom,xto;
  int drawflag;

  xfrom=x/CSIZEX;
  xfrom=max(xfrom,0);
  xto=(x+width-1)/CSIZEX;
  xto=(xto<COLS-1)?xto:COLS-1;
  yfrom=y/CSIZEY;
  yfrom=max(yfrom,0);
  yto=(y+height-1)/CSIZEY;
  yto=(yto<LINES-1)?yto:LINES-1;

  drawflag=0;
  for(i=yfrom;i<=yto;i++){
    if(mywin.scrnbuf[i]!=NULL){
      long prevattr,curattr;
      char line[COLS+1];
      int pos,s_x,e_x;
      int j,jfrom,jto;

      jfrom=xfrom;
      jto=xto;

      /* Check multibyte boudary */
      if(jfrom > 0 && (mywin.scrnbuf[i][jfrom-1].attr&CATTR_LPART))
	  jfrom--;
      if(jto < COLS-1 && (mywin.scrnbuf[i][jto].attr&CATTR_LPART))
	  jto++;

      pos=0;
      prevattr=CATTR_INVAL;
      s_x=e_x=jfrom*CSIZEX;
      for(j=jfrom;j<=jto+1;j++){
	if(j==jto+1 || mywin.scrnbuf[i][j].c==0) {
	  curattr=CATTR_INVAL;
	}
	else
	  curattr=mywin.scrnbuf[i][j].attr;
	if((prevattr&~CATTR_LPART)!=(curattr&~CATTR_LPART)){
	  XFontStruct *f=NULL;
	  int lbearing,ascent;
	  int (*DrawStringFunc)();
	  DrawStringFunc=XDrawString;
	  line[pos]=0;
	  if(prevattr<0){
	    DrawStringFunc=NULL;
	  }else if(prevattr&CATTR_16FONT){
	    f=mywin.f16;
	    DrawStringFunc=XDrawString16;
	    pos/=2;
	  }else
	    f=mywin.f8;
	  if(DrawStringFunc!=NULL){
	    XSetFont(mywin.d,mywin.gc,f->fid);
	    lbearing=f->min_bounds.lbearing;
	    ascent=f->max_bounds.ascent;
	    if(prevattr&CATTR_COLORED){
	      int tcol;
	      tcol=(prevattr&CATTR_TXTCOL_MASK)>>CATTR_TXTCOL_MASK_SHIFT;
	      XSetForeground(mywin.d,mywin.gc,
			     mywin.txtcolor[tcol].pixel);
  	    }else if(prevattr&CATTR_BGCOLORED){
  	      int tcol;
  	      tcol=(prevattr&CATTR_TXTCOL_MASK)>>CATTR_TXTCOL_MASK_SHIFT;
	      DrawStringFunc=(DrawStringFunc==XDrawString)?(int(*)())DrawReverseString:(int(*)())DrawReverseString16;
	      XSetForeground(mywin.d,mywin.gc,
			     mywin.txtcolor[tcol].pixel);
	    }
	    (*DrawStringFunc)(mywin.d,drawable,mywin.gc,
			      s_x-lbearing,i*CSIZEY+ascent,line,pos);
	    drawflag=1;
	    if((prevattr&CATTR_COLORED)||(prevattr&CATTR_BGCOLORED))
	      XSetForeground(mywin.d,mywin.gc,mywin.txtcolor[COLOR_DEFAULT].pixel);
	  }
	  prevattr=curattr;
	  s_x=e_x;
	  pos=0;
	}
	line[pos++]=mywin.scrnbuf[i][j].c;
	e_x+=CSIZEX;
      }
    }
  }
  return drawflag;
}

/* Copy disp_screen to offscr */
static void TransferArea(int sx, int sy, int width, int height)
{
    if(!truecolor)
	XCopyArea(mywin.d, mywin.disp_screen, mywin.offscr, mywin.gc,
		  sx, sy, width, height, sx, sy);
    else
    {
	XImage *im;
	int x, y, i, c;
	int units_per_line;
	int x0, y0;

	if(sx + width > SIZEX)
	    width = SIZEX - sx;
	if(sy + height > SIZEY)
	    height = SIZEY - sy;

#if XSHM_SUPPORT
	if(shm_screen)
	{
	    im = shm_screen->im;
	    XCopyArea(mywin.d, mywin.disp_screen, shm_screen->pm, mywin.gc,
		      sx, sy, width, height, 0, 0);
	    XSync(mywin.d, 0); /* Wait until ready */
	    x0 = 0;
	    y0 = 0;
	}
	else
#endif /* XSHM_SUPPORT */
	{
	    im = XGetImage(mywin.d, mywin.disp_screen,
			   sx, sy, width, height, AllPlanes, ZPixmap);
	    x0 = 0;
	    y0 = 0;
	}

	units_per_line = im->bytes_per_line / (im->bits_per_pixel / 8);

	/* Optimize 8, 16, 32 bit depth image */
	switch(im->bits_per_pixel)
	{
	  case 8:
	    for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
		{
		    i = (y0 + y) * units_per_line + x0 + x;
		    c = im->data[i];
		    im->data[i] = truecolor_palette[c];
		}
	    break;
	  case 16:
	    for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
		{
		    i = (y0 + y) * units_per_line + x0 + x;
		    c = ((uint16 *)im->data)[i];
		    ((uint16 *)im->data)[i] = truecolor_palette[c];
		}
	    break;
	  case 32:
	    for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
		{
		    i = (y0 + y) * units_per_line + x0 + x;
		    c = ((uint32 *)im->data)[i];
		    ((uint32 *)im->data)[i] = truecolor_palette[c];
		}
	    break;
	  default:
	    for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
		{
		    c = XGetPixel(im, x0 + x, y0 + y);
		    XPutPixel(im, x0 + x, y0 + y, truecolor_palette[c]);
		}
	    break;
	}

#if XSHM_SUPPORT
	if(shm_screen)
	    XCopyArea(mywin.d, shm_screen->pm, mywin.offscr, mywin.gc,
		      x0, y0, width, height, sx, sy);
	else
#endif
	{
	    XPutImage(mywin.d, mywin.offscr, mywin.gc, im,
		      x0, y0, sx, sy, width, height);
	    XDestroyImage(im);
	}
    }
}

static void Redraw(int x, int y, int width, int height)
{
  if(!mywin.redrawflag)
    return;

  if(mywin.gon)
    TransferArea(x, y, width, height);
  else {
    XSetForeground(mywin.d, mywin.gc,
		   BlackPixel(mywin.d,DefaultScreen(mywin.d)));
    XFillRectangle(mywin.d, mywin.offscr, mywin.gc, x, y, width, height);
  }

  if(mywin.ton)
    RedrawText(mywin.offscr, x, y, width, height);
  XClearArea(mywin.d, mywin.w, x, y, width, height, False);
}


/*******************************************************
 *  Utilities for VT Parser
 ********************************************************/

static void DelChar(int line,int col)
{
  int rx1,ry1,rx2,ry2;
  rx1=(col)*CSIZEX;
  rx2=(col+1)*CSIZEX;
  ry1=(line)*CSIZEY;
  ry2=(line+1)*CSIZEY;
  if(mywin.scrnbuf[line][col].attr&CATTR_16FONT){
    if(mywin.scrnbuf[line][col].attr&CATTR_LPART){
      mywin.scrnbuf[line][col+1].c=0;
      mywin.scrnbuf[line][col+1].attr=0;
      rx2+=CSIZEX;
    }
    else{
      mywin.scrnbuf[line][col-1].c=0;
      mywin.scrnbuf[line][col-1].attr=0;
      rx1-=CSIZEX;
    }
  }
  RedrawInject(rx1,ry1,rx2-rx1,ry2-ry1,False);
  mywin.scrnbuf[line][col].c=0;
  mywin.scrnbuf[line][col].attr=0;
}

static void ClearLine(int j)
{
  free(mywin.scrnbuf[j]);
  mywin.scrnbuf[j]=NULL;
  RedrawInject(0,j*CSIZEY,SIZEX,CSIZEY,False);
}
static void ClearLeft(void)
{
  memset(mywin.scrnbuf[mywin.curline],0,sizeof(Linbuf)*mywin.curcol);
  if(!(mywin.scrnbuf[mywin.curline][mywin.curcol+1].attr
       &(CATTR_LPART))){
    mywin.scrnbuf[mywin.curline][mywin.curcol+1].attr=0;
    mywin.scrnbuf[mywin.curline][mywin.curcol+1].c=0;
  }
  RedrawInject(0,(mywin.curline)*CSIZEY,SIZEX,CSIZEY,False);
}
static void ClearRight(void)
{
  /*Erase Right*/
  memset(mywin.scrnbuf[mywin.curline]+mywin.curcol,0,
	 sizeof(Linbuf)*(COLS-mywin.curcol));
  if((mywin.scrnbuf[mywin.curline][mywin.curcol-1].attr
       &(CATTR_LPART))){
    mywin.scrnbuf[mywin.curline][mywin.curcol-1].attr=0;
    mywin.scrnbuf[mywin.curline][mywin.curcol-1].c=0;
  }
  RedrawInject(0,(mywin.curline)*CSIZEY,SIZEX,CSIZEY,False);
}
static void RedrawInject(int x,int y,int width,int height,int flag){
  static int xfrom,yfrom,xto,yto;
  int x2,y2;
  x2=x+width;
  y2=y+height;
  if(x==-1){
    xfrom=yfrom=xto=yto=-1;
    return;
  }
  if(flag==False){
    if(xfrom==-1){
      xfrom=x;
      yfrom=y;
      xto=x2;
      yto=y2;
    }
    else{
      xfrom=(xfrom<x)?xfrom:x;
      yfrom=(yfrom<y)?yfrom:y;
      xto=(xto>x2)?xto:x2;
      yto=(yto>y2)?yto:y2;
    }
  }
  else if(xfrom!=-1)
    Redraw(xfrom,yfrom,xto-xfrom,yto-yfrom);
}
/************************************************************
 *   Graphic Command Functions
 *
 *
 *
 **************************************************************/
void x_GMode(int mode)
{
  int i;
  unsigned long  mask;

  if(mode == -1)
  {
      /* Initialize plane mask */
      mywin.gmode = -1;
      XSetPlaneMask(mywin.d,mywin.gcgr,AllPlanes);
      return;
  }

  mode&=15;
  mywin.gmode = mode;
  mode = (mode&8)|wrd_plane_remap[mode&7];
  mask = mywin.basepix;
  for(i=0;i<NUMPLANE;i++){
    mask|=(((mode&1)==1)?mywin.pmask[i]:0);
    mode=mode>>1;
  }
  XSetPlaneMask(mywin.d,mywin.gcgr,mask);
}
void x_GMove(int xorig,int yorig,int xend,int yend,int xdist,int ydist,
	     int srcp,int endp,int swflag)
{
  int w, h;

  w=xend-xorig+1;
  h=yend-yorig+1;
  if((srcp<2)&&(endp<2)){
    if(swflag==1){
      XSetFunction(mywin.d,mywin.gcgr,GXxor);
      XCopyArea(mywin.d,mywin.screens[endp],mywin.screens[srcp],mywin.gcgr
		,xdist,ydist,w,h,xorig,yorig);
      XCopyArea(mywin.d,mywin.screens[srcp],mywin.screens[endp],mywin.gcgr
		,xorig,yorig,w,h,xdist,ydist);
      XCopyArea(mywin.d,mywin.screens[endp],mywin.screens[srcp],mywin.gcgr
		,xdist,ydist,w,h,xorig,yorig);
      XSetFunction(mywin.d,mywin.gcgr,GXcopy);
      if(mywin.screens[srcp]==mywin.disp_screen)
	Redraw(xorig,yorig,w,h);
    }
    else
      XCopyArea(mywin.d,mywin.screens[srcp],mywin.screens[endp],mywin.gcgr
		,xorig,yorig,w,h,xdist,ydist);
    if(mywin.screens[endp]==mywin.disp_screen) {
      Redraw(xdist,ydist,w,h);
    }
  }
}
void x_VSget(int *params,int nparam)
{
  int numalloc,depth;
  depth=DefaultDepth(mywin.d,DefaultScreen(mywin.d));
  if(vgvram.vpix!=NULL)
    x_VRel();
  vgvram.vpix=safe_malloc(sizeof(Pixmap)*params[0]);
  for(numalloc=0;numalloc<params[0];numalloc++){
    vgvram.vpix[numalloc]=XCreatePixmap(mywin.d,mywin.w,SIZEX,SIZEY,depth);
  }
  vgvram.num=numalloc;
}
void x_VRel()
{
  int i;
  if(mywin.d == NULL)
      return;
  if(vgvram.vpix==NULL)
    return;
  for(i=0;i<vgvram.num;i++)
    XFreePixmap(mywin.d,vgvram.vpix[i]);
  free(vgvram.vpix);
  vgvram.vpix=NULL;
  vgvram.num=0;
}
 
void x_VCopy(int sx1,int sy1,int sx2,int sy2,int tx,int ty
	     ,int ss,int ts,int mode)
{
  int vpg,rpg,w,h;
  Pixmap srcpage,distpage,tmp;

  w=sx2-sx1+1;
  h=sy2-sy1+1;
  if(mode!=0){
    vpg=ss;
    rpg=ts;
  }else{
    vpg=ts;
    rpg=ss;
  }
  if(vpg<vgvram.num)
    srcpage=vgvram.vpix[vpg];
  else
    return;
  if(rpg<2)
    distpage=mywin.screens[rpg];
  else
    return;
  if(mode==0){
    tmp=srcpage;
    srcpage=distpage;
    distpage=tmp;
  }
  XCopyArea(mywin.d,srcpage,distpage,mywin.gc
	    ,sx1,sy1,w,h,tx,ty);
  if(distpage==mywin.disp_screen)
    Redraw(tx,ty,w,h);
}

void x_XCopy(int sx1,
	     int sy1,
	     int sx2,
	     int sy2,
	     int tx,
	     int ty,
	     int ss,
	     int ts,
	     int method,
	     int *opts,
	     int nopts)
{
    XImage *simg, *timg;
    int i, x, y, w, h;
    int gmode_save;

    w = sx2 - sx1 + 1;
    h = sy2 - sy1 + 1;
    gmode_save = mywin.gmode;

    if(w <= 0 || w > SIZEX ||
       h <= 0 || h > SIZEY ||
       ss < 0 || ss >= NUMVSCREEN ||
       ts < 0 || ts >= NUMVSCREEN)
	return;

    simg = timg = NULL;
    x_GMode(-1);
    switch(method)
    {
      default:
      case 0: /* copy */
	x_GMove(sx1, sy1, sx2, sy2, tx, ty, ss, ts, 0);
	break;

      case 1: /* copy except pallet No.0 */
	simg = XGetImage(mywin.d, mywin.screens[ss],
			 sx1, sy1, w, h, mywin.gscreen_plane_mask, ZPixmap);
	if(!simg) break;
	timg = XGetImage(mywin.d, mywin.screens[ts],
			 tx, ty, w, h, mywin.gscreen_plane_mask, ZPixmap);
	if(!timg) break;
	for(y = 0; y < h; y++)
	    for(x = 0; x < w; x++)
	    {
		int pixel = XGetPixel(simg, x, y);
		if(pixel != mywin.curcoltab[0].pixel)
		    XPutPixel(timg, x, y, pixel);
	    }
	XPutImage(mywin.d, mywin.screens[ts], mywin.gcgr, timg,
		  0, 0, tx, ty, w, h);
	if(mywin.screens[ts] == mywin.disp_screen)
	    Redraw(tx, ty, w, h);
	break;

      case 2: /* xor */
	XSetFunction(mywin.d,mywin.gcgr,GXxor);
	x_GMove(sx1, sy1, sx2, sy2, tx, ty, ss, ts, 0);
	XSetFunction(mywin.d,mywin.gcgr,GXcopy);
	break;

      case 3: /* and */
	XSetFunction(mywin.d,mywin.gcgr,GXand);
	x_GMove(sx1, sy1, sx2, sy2, tx, ty, ss, ts, 0);
	XSetFunction(mywin.d,mywin.gcgr,GXcopy);
	break;

      case 4: /* or */
	XSetFunction(mywin.d,mywin.gcgr,GXor);
	x_GMove(sx1, sy1, sx2, sy2, tx, ty, ss, ts, 0);
	XSetFunction(mywin.d,mywin.gcgr,GXcopy);
	break;

      case 5: /* reverse x */
	simg = XGetImage(mywin.d, mywin.screens[ss],
			 sx1, sy1, w, h, mywin.gscreen_plane_mask, ZPixmap);
	if(!simg)
	    break;
	for(y = 0; y < h; y++)
	{
	    for(x = 0; x < w/2; x++)
	    {
		int p1, p2;
		p1 = XGetPixel(simg, x, y);
		p2 = XGetPixel(simg, w-x-1, y);
		XPutPixel(simg, x, y, p2);
		XPutPixel(simg, w-x-1, y, p1);
	    }
	}
	XPutImage(mywin.d, mywin.screens[ts], mywin.gcgr, simg,
		  0, 0, tx, ty, w, h);
	if(mywin.screens[ts] == mywin.disp_screen)
	    Redraw(tx, ty, w, h);
	break;

      case 6: /* reverse y */
	simg = XGetImage(mywin.d, mywin.screens[ss],
			 sx1, sy1, w, h, mywin.gscreen_plane_mask, ZPixmap);
	if(!simg)
	    break;
	for(y = 0; y < h/2; y++)
	{
	    for(x = 0; x < w; x++)
	    {
		int p1, p2;
		p1 = XGetPixel(simg, x, y);
		p2 = XGetPixel(simg, x, h-y-1);
		XPutPixel(simg, x, y, p2);
		XPutPixel(simg, x, h-y-1, p1);
	    }
	}
	XPutImage(mywin.d, mywin.screens[ts], mywin.gcgr, simg,
		  0, 0, tx, ty, w, h);
	if(mywin.screens[ts] == mywin.disp_screen)
	    Redraw(tx, ty, w, h);
	break;

      case 7: /* reverse x-y */
	simg = XGetImage(mywin.d, mywin.screens[ss],
			 sx1, sy1, w, h, mywin.gscreen_plane_mask, ZPixmap);
	if(!simg)
	    break;
	for(i = 0; i < w*h/2; i++)
	{
	    int p1, p2;
	    p1 = simg->data[i];
	    p2 = simg->data[w*h-i-1];
	    simg->data[i] = p2;
	    simg->data[w*h-i-1] = p1;
	}
	XPutImage(mywin.d, mywin.screens[ts], mywin.gcgr, simg,
		  0, 0, tx, ty, w, h);
	if(mywin.screens[ts] == mywin.disp_screen)
	    Redraw(tx, ty, w, h);
	break;

      case 8: /* copy except pallet No.0 (type2) */
	if(nopts < 2)
	    break;
	simg = XGetImage(mywin.d, mywin.screens[ss],
			 sx1, sy1, w, h, mywin.gscreen_plane_mask, ZPixmap);
	if(!simg) break;
	timg = XGetImage(mywin.d, mywin.screens[ts],
			 opts[0], opts[1], w, h, mywin.gscreen_plane_mask, ZPixmap);
	if(!timg) break;
	for(y = 0; y < h; y++)
	    for(x = 0; x < w; x++)
	    {
		int pixel = XGetPixel(simg, x, y);
		if(pixel != mywin.curcoltab[0].pixel)
		    XPutPixel(timg, x, y, pixel);
	    }
	XPutImage(mywin.d, mywin.screens[ts], mywin.gcgr, timg,
		  0, 0, tx, ty, w, h);
	if(mywin.screens[ts] == mywin.disp_screen)
	    Redraw(tx, ty, w, h);
	break;

      case 9: { /* Mask copy */
	  int m, opt5, c;
	  if(nopts < 5)
	    break;
	  opt5 = opts[4];
	  simg = XGetImage(mywin.d, mywin.screens[ss],
			   sx1, sy1, w, h, mywin.gscreen_plane_mask, ZPixmap);
	  if(!simg) break;
	  timg = XGetImage(mywin.d, mywin.screens[ts],
			   tx, ty, w, h, mywin.gscreen_plane_mask, ZPixmap);
	  if(!timg) break;
	  for(y = 0; y < h; y++)
	  {
	      m = opts[y & 3] & 0xff;
	      for(x = 0; x < w; x++)
	      {
		  if((1 << (x&7)) & m)
		  {
		      if(opt5 == 16)
			  continue;
		      c = mywin.curcoltab[opt5 & 0xf].pixel;
		  }
		  else
		      c = XGetPixel(simg, x, y);
		  XPutPixel(timg, x, y, c);
	      }
	  }
	  XPutImage(mywin.d, mywin.screens[ts], mywin.gcgr, timg,
		    0, 0, tx, ty, w, h);
	  if(mywin.screens[ts] == mywin.disp_screen)
	      Redraw(tx, ty, w, h);
	}
	break;

      case 10: { /* line copy */
	  int cp, sk, i;
	  if(nopts < 2)
	      break;
	  if((cp = opts[0]) < 0)
	      break;
	  if((sk = opts[1]) < 0)
	      break;
	  if(cp + sk == 0)
	      break;
	  simg = XGetImage(mywin.d, mywin.screens[ss],
			   sx1, sy1, w, h, mywin.gscreen_plane_mask, ZPixmap);
	  if(!simg) break;
	  timg = XGetImage(mywin.d, mywin.screens[ts],
			   tx, ty, w, h, mywin.gscreen_plane_mask, ZPixmap);
	  if(!timg) break;
	  y = 0;
	  while(y < h)
	  {
	      for(i = 0; i < cp && y < h; i++, y++)
	      {
		  for(x = 0; x < w; x++)
		      XPutPixel(timg, x, y, XGetPixel(simg, x, y));
	      }
	      y += sk;
	  }
	}
	if(mywin.screens[ts] == mywin.disp_screen)
	    Redraw(tx, ty, w, h);
	break;

      case 11: {
	  int etx, ety;
	  while(tx < 0) tx += SIZEX;
	  tx %= SIZEX;
	  while(ty < 0) ty += SIZEY;
	  ty %= SIZEY;
	  etx = tx + w;
	  ety = ty + h;

	  XCopyArea(mywin.d,mywin.screens[ss],mywin.screens[ts],mywin.gcgr,
		    sx1, sx2, w, h, tx, ty);
	  if(etx > SIZEX)
	      XCopyArea(mywin.d,mywin.screens[ss],mywin.screens[ts],mywin.gcgr,
			sx1 + (etx - SIZEX),
			sy1,
			w - (etx - SIZEX),
			h,
			0, ty);
	  if(ety > SIZEY)
	      XCopyArea(mywin.d,mywin.screens[ss],mywin.screens[ts],mywin.gcgr,
			sx1,
			sy1 + (ety - SIZEY),
			w,
			h - (ety - SIZEY),
			tx, 0);
	  if(etx > SIZEX && ety > SIZEY)
	  {
	      XCopyArea(mywin.d,mywin.screens[ss],mywin.screens[ts],mywin.gcgr,
			sx1 + (etx - SIZEX),
			sy1 + (ety - SIZEY),
			w - (etx - SIZEX),
			h - (ety - SIZEY),
			0, 0);
	  }
	  if(mywin.screens[ts] == mywin.disp_screen)
	  {
	      if(etx < SIZEX && ety < SIZEY)
		  Redraw(tx, ty, w, h);
	      else
		  Redraw(0, 0, SIZEX, SIZEY);
	  }
	}
	break;

      case 12: {
	  unsigned long psm, ptm;
	  int plane_map[4] = {2, 0, 1, 3};
	  psm = mywin.pmask[plane_map[opts[0] & 3]] | mywin.basepix;
	  ptm = mywin.pmask[plane_map[opts[1] & 3]] | mywin.basepix;

	  simg = XGetImage(mywin.d, mywin.screens[ss],
			   sx1, sy1, w, h, mywin.gscreen_plane_mask, ZPixmap);
	  if(!simg) break;
	  timg = XGetImage(mywin.d, mywin.screens[ts],
			   tx, ty, w, h, mywin.gscreen_plane_mask, ZPixmap);
	  if(!timg) break;
	  for(y = 0; y < h; y++)
	      for(x = 0; x < w; x++)
	      {
		  int p1, p2;
		  p1 = XGetPixel(simg, x, y);
		  p2 = XGetPixel(timg, x, y);
		  if(p1 & psm)
		      p2 |= ptm;
		  else
		      p2 &= ~ptm;
		  XPutPixel(timg, x, y, p2);
	      }
	  if(mywin.screens[ts] == mywin.disp_screen)
	      Redraw(tx, ty, w, h);
	}
	break;
    }
    if(simg != NULL)
	XDestroyImage(simg);
    if(timg != NULL)
	XDestroyImage(timg);
    x_GMode(gmode_save);
}

static void MyDestroyImage(XImage *img)
{
    img->data = NULL; /* Don't free in XDestroyImage() */
    XDestroyImage(img);
}

void x_PLoad(char *filename){
  static XImage *image = NULL;
  if(image == NULL) {
    image=XCreateImage(mywin.d,
		       DefaultVisual(mywin.d,DefaultScreen(mywin.d)),
		       DefaultDepth(mywin.d,DefaultScreen(mywin.d)),
		       ZPixmap,0,None,SIZEX,SIZEY,8,0);
    image->data = image_buffer;
  }
  memset(image->data, 0, SIZEX * SIZEY);
  if(!pho_load_pixel(image,mywin.curcoltab,filename))
    return;
  XPutImage(mywin.d,mywin.active_screen,mywin.gc
	    ,image,0,0,0,0,SIZEX,SIZEY);
  if(mywin.active_screen==mywin.disp_screen)
    Redraw(0,0,SIZEX,SIZEY);
}

void x_Mag(magdata *mag,int32 x,int32 y,int32 s,int32 p)
{
  XImage *image;
  int pixsizex,pixsizey;
  if(mag==NULL){
    ctl->cmsg(CMSG_INFO,VERB_VERBOSE,"mag ERROR!\n");
    return;
  }
  x=(x==WRD_NOARG)?mag->xorig:x;
  y=(y==WRD_NOARG)?mag->yorig:y;
  p=(p==WRD_NOARG)?0:p;
  x=x+mag->xorig/8*8-mag->xorig;
  pixsizex=mag->xend-mag->xorig/8*8+1;
  pixsizey=mag->yend-mag->yorig+1;

  mag->pal[0]=17;
  x_Pal(mag->pal,16);
  if(mywin.active_screen==mywin.screens[0]){ /* Foreground screen */
    mag->pal[0]=18;
    x_Pal(mag->pal,16);
  } else {			/* Background screen */
    mag->pal[0]=19;
    x_Pal(mag->pal,16);
  }
  if((p&1)==0){
    mag->pal[0]=0;
    x_Pal(mag->pal,16);
  }
  if(p==2)
    return;
  image=XCreateImage(mywin.d,
		     DefaultVisual(mywin.d,DefaultScreen(mywin.d)),
		     DefaultDepth(mywin.d,DefaultScreen(mywin.d)),
		     ZPixmap,0,None,pixsizex,pixsizey,8,0);
  image->data=image_buffer;
  memset(image->data, 0, pixsizex*pixsizey);
  mag_load_pixel(image,mywin.curcoltab,mag);
  XPutImage(mywin.d,mywin.active_screen,mywin.gc
	    ,image,0,0,x,y,pixsizex,pixsizey);
  if(mywin.active_screen==mywin.disp_screen)
    Redraw(x,y,pixsizex,pixsizey);
  MyDestroyImage(image);
}
void x_Gcls(int mode)
{
  int gmode_save;
  gmode_save = mywin.gmode;

  if(mode==0)
    mode=15;
  x_GMode(mode);
  XSetFunction(mywin.d,mywin.gcgr,GXclear);
  XFillRectangle(mywin.d,mywin.active_screen,mywin.gcgr,0,0,SIZEX,SIZEY);
  XSetFunction(mywin.d,mywin.gcgr,GXcopy);
  x_GMode(gmode_save);
  Redraw(0,0,SIZEX,SIZEY);
}

void x_Ton(int param)
{
  mywin.ton=param;
  Redraw(0,0,SIZEX,SIZEY);
}

void x_Gon(int param)
{
  mywin.gon=param;
  Redraw(0,0,SIZEX,SIZEY);
}

void x_RedrawControl(int flag)
{
  mywin.redrawflag = flag;
  if(flag)
  {
    Redraw(0,0,SIZEX,SIZEY);
    store_palette();
  }
  XFlush(mywin.d);
}
void x_Gline(int *params,int nparam)
{
    int x, y, w, h; /* Update rectangle region */
    unsigned long color;
    Pixmap screen;

    x = min(params[0], params[2]);
    y = min(params[1], params[3]);
    w = max(params[0], params[2]) - x + 1;
    h = max(params[1], params[3]) - y + 1;

    screen = mywin.active_screen;

    switch(params[5])
    {
      default:
      case 0:
	if (truecolor)
	  color = (unsigned long) params[4];
	else
	  color = mywin.curcoltab[params[4]].pixel;
	XSetForeground(mywin.d,mywin.gcgr,color);
	XDrawLine(mywin.d,screen,mywin.gcgr,
		  params[0],params[1],params[2],params[3]);
	break;
      case 1:
	if (truecolor)
	  color = (unsigned long) params[4];
	else
	  color = mywin.curcoltab[params[4]].pixel;
	XSetForeground(mywin.d,mywin.gcgr,color);
	XDrawRectangle(mywin.d,screen,mywin.gcgr,x,y,w-1,h-1);

	break;
      case 2:
	if (truecolor)
	  color = (unsigned long) params[6];
	else
	  color = mywin.curcoltab[params[6]].pixel;
	XSetForeground(mywin.d,mywin.gcgr,color);
	XFillRectangle(mywin.d,screen,mywin.gcgr,x,y,w,h);

	break;
  }
  if(mywin.active_screen==mywin.disp_screen)
    Redraw(x,y,w,h);
}
void x_GCircle(int *params,int nparam)
{
  int pad=0;
  int (*Linefunc)();
  Linefunc=XDrawArc;
  if(nparam>=5){
    switch(params[4]){
    default:
    case 0:
    case 1:
      Linefunc=XDrawArc;
      if (truecolor)
	XSetForeground(mywin.d,mywin.gcgr,(unsigned long) params[3]);
      else
	XSetForeground(mywin.d,mywin.gcgr,mywin.curcoltab[params[3]].pixel);

      pad=-1;
      break;
    case 2:
      Linefunc=XFillArc;
      if (truecolor)
	XSetForeground(mywin.d,mywin.gcgr,(unsigned long) params[5]);
      else
	XSetForeground(mywin.d,mywin.gcgr,mywin.curcoltab[params[5]].pixel);
      break;
    }
  }
  if(nparam>=3){
    int xcorner,ycorner,width,height,angle;
    xcorner=params[0]-params[2];/*x_center-radius*/
    ycorner=params[1]-params[2];/*y_center-radius*/
    width=height=params[2]*2;/*radius*2*/
    angle=360*64;
    (*Linefunc)(mywin.d,mywin.active_screen,mywin.gcgr,xcorner,ycorner,
		width+pad,height+pad,
		0,angle);
    if(mywin.active_screen==mywin.disp_screen)
      Redraw(xcorner,ycorner,width,height);
  }
}


#define FOREGROUND_PALLET 0
void x_Pal(int *param,int nparam){
  int pallet;

  if(nparam==NUMPXL){
    pallet=param[0];
    nparam--;
    param++;
  }
  else
    pallet=FOREGROUND_PALLET;

  if(nparam==NUMPXL-1){
    int i;
    for(i=0;i<NUMPXL;i++){
      col12toXColor(param[i],&mywin.gcolor[pallet][i]);
    }
    if(pallet==FOREGROUND_PALLET){
      memcpy(mywin.curcoltab,mywin.gcolor[FOREGROUND_PALLET],sizeof(mywin.curcoltab));
      if(mywin.redrawflag)
	  store_palette();
    }
  }
}
void x_Palrev(int pallet)
{
  int i;
  if(pallet < 0 || pallet > MAXPAL)
    return;
  for(i = 0; i < NUMPXL; i++){
    mywin.gcolor[pallet][i].red ^= 0xffff;
    mywin.gcolor[pallet][i].green ^= 0xffff;
    mywin.gcolor[pallet][i].blue ^= 0xffff;
  }

  if(pallet == FOREGROUND_PALLET){
    memcpy(mywin.curcoltab,
	   mywin.gcolor[FOREGROUND_PALLET],
	   sizeof(mywin.curcoltab));
    if(mywin.redrawflag)
	  store_palette();
  }
}
void x_Gscreen(int active,int appear)
{
  if(active<NUMVSCREEN)
     mywin.active_screen=mywin.screens[active];
  if((appear<NUMVSCREEN)&&(mywin.disp_screen!=mywin.screens[appear])){
    mywin.disp_screen=mywin.screens[appear];
    Redraw(0,0,SIZEX,SIZEY);
  }
}

#define FADE_REDUCE_TIME 0.1
void x_Fade(int *params,int nparam,int step,int maxstep)
{
  static XColor *frompal=NULL,*topal=NULL;
  if(params==NULL){
    int i;
    if(frompal==NULL||topal==NULL)
      return;

    if(step==maxstep){
      memcpy(mywin.curcoltab,topal,sizeof(mywin.curcoltab));
      memcpy(mywin.gcolor[0],mywin.curcoltab,sizeof(mywin.curcoltab));
    }
    else{
      int tmp;
      if(!mywin.redrawflag)
	return;
      if(truecolor) {
      /* @FADE for TrueColor takes many CPU powers.
       * So reduce @FADE controls.
       */
	if((step & 1) == 0 || aq_filled() < AUDIO_BUFFER_SIZE)
	  return; /* Skip fade */
      }
      for(i=0;i<NUMPXL;i++){
	tmp=(topal[i].red-frompal[i].red)/maxstep;
	mywin.curcoltab[i].red=tmp*step+frompal[i].red;
	tmp=(topal[i].green-frompal[i].green)/maxstep;
	mywin.curcoltab[i].green=tmp*step+frompal[i].green;
	tmp=(topal[i].blue-frompal[i].blue)/maxstep;
	mywin.curcoltab[i].blue=tmp*step+frompal[i].blue;
      }
    }
    if(mywin.redrawflag)
	store_palette();
  }
  else{
    if(params[2] == 0 && params[1] < MAXPAL) {
      memcpy(mywin.curcoltab,mywin.gcolor[params[1]],sizeof(mywin.curcoltab));
      memcpy(mywin.gcolor[0],mywin.curcoltab,sizeof(mywin.curcoltab));
      if(mywin.redrawflag) {
	  store_palette();
      }
    }
    else if(params[0] < MAXPAL && params[1] < MAXPAL)
    {
      frompal=mywin.gcolor[params[0]];
      topal=mywin.gcolor[params[1]];
    }
    else
      frompal=topal=NULL;

    return;
  }
}

void x_Startup(int version)
{
    int i;
    Parse(-1);
    memset(mywin.scrnbuf, 0, LINES*sizeof(Linbuf *));
    mywin.curline = 0;
    mywin.curcol = 0;
    mywin.ton = 1;
    mywin.gon = 1;
    mywin.curattr = 0;
    x_VRel();
    x_GMode(-1);
    InitColor(mywin.cmap, False);
    mywin.active_screen = mywin.disp_screen = mywin.screens[0];

    XSetForeground(mywin.d, mywin.gcgr, mywin.curcoltab[0].pixel);
    XSetForeground(mywin.d, mywin.gc, mywin.txtcolor[COLOR_DEFAULT].pixel);
    for(i = 0; i < NUMVSCREEN; i++)
	XFillRectangle(mywin.d, mywin.screens[i], mywin.gcgr,
		       0, 0, SIZEX, SIZEY);
    XFillRectangle(mywin.d, mywin.offscr, mywin.gcgr, 0, 0, SIZEX, SIZEY);
    XSetWindowBackgroundPixmap(mywin.d, mywin.w, mywin.offscr);
    XFillRectangle(mywin.d, mywin.w, mywin.gcgr, 0, 0, SIZEX, SIZEY);
    if(truecolor && shm_screen)
	XFillRectangle(mywin.d, shm_screen->pm, mywin.gcgr,
		       0, 0, SIZEX, SIZEY);
}

/*Graphic Definition*/
#define GRPH_LINE_MODE 1
#define GRPH_CIRCLE_MODE 2
#define GRPH_PAL_CHANGE 3
#define GRPH_FADE 4
#define GRPH_FADE_STEP 5
static void GrphCMD(int *params,int nparam)
{
  switch(params[0]){
  case GRPH_LINE_MODE:
    x_Gline(params+1,nparam-1);
    break;
  case GRPH_CIRCLE_MODE:
    x_GCircle(params+1,nparam-1);
    break;
  case GRPH_PAL_CHANGE:
    x_Pal(params+1,nparam-1);
    break;
  case GRPH_FADE:
    x_Fade(params+1,nparam-1,-1,-1);
    break;
  case GRPH_FADE_STEP:
    x_Fade(NULL,0,params[1],params[2]);
    break;
  }
}
/*****************************************************
 * VT parser
 *
 *
 ******************************************************/
#define MAXPARAM 20
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
      params[nparam]*=10;
      params[nparam]+=c-'0';
    }
    break;
  case CASE_ESC_SEMI:
    nparam++;
    params[nparam]=DEFAULT;
    break;
  case CASE_TAB:
    mywin.curcol+=TAB_SET;
    mywin.curcol&=~(TAB_SET-1);
    break;
  case CASE_BS:
    if(mywin.curcol > 0)
      mywin.curcol--;
#if 0 /* ^H maybe work backward character in MIMPI's screen */
    DelChar(mywin.curline,mywin.curcol);
    mywin.scrnbuf[mywin.curline][mywin.curcol].c=0;
    mywin.scrnbuf[mywin.curline][mywin.curcol].attr=0;
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
    mywin.curattr=(mbcs)?(mywin.curattr|CATTR_16FONT):
      (mywin.curattr&~(CATTR_16FONT));
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
    mywin.curcol=0;
    prstbl=groundtable;
    break;
  case CASE_IND:
  case CASE_VMOT:
    mywin.curline++;
    mywin.curcol=0;
    prstbl=groundtable;
    break;
  case CASE_CUP:
    mywin.curline=(params[0]<1)?0:params[0]-1;
    if(nparam>=1)
      mywin.curcol=(params[1]<1)?0:params[1]-1;
    else
      mywin.curcol=0;
    prstbl=groundtable;
    break;
  case CASE_PRINT:
    if(mywin.curcol==COLS){
      mywin.curcol++;
      return 1;
    }
    if(mywin.curattr&CATTR_16FONT){
      if(!(mywin.curattr&CATTR_LPART)&&(mywin.curcol==COLS-1)){
	mywin.curcol+=2;
	return 1;
      }
      mywin.curattr^=CATTR_LPART;
    }
    else
      mywin.curattr&=~CATTR_LPART;
    DelChar(mywin.curline,mywin.curcol);
    if(hankaku==1)
      c|=0x80;
    mywin.scrnbuf[mywin.curline][mywin.curcol].attr=mywin.curattr;
    mywin.scrnbuf[mywin.curline][mywin.curcol].c=c;  
    mywin.curcol++;
    break;
  case CASE_CUU:
    mywin.curline-=((params[0]<1)?1:params[0]);
    prstbl=groundtable;
    break;
  case CASE_CUD:
    mywin.curline+=((params[0]<1)?1:params[0]);
    prstbl=groundtable;
    break;
  case CASE_CUF:
    mywin.curcol+=((params[0]<1)?1:params[0]);
    prstbl=groundtable;
    break;
  case CASE_CUB:
    mywin.curcol-=((params[0]<1)?1:params[0]);
    prstbl=groundtable;
    break;
  case CASE_ED:
    switch(params[0]){
    case DEFAULT:
    case 1:
      {
	int j;
	if(mywin.scrnbuf[mywin.curline]!=NULL)
	  ClearLeft();
	for(j=0;j<mywin.curline;j++)
	  ClearLine(j);	
      }
      break;
    case 0:
      {
	int j;
	if(mywin.scrnbuf[mywin.curline]!=NULL){
	  ClearRight();
	}
	for(j=mywin.curline;j<LINES;j++)
	  ClearLine(j);	
      }
      break;
    case 2:
      {
	int j;
	for(j=0;j<LINES;j++){
	  free(mywin.scrnbuf[j]);
	  mywin.scrnbuf[j]=NULL;
	}
	mywin.curline=0;
	mywin.curcol=0;
	break;
      }
    }
    RedrawInject(0,0,SIZEX,SIZEY,False);
    prstbl=groundtable;
    break;
  case CASE_DECSC:
    savcol=mywin.curcol;
    savline=mywin.curline;
    savattr=mywin.curattr;
    prstbl=groundtable;
  case CASE_DECRC:
    mywin.curcol=savcol;
    mywin.curline=savline;
    mywin.curattr=savattr;
    prstbl=groundtable;
    break;
  case CASE_SGR:
    {
      int i;
      for(i=0;i<nparam+1;i++)
	switch(params[i]){
	default:
	  mywin.curattr&=~(CATTR_COLORED|CATTR_BGCOLORED|CATTR_TXTCOL_MASK);
	  break;
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	  /* Remap 16-23 into 30-37 */
	  params[i] = wrd_color_remap[params[i] - 16] + 30;
	  /*FALLTHROUGH*/
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	  mywin.curattr&=~CATTR_TXTCOL_MASK;
	  mywin.curattr|=(params[i]-30)<<CATTR_TXTCOL_MASK_SHIFT;
	  mywin.curattr|=CATTR_COLORED;
	  break;
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	  mywin.curattr&=~CATTR_TXTCOL_MASK;
	  mywin.curattr&=~CATTR_COLORED;
	  mywin.curattr|=(params[i]-40)<<CATTR_TXTCOL_MASK_SHIFT;
	  mywin.curattr|=CATTR_BGCOLORED;
	  break;
	}
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
      ClearLine(mywin.curline);
      break;
    }
    prstbl=groundtable;
    break;
  case CASE_NEL:
    mywin.curline++;
    mywin.curcol=0;
    mywin.curline=(mywin.curline<LINES)?mywin.curline:LINES;
    break;
/*Graphic Commands*/
  case CASE_MY_GRAPHIC_CMD:
    GrphCMD(params,nparam);
    prstbl=groundtable;
    break;
/*Unimpremented Command*/
  case CASE_ICH:
  case CASE_IL:
  case CASE_DL:
  case CASE_DCH:
  case CASE_DECID:
  case CASE_DECKPAM:
  case CASE_DECKPNM:
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
  return 0;
}
void AddLine(const unsigned char *str,int len)
{
  Linbuf *ptr;
  int i,j;
  /*Initialize Redraw rectangle Manager*/
  RedrawInject(-1,-1,-1,-1,False);
  
  /*Allocate LineBuffer*/
  if(len==0)
    len=strlen(str);
  for(i=0;i<len;i++){
    if(mywin.scrnbuf[mywin.curline]==NULL){
      ptr=(Linbuf *)calloc(COLS,sizeof(Linbuf)+1);
      if(ptr==NULL)
	exit(-1);
      else
	mywin.scrnbuf[mywin.curline]=ptr;
    }
    /*
     * Proc Each Charactor
     * If >0 Returned unput current value
     */
    if(Parse(str[i])!=0){
      i--;
    }
    /*Wrapping Proc*/
    while(mywin.curcol>=COLS+1){
      mywin.curcol-=COLS;
      mywin.curline++;
    }
    while(mywin.curcol<0){
      mywin.curcol+=COLS;
      mywin.curline--;
    }
    /*Scroll Proc*/
    mywin.curline=(mywin.curline<0)?0:mywin.curline;
    while(mywin.curline>=LINES){
      mywin.curline--;
      free(mywin.scrnbuf[0]);
      mywin.scrnbuf[0]=NULL;
      for(j=1;j<LINES;j++){
	mywin.scrnbuf[j-1]=mywin.scrnbuf[j];
      }
      mywin.scrnbuf[LINES-1]=NULL;
      RedrawInject(0,0,SIZEX,SIZEY,False);
    }
  }
  RedrawInject(0,0,0,0,True);
}
void WinFlush(void){
  if(mywin.redrawflag)
    XFlush(mywin.d);
}
/*
 *This Function Dumps Charactor Screen buffer status
 *Purely Debugging Purpose this code should be desabled 
 *if you need not.
 */

#ifdef SCREENDEBUG
DebugDump(void)
{
  FILE *f;
  int i,j;
  f=fopen("screen","w+");
  for(i=0;i<LINES;i++){
    fprintf(f,"LINE %d \n",i);
    for(j=0;j<COLS;j++){
      if(mywin.scrnbuf[i]!=NULL){
	Linbuf *a=&mywin.scrnbuf[i][j];
	fprintf(f,"{%x %c}",a->attr,a->c);
      }
    }
   fprintf(f,"\n");
  }
  fclose(f);
}
#endif
void WinEvent(void)
{
  XEvent e;
  int rdx1, rdy1, rdx2, rdy2; 
  rdx1 = rdy1 = rdx2 = rdy2 = -1;
  XSync(mywin.d, False);
  while(QLength(mywin.d)>0){
    XNextEvent(mywin.d,&e);
    switch(e.type){
    case ButtonPress:
      Redraw(0,0,SIZEX,SIZEY);
      rdx1=0;
      rdy1=0;
      rdx2=SIZEX;
      rdy2=SIZEY;
      if(e.xbutton.button==3){
#ifdef SCREENDEBUG
	DebugDump();
#endif
      }
    }
  }
  
  if(rdx1 != -1){
    Redraw(rdx1, rdy1, rdx2 - rdx1, rdy2 - rdy1);
    XFlush(mywin.d);
  }
}
void EndWin(void)
{
  if(mywin.d!=NULL)
  {
    if(truecolor && shm_screen)
      free_image_pixmap(shm_screen);

    XCloseDisplay(mywin.d);
    free(image_buffer);
  }
  mywin.d=NULL;
}  

int OpenWRDWindow(char *opt)
{
    if(InitWin(opt) == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "WRD: Can't open WRD window becase of error");
	return -1;
    }
    XMapWindow(mywin.d, mywin.w);
    XSync(mywin.d, False);
    return 0;
}

void CloseWRDWindow(void)
{
    if(mywin.d != NULL)
    {
	XUnmapWindow(mywin.d, mywin.w);
	XSync(mywin.d, False);
    }
}

static void free_image_pixmap(ImagePixmap *ip)
{
    XFreePixmap(mywin.d, ip->pm);

#if XSHM_SUPPORT
    if(ip->shminfo.shmid != -1)
    {
	/* To destroy a shard memory XImage, you should call XShmDetach()
	 * first.
	 */
	XShmDetach(mywin.d, &ip->shminfo);

	/* Unmap shared memory segment */
	shmdt(ip->shminfo.shmaddr);

	/* Remove a shared memory ID from the system */
	shmctl(ip->shminfo.shmid, IPC_RMID, NULL);
    }
#endif /* XSHM_SUPPORT */
    if(ip->im != NULL)
      XDestroyImage(ip->im);
    free(ip);
}


#if XSHM_SUPPORT
static int shm_error;
static int my_err_handler(Display* dpy, XErrorEvent* e)
{
    shm_error = e->error_code;
    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
	      "Warning: X WRD Warning: Can't create SHM Pixmap. error-code=%d",
	      shm_error);
    return shm_error;
}

static ImagePixmap *create_shm_image_pixmap(int width, int height)
{
    XErrorHandler origh;
    ImagePixmap *ip;
    int shm_depth;

    shm_depth = theDepth;
    ip = (ImagePixmap *)safe_malloc(sizeof(ImagePixmap));

    shm_error = 0;
    origh = XSetErrorHandler(my_err_handler);

    /* There is no need to initialize XShmSegmentInfo structure
     * before the call to XShmCreateImage.
     */
    ip->im = XShmCreateImage(mywin.d, theVisual, theDepth,
			     ZPixmap, NULL,
			     &ip->shminfo, width, height);
    if(ip->im == NULL)
    {
	if(shm_error == 0)
	    shm_error = -1;
	goto done;
    }

    /* allocate n-depth Z image data structure */
    ip->im->data = (char *)safe_malloc(ip->im->bytes_per_line *
				       ip->im->height);

    /* The next step is to create the shared memory segment.
     * The return value of shmat() should be stored both
     * the XImage structure and the shminfo structure.
     */
    ip->shminfo.shmid = shmget(IPC_PRIVATE,
			       ip->im->bytes_per_line * ip->im->height,
			       IPC_CREAT | 0777);

    if(ip->shminfo.shmid == -1)
    {
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "X Sherry Warning: Can't create SHM Pixmap.\n"
		  "shmget: %s", strerror(errno));
	XDestroyImage(ip->im);
	ip->im = NULL;
	shm_error = -1;
	goto done;
    }
    ip->shminfo.shmaddr = ip->im->data =
	(char *)shmat(ip->shminfo.shmid, NULL, 0);
    if(ip->shminfo.shmaddr == (void *)-1)
    {
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "X Sherry Warning: Can't create SHM Pixmap.\n"
		  "shmget: %s", strerror(errno));
	shmctl(ip->shminfo.shmid, IPC_RMID, NULL);
	XDestroyImage(ip->im);
	ip->im = NULL;
	shm_error = -1;
	goto done;
    }

    /* If readOnly is True, XShmGetImage calls will fail. */
    ip->shminfo.readOnly = False;


    /* Tell the server to attach to your shared memory segment. */
    if(XShmAttach(mywin.d, &ip->shminfo) == 0)
    {
	if(shm_error == 0)
	{
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "X Sherry Warning: Can't create SHM Pixmap.\n"
		      "Can't attach to the shared memory segment.");
	    shm_error = -1;
	}
	shmdt(ip->shminfo.shmaddr);
	shmctl(ip->shminfo.shmid, IPC_RMID, NULL);
	XDestroyImage(ip->im);
	ip->im = NULL;
	goto done;
    }

    XSync(mywin.d, False);		/* Wait until ready. */

    ip->pm = XShmCreatePixmap(mywin.d, mywin.w, ip->im->data,
			      &ip->shminfo, width, height, shm_depth);
    if(ip->pm == None)
    {
	if(shm_error == 0)
	{
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "X Sherry Warning: Can't create SHM Pixmap.\n"
		      "XShmCreatePixmap() is failed");
	    shm_error = -1;
	}
	shmdt(ip->shminfo.shmaddr);
	shmctl(ip->shminfo.shmid, IPC_RMID, NULL);
	XDestroyImage(ip->im);
	ip->im = NULL;
	goto done;
    }

  done:
    XSetErrorHandler(origh);

    if(ip->im == NULL)
    {
	free(ip);
	return NULL;
    }
    return ip;
}
#endif
