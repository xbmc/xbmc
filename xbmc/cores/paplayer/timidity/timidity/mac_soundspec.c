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

    mac_soundspec.c

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "soundspec.h"
#include "fft.h"
#include "miditrace.h"

#include "mac_main.h"
#include "mac_skin.h"

#define FFTSIZE 1024		/* Power of 2 */
#define SCOPE_HEIGHT 512	/* You can specified any positive value */
#define SCOPE_WIDTH  512	/* You can specified any positive value */
#define SCROLL_THRESHOLD 256	/* 1 <= SCROLL_THRESHOLD <= SCOPE_WIDTH */
#define NCOLOR    32768		/* 1 <= NCOLOR <= 255 */
/*#define AMP 0.35*/
#define AMP 1.0
/*#define AMP 0.27*/
#define DEFAULT_ZOOM (44100.0/1024.0*2.0) /* ~86Hz */
#define MIN_ZOOM 15.0	/* 15Hz is lower bound that human can be heard. */
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

static Boolean disp = NULL;
#define win   mac_SpecWindow

static RGBColor color_ring[NCOLOR];

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

static RGBColor AllocRGBColor(
	double red,		/* [0, 1] */
	double green,		/* [0, 1] */
	double blue)		/* [0, 1] */
{
    RGBColor c;

    c.red = (unsigned short)(red * 0xffff);
    c.green = (unsigned short)(green * 0xffff);
    c.blue = (unsigned short)(blue * 0xffff);

    return c;
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
	color_ring[i] = AllocRGBColor( rgb.r, rgb.g, rgb.b);
    }
}

static void set_draw_pixel(double *val, RGBColor pixels[])
{
    int i;
    for(i = 0; i < SCOPE_HEIGHT; i++)
    {
	unsigned v;
	v = (unsigned)val[SCOPE_HEIGHT - i - 1];
	if(v >= NCOLOR)
	    v = NCOLOR - 1;
	pixels[i] = color_ring[v % NCOLOR];
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

static char glookup_char(EventRecord *e)
{
    return e->message & charCodeMask;
}

void HandleSpecKeydownEvent(long message, short /*modifiers*/)
{
    int nze=0;

	switch( message & charCodeMask )
	{
	case 0x1E:	nze++;	break;	//up
	case 0x1F:	nze--;	break;	//down

	case 0x1C:	//left
		soundspec_update_interval =
			(int32)(soundspec_update_interval*1.1);
		break;

	case 0x1D:	//right
		soundspec_update_interval =
			(int32)(soundspec_update_interval/1.1);
		if(soundspec_update_interval < 0.01 * play_mode->rate)
			soundspec_update_interval =
			(int32)(0.01 * play_mode->rate);
		break;
	}
	
	if(nze)
		initialize_exp_hz_table(soundspec_zoom - 4 * nze);
}

static void draw_scope(double *values)
{
    static int32 call_cnt;
    int offset, expose,i;
    RGBColor pixels[SCOPE_HEIGHT];
    double work[SCOPE_HEIGHT];
    int nze;
	Rect righthalf={0,SCOPE_WIDTH - SCROLL_THRESHOLD,
				SCOPE_HEIGHT, SCOPE_WIDTH },
		lefthalf={0,0,
				SCOPE_HEIGHT, SCOPE_WIDTH - SCROLL_THRESHOLD-1 };

	nze = 0;
    make_logspectrogram(values, work);
    if( win.show ){
		set_draw_pixel(work, pixels);

		expose = 0;

		SetPortWindowPort(win.ref);
		offset = call_cnt % SCROLL_THRESHOLD;
		if(offset == 0)
		{						//scroll the window
			EraseRect(&lefthalf);
			ScrollRect(&win.ref->portRect,
					-(SCOPE_WIDTH - SCROLL_THRESHOLD), 0, 0);
			EraseRect(&righthalf);
		}
			//draw
		for( i=0; i<SCOPE_HEIGHT; i++ ){
			RGBForeColor(&pixels[i]);
			MoveTo(SCOPE_WIDTH - SCROLL_THRESHOLD + offset,i);
			Line(0,0);
		}
	}
	if( skin_need_speana ){
		skin_draw_spectrum(work);
	    //ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "." );
	}
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
	if(!midi_trace.flush_flag && (win.show||skin_need_speana))
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
	HideWindow(win.ref);
    //view_soundspec_flag = 0;
    win.show=0;
}

void open_soundspec(void)
{
    static int initflag = 0;

	if( win.show){
		ShowWindow(win.ref);
	}

    if(disp != NULL)
    {		//already opened
	ring_index = 0;
	next_wakeup_samples = outcnt;
	memset(ring_buffer, 0, ring_buffer_len * sizeof(int32));
	view_soundspec_flag = 1;
	return;
    }

    if(soundspec_update_interval == 0)
	soundspec_update_interval = (int32)(DEFAULT_UPDATE * play_mode->rate);

	disp=true;
	
    if(!initflag)
    {
	ring_buffer = (long*)safe_malloc(ring_buffer_len * sizeof(int32));
	realfft(NULL, FFTSIZE);
	initialize_exp_hz_table(DEFAULT_ZOOM);
	initflag = 1;
    }

    set_color_ring();

    ring_index = 0;
    next_wakeup_samples = outcnt;
    memset(ring_buffer, 0, sizeof(int32));
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

    if( !win.show && !skin_need_speana)
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
