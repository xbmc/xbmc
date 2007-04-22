/* svgalib.c
 * (c) 2000-2002 Karel 'Clock' Kulhavy
 * This file is a part of the Links program, released under GPL.
 *
 * This does graphics driver of svgalib, svgalib mouse.
 * This doesn't do svgalib keyboard.
 */

#include "cfg.h"

/*
:%s/->left/->clip.x1/g
:%s/->right/->clip.x2/g
:%s/->top/->clip.y1/g
:%s/->bottom/->clip.y2/g
*/

#ifdef GRDRV_SVGALIB

#include "links.h"
#include "bits.h"

#ifdef TEXT
#undef TEXT
#endif

#include <vga.h>
#include <vgamouse.h>
#include "arrow.inc"

extern struct itrm *ditrm;

extern struct graphics_driver svga_driver;
static int mouse_x, mouse_y, mouse_buttons; /* For tracking the state of the mouse */
static int background_x, background_y; /* Where was the mouse background taken from */
static unsigned char *mouse_buffer, *background_buffer, *new_background_buffer;
static struct graphics_device *mouse_graphics_device;
static int global_mouse_hidden;
static long mouse_black, mouse_white; /* Mouse arrow pointer colors */
static int (* mouse_getscansegment)(unsigned char *, int, int, int);
static int (* mouse_drawscansegment)(unsigned char *, int, int, int);
static int mouse_works = 0;
static unsigned char *svga_driver_param; /* NULL by default */
static int vga_mode; /* The mode that has been selected */
static struct graphics_device *backup_virtual_device;
static int mouse_aggregate_flag, mouse_aggregate_action;
static int flags = 0; /* OR-ed 1: running in background
		*       2: vga_block()-ed
		*/
static int svgalib_timer_id;
static int utf8_table;

/*---------------------------LIMITATIONS---------------------------------------*/
/* pixel_set_paged works only for <=8 bytes per pixel.
 * Doesn't work on video cards which have 1 pixel spanning more that 65536 bytes! ( :-) )
 * vga_linewidth%vga_bytes must be zero.
 * The bitmaps have all consecutive data. No vidram mallocing is performed.
 */

/*------------------------STRUCTURES-------------------------------------------*/
struct modeline{
	unsigned char *name;
	int number;
};

/*-------------- GLOBAL VARIABLES --------------------------------------------*/

#define NUMBER_OF_DEVICES	10

#define TEST_INACTIVITY if (dev!=current_virtual_device) return;

#define TEST_INACTIVITY_0 if (dev!=current_virtual_device) return 0;

#define RECTANGLES_INTERSECT(xl0, xh0, xl1, xh1, yl0, yh0, yl1, yh1) (\
				   (xl0)<(xh1)\
				&& (xl1)<(xh0)\
				&& (yl0)<(yh1)\
				&& (yl1)<(yh0))

#define TEST_MOUSE(xl,xh,yl,yh) if (RECTANGLES_INTERSECT(\
					(xl),(xh),\
					background_x,background_x+arrow_width,\
					(yl),(yh),\
					background_y,background_y+arrow_height)\
					&& !global_mouse_hidden){\
					mouse_hidden=1;\
					hide_mouse();\
				}else mouse_hidden=0;

#define END_MOUSE if (mouse_hidden) show_mouse();

/* Actual vga mode definition */
int vga_linewidth;		/* Prepared out from vga_getmodeinfo */
int xsize, ysize;		/* Prepared out from vga_getmodeinfo */
int vga_bytes;			/* Prepared out from vga_getmodeinfo */
int vga_colors;			/* Prepared out from vga_getmodeinfo */
int vga_misordered;		/* Prepared out from vga_getmodeinfo */
int vga_linear;			/* 1 linear mode, 0 nonlinear mode (paged) */
int palette_depth;		/* 6 for normal VGA, 8 for VGA which supports 8 bit DAC */
int accel_avail;		/* Which accel fns are available */
int do_sync;			/* Tells the "normal" memory operations (those
				 * that do not use accelerator) to do
				 * vga_accel(ACCEL_SYNC) before writing into the
				 * memory.
				 */
int vga_page=-1;
int mode_x;			/* 1 if mode_X organization is set up */
int bmpixelsize;		/* Number of bytes per pixel in bitmap */
unsigned char *my_graph_mem;
unsigned char *scroll_buffer = NULL; /* For paged scrolling only */
struct modeline modes[]={
#ifdef G320x200x16
	{"320x200x16",G320x200x16},
#endif
#ifdef G640x200x16
	{"640x200x16",G640x200x16},
#endif
#ifdef G640x350x16
	{"640x350x16",G640x350x16},
#endif
#ifdef G640x480x16
	{"640x480x16",G640x480x16},
#endif
#ifdef G800x600x16
	{"800x600x16",G800x600x16},
#endif
#ifdef G1024x768x16
	{"1024x768x16",G1024x768x16},
#endif
#ifdef G1152x864x16
	{"1152x864x16",G1152x864x16},
#endif
#ifdef G1280x1024x16
	{"1280x1024x16",G1280x1024x16},
#endif
#ifdef G1600x1200x16
	{"1600x1200x16",G1600x1200x16},
#endif
#ifdef G320x200x256
	{"320x200x256",G320x200x256},
#endif
#ifdef G320x240x256
	{"320x240x256",G320x240x256},
#endif
#ifdef G320x400x256
	{"320x400x256",G320x400x256},
#endif
#ifdef G360x480x256
	{"360x480x256",G360x480x256},
#endif
#ifdef G640x400x256
	{"640x400x256",G640x400x256},
#endif
#ifdef G640x480x256
	{"640x480x256",G640x480x256},
#endif
#ifdef G800x600x256
	{"800x600x256",G800x600x256},
#endif
#ifdef G1024x768x256
	{"1024x768x256",G1024x768x256},
#endif
#ifdef G1152x864x256
	{"1152x864x256",G1152x864x256},
#endif
#ifdef G1280x1024x256
	{"1280x1024x256",G1280x1024x256},
#endif
#ifdef G1600x1200x256
	{"1600x1200x256",G1600x1200x256},
#endif
#ifdef G320x200x32K
	{"320x200x32K",G320x200x32K},
#endif
#ifdef G640x480x32K
	{"640x480x32K",G640x480x32K},
#endif
#ifdef G800x600x32K
	{"800x600x32K",G800x600x32K},
#endif
#ifdef G1024x768x32K
	{"1024x768x32K",G1024x768x32K},
#endif
#ifdef G1152x864x32K
	{"1152x864x32K",G1152x864x32K},
#endif
#ifdef G1280x1024x32K
	{"1280x1024x32K",G1280x1024x32K},
#endif
#ifdef G1600x1200x32K
	{"1600x1200x32K",G1600x1200x32K},
#endif
#ifdef G320x200x64K
	{"320x200x64K",G320x200x64K},
#endif
#ifdef G640x480x64K
	{"640x480x64K",G640x480x64K},
#endif
#ifdef G800x600x64K
	{"800x600x64K",G800x600x64K},
#endif
#ifdef G1024x768x64K
	{"1024x768x64K",G1024x768x64K},
#endif
#ifdef G1152x864x64K
	{"1152x864x64K",G1152x864x64K},
#endif
#ifdef G1280x1024x64K
	{"1280x1024x64K",G1280x1024x64K},
#endif
#ifdef G1600x1200x64K
	{"1600x1200x64K",G1600x1200x64K},
#endif
#ifdef G320x200x16M
	{"320x200x16M",G320x200x16M},
#endif
#ifdef G640x480x16M
	{"640x480x16M",G640x480x16M},
#endif
#ifdef G800x600x16M
	{"800x600x16M",G800x600x16M},
#endif
#ifdef G1024x768x16M
	{"1024x768x16M",G1024x768x16M},
#endif
#ifdef G1152x864x16M
	{"1152x864x16M",G1152x864x16M},
#endif
#ifdef G1280x1024x16M
	{"1280x1024x16M",G1280x1024x16M},
#endif
#ifdef G1600x1200x16M
	{"1600x1200x16M",G1600x1200x16M},
#endif
#ifdef G320x200x16M32
	{"320x200x16M32",G320x200x16M32},
#endif
#ifdef G640x480x16M32
	{"640x480x16M32",G640x480x16M32},
#endif
#ifdef G800x600x16M32
	{"800x600x16M32",G800x600x16M32},
#endif
#ifdef G1152x864x16M32
	{"1152x864x16M32",G1152x864x16M32},
#endif
#ifdef G1024x768x16M32
	{"1024x768x16M32",G1024x768x16M32},
#endif
#ifdef G1600x1200x16M32
	{"1600x1200x16M32",G1600x1200x16M32}
#endif
};

/*--------------------------- ROUTINES ---------------------------------------*/

/* Generates these palettes:
 *  7 6 5 4 3 2 1 0
 * +-----+-----+---+
 * |  R  |  G  | B |
 * +-----+-----+---+
 *
 *
 *  3 2 1 0
 * +-+---+-+
 * |R| G |B|
 * +-+---+-+
 */

static void show_mouse(void);
static void hide_mouse(void);

/* We must perform a quirkafleg
 * This is an empiric magic that ensures
 * Good white purity
 * Correct rounding and dithering prediction
 * And this is the cabbala:
 * 063 021 063 
 * 009 009 021
 * 255 085 255
 * 036 036 084
 */
static void generate_palette(struct irgb *palette)
{
	int a;

	if (vga_colors==16){
		if (palette_depth==6){
			for (a=0;a<16;a++,palette++)
			{
				palette->r=(a&8)?63:0;
				palette->g=((a>>1)&3)*21;
				palette->b=(a&1)?63:0;
			}
		}else{
			/* palette_depth==8 */
			for (a=0;a<16;a++,palette++)
			{
				palette->r=(a&8)?255:0;
				palette->g=((a>>1)&3)*85;
				palette->b=(a&1)?255:0;
			}
		}
	}else{
		/* vga_colors==256 */
		if (palette_depth==6){
			for (a=0;a<256;a++,palette++){
				palette->r=((a>>5)&7)*9;
				palette->g=((a>>2)&7)*9;
				palette->b=(a&3)*21;
			}
		}else{
			/* palette_depth==8 */
			for (a=0;a<256;a++,palette++){
				palette->r=((a>>5)&7)*36;
				palette->g=((a>>2)&7)*36;
				palette->b=(a&3)*84;
			}
		}
	}
			
}

static void set_palette(struct irgb *palette)
{
	int r,g,b,c;
	
	for (c=0;c<vga_colors;c++){
		r=palette[c].r;
		g=palette[c].g;
		b=palette[c].b;
		vga_setpalette(c,r,g,b);
	}
}

void svga_shutdown_driver(void)
{
	if (scroll_buffer) mem_free(scroll_buffer);
	if (mouse_works){
		mem_free(mouse_buffer);
		mem_free(background_buffer);
		mem_free(new_background_buffer);
		svga_driver.shutdown_device(mouse_graphics_device);
		mouse_close();
		mouse_works=0; /* To keep vga_select disabled */
	}
	shutdown_virtual_devices();
	vga_unlockvc();
	kill_timer(svgalib_timer_id);
	vga_setmode(TEXT);
	svgalib_free_trm(ditrm);
	if (svga_driver_param)mem_free(svga_driver_param);
	install_signal_handler(SIGINT, NULL, NULL, 0);
}

void svga_register_bitmap(struct bitmap *bmp)
{
}

void svga_unregister_bitmap(struct bitmap *bmp)
{
	mem_free(bmp->data);
}

#define SYNC if (do_sync) vga_accel(ACCEL_SYNC);
	
/* This assures that x, y, xs, ys, data will be sane according to clipping
 * rectangle. If nothing lies within this rectangle, the current function
 * returns. The data pointer is automatically advanced by this macro to reflect
 * the right position to start with inside the bitmap. */
#define	CLIP_PREFACE \
	int xs=hndl->x,ys=hndl->y;\
	char *data=hndl->data;\
	int mouse_hidden;\
\
 	TEST_INACTIVITY\
	if (x>=dev->clip.x2||x+xs<=dev->clip.x1) return;\
	if (y>=dev->clip.y2||y+ys<=dev->clip.y1) return;\
	if (x+xs>dev->clip.x2) xs=dev->clip.x2-x;\
	if (y+ys>dev->clip.y2) ys=dev->clip.y2-y;\
	if (dev->clip.x1-x>0){\
		xs-=(dev->clip.x1-x);\
		data+=bmpixelsize*(dev->clip.x1-x);\
		x=dev->clip.x1;\
	}\
	if (dev->clip.y1-y>0){\
		ys-=(dev->clip.y1-y);\
		data+=hndl->skip*(dev->clip.y1-y);\
		y=dev->clip.y1;\
	}\
	/* xs, ys: how much pixels to paint\
	 * data: where to start painting from\
	 */\
	TEST_MOUSE (x,x+xs,y,y+ys)

static inline void draw_bitmap_accel(struct graphics_device *dev,
	struct bitmap* hndl, int x, int y)
{
	CLIP_PREFACE
	
	if (xs*bmpixelsize==hndl->skip) vga_accel(ACCEL_PUTIMAGE,x,y,xs,ys,data);
	else for(;ys;ys--){
		vga_accel(ACCEL_PUTIMAGE,x,y,xs,1,data);
		data+=hndl->skip;
		y++;
	}
	END_MOUSE
}

static inline void my_setpage(int page)
{
	if (vga_page!=page){
		vga_page=page;
		vga_setpage(page);
	}
}

static inline void paged_memcpy(int lina, unsigned char *src, int len)
{
	int page=lina>>16;
	int paga=lina&0xffff;
	int remains;
	
	my_setpage(page);
	remains=65536-paga;
	again:
	if (remains>=len){
		memcpy(my_graph_mem+paga,src,len);
		vga_page=page;
		return;
	}else{
		memcpy(my_graph_mem+paga,src,remains);
		paga=0;
		src+=remains;
		len-=remains;
		remains=65536;
		vga_setpage(++page);
		goto again;
	}
}


inline void draw_bitmap_drawscansegment(struct graphics_device *dev, struct bitmap* hndl, int x, int y)
{
	int ys0;

	CLIP_PREFACE
	SYNC
	for (ys0=ys;ys0;ys0--){
		vga_drawscansegment(data,x,y,xs);
		y++;
		data+=hndl->skip;
	}
	END_MOUSE
}

static inline void draw_bitmap_paged(struct graphics_device *dev, struct bitmap* hndl, int x, int y)
{
	int scr_start;

	CLIP_PREFACE
	scr_start=y*vga_linewidth+x*vga_bytes;
	SYNC
	for(;ys;ys--){
		paged_memcpy(scr_start,data,xs*vga_bytes);
		data+=hndl->skip;
		scr_start+=vga_linewidth;
	}
	END_MOUSE
		
		
}

static inline void draw_bitmap_linear(struct graphics_device *dev,struct bitmap* hndl, int x, int y)
{
	unsigned char *scr_start;

	CLIP_PREFACE
	SYNC
	scr_start=my_graph_mem+y*vga_linewidth+x*vga_bytes;
	for(;ys;ys--){
		memcpy(scr_start,data,xs*vga_bytes);
		data+=hndl->skip;
		scr_start+=vga_linewidth;
	}
	END_MOUSE
}

/* fill_area: 5,5,10,10 fills in 25 pixels! */

/* This assures that left, right, top, bottom will be sane according to the
 * clipping rectangle set up by svga_driver->set_clip_area. If empty region
 * results, return from current function occurs. */
#define FILL_CLIP_PREFACE \
 	int mouse_hidden;\
	TEST_INACTIVITY\
	if (left>=right||top>=bottom) return;\
	if (left>=dev->clip.x2||right<=dev->clip.x1||top>=dev->clip.y2||bottom<=dev->clip.y1) return;\
	if (left<dev->clip.x1) left=dev->clip.x1;\
	if (right>dev->clip.x2) right=dev->clip.x2;\
	if (top<dev->clip.y1) top=dev->clip.y1;\
	if (bottom>dev->clip.y2) bottom=dev->clip.y2;\
	TEST_MOUSE(left,right,top,bottom)

	
static void fill_area_accel_box(struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	FILL_CLIP_PREFACE
	
	vga_accel(ACCEL_SETFGCOLOR,color);
	vga_accel(ACCEL_FILLBOX,left,top,right-left,bottom-top);
	END_MOUSE
}
	
static void fill_area_accel_lines(struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	int y;
	
	FILL_CLIP_PREFACE
	vga_accel(ACCEL_SETFGCOLOR,color);
	for (y=top;y<bottom;y++) vga_accel(ACCEL_DRAWLINE,left,y,right-1,y);
	END_MOUSE
}

/* n is in bytes. dest must begin on pixel boundary. If n is not a whole number
 * of pixels, rounding is performed downwards.
 * if bmpixelsize is 1, no alignment is required.
 * if bmpixelsize is 2, dest must be aligned to 2 bytes.
 * if bmpixelsize is 3, no alignment is required.
 * if bmpixelsize is 4, dest must be aligned to 4 bytes.
 * -- The following do not occur, this is only for forward compatibility.
 * if bmpixelsize is 5, no alignment is required.
 * if bmpixelsize is 6, dest must be aligned to 2 bytes.
 * if bmpixelsize is 7, no alignment is required.
 * if bmpixelsize is 8, dest must be aligned to 8 bytes.
 */
static void inline pixel_set(unsigned char *dest, int n,void * pattern)
{
	int a;

	/* Originally there was vga_bytes here but this function is not
	 * used in planar modes so that it's OK :-) */
	switch(bmpixelsize)
	{
		case 1:
		
		memset(dest,*(char *)pattern,n);
		break;

		case 2:
		{
#ifdef t2c
			t2c v=*(t2c *)pattern;
			int a;
#else
			unsigned char c1=*(unsigned char *)pattern
				,c2=((unsigned char *)pattern)[1];
#endif /* #ifdef t2c */
			
			n>>=1;
#ifdef t2c
			/* [] is here because i386 has built-in address doubling
			 * that costs nothing and we spare a register and
			 * an addition of 2
			 */
			for (a=0;a<n;a++) ((t2c *)dest)[a]=v;
#else
			for (;n;n--,dest+=2){
				*dest=c1;
				dest[1]=c2;
			}
			
#endif /* #ifdef t2c */
		}
		break;

		case 3:
		{
			unsigned char a,b,c;
			
			a=*(unsigned char *)pattern;
			b=((unsigned char *)pattern)[1];
			c=((unsigned char *)pattern)[2];
			for (n/=3;n;n--){
				dest[0]=a;
				dest[1]=b;
				dest[2]=c;
				dest+=3;
			}
		}
		break;

		case 4:
		{
#ifdef t4c
			t4c v=*(t4c *)pattern;
			int a;
#else
			unsigned char c1, c2, c3, c4;
		
			c1=*((unsigned char *) pattern);
			c2=((unsigned char *) pattern)[1];
			c3=((unsigned char *) pattern)[2];
			c4=((unsigned char *) pattern)[3];
#endif /* #ifdef t4c */

			n>>=2;
#ifdef t4c
			/* [] is here because i386 has built-in address
			 * quadrupling that costs nothing and we spare a
			 * register and an addition of 4
			 */
			for (a=0;a<n;a++) ((t4c *)dest)[a]=v;
#else
			for (;n;n--, dest+=4){
				*dest=c1;
				dest[1]=c2;
				dest[2]=c3;
				dest[3]=c4;
			}	
#endif /* #ifdef t4c */
		}
		break;

		default:
		for (a=0;a<n/bmpixelsize;a++,dest+=bmpixelsize) memcpy(dest,pattern,bmpixelsize);
		break;
	}
	
}

static void fill_area_linear(struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	unsigned char *dest;
	int y;

	FILL_CLIP_PREFACE
	SYNC
	dest=my_graph_mem+top*vga_linewidth+left*vga_bytes;
	for (y=bottom-top;y;y--){
		pixel_set(dest,(right-left)*vga_bytes,&color);
		dest+=vga_linewidth;
	}
	END_MOUSE
}

/* Params are exactly the same as in pixel_set except lina, which is offset from
 * my_graph_mem in bytes.  Works for every vga_bytes. len is in bytes. len must
 * be a whole number of pixels.
 */
static void pixel_set_paged(int lina, int len, void * color)
{
	int page=lina>>16; /* Page number */
	int paga=lina&0xffff; /* 16-bit address within a page */
	int remains=65536-paga; /* How many bytes remain within the page*/
	int offset=0; /* Offset inside the pixel */
	unsigned char color0[15];

	memcpy(color0,color,vga_bytes);
	memcpy(color0+vga_bytes,color,vga_bytes-1);
	
	my_setpage(page);
	again:
	if (remains>=len){
		int done=len-len%vga_bytes;
		pixel_set(my_graph_mem+paga,done,color0+offset);
		paga+=done;
		if (done<len)
			memcpy(my_graph_mem+paga,color0+offset,len-done);
		vga_page=page;
		return;
	}else{
		int done=remains-remains%vga_bytes;
		pixel_set(my_graph_mem+paga,done,color0+offset);
		paga+=done;
		if (done<remains)
			memcpy(my_graph_mem+paga,color0+offset,remains-done);
		offset+=(remains-done);
		if (offset>=vga_bytes) offset-=vga_bytes;
		len-=remains;
		remains=65536;
		vga_setpage(++page);
		paga=0;
		goto again;
	}
}

static void fill_area_paged(struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	int dest;
	int y;
	int len;
	FILL_CLIP_PREFACE
	SYNC
	len=(right-left)*vga_bytes;
	dest=top*vga_linewidth+left*vga_bytes;
	for (y=bottom-top;y;y--){
		pixel_set_paged(dest,len,&color);
		dest+=vga_linewidth;
	}
	END_MOUSE
}
	
#define HLINE_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY\
	if (left>=right) return;\
	if (y<dev->clip.y1||y>=dev->clip.y2||right<=dev->clip.x1||left>=dev->clip.x2) return;\
	if (left<dev->clip.x1) left=dev->clip.x1;\
	if (right>dev->clip.x2) right=dev->clip.x2;\
	TEST_MOUSE (left,right,y,y+1)
	
#define VLINE_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY\
	if (top>=bottom) return;\
	if (x<dev->clip.x1||x>=dev->clip.x2||top>=dev->clip.y2||bottom<=dev->clip.y1) return;\
	if (top<dev->clip.y1) top=dev->clip.y1;\
	if (bottom>dev->clip.y2) bottom=dev->clip.y2;\
	TEST_MOUSE(x,x+1,top,bottom)
	
static void draw_hline_accel_line(struct graphics_device *dev, int left, int y, int right, long color)
{
	HLINE_CLIP_PREFACE

	vga_accel(ACCEL_SETFGCOLOR,color);
	vga_accel(ACCEL_DRAWLINE,left,y,right-1,y);
	END_MOUSE
}

static void draw_hline_accel_box(struct graphics_device *dev, int left, int y, int right, long color)
{
	HLINE_CLIP_PREFACE

	vga_accel(ACCEL_SETFGCOLOR,color);
	vga_accel(ACCEL_FILLBOX,left,y,right-left,1);
	END_MOUSE
}

static void draw_vline_accel_line(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	VLINE_CLIP_PREFACE

	vga_accel(ACCEL_SETFGCOLOR,color);
	vga_accel(ACCEL_DRAWLINE,x,top,x,bottom-1);
	END_MOUSE
}

static void draw_vline_accel_box(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	VLINE_CLIP_PREFACE

	vga_accel(ACCEL_SETFGCOLOR,color);
	vga_accel(ACCEL_FILLBOX,x,top,1,bottom-top);
	END_MOUSE
}

static void draw_hline_linear(struct graphics_device *dev, int left, int y, int right, long color)
{
	unsigned char *dest;
	HLINE_CLIP_PREFACE
	SYNC	
	dest=my_graph_mem+y*vga_linewidth+left*vga_bytes;
	pixel_set(dest,(right-left)*vga_bytes,&color);
	END_MOUSE
}

static void draw_vline_linear(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	unsigned char *dest;
	int y;
	VLINE_CLIP_PREFACE
	SYNC
	dest=my_graph_mem+top*vga_linewidth+x*vga_bytes;
	for (y=(bottom-top);y;y--){
		memcpy(dest,&color,vga_bytes);
		dest+=vga_linewidth;
	}
	END_MOUSE
}

static void draw_hline_paged(struct graphics_device *dev, int left, int y, int right, long color)
{
	int dest;
	int len;
	HLINE_CLIP_PREFACE	
	SYNC
	len=(right-left)*vga_bytes;

	dest=y*vga_linewidth+left*vga_bytes;
	pixel_set_paged(dest,len,&color);
	END_MOUSE
}

/* Works only for pixel lengths power of two */
static void draw_vline_paged_1(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	int dest,n, page,paga,remains;
	int byte=*(char *)&color;
	VLINE_CLIP_PREFACE;
	SYNC
	dest=top*vga_linewidth+x;
	n=bottom-top;
	page=dest>>16;
	my_setpage(page);
	again:
	paga=dest&0xffff;
	remains=(65535-paga)/vga_linewidth+1;
	if (remains>=n){
		for (;n;n--){
			my_graph_mem[paga]=byte;
			paga+=vga_linewidth;
		}
		vga_page=page;
		END_MOUSE
		return;
	}else{
		dest+=remains*vga_linewidth;
		n-=remains;
		for (;remains;remains--){
			my_graph_mem[paga]=byte;
			paga+=vga_linewidth;
		}
		vga_setpage(++page);
		goto again;
	}
}
			
#ifdef t2c
/* Works only for pixel length 2 */
static void draw_vline_paged_2(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	int dest,page,n,paga,remains;
	int word=*(t2c *)&color;
	VLINE_CLIP_PREFACE;
	SYNC
	dest=top*vga_linewidth+(x<<1);
	n=bottom-top;
	page=dest>>16;
	my_setpage(page);
	again:
	paga=dest&0xffff;
	remains=(65534-paga)/vga_linewidth+1;
	if (remains>=n){
		for (;n;n--){
			*(t2c *)(my_graph_mem+paga)=word;
			paga+=vga_linewidth;
		}
		vga_page=page;
		END_MOUSE
		return;
	}else{
		dest+=remains*vga_linewidth;
		n-=remains;
		for (;remains;remains--){
			*(t2c *)(my_graph_mem+paga)=word;
			paga+=vga_linewidth;
		}
		vga_setpage(++page);
		goto again;
	}
}
#endif /* #ifdef t2c */
			
#ifdef t4c
/* Works only for pixel length 4 */
static void draw_vline_paged_4(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	unsigned long dest,page,paga,remains,n;
	t4c val=*(t4c *)&color;

	VLINE_CLIP_PREFACE;
	SYNC
	dest=top*(unsigned long)vga_linewidth+(x<<2);
	n=bottom-top;
	page=dest>>16;
	my_setpage(page);
	again:
	paga=dest&0xffffUL;
	remains=(65532-paga)/vga_linewidth+1;
	if (remains>=n){
		for (;n;n--){
			*(t4c *)(my_graph_mem+paga)=val;
			paga+=vga_linewidth;
		}
		vga_page=page;
		END_MOUSE
		return;
	}else{
		dest+=remains*vga_linewidth;
		n-=remains;
		for (;remains;remains--){
			*(t4c *)(my_graph_mem+paga)=color;
			paga+=vga_linewidth;
		}
		vga_setpage(++page);
		goto again;
	}
}
#endif /*t4c*/
			
/* Works only for pixel lengths power of two */
static void draw_vline_paged_aligned(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	int dest,page,paga,remains,n;
	VLINE_CLIP_PREFACE;
	SYNC
	dest=top*vga_linewidth+x*vga_bytes;
	n=bottom-top;
	page=dest>>16;
	my_setpage(page);
	again:
	paga=dest&0xffff;
	remains=(65536-paga-vga_bytes)/vga_linewidth+1;
	if (remains>=n){
		for (;n;n--){
			memcpy(my_graph_mem+paga,&color,vga_bytes);
			paga+=vga_linewidth;
		}
		vga_page=page;
		END_MOUSE
		return;
	}else{
		dest+=remains*vga_linewidth;
		n-=remains;
		for (;remains;remains--){
			memcpy(my_graph_mem+paga,&color,vga_bytes);
			paga+=vga_linewidth;
		}
		vga_setpage(++page);
		goto again;
	}
}
			
/* Works for any pixel length */
static void draw_vline_paged(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	int lina,page,paga,remains,n;
	/* lina: linear address withing the screen
	 * page: page number
	 * paga: 16-bit address within the page
	 * remains: how many bytes remain in the current page
	 * n: how many pixels remain to be drawn
	 */
	VLINE_CLIP_PREFACE;
	SYNC
	lina=top*vga_linewidth+x*vga_bytes;
	n=bottom-top;
	page=lina>>16;
	my_setpage(page);
	again:
	/* Invariant here: n>=1
	 * lina points to a begin of pixel
	 * page is set to page
	 */
	paga=lina&0xffff;
	remains=65536-paga;
	if (remains<vga_bytes){
		memcpy(my_graph_mem+paga,&color,remains);
		vga_setpage(++page);
		memcpy(my_graph_mem,&color+remains,vga_bytes-remains);
		lina+=vga_linewidth;
		n--;
		if (!n) goto end;
		goto again;
	}
	remains=(remains-vga_bytes)/vga_linewidth+1;
	if (remains>=n){
		for (;n;n--){
			memcpy(my_graph_mem+paga,&color,vga_bytes);
			paga+=vga_linewidth;
		}
end:
		vga_page=page;
		END_MOUSE
		return;
	}else{
		lina+=remains*vga_linewidth;
		n-=remains;
		for (;remains;remains--){
			memcpy(my_graph_mem+paga,&color,vga_bytes);
			paga+=vga_linewidth;
		}
		if (paga>=65536)vga_setpage(++page);
		goto again;
	}
}
		
#define HSCROLL_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY_0\
	if (!sc) return 0;\
	if (sc>(dev->clip.x2-dev->clip.x1)||-sc>(dev->clip.x2-dev->clip.x1))\
		return 1;\
	TEST_MOUSE (dev->clip.x1,dev->clip.x2,dev->clip.y1,dev->clip.y2)
		
							
/* When sc is <0, moves the data left. Scrolls the whole clip window */
static int hscroll_accel(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	HSCROLL_CLIP_PREFACE

	ignore=NULL;
	if (sc>0){
		/* Move data to the right */
		vga_accel(ACCEL_SCREENCOPY,dev->clip.x1,dev->clip.y1,dev->clip.x1+sc,dev->clip.y1
			,dev->clip.x2-dev->clip.x1-sc,dev->clip.y2-dev->clip.y1);
	}else{
		/* Move data to the left */
		vga_accel(ACCEL_SCREENCOPY,dev->clip.x1-sc,dev->clip.y1,dev->clip.x1,dev->clip.y1
			,dev->clip.x2-dev->clip.x1+sc,dev->clip.y2-dev->clip.y1);
	}
	END_MOUSE
	return 1;
}

#define VSCROLL_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY_0\
	if (!sc) return 0;\
	if (sc>dev->clip.y2-dev->clip.y1||-sc>dev->clip.y2-dev->clip.y1) return 1;\
	TEST_MOUSE (dev->clip.x1, dev->clip.x2, dev->clip.y1, dev->clip.y2)
	
/* Positive sc means data move down */
static int vscroll_accel(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	VSCROLL_CLIP_PREFACE

	ignore=NULL;
	if (sc>0){
		/* Move down */
		vga_accel(ACCEL_SCREENCOPY,dev->clip.x1,dev->clip.y1,dev->clip.x1,dev->clip.y1+sc
			,dev->clip.x2-dev->clip.x1,dev->clip.y2-dev->clip.y1-sc);
	}else{
		/* Move up */
		vga_accel(ACCEL_SCREENCOPY,dev->clip.x1,dev->clip.y1-sc,dev->clip.x1,dev->clip.y1
			,dev->clip.x2-dev->clip.x1,dev->clip.y2-dev->clip.y1+sc);
	}
	END_MOUSE
	return 1;
}

static int hscroll_scansegment(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	int y;
	int len;
	HSCROLL_CLIP_PREFACE
	SYNC	
	ignore=NULL;
	if (sc>0){
		/* Right */
		len=dev->clip.x2-dev->clip.x1-sc;
		for (y=dev->clip.y1;y<dev->clip.y2;y++){
			vga_getscansegment(scroll_buffer,dev->clip.x1,y,len);
			vga_drawscansegment(scroll_buffer,dev->clip.x1+sc,y,len);
		}
	}else{
		/* Left */
		len=dev->clip.x2-dev->clip.x1+sc;
		for (y=dev->clip.y1;y<dev->clip.y2;y++){
			vga_getscansegment(scroll_buffer,dev->clip.x1-sc,y,len);
			vga_drawscansegment(scroll_buffer,dev->clip.x1,y,len);
		}
	}
	END_MOUSE
	return 1;
}

static int hscroll_linear(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	unsigned char *dest, *src;
	int y;
	int len;
	HSCROLL_CLIP_PREFACE
	SYNC
	ignore=NULL;
	if (sc>0){
		len=(dev->clip.x2-dev->clip.x1-sc)*vga_bytes;
		src=my_graph_mem+vga_linewidth*dev->clip.y1+dev->clip.x1*vga_bytes;
		dest=src+sc*vga_bytes;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			memmove(dest,src,len);
			dest+=vga_linewidth;
			src+=vga_linewidth;
		}
	}else{
		len=(dev->clip.x2-dev->clip.x1+sc)*vga_bytes;
		dest=my_graph_mem+vga_linewidth*dev->clip.y1+dev->clip.x1*vga_bytes;
		src=dest-sc*vga_bytes;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			memmove(dest,src,len);
			dest+=vga_linewidth;
			src+=vga_linewidth;
		}
	}
	END_MOUSE
	return 1;
}

static int vscroll_scansegment(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	int y;
	int len;
	VSCROLL_CLIP_PREFACE
	SYNC
	ignore=NULL;
	len=dev->clip.x2-dev->clip.x1;
	if (sc>0){
		/* Down */
		for (y=dev->clip.y2-1;y>=dev->clip.y1+sc;y--){
			vga_getscansegment(scroll_buffer, dev->clip.x1,y-sc,len);
			vga_drawscansegment(scroll_buffer,dev->clip.x1,y,len);
		}
	}else{
		/* Up */
		for (y=dev->clip.y1-sc;y<dev->clip.y2;y++){
			vga_getscansegment(scroll_buffer,dev->clip.x1,y,len);
			vga_drawscansegment(scroll_buffer, dev->clip.x1,y+sc,len);
		}
	}
	END_MOUSE
	return 1;
}

static int vscroll_linear(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	unsigned char *dest, *src;
	int y;
	int len;
	VSCROLL_CLIP_PREFACE
	SYNC
	ignore=NULL;
	len=(dev->clip.x2-dev->clip.x1)*vga_bytes;
	if (sc>0){
		/* Down */
		dest=my_graph_mem+(dev->clip.y2-1)*vga_linewidth+dev->clip.x1*vga_bytes;
		src=dest-vga_linewidth*sc;
		for (y=dev->clip.y2-dev->clip.y1-sc;y;y--){
			memcpy(dest,src,len);
			dest-=vga_linewidth;
			src-=vga_linewidth;
		}
	}else{
		/* Up */
		dest=my_graph_mem+dev->clip.y1*vga_linewidth+dev->clip.x1*vga_bytes;
		src=dest-vga_linewidth*sc;
		for (y=dev->clip.y2-dev->clip.y1+sc;y;y--){
			memcpy(dest,src,len);
			dest+=vga_linewidth;
			src+=vga_linewidth;
		}
	}
	END_MOUSE
	return 1;
}

static inline void get_row(unsigned char *bptr, int lina, int len)
{
	int page=lina>>16;
	int paga=lina&0xffff;
	int remains;
	
	my_setpage(page);
	remains=65536-paga;
	again:
	if (remains>=len){
		memcpy(bptr,my_graph_mem+paga,len);
		vga_page=page;
		return;
	}else{
		memcpy(bptr,my_graph_mem+paga,remains);
		paga=0;
		bptr+=remains;
		len-=remains;
		remains=65536;
		vga_setpage(++page);
		goto again;
	}
}

static int vscroll_paged(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	int dest,src;
	int y;
	int len;
	VSCROLL_CLIP_PREFACE
	SYNC
	ignore=NULL;
	len=(dev->clip.x2-dev->clip.x1)*vga_bytes;
	if (sc>0){
		/* Down */
		dest=(dev->clip.y2-1)*vga_linewidth+dev->clip.x1*vga_bytes;
		src=dest-vga_linewidth*sc;
		for (y=dev->clip.y2-dev->clip.y1-sc;y;y--){
			get_row(scroll_buffer, src,len);
			paged_memcpy(dest,scroll_buffer,len);
			dest-=vga_linewidth;
			src-=vga_linewidth;
		}
	}else{
		/* Up */
		dest=dev->clip.y1*vga_linewidth+dev->clip.x1*vga_bytes;
		src=dest-vga_linewidth*sc;
		for (y=dev->clip.y2-dev->clip.y1+sc;y;y--){
			get_row(scroll_buffer, src,len);
			paged_memcpy(dest,scroll_buffer,len);
			dest+=vga_linewidth;
			src+=vga_linewidth;
		}
	}
	END_MOUSE
	return 1;
}

static int hscroll_paged(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	int  dest,src;
	int y;
	int len;

	HSCROLL_CLIP_PREFACE
	SYNC	
	ignore=NULL;
	if (sc>0){
		len=(dev->clip.x2-dev->clip.x1-sc)*vga_bytes;
		src=vga_linewidth*dev->clip.y1+dev->clip.x1*vga_bytes;
		dest=src+sc*vga_bytes;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			get_row(scroll_buffer, src,len);
			paged_memcpy(dest,scroll_buffer,len);
			dest+=vga_linewidth;
			src+=vga_linewidth;
		}
	}else{
		len=(dev->clip.x2-dev->clip.x1+sc)*vga_bytes;
		dest=vga_linewidth*dev->clip.y1+dev->clip.x1*vga_bytes;
		src=dest-sc*vga_bytes;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			get_row(scroll_buffer, src,len);
			paged_memcpy(dest,scroll_buffer,len);
			dest+=vga_linewidth;
			src+=vga_linewidth;
		}
	}
	END_MOUSE
	return 1;
}

static void svga_set_clip_area(struct graphics_device *dev, struct rect *r)
{
	memcpy(&dev->clip, r, sizeof(struct rect));
	if (dev->clip.x1>=dev->clip.x2||dev->clip.y2<=dev->clip.y1||dev->clip.y2<=0||dev->clip.x2<=0||dev->clip.x1>=xsize
			||dev->clip.y1>=ysize){
		/* Empty region */
		dev->clip.x1=dev->clip.x2=dev->clip.y1=dev->clip.y2=0;
	}else{
		if (dev->clip.x1<0) dev->clip.x1=0;
		if (dev->clip.x2>xsize) dev->clip.x2=xsize;
		if (dev->clip.y1<0) dev->clip.y1=0;
		if (dev->clip.y2>ysize) dev->clip.y2=ysize;
	}
}

/* For modes where video memory is not directly accessible through svgalib */
static inline void fill_area_drawscansegment(struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	int xs;
	int col=*(unsigned char *)&color;
	
	FILL_CLIP_PREFACE
	SYNC
	xs=right-left;
	memset(scroll_buffer,col,xs);
	for (;top<bottom;top++){
		vga_drawscansegment(scroll_buffer,left,top,xs);
	}
	END_MOUSE
}

/* Emulates horizontal line by calling fill_area */
static void draw_hline_fill_area(struct graphics_device *dev, int left, int y, int right, long color)
{
	dev->drv->fill_area(dev,left,y,right,y+1,color);
}


/* Emulates vline by fill_area */
static void draw_vline_fill_area(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	dev->drv->fill_area(dev,x,top,x+1,bottom, color);
}

/* This does no clipping and is used only by the mouse code
 * length is in bytes, not in pixels
 */
static int drawscansegment_linear(unsigned char *colors, int x, int y, int length)
{
	unsigned char *ptr=my_graph_mem+vga_linewidth*y+vga_bytes*x;

	memcpy (ptr,colors,length);
	return 0;
}

/* This does no clipping and is used only by the mouse code
 * length is in bytes, not in pixels
 */
static int getscansegment_linear(unsigned char *colors, int x, int y, int length)
{
	unsigned char *ptr=my_graph_mem+vga_linewidth*y+vga_bytes*x;

	memcpy (colors, ptr, length);
	return 0;
}

/* This does no clipping and is used only by the mouse code
 * length is in bytes, not in pixels
 */
static int drawscansegment_paged(unsigned char *colors, int x, int y, int length)
{
	int lina=vga_linewidth*y+vga_bytes*x;

	paged_memcpy(lina, colors, length);
	return 0;
}

/* This does no clipping and is used only by the mouse code
 * length is in the bytes, not in pixels
 */
static int getscansegment_paged(unsigned char *colors, int x, int y, int length)
{
	int lina=vga_linewidth*y+vga_bytes*x;

	get_row(colors, lina, length);
	return 0;
}

static void svga_draw_bitmaps(struct graphics_device *dev, struct bitmap **hndls, int n
	,int x, int y)
{
	void (*draw_b)(struct graphics_device *, struct bitmap *, int, int);

	TEST_INACTIVITY
	if (x>=xsize||y>ysize) return;
	while(x+(*hndls)->x<=0&&n){
		x+=(*hndls)->x;
		n--;
		hndls++;
	}
	draw_b = dev->drv->draw_bitmap;
	while(n&&x<=xsize){
		draw_b(dev, *hndls, x, y);
		x+=(*hndls)->x;
		n--;
		hndls++;
	}
		
}

static void alloc_scroll_buffer(void)
{
	if (!scroll_buffer) scroll_buffer=mem_alloc(xsize*bmpixelsize);
}

static void setup_functions(void)
{

	if (accel_avail&ACCELFLAG_SETMODE){
		do_sync=1;
		vga_accel(ACCEL_SETMODE, BLITS_IN_BACKGROUND);
	}else do_sync=0;

	svga_driver.get_color=get_color_fn(svga_driver.depth);
	switch(vga_colors){
		case 2: internal(
				"2-color modes are not supported by\
 links as they are buggy in svgalib and incapable of colors");

		case 16:
		alloc_scroll_buffer();
		svga_driver.draw_bitmap=draw_bitmap_drawscansegment;
		svga_driver.hscroll=hscroll_scansegment;
		svga_driver.vscroll=vscroll_scansegment;
		svga_driver.flags |= GD_DONT_USE_SCROLL;
		svga_driver.fill_area=fill_area_drawscansegment;
		svga_driver.draw_hline=draw_hline_fill_area;
		svga_driver.draw_vline=draw_vline_fill_area;
		mouse_getscansegment=vga_getscansegment;
		mouse_drawscansegment=vga_drawscansegment;
		break;

		default:
		mouse_getscansegment=vga_getscansegment;
		mouse_drawscansegment=vga_drawscansegment;
		if (accel_avail&ACCELFLAG_PUTIMAGE){
			svga_driver.draw_bitmap=draw_bitmap_accel;
		}else if (vga_linear){
			svga_driver.draw_bitmap=draw_bitmap_linear;
		}else if (mode_x){
			svga_driver.draw_bitmap=draw_bitmap_drawscansegment;
		}else{
			svga_driver.draw_bitmap=draw_bitmap_paged;
		}
		
		if (accel_avail&ACCELFLAG_FILLBOX) svga_driver.fill_area=fill_area_accel_box;
		else if (accel_avail&ACCELFLAG_DRAWLINE) svga_driver.fill_area=fill_area_accel_lines;
		else if (vga_linear) svga_driver.fill_area=fill_area_linear;
		else if (mode_x) svga_driver.fill_area=fill_area_drawscansegment;
		else svga_driver.fill_area=fill_area_paged;
		
		if (accel_avail&ACCEL_DRAWLINE){
			svga_driver.draw_hline=draw_hline_accel_line;
			svga_driver.draw_vline=draw_vline_accel_line;
		}else if (accel_avail&ACCEL_FILLBOX){
			svga_driver.draw_hline=draw_hline_accel_box;
			svga_driver.draw_vline=draw_vline_accel_box;
		}else if (vga_linear){
			svga_driver.draw_hline=draw_hline_linear;
			svga_driver.draw_vline=draw_vline_linear;
		}else if (mode_x){
			svga_driver.draw_hline=draw_hline_fill_area;
			svga_driver.draw_vline=draw_vline_fill_area;
		}else{
			/* Paged memory access */
			svga_driver.draw_hline=draw_hline_paged;
			switch(vga_bytes)
			{
				case 1:
				svga_driver.draw_vline=draw_vline_paged_1;
				break;
#ifdef t2c
				case 2:
				svga_driver.draw_vline=draw_vline_paged_2;
				break;
#endif /* #ifdef t2c */

#ifdef t4c
				case 4:
				svga_driver.draw_vline=draw_vline_paged_4;
				break;
#endif /* #ifdef t4c */

				default:
				if (vga_bytes&(vga_bytes-1))
					svga_driver.draw_vline=draw_vline_paged;
				else
					svga_driver.draw_vline=draw_vline_paged_aligned;
				break;
			}
		}

		if (vga_colors>=256){
			if (vga_linear){
				mouse_drawscansegment=drawscansegment_linear;
				mouse_getscansegment=getscansegment_linear;
			}else if (!mode_x){
				mouse_drawscansegment=drawscansegment_paged;
				mouse_getscansegment=getscansegment_paged;
			}
		}

		if (accel_avail&ACCEL_SCREENCOPY){
			svga_driver.hscroll=hscroll_accel;
			svga_driver.vscroll=vscroll_accel;
		}else if (vga_linear){
			svga_driver.hscroll=hscroll_linear;
			svga_driver.vscroll=vscroll_linear;
			svga_driver.flags |= GD_DONT_USE_SCROLL;
		}else if (mode_x){
			alloc_scroll_buffer();
			svga_driver.hscroll=hscroll_scansegment;
			svga_driver.vscroll=vscroll_scansegment;
			svga_driver.flags |= GD_DONT_USE_SCROLL;
		}else{
			alloc_scroll_buffer();
			svga_driver.hscroll=hscroll_paged;
			svga_driver.vscroll=vscroll_paged;
			svga_driver.flags |= GD_DONT_USE_SCROLL;
		}
	}

}

void dump_mode_info_into_file(vga_modeinfo* i)
{
 FILE *f;

 f=fopen(".links_svga_modeinfo","w");
 if (!f) return;
 fprintf(f,"Resolution %d*%d\n",i->width,i->height);
 fprintf(f,"%d bytes per screen pixel\n",i->bytesperpixel);
 fprintf(f,"%d colors\n",i->colors);
 fprintf(f,"Linewidth %d bytes\n",i->linewidth);
 fprintf(f,"Maximum logical width %d bytes\n",i->maxlogicalwidth);
 fprintf(f,"Start address rangemask 0x%x\n",i->startaddressrange);
 fprintf(f,"Max. pixels per logical screen %d\n",i->maxpixels);
 fprintf(f,"bitblt %s\n",i->haveblit&HAVE_BITBLIT?"yes":"no");
 fprintf(f,"fillblt %s\n",i->haveblit&HAVE_FILLBLIT?"yes":"no");
 fprintf(f,"imageblt %s\n",i->haveblit&HAVE_IMAGEBLIT?"yes":"no");
 fprintf(f,"hlinelistblt %s\n",i->haveblit&HAVE_HLINELISTBLIT?"yes":"no");
 fprintf(f,"read/write page %s\n",i->flags&HAVE_RWPAGE?"yes":"no");
 fprintf(f,"Interlaced %s\n",i->flags&IS_INTERLACED?"yes":"no");
 fprintf(f,"Mode X layout %s\n",i->flags&IS_MODEX?"yes":"no");
 fprintf(f,"Dynamically loaded %s\n",i->flags&IS_DYNAMICMODE?"yes":"no");
 fprintf(f,"Linear: %s\n",vga_linear?"yes":"no");
 fprintf(f,"Misordered %s\n",i->flags&RGB_MISORDERED?"yes":"no");
 if (!i->flags&EXT_INFO_AVAILABLE){
	 fprintf(f,"Old svgalib, extended info is not available\n");
 }else{
	 fprintf(f,"Chiptype 0x%x\n",i->chiptype);
	 fprintf(f,"Memory %dKB\n",i->memory);
	 fprintf(f,"Linewidth Unit %d\n",i->linewidth_unit);
	 fprintf(f,"Aperture size %d\n",i->aperture_size);
 }
 fprintf(f,"Accelerated putimage: %s\n",svga_driver.draw_bitmap==draw_bitmap_accel?"yes":"no");
 fclose(f);
}

static void svgalib_key_in(void *p, struct event *ev, int size)
{
	if (size != sizeof(struct event) || ev->ev != EV_KBD) return;
	if ((ev->y & KBD_ALT) && ev->x >= '0' && ev->x <= '9') {
		switch_virtual_device((ev->x - '1' + 10) % 10);
		return;
	}
	if (svga_driver.codepage!=utf8_table&&(ev->x)>=128&&(ev->x)<=255)
		if ((ev->x=cp2u(ev->x,svga_driver.codepage)) == -1) return;
	if (current_virtual_device && current_virtual_device->keyboard_handler) current_virtual_device->keyboard_handler(current_virtual_device, ev->x, ev->y);
}

#define BUTTON_MASK (MOUSE_RIGHTBUTTON | MOUSE_MIDDLEBUTTON | MOUSE_LEFTBUTTON )

static inline void mouse_aggregate_flush(void)
{
	if (!mouse_aggregate_flag) return;
	mouse_aggregate_flag=0;
	if (!current_virtual_device) return;
	if (!current_virtual_device->mouse_handler) return;
	current_virtual_device->mouse_handler(current_virtual_device, mouse_x, mouse_y, mouse_aggregate_action);
}

/* Only calls appropriate callbacks, doesn't draw anything. */
static void mouse_event_handler(int button, int dx, int dy, int dz, int drx, int dry, int drz)
{
	int moved,old_mouse_x,old_mouse_y;
	void (*mh)(struct graphics_device *, int, int, int);
	struct graphics_device *cd=current_virtual_device;
	
	mh=cd?cd->mouse_handler:NULL;
	old_mouse_x=mouse_x;
	old_mouse_y=mouse_y;

	mouse_x+=dx;
	if (mouse_x>=xsize) mouse_x=xsize-1;
	else if (mouse_x<0) mouse_x=0;

	mouse_y+=dy;
	if (mouse_y>=ysize) mouse_y=ysize-1;
	else if (mouse_y<0) mouse_y=0;

	moved=(old_mouse_x!=mouse_x||old_mouse_y!=mouse_y);
	
	/* Test movement without buttons */
	if (!(mouse_buttons & BUTTON_MASK) &&moved ){
		mouse_aggregate_flag=1;
		mouse_aggregate_action=B_MOVE;
	}
			
	/* Test presses */
	if ((button&MOUSE_LEFTBUTTON)&&!(mouse_buttons&MOUSE_LEFTBUTTON)){
		mouse_aggregate_flush();
		if (mh) mh(cd,mouse_x, mouse_y,B_LEFT|B_DOWN);
	}
	if ((button&MOUSE_MIDDLEBUTTON)&&!(mouse_buttons&MOUSE_MIDDLEBUTTON)){
		mouse_aggregate_flush();
		if (mh) mh(cd,mouse_x, mouse_y,B_MIDDLE|B_DOWN);
	}
	if ((button&MOUSE_RIGHTBUTTON)&&!(mouse_buttons&MOUSE_RIGHTBUTTON)){
		mouse_aggregate_flush();
		if (mh) mh(cd,mouse_x, mouse_y,B_RIGHT|B_DOWN);
	}

	/* Test releases */
	if (!(button&MOUSE_LEFTBUTTON)&&(mouse_buttons&MOUSE_LEFTBUTTON)){
		mouse_aggregate_flush();
		if (mh) mh(cd,mouse_x, mouse_y,B_LEFT|B_UP);
	}
	if (!(button&MOUSE_MIDDLEBUTTON)&&(mouse_buttons&MOUSE_MIDDLEBUTTON)){
		mouse_aggregate_flush();
		if (mh) mh(cd,mouse_x, mouse_y,B_MIDDLE|B_UP);
	}
	if (!(button&MOUSE_RIGHTBUTTON)&&(mouse_buttons&MOUSE_RIGHTBUTTON)){
		mouse_aggregate_flush();
		if (mh) mh(cd,mouse_x, mouse_y,B_RIGHT|B_UP);
	}
	
	if (drx < 0 && mh) mh(cd, mouse_x, mouse_y, B_MOVE | B_WHEELUP);
	if (drx > 0 && mh) mh(cd, mouse_x, mouse_y, B_MOVE | B_WHEELDOWN);

	if (dry < 0 && mh) mh(cd, mouse_x, mouse_y, B_MOVE | B_WHEELLEFT);
	if (dry > 0 && mh) mh(cd, mouse_x, mouse_y, B_MOVE | B_WHEELRIGHT);

	/* Test drag */
	if (! ((button^mouse_buttons) & BUTTON_MASK ) && moved && (button &
		BUTTON_MASK)){
		mouse_aggregate_flag=1;
		mouse_aggregate_action=(button&MOUSE_RIGHTBUTTON?B_RIGHT:0)|
				      (button&MOUSE_MIDDLEBUTTON?B_MIDDLE:0)|
				      (button&MOUSE_LEFTBUTTON?B_LEFT:0)|
				      B_DRAG;
	}
	mouse_buttons=button;
}

#undef BUTTON_MASK

/* Flushes the background_buffer onscreen where it was originally taken from. */
static void place_mouse_background(void)
{
	struct bitmap bmp;

	bmp.x=arrow_width;
	bmp.y=arrow_height;
	bmp.skip=arrow_width*bmpixelsize;
	bmp.data=background_buffer;

	{
		struct graphics_device * current_virtual_device_backup;

		current_virtual_device_backup=current_virtual_device;
		current_virtual_device=mouse_graphics_device;
		svga_driver.draw_bitmap(mouse_graphics_device, &bmp, background_x,
			background_y);
		current_virtual_device=current_virtual_device_backup;
	}

}

/* Only when the old and new mouse don't interfere. Using it on interfering mouses would
 * cause a flicker.
 */
static void hide_mouse(void)
{

	global_mouse_hidden=1;
	place_mouse_background();
}

/* Gets background from the screen (clipping provided only right and bottom) to the
 * passed buffer.
 */
static void get_mouse_background(unsigned char *buffer_ptr)
{
	int width,height,skip,x,y;

	skip=arrow_width*bmpixelsize;

	x=mouse_x;
	y=mouse_y;

	width=bmpixelsize*(arrow_width+x>xsize?xsize-x:arrow_width);
	height=arrow_height+y>ysize?ysize-y:arrow_height;

	SYNC
	for (;height;height--){
		mouse_getscansegment(buffer_ptr,x,y,width);
		buffer_ptr+=skip;
		y++;
	}
}

/* Overlays the arrow's image over the mouse_buffer
 * Doesn't draw anything into the screen
 */
static void render_mouse_arrow(void)
{
	int x,y, reg0, reg1;
	unsigned char *mouse_ptr=mouse_buffer;
	int *arrow_ptr=arrow;

	for (y=arrow_height;y;y--){
		reg0=*arrow_ptr;
		reg1=arrow_ptr[1];
		arrow_ptr+=2;
		for (x=arrow_width;x;)
		{
			int mask=1<<(--x);

			if (reg0&mask)
				memcpy (mouse_ptr, &mouse_black, bmpixelsize);
			else if (reg1&mask)
				memcpy (mouse_ptr, &mouse_white, bmpixelsize);
			mouse_ptr+=bmpixelsize;
		}
	}
}

static void place_mouse(void)
{
	struct bitmap bmp;

	bmp.x=arrow_width;
	bmp.y=arrow_height;
	bmp.skip=arrow_width*bmpixelsize;
	bmp.data=mouse_buffer;	
	{
		struct graphics_device * current_graphics_device_backup;

		current_graphics_device_backup=current_virtual_device;
		current_virtual_device=mouse_graphics_device;
		/* We do need to worry about SYNC because draw_bitmap already
		 * does it (if necessary)
		 */
		svga_driver.draw_bitmap(mouse_graphics_device, &bmp, mouse_x, mouse_y);
		current_virtual_device=current_graphics_device_backup;
	}
	global_mouse_hidden=0;
}

/* Only when the old and the new mouse positions do not interfere. Using this routine
 * on interfering positions would cause a flicker.
 */
static void show_mouse(void)
{

	get_mouse_background(background_buffer);
	background_x=mouse_x;
	background_y=mouse_y;
	memcpy(mouse_buffer,background_buffer,bmpixelsize*arrow_area);
	render_mouse_arrow();
	place_mouse();
}

/* Doesn't draw anything into the screen
 */
static void put_and_clip_background_buffer_over_mouse_buffer(void)
{
	unsigned char *bbufptr=background_buffer, *mbufptr=mouse_buffer;
	int left=background_x-mouse_x;
	int top=background_y-mouse_y;
	int right,bottom;
	int bmpixelsizeL=bmpixelsize;
	int number_of_bytes;
	int byte_skip;

	right=left+arrow_width;
	bottom=top+arrow_height;

	if (left<0){
		bbufptr-=left*bmpixelsizeL;
		left=0;
	}
	if (right>arrow_width) right=arrow_width;
	if (top<0){
		bbufptr-=top*bmpixelsizeL*arrow_width;
		top=0;
	}
	if (bottom>arrow_height) bottom=arrow_height;
	mbufptr+=bmpixelsizeL*(left+arrow_width*top);
	byte_skip=arrow_width*bmpixelsizeL;
	number_of_bytes=bmpixelsizeL*(right-left);
	for (;top<bottom;top++){
		memcpy(mbufptr,bbufptr,number_of_bytes);
		mbufptr+=byte_skip;
		bbufptr+=byte_skip;
	}
}

/* This draws both the contents of background_buffer and mouse_buffer in a scan
 * way (left-right, top-bottom), so the flicker is reduced.
 */
static inline void place_mouse_composite(void)
{
	int mouse_left=mouse_x;
	int mouse_top=mouse_y;
	int background_left=background_x;
	int background_top=background_y;
	int mouse_right=mouse_left+arrow_width;
	int mouse_bottom=mouse_top+arrow_height;
	int background_right=background_left+arrow_width;
	int background_bottom=background_top+arrow_height;
	int skip=arrow_width*bmpixelsize;
	int background_length,mouse_length;
	unsigned char *mouse_ptr=mouse_buffer,*background_ptr=background_buffer;
	int yend;

	/* First let's sync to the beam - wait for the beginning of vertical retrace
	 * (it would be better to wait for the beginning of the blank, however,
	 * svgalib doesn't provide it as VGA and SVGA cards don't provide it
	 */

	/* This will probably not make any good anyway.
	if (vga_colors>=256)
		vga_waitretrace();
	*/
	
	if (mouse_bottom>ysize) mouse_bottom=ysize;
	if (background_bottom>ysize) background_bottom=ysize;

	SYNC
	/* We have to sync because mouse_drawscansegment does not wait for
	 * the accelerator to finish. But we never waste time here because
	 * mouse_drawscansegment is never accelerated.
	 */
	/* Let's do the top part */
	if (background_top<mouse_top){
		/* Draw the background */
		background_length=background_right>xsize?xsize-background_left
			:arrow_width;
		for (;background_top<mouse_top;background_top++){
			mouse_drawscansegment(background_ptr,background_left
				,background_top,background_length*bmpixelsize);
			background_ptr+=skip;
		}
			
	}else if (background_top>mouse_top){
		/* Draw the mouse */
		mouse_length=mouse_right>xsize
			?xsize-mouse_left:arrow_width;
		for (;mouse_top<background_top;mouse_top++){
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,mouse_length*bmpixelsize);
			mouse_ptr+=skip;
		}
	}

	/* Let's do the middle part */
	yend=mouse_bottom<background_bottom?mouse_bottom:background_bottom;
	if (background_left<mouse_left){
		/* Draw background, mouse */
		mouse_length=mouse_right>xsize?xsize-mouse_left:arrow_width;
		for (;mouse_top<yend;mouse_top++){
			mouse_drawscansegment(background_ptr,background_left,mouse_top
				,(mouse_left-background_left)*bmpixelsize);
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,mouse_length*bmpixelsize);
			mouse_ptr+=skip;
			background_ptr+=skip;
		}
			
	}else{
		int l1, l2, l3;
		
		/* Draw mouse, background */
		mouse_length=mouse_right>xsize?xsize-mouse_left:arrow_width;
		background_length=background_right-mouse_right;
		if (background_length+mouse_right>xsize)
			background_length=xsize-mouse_right;
		l1=mouse_length*bmpixelsize;
		l2=(mouse_right-background_left)*bmpixelsize;
		l3=background_length*bmpixelsize;
		for (;mouse_top<yend;mouse_top++){
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,l1);
			if (background_length>0)
				mouse_drawscansegment(
					background_ptr +l2,
				       	mouse_right,mouse_top ,l3);
			mouse_ptr+=skip;
			background_ptr+=skip;
		}
	}

	if (background_bottom<mouse_bottom){
		/* Count over bottoms! tops will be invalid! */
		/* Draw mouse */
		mouse_length=mouse_right>xsize?xsize-mouse_left
			:arrow_width;
		for (;background_bottom<mouse_bottom;background_bottom++){
			mouse_drawscansegment(mouse_ptr,mouse_left,background_bottom
				,mouse_length*bmpixelsize);
			mouse_ptr+=skip;
		}
	}else{
		/* Draw background */
		background_length=background_right>xsize?xsize-background_left
			:arrow_width;
		for (;mouse_bottom<background_bottom;mouse_bottom++){
			mouse_drawscansegment(background_ptr,background_left,mouse_bottom
				,background_length*bmpixelsize);
			background_ptr+=skip;
		}
	}
}

/* This moves the mouse a sophisticated way when the old and new position of the
 * cursor overlap.
 */
static inline void redraw_mouse_sophisticated()
{
	int new_background_x, new_background_y;

	get_mouse_background(mouse_buffer);
	put_and_clip_background_buffer_over_mouse_buffer();
	memcpy(new_background_buffer,mouse_buffer,bmpixelsize*arrow_area);
	new_background_x=mouse_x;
	new_background_y=mouse_y;
	render_mouse_arrow();
	place_mouse_composite();
	memcpy(background_buffer,new_background_buffer,bmpixelsize*arrow_area);
	background_x=new_background_x;
	background_y=new_background_y;
}

static void redraw_mouse(void){
	
	if (flags) return; /* We are not drawing */
	if (mouse_x!=background_x||mouse_y!=background_y){
		if (RECTANGLES_INTERSECT(
			background_x, background_x+arrow_width,
			mouse_x, mouse_x+arrow_width,
			background_y, background_y+arrow_height,
			mouse_y, mouse_y+arrow_height)){
			redraw_mouse_sophisticated();
		}else{
			/* Do a normal hide/show */
			get_mouse_background(mouse_buffer);
			memcpy(new_background_buffer,
				mouse_buffer,arrow_area*bmpixelsize);
			render_mouse_arrow();
			hide_mouse();
			place_mouse();
			memcpy(background_buffer,new_background_buffer
				,arrow_area*bmpixelsize);
			background_x=mouse_x;
			background_y=mouse_y;
		}
	}
}


static unsigned char *svga_get_driver_param(void)
{
	return svga_driver_param;
}

void generate_palette_outer(void)
{
	if (vga_colors==16||vga_colors==256){
		struct irgb *palette;
		palette=mem_alloc(vga_colors*sizeof(*palette));
		generate_palette(palette);
		set_palette(palette);
		mem_free(palette);
		/* Palette in SVGAlib will be always color cube */
	}
}

/* This is to be called after vga_setmode and sets up accelerator,
 * svgalib functions */
void setup_mode(int mode)
{
	vga_modeinfo *i;
	int sig;

	accel_avail=vga_ext_set(VGA_EXT_AVAILABLE,VGA_AVAIL_ACCEL);
	if (vga_ext_set(VGA_EXT_AVAILABLE, VGA_AVAIL_SET)&VGA_CLUT8){
		vga_ext_set(VGA_EXT_SET,VGA_CLUT8);
		palette_depth=8;
	}else palette_depth=6;
	i=vga_getmodeinfo(mode);
	vga_bytes=i->bytesperpixel;
	bmpixelsize=vga_bytes?vga_bytes:1;
	vga_misordered=!!(i->flags&RGB_MISORDERED);
	mode_x=!!(i->flags&IS_MODEX);
	vga_linear=!!(i->flags&IS_LINEAR);
	/*
	if (!vga_linear && i->flags&CAPABLE_LINEAR && 0<=vga_setlinearaddressing()) vga_linear=1; 
	*/
	my_graph_mem=vga_getgraphmem();
	svga_driver.x = xsize=i->width;
	svga_driver.y = ysize=i->height;
	aspect_native=(196608*xsize+(ysize<<1))/(ysize<<2);
	aspect=aspect_native*bfu_aspect+0.5;
	vga_colors=i->colors;
	if (xsize==320&&ysize==200&&vga_colors==256) vga_linear=1; /* The mode
		does not need to page :-) */
	vga_linewidth=i->linewidth;
	if (!vga_linear){
		vga_page=-1;
	}
	vga_misordered=!!i->flags&RGB_MISORDERED;
	/*dump_mode_info_into_file(i);*/
	svga_driver.depth=0;
	svga_driver.depth|=vga_misordered<<8;
	switch (vga_colors){
		case 16:
		sig=4;
		break;
		case 256:
		sig=8;
		break;
		case 32768:
		sig=15;
		break;
		case 65536:
		sig=16;
		break;
		case 16777216:
		sig=24;
		break;
		default:
		sig=0; /* Only to suppress warning */
		break;
	}
	svga_driver.depth|=sig<<3;
	svga_driver.depth|=bmpixelsize;

	/* setup_functions uses depth. */
	setup_functions();
	generate_palette_outer();
}

void vtswitch_handler(void * nothing)
{
	int oktowrite;

	nothing=nothing;
	vga_unlockvc();
	vga_lockvc();
	oktowrite=vga_oktowrite();
	if (!oktowrite&&!flags){
		backup_virtual_device=current_virtual_device;
		current_virtual_device=NULL;
	}
	if (flags==1&&oktowrite) current_virtual_device=backup_virtual_device;
	flags=(flags&~1)|!oktowrite;
	svgalib_timer_id=install_timer(100,vtswitch_handler, NULL);
}

void svga_ctrl_c(struct itrm *i)
{
	kbd_ctrl_c();
}

/* Param: one string which is to be compared with one from modes.
 * Copies the svga_driver into gr_driver.
 * Returns: 	0 OK
 *		1 Passed mode unknown by svga_driver
 *		2 Passed mode unknown by svgalib
 * mikulas: Change: Returns:	NULL: OK
 *				non-null: poiner to string with error
 *					  description, string must be freed
 */
static unsigned char *svga_init_driver(unsigned char *param, unsigned char *display)
{
	int j;

	kbd_set_raw = 0;

	vga_init();
	svga_driver.flags |= GD_NEED_CODEPAGE;
	utf8_table=get_cp_index("UTF-8");
	j = 0;

	svga_driver_param=NULL;
	if (!param || !*param) goto not_found;
	svga_driver_param=stracpy(param);
	for (j=0;j<sizeof(modes)/sizeof(*modes);j++)
		if (!_stricmp(modes[j].name,param)) goto found;
	j = 1;
	not_found:
	{
		unsigned char *m = init_str();
		int l = 0;
		int f = 0;
		if (j) {
			add_to_str(&m, &l, "Video mode ");
			add_to_str(&m, &l, param);
			add_to_str(&m, &l, " not supported by ");
			add_to_str(&m, &l, j == 2 ? "your video card" : "svgalib");
			add_to_str(&m, &l, ".\n");
		} else add_to_str(&m, &l, "There is no default video mode.\n");
		for (j=0;j<sizeof(modes)/sizeof(*modes);j++) if (vga_hasmode(modes[j].number)) {
			if (f) add_to_str(&m, &l, ", ");
			else f = 1, add_to_str(&m, &l, "The following modes are supported:\n");
			add_to_str(&m, &l, modes[j].name);
		}
		if (f) add_to_str(&m, &l, "\nUse -mode switch to set video mode mode.\n");
		else add_to_str(&m, &l, "There are no supported video modes. Links can't run on svgalib.\n");
		if(svga_driver_param)mem_free(svga_driver_param),svga_driver_param=NULL;
		return m;
		
	}
	found:
	if (!vga_hasmode(modes[j].number)) {
		j = 2;
		goto not_found;
	}
	if (init_virtual_devices(&svga_driver, NUMBER_OF_DEVICES))
	{
		if(svga_driver_param)mem_free(svga_driver_param),svga_driver_param=NULL;
		return stracpy("Allocation of virtual devices failed.\n");
	}
	if ((vga_getmousetype()&MOUSE_TYPE_MASK)==MOUSE_NONE)
		{
			vga_setmousesupport(0);
			mouse_works=0;
		}else{
			vga_setmousesupport(1);
			mouse_works=1;
		}
	vga_lockvc();
	svgalib_timer_id=install_timer(100,vtswitch_handler,NULL);
	if (vga_runinbackground_version()>=1) vga_runinbackground(1);
	vga_setmode(vga_mode=modes[j].number);
	setup_mode(modes[j].number);
	handle_svgalib_keyboard((void (*)(void *, unsigned char *, int))svgalib_key_in);

	if (mouse_works){
		mouse_buffer=mem_alloc(bmpixelsize*arrow_area);
		background_buffer=mem_alloc(bmpixelsize*arrow_area);
		new_background_buffer=mem_alloc(bmpixelsize*arrow_area);
		mouse_black=svga_driver.get_color(0);
		mouse_white=svga_driver.get_color(0xffffff);
		mouse_graphics_device=svga_driver.init_device();
		virtual_devices[0] = NULL;
		global_mouse_hidden=1;
		background_x=mouse_x=xsize>>1;
		background_y=mouse_y=ysize>>1;
		show_mouse();
		mouse_seteventhandler(mouse_event_handler);
	}else{
		global_mouse_hidden=1;
		/* To ensure hide_mouse and show_mouse will do nothing */
	}
	signal(SIGPIPE, SIG_IGN);
	install_signal_handler(SIGINT, (void (*)(void *))svga_ctrl_c, ditrm, 0);
	return NULL;
}

/* Return value:	0 alloced on heap
 *			1 alloced in vidram
 *			2 alloced in X server shm
 */
static int svga_get_filled_bitmap(struct bitmap *dest, long color)
{
	int n=dest->x*dest->y*bmpixelsize;

	dest->data=mem_alloc(n);
	pixel_set(dest->data,n,&color);
	dest->skip=dest->x*bmpixelsize;
	dest->flags=0;
	return 0;
}

/* Return value:	0 alloced on heap
 *			1 alloced in vidram
 *			2 alloced in X server shm
 */
static int svga_get_empty_bitmap(struct bitmap *dest)
{
	dest->data=mem_alloc(dest->x*dest->y*bmpixelsize);
	dest->skip=dest->x*bmpixelsize;
	dest->flags=0;
	return 0;
}

int vga_block(struct graphics_device *dev)
{
	int overridden;

	overridden=(flags>>1)&1;
	if (!overridden){
		if (!(flags&1)){
			backup_virtual_device=current_virtual_device;
			current_virtual_device=NULL;
		}
		svgalib_block_itrm(ditrm);
		if (mouse_works){
			hide_mouse();
			/* mouse_close(); This is not necessary as it is
				handled by vga_setmode(TEXT). */
		}
		vga_setmode(TEXT);
	}
	flags|=2;
	return overridden;
}

void vga_unblock(struct graphics_device *dev)
{
#ifdef DEBUG
	if (current_virtual_device) {
		internal("vga_unblock called without vga_block");
		return;
	}
#endif /* #ifdef DEBUG */
	flags&=~2;
	if (!flags) current_virtual_device=backup_virtual_device;
	vga_setmousesupport(1);
	vga_setmode(vga_mode);
	setup_mode(vga_mode);
	if (mouse_works){
		show_mouse();
		mouse_seteventhandler(mouse_event_handler);
	}
	svgalib_unblock_itrm(ditrm);
	if (current_virtual_device) current_virtual_device->redraw_handler(current_virtual_device
			,&current_virtual_device->size);
}

void *svga_prepare_strip(struct bitmap *bmp, int top, int lines)
{
	return ((char *)bmp->data)+bmp->skip*top;
}


void svga_commit_strip(struct bitmap *bmp, int top, int lines)
{
	return;
}

/* This is a nasty hack */
#undef select

int vga_select(int  n,  fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
			      struct timeval *timeout)
{
	int retval,i;

	if (mouse_works&&!(flags&2)){
		/* The second flag here is to suppress mouse wait
		 * in blocked state */
		retval=vga_waitevent(VGA_MOUSEEVENT,readfds, writefds,
				exceptfds, timeout);
		if (retval<0) return retval;
		if (retval&VGA_MOUSEEVENT){
			mouse_aggregate_flush();
			redraw_mouse();
			check_bottom_halves();
		}
		retval=0;
		for (i=0;i<n;i++){
			if (readfds&&FD_ISSET(i,readfds)) retval++;
			if (writefds&&FD_ISSET(i,writefds)) retval++;
			if (exceptfds&&FD_ISSET(i,exceptfds)) retval++;
		}
		return retval;
	}else{
		return select(n,readfds, writefds, exceptfds, timeout);
	}
}

void svga_put_to_clipboard(struct graphics_device *gd, char *string,int length)
{
}

void svga_request_clipboard(struct graphics_device *gd)
{
}

unsigned char *svga_get_from_clipboard(struct graphics_device *gd)
{
        return NULL;
}

struct graphics_driver svga_driver={
	"svgalib",
	svga_init_driver,
	init_virtual_device,
	shutdown_virtual_device,
	svga_shutdown_driver,
	svga_get_driver_param,
	svga_get_empty_bitmap,
	svga_get_filled_bitmap,
	svga_register_bitmap,
	svga_prepare_strip,
	svga_commit_strip,
	svga_unregister_bitmap,
	NULL, /* svga_draw_bitmap */
	svga_draw_bitmaps,
	NULL, /* get_color */
	NULL, /* fill_area */
	NULL, /* draw_hline */
	NULL, /* draw_vline */
	NULL, /* hscroll */
	NULL, /* vscroll */
	svga_set_clip_area,
	vga_block, /* block */
	vga_unblock, /* unblock */
        NULL, /* set_title */
        svga_put_to_clipboard,
        svga_request_clipboard,
        svga_get_from_clipboard,
	0,				/* depth */
	0, 0,				/* size */
	0,				/* flags */
};

#define select vga_select

#endif /* GRDRV_SVGALIB */
