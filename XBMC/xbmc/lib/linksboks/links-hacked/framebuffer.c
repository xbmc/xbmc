/* framebuffer.c
 * Linux framebuffer code
 * (c) 2002 Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include "cfg.h"

#ifdef GRDRV_FB

#define USE_GPM_DX

/* #define FB_DEBUG */
/* #define SC_DEBUG */

#if defined(FB_DEBUG) || defined(SC_DEBUG)
	#define MESSAGE(a) fprintf(stderr,"%s",a);
#endif

#ifdef TEXT
#undef TEXT
#endif

#include "links.h"

#include <gpm.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <signal.h>

#include "arrow.inc"

#ifdef GPM_HAVE_SMOOTH
#define gpm_smooth GPM_SMOOTH
#else
#define gpm_smooth 0
#endif

#define TTY 0

#ifndef USE_GPM_DX
int fb_txt_xsize, fb_txt_ysize;
struct winsize fb_old_ws;
struct winsize fb_new_ws;
int fb_old_ws_v;
int fb_msetsize;
#endif
int fb_hgpm;

int fb_console;

struct itrm *fb_kbd;

struct graphics_device *fb_old_vd;

int fb_handler;
char *fb_mem;
int fb_mem_size,fb_linesize,fb_bits_pp,fb_pixelsize;
int fb_xsize,fb_ysize;
int fb_colors, fb_palette_colors;
struct fb_var_screeninfo vi;
struct fb_fix_screeninfo fi;

void fb_draw_bitmap(struct graphics_device *dev,struct bitmap* hndl, int x, int y);

static unsigned char *fb_driver_param;
struct graphics_driver fb_driver;
int have_cmap=0;
volatile int fb_active=1;

struct palette
{
	unsigned short *red;
	unsigned short *green;
	unsigned short *blue;
};

struct palette old_palette;
struct palette global_pal;
static struct vt_mode vt_mode,vt_omode;

struct fb_var_screeninfo oldmode;

static volatile int in_gr_operation;

/* mouse */
static int mouse_x, mouse_y;		/* mouse pointer coordinates */
static int mouse_black, mouse_white;
static int background_x, background_y; /* Where was the mouse background taken from */
static unsigned char *mouse_buffer, *background_buffer, *new_background_buffer;
static struct graphics_device *mouse_graphics_device;
static int global_mouse_hidden;
static int utf8_table;


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

#define START_GR in_gr_operation=1;
#define END_GR	\
		END_MOUSE\
		in_gr_operation=0;\
		if (!fb_active)ioctl(TTY,VT_RELDISP,1);
		

#define NUMBER_OF_DEVICES	10

#define TEST_INACTIVITY if (!fb_active||dev!=current_virtual_device) return;

#define TEST_INACTIVITY_0 if (!fb_active||dev!=current_virtual_device) return 0;

#define RECTANGLES_INTERSECT(xl0, xh0, xl1, xh1, yl0, yh0, yl1, yh1) (\
				   (xl0)<(xh1)\
				&& (xl1)<(xh0)\
				&& (yl0)<(yh1)\
				&& (yl1)<(yh0))

/* This assures that x, y, xs, ys, data will be sane according to clipping
 * rectangle. If nothing lies within this rectangle, the current function
 * returns. The data pointer is automatically advanced by this macro to reflect
 * the right position to start with inside the bitmap. */
#define	CLIP_PREFACE \
	int mouse_hidden;\
	int xs=hndl->x,ys=hndl->y;\
        char *data=hndl->data;\
\
 	TEST_INACTIVITY\
        if (x>=dev->clip.x2||x+xs<=dev->clip.x1) return;\
        if (y>=dev->clip.y2||y+ys<=dev->clip.y1) return;\
        if (x+xs>dev->clip.x2) xs=dev->clip.x2-x;\
        if (y+ys>dev->clip.y2) ys=dev->clip.y2-y;\
        if (dev->clip.x1-x>0){\
                xs-=(dev->clip.x1-x);\
                data+=fb_pixelsize*(dev->clip.x1-x);\
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
	START_GR\
	TEST_MOUSE (x,x+xs,y,y+ys)


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
	START_GR\
	TEST_MOUSE(left,right,top,bottom)
	

#define HLINE_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY\
	if (y<dev->clip.y1||y>=dev->clip.y2||right<=dev->clip.x1||left>=dev->clip.x2) return;\
	if (left<dev->clip.x1) left=dev->clip.x1;\
	if (right>dev->clip.x2) right=dev->clip.x2;\
	if (left>=right) return;\
	START_GR\
	TEST_MOUSE (left,right,y,y+1)
	
#define VLINE_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY\
	if (x<dev->clip.x1||x>=dev->clip.x2||top>=dev->clip.y2||bottom<=dev->clip.y1) return;\
	if (top<dev->clip.y1) top=dev->clip.y1;\
	if (bottom>dev->clip.y2) bottom=dev->clip.y2;\
	if (top>=bottom) return;\
	START_GR\
	TEST_MOUSE(x,x+1,top,bottom)

#define HSCROLL_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY_0\
	if (!sc) return 0;\
	if (sc>(dev->clip.x2-dev->clip.x1)||-sc>(dev->clip.x2-dev->clip.x1))\
		return 1;\
	START_GR\
	TEST_MOUSE (dev->clip.x1,dev->clip.x2,dev->clip.y1,dev->clip.y2)
		
#define VSCROLL_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY_0\
	if (!sc) return 0;\
	if (sc>dev->clip.y2-dev->clip.y1||-sc>dev->clip.y2-dev->clip.y1) return 1;\
	START_GR\
	TEST_MOUSE (dev->clip.x1, dev->clip.x2, dev->clip.y1, dev->clip.y2)\
	
	
/* n is in bytes. dest must begin on pixel boundary. If n is not a whole number of pixels, rounding is
 * performed downwards.
 */
static void inline pixel_set(unsigned char *dest, int n,void * pattern)
{
	int a;

	switch(fb_pixelsize)
	{
		case 1:
		memset(dest,*(char *)pattern,n);
		break;

		case 2:
		{
#ifdef t2c
			short v=*(t2c *)pattern;
			int a;
			
			for (a=0;a<(n>>1);a++) ((t2c *)dest)[a]=v;
#else
			unsigned char a,b;
			int i;
			
			a=*(char*)pattern;
			b=((char*)pattern)[1];
			for (i=0;i<=n-2;i+=2){
				dest[i]=a;
				dest[i+1]=b;
			}
#endif
		}
		break;

		case 3:
		{
			unsigned char a,b,c;
			int i;
			
			a=*(char*)pattern;
			b=((char*)pattern)[1];
			c=((char*)pattern)[2];
			for (i=0;i<=n-3;i+=3){
				dest[i]=a;
				dest[i+1]=b;
				dest[i+2]=c;
			}
		}
		break;

		case 4:
		{
#ifdef t4c
			long v=*(t4c *)pattern;
			int a;
			
			for (a=0;a<(n>>2);a++) ((t4c *)dest)[a]=v;
#else
			unsigned char a,b,c,d;
			int i;
			
			a=*(char*)pattern;
			b=((char*)pattern)[1];
			c=((char*)pattern)[2];
			d=((char*)pattern)[3];
			for (i=0;i<=n-4;i+=4){
				dest[i]=a;
				dest[i+1]=b;
				dest[i+2]=c;
				dest[i+3]=d;
			}
#endif
		}
		break;

		default:
		for (a=0;a<n/fb_pixelsize;a++,dest+=fb_pixelsize) memcpy(dest,pattern,fb_pixelsize);
		break;
	}
}

static void redraw_mouse(void);

static void fb_mouse_move(int dx, int dy)
{
	struct event ev;
	mouse_x += dx;
	mouse_y += dy;
	ev.ev = EV_MOUSE;
	if (mouse_x >= fb_xsize) mouse_x = fb_xsize - 1;
	if (mouse_y >= fb_ysize) mouse_y = fb_ysize - 1;
	if (mouse_x < 0) mouse_x = 0;
	if (mouse_y < 0) mouse_y = 0;
	ev.x = mouse_x;
	ev.y = mouse_y;
	ev.b = B_MOVE;
	if (!current_virtual_device) return;
	if (current_virtual_device->mouse_handler) current_virtual_device->mouse_handler(current_virtual_device, ev.x, ev.y, ev.b);
	redraw_mouse();
}

static void fb_key_in(void *p, struct event *ev, int size)
{
	if (size != sizeof(struct event) || ev->ev != EV_KBD) return;
	if ((ev->y & KBD_ALT) && ev->x >= '0' && ev->x <= '9') {
		switch_virtual_device((ev->x - '1' + 10) % 10);
		return;
	}
	if (!current_virtual_device) return;
	if (!ev->y && ev->x == KBD_F5) fb_mouse_move(-3, 0);
	else if (!ev->y && ev->x == KBD_F6) fb_mouse_move(0, 3);
	else if (!ev->y && ev->x == KBD_F7) fb_mouse_move(0, -3);
	else if (!ev->y && ev->x == KBD_F8) fb_mouse_move(3, 0);
	else 
	{
		if (fb_driver.codepage!=utf8_table&&(ev->x)>=128&&(ev->x)<=255)
			if ((ev->x=cp2u(ev->x,fb_driver.codepage)) == -1) return;
		if (current_virtual_device->keyboard_handler) current_virtual_device->keyboard_handler(current_virtual_device, ev->x, ev->y);
	}
}




#define mouse_getscansegment(buf,x,y,w) memcpy(buf,fb_mem+y*fb_linesize+x*fb_pixelsize,w)
#define mouse_drawscansegment(ptr,x,y,w) memcpy(fb_mem+y*fb_linesize+x*fb_pixelsize,ptr,w);

/* Flushes the background_buffer onscreen where it was originally taken from. */
static void place_mouse_background(void)
{
	struct bitmap bmp;

	bmp.x=arrow_width;
	bmp.y=arrow_height;
	bmp.skip=arrow_width*fb_pixelsize;
	bmp.data=background_buffer;

	{
		struct graphics_device * current_virtual_device_backup;

		current_virtual_device_backup=current_virtual_device;
		current_virtual_device=mouse_graphics_device;
		fb_draw_bitmap(mouse_graphics_device, &bmp, background_x,
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

	skip=arrow_width*fb_pixelsize;

	x=mouse_x;
	y=mouse_y;

	width=fb_pixelsize*(arrow_width+x>fb_xsize?fb_xsize-x:arrow_width);
	height=arrow_height+y>fb_ysize?fb_ysize-y:arrow_height;

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
				memcpy (mouse_ptr, &mouse_black, fb_pixelsize);
			else if (reg1&mask)
				memcpy (mouse_ptr, &mouse_white, fb_pixelsize);
			mouse_ptr+=fb_pixelsize;
		}
	}
}

static void place_mouse(void)
{
	struct bitmap bmp;

	bmp.x=arrow_width;
	bmp.y=arrow_height;
	bmp.skip=arrow_width*fb_pixelsize;
	bmp.data=mouse_buffer;	
	{
		struct graphics_device * current_graphics_device_backup;

		current_graphics_device_backup=current_virtual_device;
		current_virtual_device=mouse_graphics_device;
		fb_draw_bitmap(mouse_graphics_device, &bmp, mouse_x, mouse_y);
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
	memcpy(mouse_buffer,background_buffer,fb_pixelsize*arrow_area);
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
	int bmpixelsizeL=fb_pixelsize;
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
	int skip=arrow_width*fb_pixelsize;
	int background_length,mouse_length;
	unsigned char *mouse_ptr=mouse_buffer,*background_ptr=background_buffer;
	int yend;

	if (mouse_bottom>fb_ysize) mouse_bottom=fb_ysize;
	if (background_bottom>fb_ysize) background_bottom=fb_ysize;

	/* Let's do the top part */
	if (background_top<mouse_top){
		/* Draw the background */
		background_length=background_right>fb_xsize?fb_xsize-background_left
			:arrow_width;
		for (;background_top<mouse_top;background_top++){
			mouse_drawscansegment(background_ptr,background_left
				,background_top,background_length*fb_pixelsize);
			background_ptr+=skip;
		}
			
	}else if (background_top>mouse_top){
		/* Draw the mouse */
		mouse_length=mouse_right>fb_xsize
			?fb_xsize-mouse_left:arrow_width;
		for (;mouse_top<background_top;mouse_top++){
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,mouse_length*fb_pixelsize);
			mouse_ptr+=skip;
		}
	}

	/* Let's do the middle part */
	yend=mouse_bottom<background_bottom?mouse_bottom:background_bottom;
	if (background_left<mouse_left){
		/* Draw background, mouse */
		mouse_length=mouse_right>fb_xsize?fb_xsize-mouse_left:arrow_width;
		for (;mouse_top<yend;mouse_top++){
			mouse_drawscansegment(background_ptr,background_left,mouse_top
				,(mouse_left-background_left)*fb_pixelsize);
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,mouse_length*fb_pixelsize);
			mouse_ptr+=skip;
			background_ptr+=skip;
		}
			
	}else{
		int l1, l2, l3;
		
		/* Draw mouse, background */
		mouse_length=mouse_right>fb_xsize?fb_xsize-mouse_left:arrow_width;
		background_length=background_right-mouse_right;
		if (background_length+mouse_right>fb_xsize)
			background_length=fb_xsize-mouse_right;
		l1=mouse_length*fb_pixelsize;
		l2=(mouse_right-background_left)*fb_pixelsize;
		l3=background_length*fb_pixelsize;
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
		mouse_length=mouse_right>fb_xsize?fb_xsize-mouse_left
			:arrow_width;
		for (;background_bottom<mouse_bottom;background_bottom++){
			mouse_drawscansegment(mouse_ptr,mouse_left,background_bottom
				,mouse_length*fb_pixelsize);
			mouse_ptr+=skip;
		}
	}else{
		/* Draw background */
		background_length=background_right>fb_xsize?fb_xsize-background_left
			:arrow_width;
		for (;mouse_bottom<background_bottom;mouse_bottom++){
			mouse_drawscansegment(background_ptr,background_left,mouse_bottom
				,background_length*fb_pixelsize);
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
	memcpy(new_background_buffer,mouse_buffer,fb_pixelsize*arrow_area);
	new_background_x=mouse_x;
	new_background_y=mouse_y;
	render_mouse_arrow();
	place_mouse_composite();
	memcpy(background_buffer,new_background_buffer,fb_pixelsize*arrow_area);
	background_x=new_background_x;
	background_y=new_background_y;
}

static void redraw_mouse(void){
	
	if (!fb_active) return; /* We are not drawing */
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
				mouse_buffer,arrow_area*fb_pixelsize);
			render_mouse_arrow();
			hide_mouse();
			place_mouse();
			memcpy(background_buffer,new_background_buffer
				,arrow_area*fb_pixelsize);
			background_x=mouse_x;
			background_y=mouse_y;
		}
	}
}

/* This is an empiric magic that ensures
 * Good white purity
 * Correct rounding and dithering prediction
 * And this is the cabbala:
 * 063 021 063 
 * 009 009 021
 * 255 085 255
 * 036 036 084
 */
static void generate_palette(struct palette *palette)
{
        int a;

	switch (fb_colors)
	{
		case 16:
               	for (a=0;a<fb_palette_colors;a++)
                {
       	                palette->red[a]=(a&8)?65535:0;
               	        palette->green[a]=((a>>1)&3)*(65535/3);
                       	palette->blue[a]=(a&1)?65535:0;
		}
		break;
		case 256:
                for (a=0;a<fb_palette_colors;a++){
                       	palette->red[a]=((a>>5)&7)*(65535/7);
                        palette->green[a]=((a>>2)&7)*(65535/7);
       	                palette->blue[a]=(a&3)*(65535/3);
                }
		break;
		case 32768:
                for (a=0;a<fb_palette_colors;a++){
                       	palette->red[a]=((a>>10)&31)*(65535/31);
                        palette->green[a]=((a>>5)&31)*(65535/31);
       	                palette->blue[a]=(a&31)*(65535/31);
                }
		break;
		case 65536:
                for (a=0;a<fb_palette_colors;a++){
                       	palette->red[a]=((a>>11)&31)*(65535/31);
                        palette->green[a]=((a>>5)&63)*(65535/63);
       	                palette->blue[a]=(a&31)*(65535/31);
                }
		break;
	}
}

static void alloc_palette(struct palette *pal)
{
	pal->red=mem_calloc(sizeof(unsigned short)*fb_palette_colors);
	pal->green=mem_calloc(sizeof(unsigned short)*fb_palette_colors);
	pal->blue=mem_calloc(sizeof(unsigned short)*fb_palette_colors);

	if (!pal->red||!pal->green||!pal->blue)/*internal("Cannot create palette.\n")*/;
}


static void free_palette(struct palette *pal)
{
	mem_free(pal->red);
	mem_free(pal->green);
	mem_free(pal->blue);
}


static void set_palette(struct palette *pal)
{
	struct fb_cmap cmap;
	unsigned i;
	unsigned short *red=pal->red;
	unsigned short *green=pal->green;
	unsigned short *blue=pal->blue;
	__u16 *r, *g, *b, *t;

	r=mem_alloc(fb_palette_colors*sizeof(__u16));
	g=mem_alloc(fb_palette_colors*sizeof(__u16));
	b=mem_alloc(fb_palette_colors*sizeof(__u16));
	t=mem_calloc(fb_palette_colors*sizeof(__u16));

	if (!r||!g||!b||!t)/*internal("Cannot allocate memory.\n")*/;

	for (i = 0; i < fb_palette_colors; i++)
	{
	        r[i] = red[i];
	        g[i] = green[i];
	        b[i] = blue[i];
		/*fprintf(stderr, "%d %d %d\n", r[i], g[i], b[i]);*/

	}

	cmap.start = 0;
	cmap.len = fb_palette_colors;
	cmap.red = r;
	cmap.green = g;
	cmap.blue = b;
	cmap.transp = t;

	if ((ioctl(fb_handler, FBIOPUTCMAP, &cmap))==-1)/*internal("Cannot set palette\n")*/;

	mem_free(r);mem_free(g);mem_free(b);mem_free(t);
}


static void get_palette(struct palette *pal)
{
	struct fb_cmap cmap;
	unsigned i;
	__u16 *r, *g, *b, *t;

	r=mem_alloc(fb_palette_colors*sizeof(__u16));
	g=mem_alloc(fb_palette_colors*sizeof(__u16));
	b=mem_alloc(fb_palette_colors*sizeof(__u16));
	t=mem_alloc(fb_palette_colors*sizeof(__u16));

	if (!r||!g||!b||!t)/*internal("Cannot allocate memory.\n")*/;

	cmap.start = 0;
	cmap.len = fb_palette_colors;
	cmap.red = r;
	cmap.green = g;
	cmap.blue = b;
	cmap.transp = t;

	if (ioctl(fb_handler, FBIOGETCMAP, &cmap))
		/*internal("Cannot get palette\n")*/;

	for (i = 0; i < fb_palette_colors; i++)
	{
		/*printf("%d %d %d\n",r[i],g[i],b[i]);*/
	        pal->red[i] = r[i];
	        pal->green[i] = g[i];
	        pal->blue[i] = b[i];
	}

	mem_free(r);mem_free(g);mem_free(b);mem_free(t);
}


static void fb_switch_signal(void *data)
{
	struct vt_stat st;
	struct rect r;
	int signal=(int)data;

	switch(signal)
	{
		case SIGUSR1: /* release */
		fb_active=0;
		if (!in_gr_operation)ioctl(TTY,VT_RELDISP,1);
		break;

		case SIGUSR2: /* acq */
		if (ioctl(TTY,VT_GETSTATE,&st)) return;
		if (st.v_active != fb_console) return;
		fb_active=1;
		ioctl(TTY,VT_RELDISP,VT_ACKACQ);
		if (have_cmap && current_virtual_device)
			set_palette(&global_pal);
		r.x1=0;
		r.y1=0;
		r.x2=fb_xsize;
		r.y2=fb_ysize;
		if (current_virtual_device) current_virtual_device->redraw_handler(current_virtual_device,&r);
		break;
	}
}


static unsigned char *fb_switch_init(void)
{

	install_signal_handler(SIGUSR1, fb_switch_signal, (void*)SIGUSR1, 1);
	install_signal_handler(SIGUSR2, fb_switch_signal, (void*)SIGUSR2, 0);
	if (-1 == ioctl(TTY,VT_GETMODE, &vt_omode)) {
		return stracpy("Could not get VT mode.\n");
	}
	memcpy(&vt_mode, &vt_omode, sizeof(vt_mode));

	vt_mode.mode   = VT_PROCESS;
	vt_mode.waitv  = 0;
	vt_mode.relsig = SIGUSR1;
	vt_mode.acqsig = SIGUSR2;

	if (-1 == ioctl(TTY,VT_SETMODE, &vt_mode)) {
		return stracpy("Could not set VT mode.\n");
	}
	return NULL;
}

static void fb_switch_shutdown(void)
{
	ioctl(TTY,VT_SETMODE, &vt_omode);
}

void fb_shutdown_palette()
{
	if (have_cmap)
	{
		set_palette(&old_palette);
		free_palette(&old_palette);
		free_palette(&global_pal);
	}
}

void fb_ctrl_c(struct itrm *i)
{
	kbd_ctrl_c();
}

#ifndef USE_GPM_DX
void fb_mouse_setsize()
{
	struct vt_stat vs;
	if (!ioctl(0, VT_GETSTATE, &vs)) {
		fd_set zero;
		struct timeval tv;
		FD_ZERO(&zero);
		memset(&tv, 0, sizeof tv);
		ioctl(0, VT_ACTIVATE, vs.v_active > 1 ? 1 : 2);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		select(0, &zero, &zero, &zero, &tv);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		select(0, &zero, &zero, &zero, &tv);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		select(0, &zero, &zero, &zero, &tv);
		ioctl(0, VT_ACTIVATE, vs.v_active);
	}
}
#endif

void unhandle_fb_mouse();

void fb_gpm_in(void *nic)
{
#ifndef USE_GPM_DX
	static int lx = -1, ly = -1;
#endif
	struct event ev;
	Gpm_Event gev;
	again:
	if (Gpm_GetEvent(&gev) <= 0) {
		unhandle_fb_mouse();
		return;
	}
	/*fprintf(stderr, "%d %d\n", gev.x, gev.y);*/
#ifndef USE_GPM_DX
	if (gev.x != lx || gev.y != ly) {
		mouse_x = (gev.x - 1) * fb_xsize / fb_txt_xsize + fb_xsize / fb_txt_xsize / 2 - 1;
		mouse_y = (gev.y - 1) * fb_ysize / fb_txt_ysize + fb_ysize / fb_txt_ysize / 2 - 1;
		lx = gev.x, ly = gev.y;
	}
#else
	if (gev.dx || gev.dy) {
		if (!(gev.type & gpm_smooth)) {
			mouse_x += gev.dx * 8;
			mouse_y += gev.dy * 8;
		} else {
			mouse_x += gev.dx;
			mouse_y += gev.dy;
		}
	}
#endif
	ev.ev = EV_MOUSE;
	if (mouse_x >= fb_xsize) mouse_x = fb_xsize - 1;
	if (mouse_y >= fb_ysize) mouse_y = fb_ysize - 1;
	if (mouse_x < 0) mouse_x = 0;
	if (mouse_y < 0) mouse_y = 0;

	if (!(gev.type & gpm_smooth) && (gev.dx || gev.dy)) {
		mouse_x = (mouse_x + 8) / 8 * 8 - 4;
		mouse_y = (mouse_y + 8) / 8 * 8 - 4;
		if (mouse_x >= fb_xsize) mouse_x = fb_xsize - 1;
		if (mouse_y >= fb_ysize) mouse_y = fb_ysize - 1;
		if (mouse_x < 0) mouse_x = 0;
		if (mouse_y < 0) mouse_y = 0;
	}

	ev.x = mouse_x;
	ev.y = mouse_y;
	if (gev.buttons & GPM_B_LEFT) ev.b = B_LEFT;
	else if (gev.buttons & GPM_B_MIDDLE) ev.b = B_MIDDLE;
	else if (gev.buttons & GPM_B_RIGHT) ev.b = B_RIGHT;
	else ev.b = 0;
	if (gev.type & GPM_DOWN) ev.b |= B_DOWN;
	else if (gev.type & GPM_UP) ev.b |= B_UP;
	else if (gev.type & GPM_DRAG) ev.b |= B_DRAG;
	else ev.b |= B_MOVE;

#ifndef USE_GPM_DX
	if (fb_msetsize < 0) {
	} else if (fb_msetsize < 10) {
		fb_msetsize++;
	} else if ((ev.b & BM_ACT) == B_MOVE && !(ev.b & BM_BUTT)) {
		fb_mouse_setsize();
		fb_msetsize = -1;
	}
#endif

	if (((ev.b & BM_ACT) == B_MOVE && !(ev.b & BM_BUTT)) || (ev.b & BM_ACT) == B_DRAG) {
		if (can_read(fb_hgpm)) goto again;
	}

	if (!current_virtual_device) return;
	if (current_virtual_device->mouse_handler) current_virtual_device->mouse_handler(current_virtual_device, ev.x, ev.y, ev.b);
	redraw_mouse();
}

int handle_fb_mouse()
{
	Gpm_Connect conn;
#ifndef USE_GPM_DX
	int gpm_ver = 0;
	struct winsize ws;
	fb_old_ws_v = 0;
#endif
	fb_hgpm = -1;
#ifndef USE_GPM_DX
	Gpm_GetLibVersion(&gpm_ver);
	if (gpm_ver >= 11900 && ioctl(1, TIOCGWINSZ, &ws) != -1) {
		memcpy(&fb_old_ws, &ws, sizeof(struct winsize));
		fb_old_ws_v = 1;
		ws.ws_row *= 2;
		ioctl(1, TIOCSWINSZ, &ws);
		fb_msetsize = 0;
		memcpy(&fb_new_ws, &ws, sizeof ws);
	} else fb_msetsize = -1;
	get_terminal_size(1, &fb_txt_xsize, &fb_txt_ysize);
#endif
	conn.eventMask = ~0;
	conn.defaultMask = gpm_smooth;
	conn.minMod = 0;
	conn.maxMod = -1;
	if ((fb_hgpm = Gpm_Open(&conn, 0)) < 0) {
		unhandle_fb_mouse();
		return -1;
	}
	set_handlers(fb_hgpm, fb_gpm_in, NULL, NULL, NULL);
#ifdef SIGTSTP
	install_signal_handler(SIGTSTP, (void (*)(void *))sig_ign, NULL, 0);
#endif
#ifdef SIGCONT
	install_signal_handler(SIGCONT, (void (*)(void *))sig_ign, NULL, 0);
#endif
	return 0;
}

void unhandle_fb_mouse()
{
#ifdef SIGTSTP
	install_signal_handler(SIGTSTP, NULL, NULL, 0);
#endif
#ifdef SIGCONT
	install_signal_handler(SIGCONT, NULL, NULL, 0);
#endif
	if (fb_hgpm >= 0) set_handlers(fb_hgpm, NULL, NULL, NULL, NULL);
#ifndef USE_GPM_DX
	fb_hgpm = -1;
	if (fb_old_ws_v) {
		ioctl(1, TIOCSWINSZ, &fb_old_ws);
		fb_old_ws_v = 0;
	}
#endif
	Gpm_Close();
}

void block_fb_mouse()
{
	if (fb_hgpm >= 0) set_handlers(fb_hgpm, NULL, NULL, NULL, NULL);
#ifndef USE_GPM_DX
	if (fb_old_ws_v) {
		ioctl(1, TIOCSWINSZ, &fb_old_ws);
	}
#endif
}

void unblock_fb_mouse()
{
	if (fb_hgpm >= 0) set_handlers(fb_hgpm, fb_gpm_in, NULL, NULL, NULL);
#ifndef USE_GPM_DX
	if (fb_old_ws_v) {
		ioctl(1, TIOCSWINSZ, &fb_new_ws);
		fb_msetsize = 0;
	}
#endif
}

unsigned char *fb_init_driver(unsigned char *param, unsigned char *ignore)
{
	unsigned char *e;
	struct stat st;
	kbd_set_raw = 1;
	fb_old_vd = NULL;
	ignore=ignore;
	fb_driver_param=param;

	fb_driver.flags |= GD_NEED_CODEPAGE;
	utf8_table=get_cp_index("UTF-8");

	if (fstat(TTY, &st)) return stracpy("Cannon stat stdin.\n");

	fb_console = st.st_rdev & 0xff;

	ioctl(TTY, VT_WAITACTIVE, fb_console);
	if ((e = fb_switch_init())) return e;

	fb_handler=open("/dev/fb0",O_RDWR);
	if (fb_handler==-1) {
		fb_switch_shutdown();
		return stracpy("Cannot open /dev/fb0.\n");
	}

	if ((ioctl (fb_handler, FBIOGET_VSCREENINFO, &vi))==-1)
	{
		close(fb_handler);
		fb_switch_shutdown();
		return stracpy("Cannot get FB VSCREENINFO.\n");
	}

	oldmode=vi;

	if ((ioctl (fb_handler, FBIOGET_FSCREENINFO, &fi))==-1)
	{
		close(fb_handler);
		fb_switch_shutdown();
		return stracpy("Cannot get FB FSCREENINFO.\n");
	}

	fb_driver.x=fb_xsize=vi.xres;
	fb_driver.y=fb_ysize=vi.yres;
	fb_bits_pp=vi.bits_per_pixel;

	 switch(fb_bits_pp)
	{
		case 8:
		fb_pixelsize=1;
		fb_palette_colors=256;
		break;

		case 15:
		case 16:
		fb_pixelsize=2;
		fb_palette_colors=32;
		break;

		case 24:
		fb_palette_colors=256;
		fb_pixelsize=3;
		break;

		case 32:
		fb_palette_colors=256;
		fb_pixelsize=4;
		fb_bits_pp=24;
		break;

		default:
		close(fb_handler);
		fb_switch_shutdown();
		return stracpy("Unknown bit depth");
	}
	fb_colors=1<<fb_bits_pp;

	if (fi.visual==FB_VISUAL_PSEUDOCOLOR) /* set palette */
	{
		have_cmap=1;
		fb_palette_colors=fb_colors;
		alloc_palette(&old_palette);
		get_palette(&old_palette);

		alloc_palette(&global_pal);
		generate_palette(&global_pal);
		set_palette(&global_pal);
	}
	if (fi.visual==FB_VISUAL_DIRECTCOLOR) /* set pseudo palette */
	{
		have_cmap=2;
		alloc_palette(&old_palette);
		get_palette(&old_palette);

		alloc_palette(&global_pal);
		generate_palette(&global_pal);
		set_palette(&global_pal);
	}
	
	fb_linesize=fb_xsize*fb_pixelsize;
	fb_mem_size=fi.smem_len;

	vi.xoffset=0;
	vi.yoffset=0;
	if ((ioctl(fb_handler, FBIOPAN_DISPLAY, &vi))==-1)
	{
	/* mikulas : nechodilo mi to, tak jsem tohle vyhodil a ono to chodi */
		/*fb_shutdown_palette();
		close(fb_handler);
		return stracpy("Cannot pan display.\n");
		*/
	}

	if (init_virtual_devices(&fb_driver, NUMBER_OF_DEVICES)){
		fb_shutdown_palette();
		close(fb_handler);
		fb_switch_shutdown();
		return stracpy("Allocation of virtual devices failed.\n");
	}
	fb_kbd = handle_svgalib_keyboard((void (*)(void *, unsigned char *, int))fb_key_in);

	/* Mikulas: nechodi to na sparcu */
	if (fb_mem_size < fb_linesize * fb_ysize)
	{
		fb_shutdown_palette();
		svgalib_free_trm(fb_kbd);
		shutdown_virtual_devices();
		close(fb_handler);
		fb_switch_shutdown();
		return stracpy("Nonlinear mapping of graphics memory not supported.\n");
	}
		
	
	if ((fb_mem=mmap(0,fb_mem_size,PROT_READ|PROT_WRITE,MAP_SHARED,fb_handler,0))==MAP_FAILED)
	{
		fb_shutdown_palette();
		svgalib_free_trm(fb_kbd);
		shutdown_virtual_devices();

		close(fb_handler);
		fb_switch_shutdown();
		return stracpy("Cannot mmap graphics memory.\n");
	}

		
	fb_driver.depth=fb_pixelsize&7;
	fb_driver.depth|=(fb_bits_pp&31)<<3;
	fb_driver.depth|=(!!(vi.nonstd))<<8;	/* nonstd byte order */
	
	fb_driver.get_color=get_color_fn(fb_driver.depth);
	fb_switch_init();
	install_signal_handler(SIGINT, (void (*)(void *))fb_ctrl_c, fb_kbd, 0);

	/* mouse */
	mouse_buffer=mem_alloc(fb_pixelsize*arrow_area);
	background_buffer=mem_alloc(fb_pixelsize*arrow_area);
	new_background_buffer=mem_alloc(fb_pixelsize*arrow_area);
	background_x=mouse_x=fb_xsize>>1;
	background_y=mouse_y=fb_ysize>>1;
	mouse_black=fb_driver.get_color(0);
	mouse_white=fb_driver.get_color(0xffffff);
	mouse_graphics_device=fb_driver.init_device();
	virtual_devices[0] = NULL;
	global_mouse_hidden=1;
	show_mouse();
	if (handle_fb_mouse()) {
		fb_driver.shutdown_device(mouse_graphics_device);
		mem_free(mouse_buffer);
		mem_free(background_buffer);
		mem_free(new_background_buffer);
		fb_shutdown_palette();
		svgalib_free_trm(fb_kbd);
		shutdown_virtual_devices();

		close(fb_handler);
		fb_switch_shutdown();
		return stracpy("Cannot open GPM mouse.\n");
	}
	/* hide cursor */
	printf("\033[?25l");
	fflush(stdout);
	return NULL;
}

void fb_shutdown_driver(void)
{
	mem_free(mouse_buffer);
	mem_free(background_buffer);
	mem_free(new_background_buffer);
	fb_driver.shutdown_device(mouse_graphics_device);
	unhandle_fb_mouse();
	ioctl (fb_handler, FBIOPUT_VSCREENINFO, &oldmode);
	fb_shutdown_palette();
	install_signal_handler(SIGINT, NULL, NULL, 0);

	close(fb_handler);

	memset(fb_mem,0,fb_mem_size);
	munmap(fb_mem,fb_mem_size);
	shutdown_virtual_devices();
	fb_switch_shutdown();
	svgalib_free_trm(fb_kbd);
	/* show cursor */
	printf("\033[?25h");
	fflush(stdout);
}


static unsigned char *fb_get_driver_param(void)
{
	        return fb_driver_param;
}


/* Return value:        0 alloced on heap
 *                      1 alloced in vidram
 *                      2 alloced in X server shm
 */
static int fb_get_empty_bitmap(struct bitmap *dest)
{
	dest->data=mem_alloc(dest->x*dest->y*fb_pixelsize);
	dest->skip=dest->x*fb_pixelsize;
	dest->flags=0;
	return 0;
}

/* Return value:        0 alloced on heap
 *                      1 alloced in vidram
 *                      2 alloced in X server shm
 */
static int fb_get_filled_bitmap(struct bitmap *dest, long color)
{
	int n=dest->x*dest->y*fb_pixelsize;

	dest->data=mem_alloc(n);
	pixel_set(dest->data,n,&color);
	dest->skip=dest->x*fb_pixelsize;
	dest->flags=0;
	return 0;
}

void fb_register_bitmap(struct bitmap *bmp)
{
}

void fb_unregister_bitmap(struct bitmap *bmp)
{
	mem_free(bmp->data);
}

void *fb_prepare_strip(struct bitmap *bmp, int top, int lines)
{
	return ((char *)bmp->data)+bmp->skip*top;
}


void fb_commit_strip(struct bitmap *bmp, int top, int lines)
{
	return;
}


void fb_draw_bitmap(struct graphics_device *dev,struct bitmap* hndl, int x, int y)
{
	unsigned char *scr_start;

	CLIP_PREFACE

	scr_start=fb_mem+y*fb_linesize+x*fb_pixelsize;
	for(;ys;ys--){
		memcpy(scr_start,data,xs*fb_pixelsize);
		data+=hndl->skip;
		scr_start+=fb_linesize;
	}
	END_GR
}


void fb_draw_bitmaps(struct graphics_device *dev, struct bitmap **hndls, int n, int x, int y)
{
	TEST_INACTIVITY

	if (x>=fb_xsize||y>fb_ysize) return;
	while(x+(*hndls)->x<=0&&n){
		x+=(*hndls)->x;
		n--;
		hndls++;
	}
	while(n&&x<=fb_xsize){
		fb_draw_bitmap(dev, *hndls, x, y);
		x+=(*hndls)->x;
		n--;
		hndls++;
	}
}



void fb_fill_area(struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	unsigned char *dest;
	int y;

	FILL_CLIP_PREFACE

	dest=fb_mem+top*fb_linesize+left*fb_pixelsize;
	for (y=bottom-top;y;y--){
		pixel_set(dest,(right-left)*fb_pixelsize,&color);
		dest+=fb_linesize;
	}
	END_GR
}


void fb_draw_hline(struct graphics_device *dev, int left, int y, int right, long color)
{
	unsigned char *dest;
	HLINE_CLIP_PREFACE
	
	dest=fb_mem+y*fb_linesize+left*fb_pixelsize;
	pixel_set(dest,(right-left)*fb_pixelsize,&color);
	END_GR
}


void fb_draw_vline(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	unsigned char *dest;
	int y;
	VLINE_CLIP_PREFACE

	dest=fb_mem+top*fb_linesize+x*fb_pixelsize;
	for (y=(bottom-top);y;y--){
		memcpy(dest,&color,fb_pixelsize);
		dest+=fb_linesize;
	}
	END_GR
}


int fb_hscroll(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	unsigned char *dest, *src;
	int y;
	int len;
	HSCROLL_CLIP_PREFACE
	
	ignore=NULL;
	if (sc>0){
		len=(dev->clip.x2-dev->clip.x1-sc)*fb_pixelsize;
		src=fb_mem+fb_linesize*dev->clip.y1+dev->clip.x1*fb_pixelsize;
		dest=src+sc*fb_pixelsize;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			memmove(dest,src,len);
			dest+=fb_linesize;
			src+=fb_linesize;
		}
	}else{
		len=(dev->clip.x2-dev->clip.x1+sc)*fb_pixelsize;
		dest=fb_mem+fb_linesize*dev->clip.y1+dev->clip.x1*fb_pixelsize;
		src=dest-sc*fb_pixelsize;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			memmove(dest,src,len);
			dest+=fb_linesize;
			src+=fb_linesize;
		}
	}
	END_GR
	return 1;
}


int fb_vscroll(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	unsigned char *dest, *src;
	int y;
	int len;

	VSCROLL_CLIP_PREFACE

	ignore=NULL;
	len=(dev->clip.x2-dev->clip.x1)*fb_pixelsize;
	if (sc>0){
		/* Down */
		dest=fb_mem+(dev->clip.y2-1)*fb_linesize+dev->clip.x1*fb_pixelsize;
		src=dest-fb_linesize*sc;
		for (y=dev->clip.y2-dev->clip.y1-sc;y;y--){
			memcpy(dest,src,len);
			dest-=fb_linesize;
			src-=fb_linesize;
		}
	}else{
		/* Up */
		dest=fb_mem+dev->clip.y1*fb_linesize+dev->clip.x1*fb_pixelsize;
		src=dest-fb_linesize*sc;
		for (y=dev->clip.y2-dev->clip.y1+sc;y;y--){
			memcpy(dest,src,len);
			dest+=fb_linesize;
			src+=fb_linesize;
		}
	}
	END_GR
	return 1;
}


void fb_set_clip_area(struct graphics_device *dev, struct rect *r)
{
	memcpy(&dev->clip, r, sizeof(struct rect));
	if (dev->clip.x1>=dev->clip.x2||dev->clip.y2<=dev->clip.y1||dev->clip.y2<=0||dev->clip.x2<=0||dev->clip.x1>=fb_xsize
			||dev->clip.y1>=fb_ysize){
		/* Empty region */
		dev->clip.x1=dev->clip.x2=dev->clip.y1=dev->clip.y2=0;
	}else{
		if (dev->clip.x1<0) dev->clip.x1=0;
		if (dev->clip.x2>fb_xsize) dev->clip.x2=fb_xsize;
		if (dev->clip.y1<0) dev->clip.y1=0;
		if (dev->clip.y2>fb_ysize) dev->clip.y2=fb_ysize;
	}
}

int fb_block(struct graphics_device *dev)
{
	if (fb_old_vd) return 1;
	unhandle_fb_mouse();
	fb_old_vd = current_virtual_device;
	current_virtual_device=NULL;
	svgalib_block_itrm(fb_kbd);
	if (have_cmap) set_palette(&old_palette);
	printf("\033[?25h");
	fflush(stdout);
	return 0;
}

void fb_unblock(struct graphics_device *dev)
{
#ifdef DEBUG
	if (current_virtual_device) {
		internal("fb_unblock called without fb_block");
		return;
	}
#endif /* #ifdef DEBUG */
	current_virtual_device = fb_old_vd;
	fb_old_vd = NULL;
	if (have_cmap) set_palette(&global_pal);
	printf("\033[?25l");
	fflush(stdout);
	svgalib_unblock_itrm(fb_kbd);
	handle_fb_mouse();
	if (current_virtual_device) current_virtual_device->redraw_handler(current_virtual_device
			,&current_virtual_device->size);
}

void fb_put_to_clipboard(struct graphics_device *gd, char *string,int length)
{
}

void fb_request_clipboard(struct graphics_device *gd)
{
}

unsigned char *fb_get_from_clipboard(struct graphics_device *gd)
{
        return NULL;
}

struct graphics_driver fb_driver={
	"fb",
	fb_init_driver,
	init_virtual_device,
	shutdown_virtual_device,
	fb_shutdown_driver,
	fb_get_driver_param,
	fb_get_empty_bitmap,
	fb_get_filled_bitmap,
	fb_register_bitmap,
	fb_prepare_strip,
	fb_commit_strip,
	fb_unregister_bitmap,
	fb_draw_bitmap,
	fb_draw_bitmaps,
	NULL,	/* fb_get_color */
	fb_fill_area,
	fb_draw_hline,
	fb_draw_vline,
	fb_hscroll,
	fb_vscroll,
	fb_set_clip_area,
	fb_block,
	fb_unblock,
	NULL,	/* set_title */
        fb_put_to_clipboard,
        fb_request_clipboard,
        fb_get_from_clipboard,
        0,				/* depth (filled in fb_init_driver function) */
	0, 0,				/* size (in X is empty) */
	GD_DONT_USE_SCROLL,		/* flags */
};

#endif /* GRDRV_FB */
