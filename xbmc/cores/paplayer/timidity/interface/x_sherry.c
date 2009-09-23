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

    x_sherry.c - Sherry WRD for X Window written by Masanao Izumo
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef ENABLE_SHERRY
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdlib.h>
#include <png.h>

#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "wrd.h"
#include "aq.h"
#include "x_sherry.h"

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

#define MAX_PLANES		8
#define MAX_COLORS		(1<<MAX_PLANES)
#define MAX_VIRTUAL_SCREENS	65536
#define MAX_PALETTES		256
#define MAX_VIRTUAL_PALETTES	65536
#define REAL_SCREEN_SIZE_X	640
#define REAL_SCREEN_SIZE_Y	480
#define CHAR_WIDTH1		8
#define CHAR_WIDTH2		16
#define CHAR_HEIGHT		16
#define SHERRY_MAX_VALUE	65536
#define JISX0201 "-*-fixed-*-r-normal--16-*-*-*-*-*-jisx0201.1976-*"
#define JISX0208 "-*-fixed-*-r-normal--16-*-*-*-*-*-jisx0208.1983-*"

/* (mask & src) | (~mask & dst) equals as Windows API BitBlt ROP 0x00CA0749 */
/* #define ROP3_CA0749(ptn, src, dst) (((ptn) & (src)) | (~(ptn) & (dst))) */
#define ROP3_CA0749(ptn, src, dst) ((dst) ^ ((ptn) & ((src) ^ (dst))))

typedef struct _ImagePixmap
{
    Pixmap		pm;
    XImage		*im;
    GC			gc;
    int			depth;
#if XSHM_SUPPORT
    XShmSegmentInfo	shminfo;
#endif /* XSHM_SUPPORT */
} ImagePixmap;

typedef struct _VirtualScreen
{
    uint16 width, height;
    uint8 transParent;
    uint8 data[1]; /* Pseudo 256 color, size = width * height */
} VirtualScreen;

typedef struct _SherryPaletteEntry
{
    uint8 r;
    uint8 g;
    uint8 b;
} SherryPaletteEntry;

typedef struct _SherryPalette
{
    SherryPaletteEntry entry[MAX_PALETTES];
} SherryPalette;


static Display	*theDisplay;
static int	theScreen;

/* real screen */
static Window	theWindow;
static ImagePixmap *theRealScreen;
static Visual	*theVisual;	/* Visual of main window */
static Colormap	theColormap;	/* Colormap of main window */
static int	theDepth;	/* Depth of main window */
static int	theClass;	/* Visual class */
static XFontStruct *theFont8;	/* 8bit Font */
static XFontStruct *theFont16;	/* 16bit Font */
static int lbearing8, lbearing16, ascent8, ascent16;
static GC theGC;

/* For memory draw string */
#define IMAGEBITMAPLEN 40
static ImagePixmap *imageBitmap;
static int bitmap_drawimage(ImagePixmap *ip, char *sjis_str, int nbytes);


static VirtualScreen **virtualScreen; /* MAX_VIRTUAL_SCREENS */
static VirtualScreen  *tmpScreen;
static VirtualScreen  *alloc_vscreen(int width, int height, int transParent);
static void	       free_vscreen(VirtualScreen *scr);

#define VSCREEN_PIXEL(scr, x, y) ((scr)->data[(scr)->width * (y) + (x)])

static unsigned long basePixel;	/* base pixel */
static unsigned long planePixel[MAX_PLANES]; /* plane pixel */
static unsigned long palette2Pixel[MAX_COLORS];
static int currentPalette;

static SherryPalette **virtualPalette; /* MAX_VIRTUAL_PALETTES, 仮想パレット */
static SherryPaletteEntry realPalette[MAX_PALETTES];
static uint8 *pseudoImage = NULL; /* For TrueColor */

static int draw_ctl_flag = True;
static int err_to_stop = 0;


/* Image Pixmap */
static ImagePixmap *create_image_pixmap(int width, int height, int depth);
static void clear_image_pixmap(ImagePixmap *ip, unsigned long pixel);
static void free_image_pixmap(ImagePixmap *ip);

#if XSHM_SUPPORT
static ImagePixmap *create_shm_image_pixmap(int width, int height, int depth);
#endif /* XSHM_SUPPORT */

static int isRealPaletteChanged, isRealScreenChanged;
static int updateClipX1, updateClipY1, updateClipX2, updateClipY2;

static int check_range(VirtualScreen *scr, int x1, int y1, int x2, int y2);


static Window try_create_window(Display *disp, int width, int height,
				int want_class, int want_depth,
				Visual **newVisual,
				Colormap *newColormap)
{
    Visual *defVisual;
    int defScreen;
    int defDepth;

    XSetWindowAttributes xswa;
    unsigned long xswamask;
    int numvis;

    defScreen = DefaultScreen(disp);
    defVisual = DefaultVisual(disp, defScreen);
    defDepth  = DefaultDepth(disp, defScreen);

    if(want_class == -1)
	want_class = defVisual->class;
    if(want_depth == -1)
	want_depth = defDepth;

    if(defVisual->class == want_class && defDepth == want_depth)
	*newVisual = defVisual;
    else
    {
	XVisualInfo *vinfo, rvinfo;

	rvinfo.class  = want_class;
	rvinfo.screen = defScreen;
	rvinfo.depth = want_depth;
	vinfo = XGetVisualInfo(disp, 
			       VisualClassMask |
			       VisualScreenMask |
			       VisualDepthMask,
			       &rvinfo, &numvis);
	if(vinfo == NULL)
	    return None;
	if(numvis == 0)
	{
	    XFree((char *)vinfo);
	    return None;
	}

	*newVisual = vinfo[0].visual;
    }
    *newColormap = XCreateColormap(disp,
				   RootWindow(disp, defScreen),
				   *newVisual, AllocNone);

    xswa.background_pixel = 0;
    xswa.border_pixel     = 0;
    xswa.colormap         = *newColormap;
    xswamask = CWBackPixel | CWBorderPixel | CWColormap;
    return XCreateWindow(disp, RootWindow(disp, defScreen),
			 0, 0, width, height, 0, want_depth,
			 InputOutput, *newVisual, xswamask, &xswa);
}


/* Create real-screen for Sherry WRD Window
 * sutable theVisual, theColormap, theDepth, theClass is selected.
 */
static Window create_main_window(int want_class, int want_depth)
{
    Window win;
    int i;
    struct {
	int want_class, want_depth;
    } target_visuals[] = {
    {TrueColor, 24},
    {TrueColor, 32},
    {TrueColor, 16},
    {PseudoColor, 8},
    {-1, -1}};

    if(want_class != -1)
    {
	win = try_create_window(theDisplay,
				REAL_SCREEN_SIZE_X, REAL_SCREEN_SIZE_Y,
				want_class, want_depth,
				&theVisual, &theColormap);
	theClass = want_class;
	theDepth = want_depth;
	return win;
    }

    for(i = 0; target_visuals[i].want_class != -1; i++)
	if((win = try_create_window(theDisplay,
				    REAL_SCREEN_SIZE_X, REAL_SCREEN_SIZE_Y,
				    target_visuals[i].want_class,
				    target_visuals[i].want_depth,
				    &theVisual, &theColormap)) != None)
	{
	    theClass = target_visuals[i].want_class;
	    theDepth = target_visuals[i].want_depth;
	    return win;
	}

    /* Use default visual */
    if((win = try_create_window(theDisplay,
				REAL_SCREEN_SIZE_X, REAL_SCREEN_SIZE_Y,
				-1, -1, &theVisual, &theColormap)) != None)
    {
	theClass = theVisual->class;
	theDepth = DefaultDepth(theDisplay, theScreen);
	return win;
    }
    return None;
}

static GC createGC(Display *disp, Drawable draw)
{
    XGCValues gv;
    gv.graphics_exposures = False;
    return XCreateGC(disp, draw, GCGraphicsExposures, &gv);
}

static int highbit(unsigned long ul)
{
    int i;  unsigned long hb;
    hb = 0x80000000UL;
    for(i = 31; ((ul & hb) == 0) && i >= 0;  i--, ul<<=1)
	;
    return i;
}

static inline unsigned long trueColorPixel(unsigned long r, /* 0..255 */
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

static Bool init_palette(void)
{
    static int first_flag = 1;
    int i, j;
    XColor xcolors[MAX_COLORS];

    if(first_flag)
    {
	first_flag = 0;

	if(theClass == TrueColor)
	{
	    basePixel = 0;
	    trueColorPixel(0xffffffff, 0, 0);
	    return True;
	}

	if(!XAllocColorCells(theDisplay, theColormap, False,
			     planePixel, MAX_PLANES,
			     &basePixel, 1))
	    return False;
    }

    if(theClass == TrueColor)
	return True;

    for(i = 0; i < MAX_COLORS; i++)
    {
	palette2Pixel[i] = basePixel;
	for(j = 0; j < MAX_PLANES; j++)
	    if((i & (1 << j)) != 0) palette2Pixel[i] |= planePixel[j];
    }

    for(i = 0; i < MAX_COLORS; i++)
    {
	xcolors[i].pixel = palette2Pixel[i];
	xcolors[i].red   = 0;
	xcolors[i].green = 0;
	xcolors[i].blue  = 0;
	xcolors[i].flags = DoRed | DoGreen | DoBlue;
    }
    XStoreColors(theDisplay, theColormap, xcolors, MAX_COLORS);
    return True;
}

static Bool init_font(void)
{
    if((theFont8 = XLoadQueryFont(theDisplay, JISX0201)) == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: Can't load font", JISX0201);
	return False;
    }
    if((theFont16 = XLoadQueryFont(theDisplay, JISX0208)) == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: Can't load font", JISX0208);
	XFreeFont(theDisplay, theFont8);
	return False;
    }

    lbearing8 = theFont8->min_bounds.lbearing;
    ascent8   = theFont8->max_bounds.ascent;
    lbearing16 = theFont16->min_bounds.lbearing;
    ascent16   = theFont16->max_bounds.ascent;

    return True;
}

static void free_font(void)
{
    XFreeFont(theDisplay, theFont8);
    XFreeFont(theDisplay, theFont16);
}

/* open */
/* return:
 * -1: error
 *  0: success
 *  1: already initialized
 */
static int x_sry_open(char *opts)
{
    static Bool error_flag = False;
    int i;
    Bool try_pseudo;

    if(error_flag)
    {
	if(error_flag == -1)
	    ctl->cmsg(CMSG_ERROR, VERB_NOISY,
		      "Sherry WRD: Can't work because of error");
	else if(error_flag == 1) /* Already initialized */
	{
	    XMapWindow(theDisplay, theWindow);
	    clear_image_pixmap(theRealScreen, basePixel);
	    XFlush(theDisplay);
	    return 0;
	}
	return error_flag;
    }

    try_pseudo = 0;
    if(opts)
    {
	if(strchr(opts, 'p'))
	    try_pseudo = 1;
    }

    if((theDisplay = XOpenDisplay(NULL)) == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: Can't open display: %s", XDisplayName(NULL));
	error_flag = -1;
	return -1;
    }

    if(!init_font())
    {
	XCloseDisplay(theDisplay);
	theDisplay = NULL;
	error_flag = -1;
	return -1;
    }

    theScreen = DefaultScreen(theDisplay);
    if(try_pseudo)
	theWindow = create_main_window(PseudoColor, 8);
    else
	theWindow = create_main_window(-1, -1);
    if(theWindow == None)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: Can't create Sherry WRD Window");
	free_font();
	XCloseDisplay(theDisplay);
	theDisplay = NULL;
	error_flag = -1;
	return -1;
    }
    XSelectInput(theDisplay, theWindow, ExposureMask);

#if XSHM_SUPPORT
    theRealScreen = create_shm_image_pixmap(REAL_SCREEN_SIZE_X,
					    REAL_SCREEN_SIZE_Y,
					    theDepth);
    imageBitmap = create_shm_image_pixmap(IMAGEBITMAPLEN * CHAR_WIDTH1,
					  CHAR_HEIGHT,
					  theDepth);
#else
    theRealScreen = create_image_pixmap(REAL_SCREEN_SIZE_X,
					REAL_SCREEN_SIZE_Y,
					theDepth);
    imageBitmap = create_image_pixmap(IMAGEBITMAPLEN * CHAR_WIDTH1,
				      CHAR_HEIGHT,
				      1);
#endif

    init_palette();
    XSetWindowBackground(theDisplay, theWindow, basePixel);
    XSetForeground(theDisplay, imageBitmap->gc, 1);
    XSetBackground(theDisplay, imageBitmap->gc, 0);

    theGC = createGC(theDisplay, theWindow);

    virtualScreen = (VirtualScreen **)safe_malloc(MAX_VIRTUAL_SCREENS *
						  sizeof(VirtualScreen *));
    for(i = 0; i < MAX_VIRTUAL_SCREENS; i++)
	virtualScreen[i] = NULL;
    tmpScreen = NULL;

    virtualPalette = (SherryPalette **)safe_malloc(MAX_VIRTUAL_PALETTES *
						   sizeof(SherryPalette *));
    for(i = 0; i < MAX_VIRTUAL_PALETTES; i++)
	virtualPalette[i] = NULL;
    memset(realPalette, 0, sizeof(realPalette));

    if(theClass == TrueColor)
    {
	pseudoImage = (uint8 *)safe_malloc(REAL_SCREEN_SIZE_X *
					   REAL_SCREEN_SIZE_Y);
	memset(pseudoImage, 0, REAL_SCREEN_SIZE_X * REAL_SCREEN_SIZE_Y);
    }

    XMapWindow(theDisplay, theWindow);
    XFlush(theDisplay);
    error_flag = 1;

    return 0;
}

void CloseSryWindow(void)
{
    XUnmapWindow(theDisplay, theWindow);
    XFlush(theDisplay);
}

int OpenSryWindow(char *opts)
{
    return x_sry_open(opts);
}

void x_sry_close(void)
{
    int i;

    if(theDisplay == NULL)
	return;
    free_image_pixmap(theRealScreen);
    free_image_pixmap(imageBitmap);
    for(i = 0; i < MAX_VIRTUAL_SCREENS; i++)
	if(virtualScreen[i] != NULL)
	{
	    free_vscreen(virtualScreen[i]);
	    virtualScreen[i] = NULL;
	}
    free(virtualScreen);
    if(tmpScreen != NULL)
    {
	free(tmpScreen);
	tmpScreen = NULL;
    }
    for(i = 0; i < MAX_VIRTUAL_PALETTES; i++)
	if(virtualPalette[i] != NULL)
	{
	    free(virtualPalette[i]);
	    virtualPalette[i] = NULL;
	}
    free(virtualPalette);
    if(pseudoImage)
	free(pseudoImage);
    XFreeFont(theDisplay, theFont8);
    XFreeFont(theDisplay, theFont16);
    XCloseDisplay(theDisplay);
    theDisplay = NULL;
}

static void sry_new_vpal(uint8 *data, int len)
{
    int i, n;

    for(i = 0; i < len-1; i += 2)
    {
	n = SRY_GET_SHORT(data + i) & 0xffff;
#ifdef SRY_DEBUG
	printf("NEW palette %d\n", n);
#endif /* SRY_DEBUG */
	if(virtualPalette[n] == NULL)
	    virtualPalette[n] =
		(SherryPalette *)safe_malloc(sizeof(SherryPalette));
	memset(virtualPalette[n], 0, sizeof(SherryPalette));
    }
}

static void sry_free_vpal(uint8 *data, int len)
{
    int i, n;

    for(i = 0; i < len-1; i += 2)
    {
	n = SRY_GET_SHORT(data + i) & 0xffff;
	if(virtualPalette[n] != NULL)
	{
	    free(virtualPalette[n]);
	    virtualPalette[n] = NULL;
	}
    }
}

static void sry_new_vram(uint8 *data, int len)
{
    int i, n, width, height, c;

    for(i = 0; i < len-6; i += 7)
    {
	n      = SRY_GET_SHORT(data + i) & 0xffff;
	width  = SRY_GET_SHORT(data + i + 2) & 0xffff;
	height = SRY_GET_SHORT(data + i + 4) & 0xffff;
	c      = data[6];
#ifdef SRY_DEBUG
	printf("sherry: new vram[%d] %dx%d (t=%d)\n",
	       n, width, height, c);
#endif /* SRY_DEBUG */

	if(virtualScreen[n] != NULL)
	    free_vscreen(virtualScreen[n]);
	virtualScreen[n] = alloc_vscreen(width, height, c);
    }
}

static void sry_free_vram(uint8 *data, int len)
{
    int i, n;
    for(i = 0; i < len-1; i += 2)
    {
	n = SRY_GET_SHORT(data + i) & 0xffff;
	if(virtualScreen[n] != NULL)
	{
	    free_vscreen(virtualScreen[n]);
	    virtualScreen[n] = NULL;
	}
    }
}

static void sry_pal_set(uint8 *data, int len)
{
    int pg, i, p;
    SherryPaletteEntry *entry;

    pg = SRY_GET_SHORT(data) & 0xffff;
    if(virtualPalette[pg] == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "WRD Sherry: virtual palette %d is not allocated", pg);
	err_to_stop = 1;
	return;
    }
    entry = virtualPalette[pg]->entry;
    data += 2;
    len -= 2;

#ifdef SRY_DEBUG
    printf("set palette color of %d (n=%d)\n", pg, len / 4);
#endif /* SRY_DEBUG */

    for(i = 0; i < len - 3; i += 4)
    {
	p = data[i];
	entry[p].r = data[i + 1];
	entry[p].g = data[i + 2];
	entry[p].b = data[i + 3];
    }
}

static void update_real_screen(int x, int y, int width, int height)
{
  if(!draw_ctl_flag)
    return;
#if XSHM_SUPPORT
    if(theRealScreen->shminfo.shmid != -1)
    {
	XShmPutImage(theDisplay, theWindow, theGC, theRealScreen->im,
		     x, y, x, y, width, height, False);
	return;
    }
#endif

    XPutImage(theDisplay, theWindow, theGC, theRealScreen->im,
	      x, y, x, y, width, height);
}

static void updateScreen(int sx, int sy, int width, int height)
{
    if(theClass == TrueColor)
    {
	XImage *im;
	int x, y, c;
	int units_per_line;
	unsigned long pixels[MAX_COLORS];

	for(c = 0; c < MAX_COLORS; c++)
	    pixels[c] = trueColorPixel(realPalette[c].r,
				       realPalette[c].g,
				       realPalette[c].b);

	im = theRealScreen->im;
	units_per_line = im->bytes_per_line / (im->bits_per_pixel / 8);

	switch(im->bits_per_pixel)
	{
#if 1
	  case 8:
	    for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
		{
		    c = pseudoImage[(sy + y) * REAL_SCREEN_SIZE_X + sx + x];
		    im->data[(sy + y) * units_per_line + sx + x] = pixels[c];
		}
	    break;
	  case 16:
	    for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
		{
		    c = pseudoImage[(sy + y) * REAL_SCREEN_SIZE_X + sx + x];
		    ((uint16 *)im->data)[(sy + y) * units_per_line + sx + x]
			= pixels[c];
		}
	    break;
	  case 32:
	    for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
		{
		    c = pseudoImage[(sy + y) * REAL_SCREEN_SIZE_X + sx + x];
		    ((uint32 *)im->data)[(sy + y) * units_per_line + sx + x]
			= pixels[c];
		}
	    break;
#endif
	  default: /* Generic routine */
	    for(y = 0; y < height; y++)
		for(x = 0; x < width; x++)
		{
		    c = pseudoImage[(sy + y) * REAL_SCREEN_SIZE_X + sx + x];
		    XPutPixel(theRealScreen->im, sx + x, sy + y, pixels[c]);
		}
	    break;
	}
    }

    update_real_screen(sx, sy, width, height);
    isRealScreenChanged = 0;
}

static void updatePalette(void)
{
    int i;
    XColor xc[256];

    if(theClass != TrueColor)
    {
	for(i = 0; i < MAX_COLORS; i++)
	{
	    xc[i].pixel = palette2Pixel[i];
	    xc[i].red   = realPalette[i].r * 257;
	    xc[i].green = realPalette[i].g * 257;
	    xc[i].blue  = realPalette[i].b * 257;
	    xc[i].flags = DoRed | DoGreen | DoBlue;
	}
	XStoreColors(theDisplay, theColormap, xc, MAX_COLORS);
    }
    else /* True color */
    {
	updateScreen(0, 0, REAL_SCREEN_SIZE_X, REAL_SCREEN_SIZE_Y);
	isRealScreenChanged = 0;
    }
    isRealPaletteChanged = 0;
}


static void sry_pal_v2r(uint8 *data)
{
    int n;
    n = SRY_GET_SHORT(data) & 0xffff;

#ifdef SRY_DEBUG
    printf("Transfer palette %d\n", n);
#endif /* SRY_DEBUG */

    if(virtualPalette[n] == NULL)
    {
	virtualPalette[n] =
	    (SherryPalette *)safe_malloc(sizeof(SherryPalette));
	memset(virtualPalette[n], 0, sizeof(SherryPalette));
    }

    memcpy(realPalette, virtualPalette[n]->entry, sizeof(realPalette));
    isRealPaletteChanged = 1;
    currentPalette = n;
}

static void png_read_func(png_structp png_ptr, char *buff, size_t n)
{
    struct timidity_file *tf;
    tf = (struct timidity_file *)png_ptr->io_ptr;
    tf_read(buff, 1, n, tf);
}

#define PNG_BYTES_TO_CHECK	8
#define MAX_SCREEN_COLORS	256
static void sry_load_png(uint8 *data)
{
    int i;
    int x, y;
    int screen, vpalette;
    png_structp pngPtr;
    png_infop infoPtr;
    png_infop endInfo;
    VirtualScreen *scr;
    SherryPaletteEntry *entry;

    char *filename;
    struct timidity_file *tf;
    char sig[PNG_BYTES_TO_CHECK];

    png_uint_32 width, height;
    int bitDepth, colorType, interlaceType, compressionType, filterType;

    int numPalette;
    png_colorp palette;
    png_uint_16p hist;
    png_bytep *rowPointers;
    png_uint_32 rowbytes;
    png_bytep trans;
    int transParent;
    int numTrans;
    png_color_16p transValues;

    numPalette = -1;
    memset(&palette, 0, sizeof(palette));
    hist = NULL;
    rowbytes = 0;
    rowPointers = NULL;
    trans = NULL;
    numTrans = 0;
    transValues = NULL;

    screen = SRY_GET_SHORT(data) & 0xffff;
    vpalette = SRY_GET_SHORT(data + 2) & 0xffff;
    filename = data + 4;

#ifdef SRY_DEBUG
    printf("Load png: %s: scr=%d pal=%d\n", filename, screen, vpalette);
#endif /* SRY_DEBUG */

    if((tf = wrd_open_file(filename)) == NULL)
    {
	err_to_stop = 1;
	return;
    }
    if(tf_read(sig, 1, sizeof(sig), tf) != sizeof(sig))
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "WRD Sherry: %s is too short to be png format.", filename);
	err_to_stop = 1;
	return;
    }
    if(png_sig_cmp(sig, 0, sizeof(sig)))
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: Not a png file", filename);
	err_to_stop = 1;
	return;
    }


    pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
				    NULL, NULL, NULL);
    infoPtr = png_create_info_struct(pngPtr);
    endInfo = png_create_info_struct(pngPtr);

    png_set_read_fn(pngPtr, (void *)tf, (png_rw_ptr)png_read_func);
    png_set_sig_bytes(pngPtr, sizeof(sig));
    png_read_info(pngPtr, infoPtr);

    /* get info */
    png_get_IHDR(pngPtr, infoPtr, &width, &height,
		 &bitDepth, &colorType,
		 &interlaceType, &compressionType, &filterType);
    
    /* transformation */
    /* contert to 256 palette */
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
	png_set_expand(pngPtr);
    if(bitDepth == 16)
	png_set_strip_16(pngPtr);
    if(bitDepth < 8)
	png_set_packing(pngPtr);
    if(colorType == PNG_COLOR_TYPE_GRAY ||
       colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
	png_set_gray_to_rgb(pngPtr);

#if 0
  double screenGamma;
  const char* const gammaStr = getenv ("SCREEN_GAMMA");
  if (gammaStr != NULL)
    {
      screenGamma = atof (gammaStr);
    }
  else
    {
      screenGamma = 2.2;
/*
      screenGamms = 2.0;
      screenGamms = 1.7;
      screenGamms = 1.0;
*/
    }

  if (png_get_gAMA (pngPtr, infoPtr, &gamma))
    png_set_gamma (pngPtr, screenGamma, gamma);
  else
    png_set_gamma (pngPtr, screenGamma, 0.50);
#endif

    if(png_get_valid(pngPtr, infoPtr, PNG_INFO_PLTE))
    {
	png_get_PLTE(pngPtr, infoPtr, &palette, &numPalette);
	if(numPalette > MAX_SCREEN_COLORS)
	{
	    if(png_get_valid(pngPtr, infoPtr, PNG_INFO_hIST))
		png_get_hIST(pngPtr, infoPtr, &hist);
	    png_set_dither(pngPtr, palette,
			   numPalette, MAX_SCREEN_COLORS, hist, 1);
	}
    }
    else
    {
	/* XXX */
	/* NOTE 6*7*6 = 252 */
	/* 6*7*6 = 5*7*6 + 6*6 + 6 */
	png_color stdColorCube[6*7*6];
	png_byte r, g, b;

	for(r = 0; r < 6; r++)
	{
	    for(g = 0; g < 7; g++)
	    {
		for(b = 0; b < 6; b++)
		{
		    png_byte index = r*7*6+g*6+b;
		    stdColorCube[index].red   = r;
		    stdColorCube[index].green = g;
		    stdColorCube[index].blue  = b;
		}
	    }
	}
	png_set_dither(pngPtr, stdColorCube,
		       6*7*6, MAX_SCREEN_COLORS,
		       NULL, 1);
	/*???*/
	png_set_PLTE(pngPtr, infoPtr, pngPtr->palette, pngPtr->num_palette);
	palette = pngPtr->palette;
	numPalette = pngPtr->num_palette;
    }

    if(png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
	png_get_tRNS(pngPtr, infoPtr, &trans, &numTrans, &transValues);

    png_read_update_info(pngPtr, infoPtr);

    rowbytes = png_get_rowbytes(pngPtr, infoPtr);
    /* rowbytes == width */
    rowPointers = (png_bytep *)safe_malloc(height * sizeof(png_bytep));
    rowPointers[0] = (png_byte *)safe_malloc(rowbytes * height *
					     sizeof(png_byte));
    for(i = 1; i < height; i++)
	rowPointers[i] = rowPointers[0] + i * rowbytes;

    png_read_image(pngPtr, rowPointers);
    png_read_end(pngPtr, endInfo);

    if(trans == NULL || numTrans == 0)
	transParent = 0;
    else
	transParent = trans[0] & 0xff;

    scr = virtualScreen[screen];
    if(scr != NULL && (scr->width != width || scr->height != height))
    {
	free_vscreen(scr);
	scr = NULL;
    }
    if(scr == NULL)
	scr = virtualScreen[screen] =
	    alloc_vscreen(width, height*2, transParent);
    else
	scr->transParent = transParent;

    if(virtualPalette[vpalette] == NULL)
    {
	virtualPalette[vpalette] =
	    (SherryPalette *)safe_malloc(sizeof(SherryPalette));
	memset(virtualPalette[vpalette], 0, sizeof(SherryPalette));
    }

    entry = virtualPalette[vpalette]->entry;
    for(i = 0; i < numPalette; i++)
    {
	entry[i].r = palette[i].red;
	entry[i].g = palette[i].green;
	entry[i].b = palette[i].blue;
    }

    for(y = 0; y < height; y++)
	for(x = 0; x < width; x++)
	    VSCREEN_PIXEL(scr, x, y) = rowPointers[y][x];

    free(rowPointers[0]);
    free(rowPointers);
    png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
    close_file(tf);
#ifdef SRY_DEBUG
    printf("Load png ok: width=%d height=%d\n", width, height);
#endif /* SRY_DEBUG */
}

static void sry_trans_all(uint8 *data)
{
    int screen, x, y;
    int sx, sy;
    VirtualScreen *scr;
    XImage *im;

    screen = SRY_GET_SHORT(data + 0) & 0xffff;
    sx     = SRY_GET_SHORT(data + 2) & 0xffff;
    sy     = SRY_GET_SHORT(data + 4) & 0xffff;
    im = theRealScreen->im;
    scr = virtualScreen[screen];

#ifdef SRY_DEBUG
    printf("Trans all: %d:(%d,%d)\n", screen, sx, sy);
#endif /* SRY_DEBUG */

    if(!check_range(scr, sx, sy,
		    sx + REAL_SCREEN_SIZE_X - 1, sy + REAL_SCREEN_SIZE_Y - 1))
	return;

    if(theClass != TrueColor)
    {
	for(y = 0; y < REAL_SCREEN_SIZE_Y; y++)
	    for(x = 0; x < REAL_SCREEN_SIZE_X; x++)
		XPutPixel(im, x, y, VSCREEN_PIXEL(scr, sx + x, sy + y));
    }
    else /* TrueColor */
    {
	for(y = 0; y < REAL_SCREEN_SIZE_Y; y++)
	    memcpy(pseudoImage + y * REAL_SCREEN_SIZE_X,
		   scr->data + (sy + y) * scr->width + sx,
		   REAL_SCREEN_SIZE_X);
    }

    isRealScreenChanged = 1;
    updateClipX1 = 0;
    updateClipY1 = 0;
    updateClipX2 = REAL_SCREEN_SIZE_X-1;
    updateClipY2 = REAL_SCREEN_SIZE_Y-1;
}

static void sry_trans_partial_real(uint8 *data)
{
    int screen, sx, sy, tx, ty, w, h, x, y;
    int x2, y2;
    VirtualScreen *scr;
    XImage *im;

    screen = SRY_GET_SHORT(data + 0) & 0xffff;
    sx     = SRY_GET_SHORT(data + 2) & 0xffff;
    sy     = SRY_GET_SHORT(data + 4) & 0xffff;
    tx     = SRY_GET_SHORT(data + 6) & 0xffff;
    ty     = SRY_GET_SHORT(data + 8) & 0xffff;
    w      = SRY_GET_SHORT(data +10) & 0xffff;
    h      = SRY_GET_SHORT(data +12) & 0xffff;
    x2     = tx + w - 1;
    if(x2 >= REAL_SCREEN_SIZE_X)
    {
	x2 = REAL_SCREEN_SIZE_X - 1;
	w = x2 - tx + 1;
    }
    y2     = ty + h - 1;
    if(y2 >= REAL_SCREEN_SIZE_Y)
    {
	y2 = REAL_SCREEN_SIZE_Y - 1;
	h = y2 - ty + 1;
    }

#ifdef SRY_DEBUG
    printf("Trans partial: %d:(%d,%d) (%d,%d) %dx%d\n",
	   screen, sx, sy, tx, ty, w, h);
#endif /* SRY_DEBUG */

    im = theRealScreen->im;
    scr = virtualScreen[screen];

    if(theClass != TrueColor)
    {
	for(y = 0; y < h; y++)
	    for(x = 0; x < w; x++)
		XPutPixel(im, tx + x, ty + y,
			  VSCREEN_PIXEL(scr, sx + x, sy + y));
    }
    else
    {
	for(y = 0; y < h; y++)
	{
	    memcpy(pseudoImage + (ty + y) * REAL_SCREEN_SIZE_X + tx,
		   scr->data   + (sy + y) * scr->width + sx,
		   w);
	}
    }

    if(isRealScreenChanged)
    {
	/*???*/
	x_sry_update();
    }
    updateClipX1 = tx;
    updateClipY1 = ty;
    updateClipX2 = x2;
    updateClipY2 = y2;
    isRealScreenChanged = 1;
}

static VirtualScreen *get_tmp_screen(int width_require,
				     int height_require)
{
    if(tmpScreen == NULL)
    {
	if(width_require < REAL_SCREEN_SIZE_X)
	    width_require = REAL_SCREEN_SIZE_X;
	if(height_require < REAL_SCREEN_SIZE_Y)
	    height_require = REAL_SCREEN_SIZE_Y;
	return tmpScreen = alloc_vscreen(width_require, height_require, 0);
    }
    if(tmpScreen->width * tmpScreen->height >= width_require * height_require)
    {
	tmpScreen->width = width_require;
	tmpScreen->height = height_require;
	return tmpScreen;
    }
    if(tmpScreen)
	free_vscreen(tmpScreen);
    return tmpScreen = alloc_vscreen(width_require, height_require, 0);
}

static void normalize_rect(int *x1, int *y1, int *x2, int *y2)
{
    int w, h, x0, y0;

    w = *x2 - *x1;
    if(w >= 0)
	x0 = *x1;
    else
    {
	w = -w;
	x0 = *x2;
    }

    h = *y2 - *y1;
    if(h >= 0)
	y0 = *y1;
    else
    {
	h = -h;
	y0 = *y2;
    }

    *x1 = x0;
    *y1 = y0;
    *x2 = x0 + w;
    *y2 = y0 + h;
}

static int check_range(VirtualScreen *scr,
		       int x1, int y1, int x2, int y2)
{
    if(x1 < 0 ||
       y1 < 0 ||
       x2 >= scr->width ||
       y2 >= scr->height ||
       x1 > x2 ||
       y1 > y2)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: Out of range: (%d,%d)-(%d,%d)"
		  " > %dx%d, stop sherring",
		  x1, y1, x2, y2, scr->width, scr->height);
	err_to_stop = 1;
	return 0;
    }
    return 1;
}

static VirtualScreen *virtual_screen(int scr)
{
    if(virtualScreen[scr] == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: screen %d is not allocated, stop sherring",
		  scr);
	err_to_stop = 1;
	return NULL;
    }
    return virtualScreen[scr];
}

static void copy_vscreen_area(VirtualScreen *src,
			      VirtualScreen *dest,
			      int src_x1,
			      int src_y1,
			      int src_x2,
			      int src_y2,
			      int dest_x,
			      int dest_y,
			      int mask,
			      int trans)
{
    int x, y, sw, dw;
    uint8 *source, *target;
    int ww, hh;

    sw = src->width;
    dw = dest->width;
    source = src->data;
    target = dest->data;
    ww = src_x2 - src_x1 + 1;
    hh = src_y2 - src_y1 + 1;

    if(mask == 0xff && !trans)
    {
	for(y = 0; y < hh; y++)
	{
	    memcpy(target + (dest_y + y) * dw + dest_x,
		   source + (src_y1 + y) * sw + src_x1, ww);
	}
    }
    else
    {
	int p, q, i;
	int skip_color;

	skip_color = trans? src->transParent : -1;
	for(y = 0; y < hh; y++)
	{
	    for(x = 0; x < ww; x++)
	    {
		p = source[(src_y1 + y) * sw + src_x1 + x];
		if(p == skip_color)
		    continue;
		i = (dest_y + y) * dw + dest_x + x;
		q = target[i];
		target[i] = ROP3_CA0749(mask, p, q);
	    }
	}
    }
}

static void sry_trans_partial(uint8 *data)
{
    VirtualScreen *src, *dest;
    int from, to;
    int x1, y1, x2, y2, tx, ty;
    int mask, trans;

    from = SRY_GET_SHORT(data + 0) & 0xffff;
    to   = SRY_GET_SHORT(data + 2) & 0xffff;
    mask = data[4];
    trans = data[5];
    x1   = SRY_GET_SHORT(data + 6) & 0xffff;
    y1   = SRY_GET_SHORT(data + 8) & 0xffff;
    x2   = SRY_GET_SHORT(data +10) & 0xffff;
    y2   = SRY_GET_SHORT(data +12) & 0xffff;
    tx   = SRY_GET_SHORT(data +14) & 0xffff;
    ty   = SRY_GET_SHORT(data +16) & 0xffff;

    if((src = virtual_screen(from)) == NULL)
	return;
    if((dest = virtual_screen(to)) == NULL)
	return;

#ifdef SRY_DEBUG
    printf("Screen copy: %d:(%d,%d)-(%d,%d) -> %d:(%d,%d)  mask=0x%02x trans=%d\n",
	   from, x1, y1, x2, y2, to, tx, ty,
	   mask, trans);
#endif /* SRY_DEBUG */

    normalize_rect(&x1, &y1, &x2, &y2);
    if(!check_range(src, x1, y1, x2, y2))
	return;
    if(!check_range(dest, tx, ty, tx + x2 - x1, ty + y2 - y1))
	return;

    if(src == dest)
    {
	VirtualScreen *tmp;
	tmp = get_tmp_screen(x2 + 1, y2 + 1);
	tmp->transParent = src->transParent;
	copy_vscreen_area(src, tmp, x1, y1, x2, y2, x1, y1, 0xff, 0);
	src = tmp;
    }

    copy_vscreen_area(src, dest, x1, y1, x2, y2, tx, ty, mask, trans);
}

static void sry_trans_partial_mask(uint8 *data)
{
    int from, to;
    int planeMask, transparencyFlag;
    int x1, y1, x2, y2, toX, toY;
    int maskX, maskY;
    uint8 *maskData;
    VirtualScreen *src, *dest;
    int x, y, width, height;
    int mx, my;
    int bytesPerLine;
    int srcP, dstP;

    from = SRY_GET_SHORT(data + 0) & 0xffff;
    to = SRY_GET_SHORT(data + 2) & 0xffff;
    planeMask = data[4];
    transparencyFlag = data[5];
    x1 = SRY_GET_SHORT(data + 6) & 0xffff;
    y1 = SRY_GET_SHORT(data + 8) & 0xffff;
    x2 = SRY_GET_SHORT(data +10) & 0xffff;
    y2 = SRY_GET_SHORT(data +12) & 0xffff;
    toX = SRY_GET_SHORT(data +14) & 0xffff;
    toY = SRY_GET_SHORT(data +16) & 0xffff;
    maskX = data[18];
    maskY = data[19];
    maskData = data + 20;

    width = x2 - x1 + 1;
    height = y2 - y1 + 1;

    if((src = virtual_screen(from)) == NULL)
	return;
    if((dest = virtual_screen(to)) == NULL)
	return;
    normalize_rect(&x1, &y1, &x2, &y2);
    if(!check_range(src, x1, y1, x2, y2))
	return;
    if(!check_range(dest, toX, toY, toX + x2 - x1, toY + y2 - y1))
	return;

    if(src == dest)
    {
	VirtualScreen *tmp;
	tmp = get_tmp_screen(x2 + 1, y2 + 1);
	tmp->transParent = src->transParent;
	copy_vscreen_area(src, tmp, x1, y1, x2, y2, x1, y1, 0xff, 0);
	src = tmp;
    }

    bytesPerLine = maskX;
    bytesPerLine = ((bytesPerLine + 7) & ~7); /* round up to 8 */
    bytesPerLine /= 8;

    my = y1 % maskY;
    for(y = 0; y < height; y++, my++)
    {
	if(my == maskY)
	    my = 0;
	mx = x1 % maskX;
	for(x = 0; x < width; x++, mx++)
	{
	    if(mx == maskX)
		mx = 0;
	    if(maskData[my * bytesPerLine + mx / 8] & (0x80 >> (mx%8)))
	    {
		srcP = VSCREEN_PIXEL(src, x1 + x, y1 + y);
		if(!transparencyFlag || srcP != src->transParent)
		{
		    dstP = VSCREEN_PIXEL(dest, toX + x, toY + y);
		    VSCREEN_PIXEL(dest, toX + x, toY + y) =
			ROP3_CA0749(planeMask, srcP, dstP);
		}
	    }
	}
    }
}


static void sry_draw_box(uint8 *data)
{
    int screen;
    int mask;
    int x1, y1, x2, y2, x, y;
    int color;
    VirtualScreen *v;

    screen = SRY_GET_SHORT(data + 0) & 0xffff;
    mask   = data[2];
    x1     = SRY_GET_SHORT(data + 3) & 0xffff;
    y1     = SRY_GET_SHORT(data + 5) & 0xffff;
    x2     = SRY_GET_SHORT(data + 7) & 0xffff;
    y2     = SRY_GET_SHORT(data + 9) & 0xffff;
    color  = data[11];

#ifdef SRY_DEBUG
    printf("Box %d 0x%02x (%d,%d)-(%d,%d)\n", screen, mask, x1, y1, x2, y2);
#endif /* SRY_DEBUG */

    if((v = virtual_screen(screen)) == NULL)
	return;
    normalize_rect(&x1, &y1, &x2, &y2);
    if(!check_range(v, x1, y1, x2, y2))
	return;

    if(mask == 0xff)
    {
	for(y = y1; y <= y2; y++)
	    memset(v->data + v->width * y + x1, color, x2 - x1 + 1);
    }
    else
    {
	int p;
	for(y = y1; y <= y2; y++)
	    for(x = x1; x <= x2; x++)
	    {
		int i;
		i = v->width * y + x;
		p = v->data[i];
		v->data[i] = ROP3_CA0749(mask, color, p);
	    }
    }
}

static void vscreen_drawline(VirtualScreen* scr,
			     int x1, int y1, int x2, int y2,
			     int pixel, int mask)
{
    int dx, dy, incr1, incr2, d, x, y, xend, yend, xdirflag, ydirflag, idx;

    if(scr == NULL)
	return;

    if(x1 < 0 ||
       y1 < 0 ||
       x2 >= scr->width ||
       y2 >= scr->height)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: Out of range: (%d,%d)-(%d,%d)"
		  " > %dx%d, stop sherring",
		  x1, y1, x2, y2, scr->width, scr->height);
	err_to_stop = 1;
	return;
    }

    dx = x2 - x1; if(dx < 0) dx = -dx; /* dx = |x2-x1| */
    dy = y2 - y1; if(dy < 0) dy = -dy; /* dy = |y2-y1| */

    if(dy <= dx)
    {
	d = 2 * dy - dx;
	incr1 = 2 * dy;
	incr2 = 2 * (dy - dx);
	if(x1 > x2)
	{
	    x = x2;
	    y = y2;
	    ydirflag = -1;
	    xend = x1;
	}
	else
	{
	    x = x1;
	    y = y1;
	    ydirflag = 1;
	    xend = x2;
	}

	idx = scr->width * y + x;
	scr->data[idx] = ROP3_CA0749(mask, pixel, scr->data[idx]);
	if((y2 - y1) * ydirflag > 0)
	{
	    while(x < xend)
	    {
		x++;
		if(d < 0)
		    d += incr1;
		else
		{
		    y++;
		    d += incr2;
		}
		idx = scr->width * y + x;
		scr->data[idx] = ROP3_CA0749(mask, pixel, scr->data[idx]);
	    }
	}
	else
	{
	    while(x < xend)
	    {
		x++;
		if(d < 0)
		    d += incr1;
		else
		{
		    y--;
		    d += incr2;
		}
		idx = scr->width * y + x;
		scr->data[idx] = ROP3_CA0749(mask, pixel, scr->data[idx]);
	    }
	}		
    }
    else
    {
	d = 2 * dx - dy;
	incr1 = 2 * dx;
	incr2 = 2 * (dx - dy);
	if(y1 > y2)
	{
	    y = y2;
	    x = x2;
	    yend = y1;
	    xdirflag = -1;
	}
	else
	{
	    y = y1;
	    x = x1;
	    yend = y2;
	    xdirflag = 1;
	}
	idx = scr->width * y + x;
	scr->data[idx] = ROP3_CA0749(mask, pixel, scr->data[idx]);
	if((x2 - x1) * xdirflag > 0)
	{
	    while(y < yend)
	    {
		y++;
		if(d <0)
		    d += incr1;
		else
		{
		    x++;
		    d += incr2;
		}
		idx = scr->width * y + x;
		scr->data[idx] = ROP3_CA0749(mask, pixel, scr->data[idx]);
	    }
	}
	else
	{
	    while(y < yend)
	    {
		y++;
		if(d < 0)
		    d += incr1;
		else
		{
		    x--;
		    d += incr2;
		}
		idx = scr->width * y + x;
		scr->data[idx] = ROP3_CA0749(mask, pixel, scr->data[idx]);
	    }
	}
    }
}

static void sry_draw_vline(uint8 *data)
{
    int screen = SRY_GET_SHORT(data + 0) & 0xffff;
    int mask   = data[2];
    int x1     = SRY_GET_SHORT(data + 3) & 0xffff;
    int y1     = SRY_GET_SHORT(data + 5) & 0xffff;
    int y2     = SRY_GET_SHORT(data + 7) & 0xffff;
    int color  = data[9];

    vscreen_drawline(virtual_screen(screen),
		     x1, y1, x1, y2, color, mask);
}

static void sry_draw_hline(uint8 *data)
{
    int screen = SRY_GET_SHORT(data + 0) & 0xffff;
    int mask   = data[2];
    int x1     = SRY_GET_SHORT(data + 3) & 0xffff;
    int x2     = SRY_GET_SHORT(data + 5) & 0xffff;
    int y1     = SRY_GET_SHORT(data + 7) & 0xffff;
    int color  = data[9];

    vscreen_drawline(virtual_screen(screen),
		     x1, y1, x2, y1, color, mask);
}

static void sry_draw_line(uint8 *data)
{
    int screen = SRY_GET_SHORT(data + 0) & 0xffff;
    int mask   = data[2];
    int x1     = SRY_GET_SHORT(data + 3) & 0xffff;
    int y1     = SRY_GET_SHORT(data + 5) & 0xffff;
    int x2     = SRY_GET_SHORT(data + 7) & 0xffff;
    int y2     = SRY_GET_SHORT(data + 9) & 0xffff;
    int color  = data[11];

    vscreen_drawline(virtual_screen(screen),
		     x1, y1, x2, y2, color, mask);
}

static void sry_pal_merge(uint8 *data)
{
    int Pal1, Pal2, PalResult;
    int Pal1in, Pal1bit, Pal1out;
    int Pal2in, Pal2bit, Pal2out;
    int Per1, Per2;
    int Pal1Mask, Pal2Mask, PalMask;
    int i;
    int i1, i2;
    SherryPaletteEntry *pal_in1, *pal_in2, *pal_res;

    Pal1 = SRY_GET_SHORT(data + 0) & 0xffff;
    Pal2 = SRY_GET_SHORT(data + 2) & 0xffff;
    PalResult = SRY_GET_SHORT(data + 4) & 0xffff;
    Pal1in = data[6];
    Pal1bit = data[7];
    Pal1out = data[8];
    Pal2in = data[9];
    Pal2bit = data[10];
    Pal2out = data[11];
    Per1 = data[12];
    Per2 = data[13];

    Pal1Mask = 0xff >> ( 8 - Pal1bit );
    Pal1Mask <<= Pal1out;
    Pal2Mask = 0xff >> ( 8 - Pal2bit );
    Pal2Mask <<= Pal2out;
    PalMask  = Pal1Mask | Pal2Mask;

#ifdef SRY_DEBUG
    printf("palette merge %d(%d%%) %d(%d%%) => %d (mask=0x%x)\n",
	   Pal1, Per1,
	   Pal2, Per2,
	   PalResult,
	   PalMask);
#endif /* SRY_DEBUG */

    if(virtualPalette[Pal1] == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: palette %d is not allocated, stop sherring",
		  Pal1);
	err_to_stop = 1;
	return;
    }

    if(virtualPalette[Pal2] == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: palette %d is not allocated, stop sherring",
		  Pal2);
	err_to_stop = 1;
	return;
    }

    if(virtualPalette[PalResult] == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: palette %d is not allocated, stop sherring",
		  PalResult);
	err_to_stop = 1;
	return;
    }

    pal_in1 = virtualPalette[Pal1]->entry;
    pal_in2 = virtualPalette[Pal2]->entry;
    pal_res = virtualPalette[PalResult]->entry;

    for(i = 0; i < MAX_COLORS; i++)
    {
	if((i & PalMask) == i)
	{
	    int p;

	    i1 = (i & Pal1Mask) >> Pal1in;
	    i2 = (i & Pal2Mask) >> Pal2in;

	    p = pal_in1[i1].r * Per1 / 100 + pal_in2[i2].r * Per2 / 100;
	    if(p > 0xff) p = 0xff;
	    pal_res[i].r = p;

	    p = pal_in1[i1].g * Per1 / 100 + pal_in2[i2].g * Per2 / 100;
	    if(p > 0xff) p = 0xff;
	    pal_res[i].g = p;

	    p = pal_in1[i1].b * Per1 / 100 + pal_in2[i2].b * Per2 / 100;
	    if(p > 0xff) p = 0xff;
	    pal_res[i].b = p;
	}
    }
}

static void sry_pal_copy(uint8 *data)
{
    int pal1, pal2;

    pal1 = SRY_GET_SHORT(data + 0) & 0xffff;
    pal2 = SRY_GET_SHORT(data + 2) & 0xffff;

#ifdef SRY_DEBUG
    printf("Copy palette %d->%d\n", pal1, pal2);
#endif /* SRY_DEBUG */

    if(virtualPalette[pal1] == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD: palette %d is not allocated, stop sherring",
		  pal1);
	err_to_stop = 1;
	return;
    }

    if(virtualPalette[pal2] == NULL)
	virtualPalette[pal2] =
	    (SherryPalette *)safe_malloc(sizeof(SherryPalette));
    memcpy(virtualPalette[pal2], virtualPalette[pal1], sizeof(SherryPalette));
}

static void sry_text(uint8 *data)
{
    int screen, mask, mode, bg, fg, tx, ty;
    char *str;
    int len, advance;
    VirtualScreen *scr;

    screen = SRY_GET_SHORT(data + 0);
    mask = data[2];
    mode = data[3];
    fg   = data[4];
    bg   = data[5];
    tx    = SRY_GET_SHORT(data + 6);
    ty    = SRY_GET_SHORT(data + 8);
    str  = (char *)data + 10;
    len  = strlen(str);

    if(len == 0)
	return;

#ifdef SRY_DEBUG
    printf("%d 0x%02x %d %d %d (%d,%d)\n%s\n",
	   screen, mask, mode, fg, bg, tx, ty, str);
#endif /* SRY_DEBUG */

    if((scr = virtual_screen(screen)) == NULL)
	return;

    if(!check_range(scr,
		    tx, ty, tx + CHAR_WIDTH1 * len - 1, ty + CHAR_HEIGHT - 1))
	return;

    while((advance = bitmap_drawimage(imageBitmap, str, len)) > 0)
    {
	int width, bx, by, p;
	width = advance * CHAR_WIDTH1;

	for(by = 0; by < CHAR_HEIGHT; by++)
	{
	    for(bx = 0; bx < width; bx++)
	    {
		if(XGetPixel(imageBitmap->im, bx, by))
		{
		    if(mode & 1)
		    {
			p = VSCREEN_PIXEL(scr, tx + bx, ty + by);
			VSCREEN_PIXEL(scr, tx + bx, ty + by) =
			    ROP3_CA0749(mask, fg, p);
		    }
		}
		else
		{
		    if(mode & 2)
		    {
			p = VSCREEN_PIXEL(scr, tx + bx, ty + by);
			VSCREEN_PIXEL(scr, tx + bx, ty + by) =
			    ROP3_CA0749(mask, bg, p);
		    }
		}
	    }
	}

	str += advance;
	len -= advance;
	tx  += advance * CHAR_WIDTH1;
    }
}

void x_sry_clear(void)
{
    int i;

    if(theDisplay == NULL)
	return;

    isRealPaletteChanged = 0;
    isRealScreenChanged = 0;
    err_to_stop = 0;

    init_palette();

    XSetWindowBackground(theDisplay, theWindow, basePixel);
    XClearWindow(theDisplay, theWindow);

    clear_image_pixmap(theRealScreen, basePixel);

    for(i = 0; i < MAX_VIRTUAL_SCREENS; i++)
	if(virtualScreen[i] != NULL)
	{
	    free_vscreen(virtualScreen[i]);
	    virtualScreen[i] = NULL;
	}
    if(tmpScreen)
    {
	free(tmpScreen);
	tmpScreen = NULL;
    }

    for(i = 0; i < MAX_VIRTUAL_PALETTES; i++)
	if(virtualPalette[i] != NULL)
	{
	    free(virtualPalette[i]);
	    virtualPalette[i] = NULL;
	}
    memset(realPalette, 0, sizeof(realPalette));
    if(theClass == TrueColor)
	memset(pseudoImage, 0, REAL_SCREEN_SIZE_X * REAL_SCREEN_SIZE_Y);
}


#ifdef SRY_DEBUG
static void print_command(uint8 *data, int len)
{
    int i;
    printf("sherry:");
    for(i = 0; i < len; i++)
    {
	printf(" 0x%02x", data[i]);
	if(i >= 28)
	{
	    printf("...");
	    break;
	}
    }
    printf("\n");
    fflush(stdout);
}
#endif /* SRY_DEBUG */

void x_sry_wrdt_apply(uint8 *data, int len)
{
    int op, skip_bit;

    if(err_to_stop || theDisplay == NULL)
	return;

#ifdef SRY_DEBUG
    print_command(data, len);
#endif /* SRY_DEBUG */

    op = *data++;
    len--;

    skip_bit = op & 0x80;
    if(skip_bit && aq_filled_ratio() < 0.2)
	return;
    op &= 0x7F;

    switch(op)
    {
      case 0x00: /* DataEnd */
	break;
      case 0x01:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "sherry start");
	wrd_init_path();
	x_sry_clear();
	break;
      case 0x21:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "new pal 0x21");
	sry_new_vpal(data, len);
	break;
      case 0x22:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "free pal 0x22");
	sry_free_vpal(data, len);
	break;
      case 0x25:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "new vram 0x25");
	sry_new_vram(data, len);
	break;
      case 0x26:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "free vram 0x26");
	sry_free_vram(data, len);
	break;
      case 0x27:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "load PNG 0x27");
	sry_load_png(data);
	break;
      case 0x31:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "palette trans 0x31 (%d)", SRY_GET_SHORT(data));
	sry_pal_v2r(data);
	break;
      case 0x35:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "image copy 0x35 (%d)", SRY_GET_SHORT(data));
	sry_trans_all(data);
	break;
      case 0x36:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "image copy 0x36 (%d)", SRY_GET_SHORT(data));
	sry_trans_partial_real(data);
	break;
      case 0x41:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "set pal 0x41");
	sry_pal_set(data, len);
	break;
      case 0x42:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "pal merge 0x42");
	sry_pal_merge(data);
	break;
      case 0x43:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "pal copy 0x43");
	sry_pal_copy(data);
	break;
      case 0x51:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "text 0x51");
	sry_text(data);
	break;
      case 0x52:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "draw box 0x52");
	sry_draw_box(data);
	break;
      case 0x53:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "draw line 0x53");
	sry_draw_vline(data);
	break;
      case 0x54:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "draw line 0x54");
	sry_draw_hline(data);
	break;
      case 0x55:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "draw line 0x55");
	sry_draw_line(data);
	break;
      case 0x61:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "image copy 0x61");
	sry_trans_partial(data);
	break;
      case 0x62:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "image copy 0x62");
	sry_trans_partial_mask(data);
	break;
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x1e:
      case 0x1f:
      case 0x20:
      case 0x71:
      case 0x72:
      case 0x7f:
	ctl->cmsg(CMSG_WARNING, VERB_DEBUG,
		  "Sherry WRD 0x%x: not supported, ignore", op);
	break;

      default:
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sherry WRD 0x%x: not defined, stop sherring", op);
	err_to_stop = 1;
	break;
    }
}

void x_sry_redraw_ctl(int flag)
{
    draw_ctl_flag = flag;
    if(draw_ctl_flag)
	update_real_screen(0, 0, REAL_SCREEN_SIZE_X, REAL_SCREEN_SIZE_Y);
}

void x_sry_update(void)
{
    if(err_to_stop)
	return;

#ifdef SRY_DEBUG
    puts("Update pallete & screen");
#endif /* SRY_DEBUG */

    if(!isRealPaletteChanged && !isRealScreenChanged)
	return;
    if(isRealPaletteChanged)
	updatePalette();
    if(isRealScreenChanged)
	updateScreen(updateClipX1, updateClipY1,
		     updateClipX2 - updateClipX1 + 1,
		     updateClipY2 - updateClipY1 + 1);
    XSync(theDisplay, False);
}


/**** ImagePixmap interfaces ****/
static ImagePixmap *create_image_pixmap(int width, int height, int depth)
{
    ImagePixmap *ip;

    ip = (ImagePixmap *)safe_malloc(sizeof(ImagePixmap));
    ip->pm = XCreatePixmap(theDisplay, theWindow, width, height, depth);
    ip->im = XGetImage(theDisplay, ip->pm, 0, 0, width, height,
		       AllPlanes, ZPixmap);
    ip->gc = createGC(theDisplay, ip->pm);
    ip->depth = depth;
#if XSHM_SUPPORT
    ip->shminfo.shmid = -1;
#endif

    return ip;
}

#if XSHM_SUPPORT
static int shm_error;
static int my_err_handler(Display* dpy, XErrorEvent* e)
{
    shm_error = e->error_code;
    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
	      "Warning: X Sherry Warning: Can't create SHM Pixmap. error-code=%d",
	      shm_error);
    return shm_error;
}
static ImagePixmap *create_shm_image_pixmap(int width, int height, int depth)
{
    XErrorHandler origh;
    ImagePixmap *ip;
    int shm_depth;

    shm_depth = depth;
    ip = (ImagePixmap *)safe_malloc(sizeof(ImagePixmap));

    shm_error = 0;
    origh = XSetErrorHandler(my_err_handler);

    /* There is no need to initialize XShmSegmentInfo structure
     * before the call to XShmCreateImage.
     */
    ip->im = XShmCreateImage(theDisplay, theVisual, shm_depth,
			     ZPixmap, NULL,
			     &ip->shminfo, width, height);
    if(ip->im == NULL)
    {
	if(shm_error == 0)
	    shm_error = -1;
	goto done;
    }

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
    if(XShmAttach(theDisplay, &ip->shminfo) == 0)
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

    XSync(theDisplay, False);		/* Wait until ready. */

    ip->pm = XShmCreatePixmap(theDisplay, theWindow, ip->im->data,
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

    if(ip->im != NULL)
    {
	ip->gc = createGC(theDisplay, ip->pm);
	ip->depth = shm_depth;
    }
    else
    {
	free(ip);
	ip = create_image_pixmap(width, height, depth);
    }
    return ip;
}
#endif

static void clear_image_pixmap(ImagePixmap *ip, unsigned long pixel)
{
    int x, y, w, h;

    w = ip->im->width;
    h = ip->im->height;

    XSetForeground(theDisplay, ip->gc, pixel);
    XFillRectangle(theDisplay, ip->pm, ip->gc,
		   0, 0, ip->im->width, ip->im->height);
#if XSHM_SUPPORT
    if(ip->shminfo.shmid != -1)
	return;
#endif /* XSHM_SUPPORT */
    for(y = 0; y < h; y++)
	for(x = 0; x < w; x++){
	    XPutPixel(ip->im, x, y, pixel);
	}
}

static void free_image_pixmap(ImagePixmap *ip)
{
    XFreePixmap(theDisplay, ip->pm);

#if XSHM_SUPPORT
    if(ip->shminfo.shmid != -1)
    {
	/* To destory a shard memory XImage, you should call XShmDetach()
	 * first.
	 */
	XShmDetach(theDisplay, &ip->shminfo);

	/* Unmap shared memory segment */
	shmdt(ip->shminfo.shmaddr);

	/* Remove a shared memory ID from the system */
	shmctl(ip->shminfo.shmid, IPC_RMID, NULL);
    }
#endif /* XSHM_SUPPORT */
    XDestroyImage(ip->im);
    free(ip);
}


#define IS_MULTI_BYTE(c) (((c)&0x80) && \
			  ((0x1 <= ((c)&0x7F) && ((c)&0x7F) <= 0x1f) || \
			   (0x60 <= ((c)&0x7F) && ((c)&0x7F) <= 0x7c)))

/* bitmap_drawimage() Draws string into image
 * It returns number of bytes drawn
 */
static int bitmap_drawimage(ImagePixmap *ip, char *sjis_str, int nbytes)
{
    int write_len;
    int sjis_c1, sjis_c2;
    int x, width, height;

    x = write_len = 0;
    while(*sjis_str && write_len < IMAGEBITMAPLEN)
    {
	sjis_c1 = *sjis_str & 0xff;
	sjis_str++;
	
	if(IS_MULTI_BYTE(sjis_c1))
	{
	    int e1, e2;
	    XChar2b b;

	    if(write_len+1 == IMAGEBITMAPLEN)
		break;
	    sjis_c2 = *sjis_str & 0xff;
	    if(sjis_c2 == 0)
		break;
	    sjis_str++;

	    /* SJIS to EUC */
	    if(sjis_c2 >= 0x9f)
	    {
		if(sjis_c1 >= 0xe0)
		    e1 = sjis_c1 * 2 - 0xe0;
		else
		    e1 = sjis_c1 * 2 - 0x60;
		e2 = sjis_c2 + 2;
	    }
	    else
	    {
		if(sjis_c1 >= 0xe0)
		    e1 = sjis_c1 * 2 - 0xe1;
		else
		    e1 = sjis_c1 * 2 - 0x61;
		if(sjis_c2 >= 0x7f)
		    e2 = sjis_c2 + 0x60;
		else
		    e2 = sjis_c2 +  0x61;
	    }

	    b.byte1 = e1 & 0x7f;
	    b.byte2 = e2 & 0x7f;
	    XSetFont(theDisplay, ip->gc, theFont16->fid);
	    XDrawImageString16(theDisplay, ip->pm, ip->gc,
			       x - lbearing16, ascent16, &b, 1);
	    x += CHAR_WIDTH2;
	    write_len += 2;
	}
	else
	{
	    char c = sjis_c1;
	    XSetFont(theDisplay, ip->gc, theFont8->fid);
	    XDrawImageString(theDisplay, ip->pm, ip->gc,
			     x - lbearing8, ascent8, &c, 1);
	    x += CHAR_WIDTH1;
	    write_len++;
	}
    }

    if(write_len == 0)
	return 0; /* Terminate repeating call */

#if XSHM_SUPPORT
    if(ip->shminfo.shmid != -1)
    {
        XSync(theDisplay, 0); /* Wait until ready */
	return write_len;
    }
#endif /* XSHM_SUPPORT */

    /* XSHM is not supported.
     * Now, re-allocate XImage structure from the pixmap.
     */
    width = ip->im->width;
    height = ip->im->height;
    XDestroyImage(ip->im);
    ip->im = XGetImage(theDisplay, ip->pm, 0, 0, width, height,
		       AllPlanes, ZPixmap);

    return write_len;
}


/**** VirtualScreen intarfaces ****/
static VirtualScreen *alloc_vscreen(int width, int height, int transParent)
{
    VirtualScreen *scr;
    int size = width * height;

    /* Shared the allocated memory for data and the structure */
    scr = (VirtualScreen *)safe_malloc(sizeof(VirtualScreen) + size);
    scr->width = width;
    scr->height = height;
    scr->transParent = transParent;
    memset(scr->data, transParent, size);
    return scr;
}

static void free_vscreen(VirtualScreen *scr)
{
    free(scr);
}

void x_sry_event(void)
{
    if(QLength(theDisplay) == 0)
	XSync(theDisplay, False);
    while(QLength(theDisplay) > 0)
    {
	XEvent e;
	XNextEvent(theDisplay, &e);
	switch(e.type)
	{
	  case Expose:
	    update_real_screen(e.xexpose.x, e.xexpose.y,
			       e.xexpose.width, e.xexpose.height);
	    break;
	}
	if(QLength(theDisplay) == 0)
	    XSync(theDisplay, False);
    }
}
#endif /* ENABLE_SHERRY */
