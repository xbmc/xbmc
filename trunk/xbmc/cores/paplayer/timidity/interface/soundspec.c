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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <math.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#define DEBUG 1
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "soundspec.h"
#include "fft.h"
#include "miditrace.h"

#define FFTSIZE 1024		/* Power of 2 */
#define SCOPE_HEIGHT 512	/* You can specified any positive value */
#define SCOPE_WIDTH  512	/* You can specified any positive value */
#define SCROLL_THRESHOLD 256	/* 1 <= SCROLL_THRESHOLD <= SCOPE_WIDTH */
#define NCOLOR    64		/* 1 <= NCOLOR <= 255 */
#define AMP 1.0
#define AMP2 1.5
#define DEFAULT_ZOOM (44100.0/1024.0*2.0) /* ~86Hz */
#define MIN_ZOOM 15.0	/* 15Hz is the lowest bound that human can be heard. */
#define MAX_ZOOM 440.0
#define DEFAULT_UPDATE 0.05

static int32 *ring_buffer = NULL;
#define ring_buffer_len (8 * AUDIO_BUFFER_SIZE)
static int ring_index;
static int32 outcnt;
static double exp_hz_table[SCOPE_HEIGHT+1];

int view_soundspec_flag = 0;
int ctl_speana_flag = 0;
int32 soundspec_update_interval = 0;
static int32 next_wakeup_samples;
static double soundspec_zoom = DEFAULT_ZOOM;

static Display *disp = NULL;
static Window win;
static GC gc;
static int depth;
static Pixmap offscr;
static XImage *img;
static Atom wm_delete_window;

static unsigned long color_ring[NCOLOR];

#define XCMAP(disp) XDefaultColormap(disp, DefaultScreen(disp))

typedef struct _rgb_t {
    double r, g, b;
} rgb_t;
static void hsv_to_rgb(double h, double s, double v, rgb_t *rgb)
{
    double f;
    double i;
    double p1, p2, p3;

    if (s < 0)
	s = 0;
    if (v < 0)
	v = 0;

    if (s > 1)
	s = 1;
    if (v > 1)
	v = 1;

    h = fmod(h, 360.0);
    if (h < 0)
	h += 360;

    h /= 60;
    f = modf(h, &i);
    p1 = v * (1 - s);
    p2 = v * (1 - s * f);
    p3 = v * (1 - s * (1 - f));

    switch ((int)i) {
      case 0:
	rgb->r = v;
	rgb->g = p3;
	rgb->b = p1;
	return;
      case 1:
	rgb->r = p2;
	rgb->g = v;
	rgb->b = p1;
	return;
      case 2:
	rgb->r = p1;
	rgb->g = v;
	rgb->b = p3;
	return;
      case 3:
	rgb->r = p1;
	rgb->g = p2;
	rgb->b = v;
	return;
      case 4:
	rgb->r = p3;
	rgb->g = p1;
	rgb->b = v;
	return;
      case 5:
	rgb->r = v;
	rgb->g = p1;
	rgb->b = p2;
	return;
    }
    return;
}

static double calc_color_diff(int r1, int g1, int b1,
			      int r2, int g2, int b2)
{
    double rd, gd, bd;

    rd = r2 - r1;
    gd = g2 - g1;
    bd = b2 - b1;

    return rd * rd + gd * gd + bd * bd;
}

static long search_near_color(Display *disp, int r, int g, int b)
{
    double d, mind;
    int scr;
    static XColor *xc = NULL;
    static int xc_size = 0;
    long i, k;

    scr = DefaultScreen(disp);

    if(depth == 1)		/* black or white */
    {
	d = (double)r * r
	  + (double)g * g
	  + (double)b * b;

	if(d > 3.0 * 32768.0 * 32768.0)
	    return (long)WhitePixel(disp, scr);
	return (long)BlackPixel(disp, scr);
    }

    if(xc_size == 0)
    {
	xc_size = DisplayCells(disp, scr);
	if(xc_size > 256)
	    return 0; /* Colormap size is too large */
	xc = (XColor *)safe_malloc(sizeof(XColor) * xc_size);
	for(i = 0; i < xc_size; i++)
	    xc[i].pixel = i;
    }
    XQueryColors(disp, DefaultColormap(disp, scr), xc, xc_size);

    mind = calc_color_diff(r, g, b, xc[0].red, xc[0].green, xc[0].blue);
    k = 0;
    for(i = 1; i < xc_size; i++)
    {
	d = calc_color_diff(r, g, b, xc[i].red, xc[i].green, xc[i].blue);
	if(d < mind)
	{
	    mind = d;
	    k = i;
	    if(mind == 0.0)
		break;
	}
    }

#ifdef DEBUG
    printf("color [%04x %04x %04x]->[%04x %04x %04x] (d^2=%f, k=%d)\n",
	   r, g, b,
	   xc[k].red, xc[k].green, xc[k].blue,
	   mind, (int)k);
#endif

    return k;
}

#if 0
static int highbit(unsigned long ul)
{
    int i;  unsigned long hb;
    hb = 0x80000000UL;
    for(i = 31; ((ul & hb) == 0) && i >= 0;  i--, ul<<=1)
	;
    return i;
}
#endif

static unsigned long AllocRGBColor(
	Display *disp,
	double red,		/* [0, 1] */
	double green,		/* [0, 1] */
	double blue)		/* [0, 1] */
{
    XColor c;

    c.red = (unsigned short)(red * 0xffff);
    c.green = (unsigned short)(green * 0xffff);
    c.blue = (unsigned short)(blue * 0xffff);

    if(!XAllocColor(disp, XCMAP(disp), &c))
    {
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		  "Warning: Can't allocate color: "
		  "r = %04x, g = %04x, b = %04x", c.red, c.green, c.blue);
	return search_near_color(disp, c.red, c.green, c.blue);
    }

    return c.pixel;
}

static void set_color_ring(void)
{
    int i;

    /*i = 0          ...        NCOLOR
     *  Blue -> Green -> Red -> White
     */
    for(i = 0; i < NCOLOR; i++)
    {
	rgb_t rgb;
	double h = 240 - 360 * sqrt((1.0/NCOLOR) * i);
	double s, v;

	if(h >= 0)
	    s = 0.9;
	else
	{
	    s = 0.9 + h / 120.0 * 0.9;
	    h = 0.0;
	}
/*	v = sqrt(i * (1.0/NCOLOR) * 0.6 + 0.4); */
	v = 1.0;
	hsv_to_rgb(h, s, v, &rgb);
	color_ring[i] = AllocRGBColor(disp, rgb.r, rgb.g, rgb.b);
    }
}

static void set_draw_pixel(double *val, char *pixels)
{
    int i;
    unsigned v;

    for(i = 0; i < SCOPE_HEIGHT; i++)
    {
	v = (unsigned)(val[i] * AMP2);
	if(v > NCOLOR - 1)
	    val[i] = NCOLOR - 1;
	else
	    val[i] = v;
    }

    switch(depth) {
      case 32:
      case 24:
	for(i = 0; i < SCOPE_HEIGHT; i++)
	    ((uint32 *)pixels)[i] = (uint32)color_ring[(int)val[SCOPE_HEIGHT - i - 1]];
	break;
      case 16:
	for(i = 0; i < SCOPE_HEIGHT; i++)
	    ((uint16 *)pixels)[i] = (uint16)color_ring[(int)val[SCOPE_HEIGHT - i - 1]];
	break;
      default:
	for(i = 0; i < SCOPE_HEIGHT; i++)
	    ((uint8 *)pixels)[i] = (uint8)color_ring[(int)val[SCOPE_HEIGHT - i - 1]];
	break;
    }
}

static void make_logspectrogram(double *from, double *to)
{
    double px;
    int i;

    to[0] = from[0];
    px = 0.0;
    for(i = 1; i < SCOPE_HEIGHT - 1; i++)
    {
        double tx, s;
        int x1, n;

        tx = exp_hz_table[i];
        x1 = (int)px;
        n = 0;
        s = 0.0;
        do
        {
	    double a;
            a = from[x1];
            s += a + (tx - x1) * (from[x1 + 1] - a);
            n++;
            x1++;
        } while(x1 < tx);
        to[i] = s / n;
        px = tx;
    }
    to[SCOPE_HEIGHT - 1] = from[FFTSIZE / 2 - 1];

    for(i = 0; i < SCOPE_HEIGHT; i++)
	if(to[i] <= -0.0)
	    to[i] = 0.0;
}

static void initialize_exp_hz_table(double zoom)
{
    int i;
    double r, x, w;

    if(zoom < MIN_ZOOM)
	soundspec_zoom = MIN_ZOOM;
    else if(zoom > MAX_ZOOM)
	soundspec_zoom = MAX_ZOOM;
    else
	soundspec_zoom = zoom;

    w = (double)play_mode->rate * 0.5 / zoom;
    r = exp(log(w) * (1.0/SCOPE_HEIGHT));
    w = (FFTSIZE/2.0) / (w - 1.0);
    for(i = 0, x = 1.0; i <= SCOPE_HEIGHT; i++, x *= r)
	exp_hz_table[i] = (x - 1.0) * w;
}

static KeySym xlookup_key(XKeyEvent *e)
{
    char str[10];
    KeySym key;

    XLookupString(e, str, 10, &key, NULL);
    return key;
}

static void draw_scope(double *values)
{
    static int32 call_cnt;
    int offset, expose;
    XEvent e;
    char pixels[SCOPE_HEIGHT*32];
    double work[SCOPE_HEIGHT];
    int nze;
    char *mname;
    KeySym k;

    make_logspectrogram(values, work);
    set_draw_pixel(work, pixels);

    expose = 0;
    nze = 0;
    while(QLength(disp) || XPending(disp))
    {
	XNextEvent(disp, &e);
	switch(e.type)
	{
	  case Expose:
	    expose++;
	    break;
	  case KeyPress:
	    k = xlookup_key(&e.xkey);
	    switch(k)
	    {
#ifdef XK_Down
	      case XK_Up:
#endif /* XK_Up */
#ifdef XK_KP_Up
	      case XK_KP_Up:
#endif /* XK_KP_Up */
		nze++;
		break;

#ifdef XK_Down
	      case XK_Down:
#endif /* XK_Down */
#ifdef XK_KP_Down
	      case XK_KP_Down:
#endif /* XK_KP_Down */
		nze--;
		break;

#ifdef XK_Left
	      case XK_Left:
#endif /* XK_Left */
#ifdef XK_KP_Left
	      case XK_KP_Left:
#endif /* XK_KP_Left */
		soundspec_update_interval =
		    (int32)(soundspec_update_interval*1.1);
		break;

#ifdef XK_Right
	      case XK_Right:
#endif /* XK_Right */
#ifdef XK_KP_Right
	      case XK_KP_Right:
#endif /* XK_KP_Right */
		soundspec_update_interval =
		    (int32)(soundspec_update_interval/1.1);
		if(soundspec_update_interval < 0.01 * play_mode->rate)
		    soundspec_update_interval =
			(int32)(0.01 * play_mode->rate);
		break;
	    }
	    break;

	  case ClientMessage:
	    if(wm_delete_window == e.xclient.data.l[0])
	    {
		mname = XGetAtomName(disp, e.xclient.message_type);
		if(mname != NULL && strcmp(mname, "WM_PROTOCOLS") == 0)
		{
		    /* Delete message from WM */
		    ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
			      "Sound Spectrogram Window is closed");
		    close_soundspec();
		    XCloseDisplay(disp);
		    disp = NULL;
		    return;
		}
	    }
	    break;
	}
    }

    offset = call_cnt % SCROLL_THRESHOLD;
    if(offset == 0)
    {
	XCopyArea(disp, offscr, offscr, gc,
		  SCROLL_THRESHOLD, 0,
		  SCOPE_WIDTH - SCROLL_THRESHOLD,
		  SCOPE_HEIGHT,
		  0, 0);
	XSetForeground(disp, gc, BlackPixel(disp, DefaultScreen(disp)));
	XFillRectangle(disp, offscr, gc,
		       SCOPE_WIDTH - SCROLL_THRESHOLD, 0,
		       SCROLL_THRESHOLD, SCOPE_HEIGHT);
	XCopyArea(disp, offscr, win, gc,
		  0, 0, SCOPE_WIDTH, SCOPE_HEIGHT, 0, 0);
    }

    img->data = (char *)pixels;
    XPutImage(disp, offscr, gc, img, 0, 0,
	      SCOPE_WIDTH - SCROLL_THRESHOLD + offset, 0,
	      1, SCOPE_HEIGHT);
    if(!expose)
	XCopyArea(disp, offscr, win, gc,
		  SCOPE_WIDTH - SCROLL_THRESHOLD + offset, 0,
		  1, SCOPE_HEIGHT, SCOPE_WIDTH - SCROLL_THRESHOLD + offset, 0);
    else
    {
	XCopyArea(disp, offscr, win, gc,
		  0, 0, SCOPE_WIDTH, SCOPE_HEIGHT, 0, 0);
    }

    XSync(disp, False);
    if(nze)
	initialize_exp_hz_table(soundspec_zoom - 4 * nze);
    call_cnt++;
}

struct drawing_queue
{
    double values[FFTSIZE/2 + 1];
    struct drawing_queue *next;
};
static struct drawing_queue *free_queue_list = NULL;

static struct drawing_queue *new_queue(void)
{
    struct drawing_queue *p;

    if(free_queue_list)
    {
	p = free_queue_list;
	free_queue_list = free_queue_list->next;
    }
    else
	p = (struct drawing_queue *)safe_malloc(sizeof(struct drawing_queue));
    p->next = NULL;
    return p;
}
static void free_queue(struct drawing_queue *p)
{
    p->next = free_queue_list;
    free_queue_list = p;
}

static void trace_draw_scope(void *vp)
{
    struct drawing_queue *q;
    q = (struct drawing_queue *)vp;
    if(!midi_trace.flush_flag)
    {
	if(view_soundspec_flag)
	    draw_scope(q->values);
	if(ctl_speana_flag)
	{
	    CtlEvent e;
	    e.type = CTLE_SPEANA;
	    e.v1 = (long)q->values;
	    e.v2 = FFTSIZE/2;
	    ctl->event(&e);
	}
    }
    free_queue(q);
}

static void decibelspec(double *from, double *to)
{
    double p, hr;
    int i, j;
    static double *w_table = NULL;

    if(w_table == NULL)
    {
	double t;

	w_table = (double *)safe_malloc(FFTSIZE * sizeof(double));
	t = -M_PI;
	for(i = 0; i < FFTSIZE; i++)
	{
	    w_table[i] = 0.50 + 0.50 * cos(t);
	    t += 2.0 * M_PI / FFTSIZE;
	}
    }
    for(i = 0; i < FFTSIZE; i++)
	from[i] *= w_table[i];
    realfft(from, FFTSIZE);

    hr = AMP * NCOLOR;
    if(from[0] >= 0)
	p = from[0];
    else
	p = -from[0];
    to[0] = log(1.0 + (128.0 / FFTSIZE) * p) * hr;
    for(i = 1, j = FFTSIZE - 1; i < FFTSIZE/2; i++, j--)
    {
	double t, u;

	t = from[i];
	u = from[j];
	to[i] = log(1.0 + (128.0 / FFTSIZE) * sqrt(t*t + u*u)) * hr;
    }
    p = from[FFTSIZE/2];
    to[FFTSIZE/2] = log(1.0 + (128.0 / FFTSIZE) * sqrt(2 * p*p)) * hr;
}

void close_soundspec(void)
{
    XUnmapWindow(disp, win);
    XSync(disp, True); /* Discard all remained X Events */
    view_soundspec_flag = 0;
}

void open_soundspec(void)
{
    int scr;
    XGCValues gcv;

    if(disp != NULL)
    {
	XMapWindow(disp, win);
	XSync(disp, False);
	view_soundspec_flag = 1;
	return;
    }

    if((disp = XOpenDisplay(NULL)) == NULL)
    {
	ctl->cmsg(CMSG_FATAL, VERB_NORMAL, "Can't open display");
	ctl->close();
	exit(1);
    }

    set_color_ring();
    scr = DefaultScreen(disp);
    depth = DefaultDepth(disp, scr);
    win = XCreateSimpleWindow(disp, DefaultRootWindow(disp),
			      0, 0, SCOPE_WIDTH, SCOPE_HEIGHT,
			      0, 0, BlackPixel(disp, scr));
    wm_delete_window = XInternAtom(disp, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(disp, win, &wm_delete_window, 1);

    XSelectInput(disp, win, ExposureMask | KeyPressMask);

    XStoreName(disp, win, "Sound Spectrogram");
    XSetIconName(disp, win, "Sound Spectrogram");

    gcv.graphics_exposures = False;
    gc = XCreateGC(disp, win, GCGraphicsExposures, &gcv);

    offscr = XCreatePixmap(disp, win, SCOPE_WIDTH, SCOPE_HEIGHT, depth);
    XSetForeground(disp, gc, BlackPixel(disp, scr));
    XFillRectangle(disp, offscr, gc, 0, 0, SCOPE_WIDTH, SCOPE_HEIGHT);

    img = XCreateImage(disp, DefaultVisual(disp, scr),
		       depth, ZPixmap, 0, 0,
		       1, SCOPE_HEIGHT, 8, 0);
    XMapWindow(disp, win);
    XSync(disp, False);

    view_soundspec_flag = 1;
}

void soundspec_setinterval(double sec)
{
    soundspec_update_interval = (int32)(sec * play_mode->rate);
}

static void ringsamples(double *x, int pos, int n)
{
    int i, upper;
    double r;

    upper = ring_buffer_len;
    r = 1.0 / pow(2.0, 32.0);
    for(i = 0; i < n; i++, pos++)
    {
	if(pos >= upper)
	    pos = 0;
	x[i] = (double)ring_buffer[pos] * r;
    }
}

void soundspec_update_wave(int32 *buff, int samples)
{
    int i;

    if(buff == NULL) /* Initialize */
    {
	ring_index = 0;
	if(samples == 0)
	{
	    outcnt = 0;
	    next_wakeup_samples = 0;
	}
	if(ring_buffer != NULL)
	    memset(ring_buffer, 0, sizeof(int32));
	return;
    }

    if(!view_soundspec_flag && !ctl_speana_flag)
    {
	outcnt += samples;
	return;
    }

    if(ring_buffer == NULL)
    {
	ring_buffer = safe_malloc(ring_buffer_len * sizeof(int32));
	memset(ring_buffer, 0, sizeof(int32));
	if(soundspec_update_interval == 0)
	    soundspec_update_interval =
		(int32)(DEFAULT_UPDATE * play_mode->rate);
	realfft(NULL, FFTSIZE);
	initialize_exp_hz_table(soundspec_zoom);
    }

    if(ring_index + samples > ring_buffer_len)
    {
	int d;

	d = ring_buffer_len - ring_index;
	if(play_mode->encoding & PE_MONO)
	    memcpy(ring_buffer + ring_index, buff, d * 4);
	else
	{
	    int32 *p;
	    int n;

	    p = ring_buffer + ring_index;
	    n = d * 2;
	    for(i = 0; i < n; i += 2)
		*p++ = (buff[i] + buff[i + 1]) / 2;
	}
	ring_index = 0;
	outcnt += d;
	samples -= d;
    }

    if(play_mode->encoding & PE_MONO)
	memcpy(ring_buffer + ring_index, buff, samples * 4);
    else
    {
	int32 *p;
	int n;

	p = ring_buffer + ring_index;
	n = samples * 2;
	for(i = 0; i < n; i += 2)
	    *p++ = (buff[i] + buff[i + 1]) / 2;
    }

    ring_index += samples;
    outcnt += samples;
    if(ring_index == ring_buffer_len)
	ring_index = 0;

    if(next_wakeup_samples < outcnt - (ring_buffer_len - FFTSIZE))
    {
	/* next_wakeup_samples is too small */
	next_wakeup_samples = outcnt - (ring_buffer_len - FFTSIZE);
    }

    while(next_wakeup_samples < outcnt - FFTSIZE)
    {
	double x[FFTSIZE];
	struct drawing_queue *q;

	ringsamples(x, next_wakeup_samples % ring_buffer_len, FFTSIZE);
	q = new_queue();
	decibelspec(x, q->values);
	push_midi_time_vp(midi_trace.offset + next_wakeup_samples,
			  trace_draw_scope,
			  q);
	next_wakeup_samples += soundspec_update_interval;
    }
}

/* Re-initialize something */
void soundspec_reinit(void)
{
    initialize_exp_hz_table(soundspec_zoom);
}
