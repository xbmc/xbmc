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

    WRD reader - Written by Masanao Izumo <mo@goice.co.jp>
		 Modified by Takaya Nogami <t-nogami@happy.email.ne.jp> for
			Sherry WRD.

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
#include <ctype.h>

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include "wrd.h"
#include "strtab.h"

/*#define DEBUG 1*/

#if  defined(JAPANESE) || defined(__MACOS__)
#define IS_MULTI_BYTE(c)	( ((c)&0x80) && ((0x1 <= ((c)&0x7F) && ((c)&0x7F) <= 0x1f) ||\
				 (0x60 <= ((c)&0x7F) && ((c)&0x7F) <= 0x7c)))
#define IS_SJIS_ZENKAKU_SPACE(p) ((p)[0] == 0x81 && (p)[1] == 0x40)
#else
#define IS_MULTI_BYTE(c)	0
#endif /* JAPANESE */

#define WRDENDCHAR 26 /* ^Z */
#define MAXTOKLEN 255
#define MAXTIMESIG 256

/*
 * Define Bug emulation level.
 * 0: No emulatoin.
 * 1: Standard emulation (emulate if the bugs is well known).
 * 2: More emulation (including unknown bugs).
 * 3-9: Danger level!! (special debug level)
 */
#ifndef MIMPI_BUG_EMULATION_LEVEL
#define MIMPI_BUG_EMULATION_LEVEL 1
#endif
static int mimpi_bug_emulation_level = MIMPI_BUG_EMULATION_LEVEL;
static int wrd_bugstatus;
static int wrd_wmode_prev_step;
#ifdef DEBUG
#define WRD_BUGEMUINFO(code) ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, \
    "WRD: Try to emulate bug of MIMPI at line %d (code=%d)", lineno, code)
#else
#define WRD_BUGEMUINFO(code) ctl->cmsg(CMSG_WARNING, VERB_NOISY, \
    "WRD: Try to emulate bug of MIMPI at line %d", lineno)
#endif /* DEBUG */
/* Current max code: 13 */

#define FADE_SPEED_BASE 24 /* 24 or 48?? */

StringTable wrd_read_opts;
static int version;

struct wrd_delayed_event
{
    int32 waittime;
    int cmd, arg;
    struct wrd_delayed_event* next;
};

struct wrd_step_tracer
{
    int32 at;		/* total step count */
    int32 last_at;	/* estimated maxmum steps */
    int32 step_inc;	/* increment steps per newline or character */
    int bar;		/* total bar count */
    int step;		/* step in current bar */
    int barstep;	/* step count of current bar */
    MidiEvent timesig[MAXTIMESIG]; /* list of time signature code */
    int timeidx;	/* index of current timesig */
    int ntimesig;	/* number of time signatures */
    int timebase;	/* divisions */
    int offset;		/* @OFFSET */
    int wmode0, wmode1;	/* @WMODE */

    /* Delayed MIDI event list */
    struct wrd_delayed_event *de;
    struct wrd_delayed_event *free_de;

    MBlockList pool;	/* memory buffer */
};

static MBlockList sry_pool; /* data buffer */
sry_datapacket *datapacket = NULL;
#ifdef ENABLE_SHERRY
static int datapacket_len, datapacket_cnt;
#define DEFAULT_DATAPACKET_LEN 16384
static int import_sherrywrd_file(const char * );
#endif /* ENABLE_SHERRY */

static uint8 cmdlookup(uint8 *cmd);
static int wrd_nexttok(struct timidity_file *tf);
static void wrd_readinit(void);
static struct timidity_file *open_wrd_file(char *fn);
static int wrd_hexval(char *hex);
static int wrd_eint(char *hex);
static int wrd_atoi(char *val, int default_value);
static void wrd_add_lyric(int32 at, char *lyric, int len);
static int wrd_split(char* arg, char** argv, int maxarg);
static void wrdstep_inc(struct wrd_step_tracer *wrdstep, int32 inc);
static void wrdstep_update_forward(struct wrd_step_tracer *wrdstep);
static void wrdstep_update_backward(struct wrd_step_tracer *wrdstep);
static void wrdstep_nextbar(struct wrd_step_tracer *wrdstep);
static void wrdstep_prevbar(struct wrd_step_tracer *wrdstep);
static void wrdstep_wait(struct wrd_step_tracer *wrdstep, int bar, int step);
static void wrdstep_rest(struct wrd_step_tracer *wrdstep, int bar, int step);
static struct wrd_delayed_event *wrd_delay_cmd(struct wrd_step_tracer *wrdstep,
					int32 waittime, int cmd, int arg);
static uint8 wrd_tokval[MAXTOKLEN + 1]; /* Token value */
static uint8 wrd_tok;		/* Token type */
static int lineno;		/* linenumber */
static int32 last_event_time;

#define WRD_ADDEVENT(at, cmd, arg) \
    { MidiEvent e; e.time = (at); e.type = ME_WRD; e.channel = (cmd); \
      e.a = (uint8)((arg) & 0xFF); e.b = (uint8)(((arg) >> 8) & 0xFF); \
      if(mimpi_bug_emulation_level > 0){ if(at < last_event_time){ e.time = \
      last_event_time; }else{ last_event_time = e.time; }} \
      readmidi_add_event(&e); }

#define WRD_ADDSTREVENT(at, cmd, str) \
    { MidiEvent e; readmidi_make_string_event(ME_WRD, (str), &e, 0); \
      e.channel = (cmd); e.time = (at); \
      if(mimpi_bug_emulation_level > 0){ if(at < last_event_time){ e.time = \
      last_event_time; }else{ last_event_time = e.time; }} \
      readmidi_add_event(&e); }

#define SETMIDIEVENT(e, at, t, ch, pa, pb) \
    { (e).time = (at); (e).type = (t); \
      (e).channel = (uint8)(ch); (e).a = (uint8)(pa); (e).b = (uint8)(pb); }
#define MIDIEVENT(at, t, ch, pa, pb) \
    { MidiEvent event; SETMIDIEVENT(event, at, t, ch, pa, pb); \
      readmidi_add_event(&event); }

#ifdef DEBUG
static char *wrd_name_string(int cmd);
#endif /* DEBUG */

int import_wrd_file(char *fn)
{
    struct timidity_file *tf;
    char *args[WRD_MAXPARAM], *arg0;
    int argc;
    int32 i, num;
    struct wrd_step_tracer wrdstep;
#define step_at wrdstep.at

    static int initflag = 0;
    static char *default_wrd_file1, /* Default */
		*default_wrd_file2; /* Always */
    char *wfn; /* opened WRD filename */
    StringTableNode *stn; /* Chain list of string */

    if(!initflag) /* Initialize at once */
    {
	char *read_opts[WRD_MAXPARAM];

	initflag = 1;
	for(stn = wrd_read_opts.head; stn; stn = stn->next)
	{
	    int nopts;

	    nopts = wrd_split(stn->string, read_opts, WRD_MAXPARAM);
	    for(i = 0; i < nopts; i++)
	    {
		char *a, *b;
		a = read_opts[i];
		if((b = strchr(a, '=')) != NULL)
		    *b++ = '\0';
		if(strcmp(a, "d") == 0)
		    mimpi_bug_emulation_level = (b ? atoi(b) : 0);
		else if(strcmp(a, "f") == 0)
		{
		    if(default_wrd_file1 != NULL)
			free(default_wrd_file1);
		    default_wrd_file1 = (b ? safe_strdup(b) : NULL);
		}
		else if(strcmp(a, "F") == 0)
		{
		    if(default_wrd_file2 != NULL)
			free(default_wrd_file2);
		    default_wrd_file2 = (b ? safe_strdup(b) : NULL);
		}
		else if(strcmp(a, "p") == 0)
		{
		    if(b != NULL)
			wrd_add_default_path(b);
		}
	    }
	}
    }

    if(datapacket == NULL)
	init_mblock(&sry_pool);
    else
    {
	free(datapacket);
	datapacket = NULL;
	reuse_mblock(&sry_pool);
    }

    wrd_init_path();
    if(default_wrd_file2 != NULL)
	tf = open_file((wfn = default_wrd_file2), 0, OF_NORMAL);
    else
	tf = open_wrd_file(wfn = fn);
    if(tf == NULL && default_wrd_file1 != NULL)
	tf = open_file((wfn = default_wrd_file1), 0, OF_NORMAL);
    if(tf == NULL)
    {
	default_wrd_file1 = default_wrd_file2 = NULL;
#ifdef ENABLE_SHERRY
	if(import_sherrywrd_file(fn))
	    return WRD_TRACE_SHERRY;
#endif
	return WRD_TRACE_NOTHING;
    }

    wrd_readinit();

    memset(&wrdstep, 0, sizeof(wrdstep));
    init_mblock(&wrdstep.pool);
    wrdstep.de = wrdstep.free_de = NULL;
    wrdstep.timebase = current_file_info->divisions;
    wrdstep.ntimesig = dump_current_timesig(wrdstep.timesig, MAXTIMESIG - 1);
    if(wrdstep.ntimesig > 0)
    {
	wrdstep.timesig[wrdstep.ntimesig] =
	    wrdstep.timesig[wrdstep.ntimesig - 1];
	wrdstep.timesig[wrdstep.ntimesig].time = 0x7fffffff; /* stopper */
#ifdef DEBUG
	printf("Time signatures:\n");
	for(i = 0; i < wrdstep.ntimesig; i++)
	    printf("  %d: %d/%d\n",
		   wrdstep.timesig[i].time,
		   wrdstep.timesig[i].a,
		   wrdstep.timesig[i].b);
#endif /* DEBUG */
	wrdstep.barstep =
	    wrdstep.timesig[0].a * wrdstep.timebase * 4 / wrdstep.timesig[0].b;
    }
    else
	wrdstep.barstep = 4 * wrdstep.timebase;
    wrdstep.step_inc = wrdstep.barstep;
    wrdstep.last_at = readmidi_set_track(0, 0);

    readmidi_set_track(0, 1);

#ifdef DEBUG
    printf("Timebase: %d\n", wrdstep.timebase);
    printf("Step: %d\n", wrdstep.step_inc);
#endif /* DEBUG */

    while(!readmidi_error_flag && wrd_nexttok(tf))
    {
	if(version == -1 &&
	   (wrd_tok != WRD_COMMAND || wrd_tokval[0] != WRD_STARTUP))
	{
	    /* WRD_STARTUP must be first */
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "WRD: No @STARTUP");
	    version = 0;
	    WRD_ADDEVENT(0, WRD_STARTUP, 0);
	}

#ifdef DEBUG
	printf("%d: [%d,%d]/%d %s: ",
	       lineno,
	       wrdstep.bar,
	       wrdstep.step,
	       wrd_bugstatus,
	       wrd_name_string(wrd_tok));
	if(wrd_tok == WRD_COMMAND)
	    printf("%s(%s)", wrd_name_string(wrd_tokval[0]), wrd_tokval + 1);
	else if(wrd_tok == WRD_LYRIC)
	    printf("<%s>", wrd_tokval);
	printf("\n");
	fflush(stdout);
#endif /* DEBUG */

	switch(wrd_tok)
	{
	  case WRD_COMMAND:
	    arg0 = (char *)wrd_tokval + 1;
	    switch(wrd_tokval[0])
	    {
	      case WRD_COLOR:
		num = atoi(arg0);
		WRD_ADDEVENT(step_at, WRD_COLOR, num);
		break;
	      case WRD_END:
		while(step_at < wrdstep.last_at)
		    wrdstep_nextbar(&wrdstep);
		break;
	      case WRD_ESC:
		WRD_ADDSTREVENT(step_at, WRD_ESC, arg0);
		break;
	      case WRD_EXEC:
		WRD_ADDSTREVENT(step_at, WRD_EXEC, arg0);
		break;
	      case WRD_FADE:
		argc = wrd_split(arg0, args, 3);
		for(i = 0; i < 2; i++)
		{
		    num = atoi(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}

		num = wrd_atoi(args[i], 1);
		WRD_ADDEVENT(step_at, WRD_FADE, num);

		if(num >= 1)
		{
		    int32 delay, fade_speed;

		    fade_speed = (num + 1) * wrdstep.timebase / FADE_SPEED_BASE;
		    for(i = 1; i < WRD_MAXFADESTEP; i++)
		    {
			delay =	(int32)((double)fade_speed *
				       i / WRD_MAXFADESTEP);
			wrd_delay_cmd(&wrdstep, delay, WRD_ARG,
				      i);
			wrd_delay_cmd(&wrdstep, delay, WRD_FADESTEP,
				      WRD_MAXFADESTEP);
		    }
		    wrd_delay_cmd(&wrdstep, fade_speed, WRD_ARG,
				  WRD_MAXFADESTEP);
		    wrd_delay_cmd(&wrdstep, fade_speed, WRD_FADESTEP,
				  WRD_MAXFADESTEP);
		}
		break;
	      case WRD_GCIRCLE:
		argc = wrd_split(arg0, args, 6);
		if(argc < 5)
		{
		    /* Error : Too few argument */
		    break;
		}
		for(i = 0; i < 5; i++)
		{
		    num = atoi(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = atoi(args[i]);
		WRD_ADDEVENT(step_at, WRD_GCIRCLE, num);
		break;
	      case WRD_GCLS:
		num = atoi(arg0);
		WRD_ADDEVENT(step_at, WRD_GCLS, num);
		break;
	      case WRD_GINIT:
		WRD_ADDEVENT(step_at, WRD_GINIT, 0);
		break;
	      case WRD_GLINE:
		argc = wrd_split(arg0, args, 7);
		if(argc < 4)
		{
		    /* Error: Too few arguments */
		    break;
		}
		for(i = 0; i < 4; i++)
		{
		    num = atoi(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num)
		}
		WRD_ADDEVENT(step_at, WRD_ARG,   atoi(args[4]));
		WRD_ADDEVENT(step_at, WRD_ARG,   atoi(args[5]));
		WRD_ADDEVENT(step_at, WRD_GLINE, atoi(args[6]));
		break;
	      case WRD_GMODE:
		num = atoi(arg0);
		WRD_ADDEVENT(step_at, WRD_GMODE, num)
		break;
	      case WRD_GMOVE:
		argc = wrd_split(arg0, args, 9);
		if(argc < 6)
		{
		    /* Error: Too few arguments */
		    break;
		}
		for(i = 0; i < 8; i++)
		{
		    num = atoi(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = atoi(args[i]);
		WRD_ADDEVENT(step_at, WRD_GMOVE, num)
		break;
	      case WRD_GON:
		num = atoi(arg0);
		WRD_ADDEVENT(step_at, WRD_GON, num);
		break;
	      case WRD_GSCREEN:
		if(mimpi_bug_emulation_level >= 1)
		{
		    for(i = 0; arg0[i]; i++)
			if(arg0[i] == '.')
			{
			    WRD_BUGEMUINFO(110);
			    arg0[i] = ',';
			}
		}
		argc = wrd_split(arg0, args, 2);
		if(argc != 2)
		{
		    /* Error: Number of arguments miss match */
		    break;
		}
		num = atoi(args[0]);
		WRD_ADDEVENT(step_at, WRD_ARG, num);
		num = atoi(args[1]);
		WRD_ADDEVENT(step_at, WRD_GSCREEN, num);
		break;
	      case WRD_INKEY: /* FIXME */
		num = atoi(arg0);
		if(num < wrdstep.bar)
		{
		    /* Error */
		    break;
		}
		WRD_ADDEVENT(step_at, WRD_INKEY, WRD_NOARG);
		num = (num - wrdstep.bar) * wrdstep.barstep;
		wrd_delay_cmd(&wrdstep, num, WRD_OUTKEY, WRD_NOARG);
		break;
	      case WRD_LOCATE:
		if(strchr(arg0, ';') != NULL)
		    i = 1; /* Swap argument */
		else
		    i = 0;
		argc = wrd_split(arg0, args, 2);
		if(argc != 2)
		{
		    /* Error: Number of arguments miss match */
		    break;
		}
		if(i)
		{
		    num = atoi(args[1]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		    num = atoi(args[0]);
		    WRD_ADDEVENT(step_at, WRD_LOCATE, num);
		}
		else
		{
		    num = atoi(args[0]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		    num = atoi(args[1]);
		    WRD_ADDEVENT(step_at, WRD_LOCATE, num);
		}
		break;
	      case WRD_LOOP: /* Not supported */
		break;
	      case WRD_MAG:
		argc = wrd_split(arg0, args, 5);
		if(!*args[0])
		{
		    /* Error: @MAG No file name */
		    break;
		}
		WRD_ADDSTREVENT(step_at, WRD_ARG, args[0]);
		for(i = 1; i < 3; i++)
		    WRD_ADDEVENT(step_at, WRD_ARG,
				 wrd_atoi(args[i], WRD_NOARG));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[3], 1));
		WRD_ADDEVENT(step_at, WRD_MAG, atoi(args[4]));
		break;
	      case WRD_MIDI:
		argc = wrd_split(arg0, args, WRD_MAXPARAM);
		for(i = 0; i < argc; i++)
		    WRD_ADDEVENT(step_at, WRD_ARG, wrd_hexval(args[i]));
		WRD_ADDEVENT(step_at, WRD_MIDI, atoi(args[i]));
		break;
	      case WRD_OFFSET:
		wrdstep.offset = atoi(arg0);
		break;
	      case WRD_PAL:
		argc = wrd_split(arg0, args, 17);
		if(argc != 16 && argc != 17)
		{
		    /* Error: Number of arguments miss match */
		    break;
		}

		if(argc == 16)
		{
		    WRD_ADDEVENT(step_at, WRD_ARG, 0);
		    i = 0;
		}
		else
		{
		    if(*args[0] == '#')
			num = atoi(args[0] + 1);
		    else
			num = atoi(args[0]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		    i = 1;
		}
		for(; i < argc - 1; i++)
		{
		    if(!*args[i])
			num = 0;
		    else
			num = wrd_hexval(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		if(!*args[i])
		    num = 0;
		else
		    num = wrd_hexval(args[i]);
		WRD_ADDEVENT(step_at, WRD_PAL, num)
		break;
	      case WRD_PALCHG:
		WRD_ADDSTREVENT(step_at, WRD_PALCHG, arg0);
		break;
	      case WRD_PALREV:
		num = atoi(arg0);
		WRD_ADDEVENT(step_at, WRD_PALREV, num)
		break;
	      case WRD_PATH:
		WRD_ADDSTREVENT(step_at, WRD_PATH, arg0);
		break;
	      case WRD_PLOAD:
		WRD_ADDSTREVENT(step_at, WRD_PLOAD, arg0);
		break;
	      case WRD_REM:
		WRD_ADDSTREVENT(step_at, WRD_REM, arg0);
		break;
	      case WRD_REMARK:
		WRD_ADDSTREVENT(step_at, WRD_REMARK, arg0);
		break;
	      case WRD_REST:
		argc = wrd_split(arg0, args, 2);
		num = atoi(args[0]);
		if(mimpi_bug_emulation_level >= 9 && /* For testing */
		   num == 5 &&
		   wrdstep.wmode0 == 1)
		{
		    WRD_BUGEMUINFO(901);
		    num--; /* Why??? */
		}
		wrdstep_rest(&wrdstep, num, atoi(args[1]));
		break;
	      case WRD_SCREEN: /* Not supported */
		break;
	      case WRD_SCROLL:
		argc = wrd_split(arg0, args, 7);
		/*for(i = 0; i < 6; i++)
		{
		    num = atoi(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = atoi(args[i]);
		WRD_ADDEVENT(step_at, WRD_SCROLL, num);*/
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[0], 1));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[1], 1));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[2], 80));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[3], 25));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[4], 0));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[5], 0));
		WRD_ADDEVENT(step_at, WRD_SCROLL,wrd_atoi(args[6], 32));
		break;
	      case WRD_STARTUP:
		version = atoi(arg0);
		WRD_ADDEVENT(step_at, WRD_STARTUP, version);
		break;
	      case WRD_STOP: {
		  MidiEvent e;
		  e.time = step_at;
		  e.type = ME_EOT;
		  e.channel = e.a = e.b = 0;
		  readmidi_add_event(&e);
		}
		break;
	      case WRD_TCLS:
		argc = wrd_split(arg0, args, 6);
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[0], 1));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[1], 1));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[2], 80));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[3], 25));
		WRD_ADDEVENT(step_at, WRD_ARG, wrd_atoi(args[4], 0));
		i = wrd_atoi(args[5], ' ');
		if(i == 0)
		    i = ' ';
		WRD_ADDEVENT(step_at, WRD_TCLS,i);
		break;
	      case WRD_TON:
		num = atoi(arg0);
		WRD_ADDEVENT(step_at, WRD_TON, num);
		break;
	      case WRD_WAIT:
		argc = wrd_split(arg0, args, 2);
		wrdstep_wait(&wrdstep, atoi(args[0]), atoi(args[1]));
		break;
	      case WRD_WMODE:
		argc = wrd_split(arg0, args, 2);
		wrdstep.wmode0 = wrd_atoi(args[0], wrdstep.wmode0); /* n */
		wrdstep.wmode1 = wrd_atoi(args[1], wrdstep.wmode1); /* mode */
		if(mimpi_bug_emulation_level >= 1 &&
		   (version <= 0 || version == 400))
		    wrd_wmode_prev_step = wrdstep.step_inc;
		if(argc == 1 && wrdstep.wmode0 == 0)
		    wrdstep.step_inc = wrdstep.barstep;
		else
		{
		    if(wrdstep.wmode0 <= 0 || wrdstep.wmode0 >= 256)
		    {
			ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
				  "WRD: Out of value range: "
				  "@WMODE(%d,%d) at line %d",
				  wrdstep.wmode0,wrdstep.wmode1,lineno);
			wrdstep.step_inc = wrdstep.barstep;
		    } else
			wrdstep.step_inc =
			    (wrdstep.wmode0 + 1) * wrdstep.timebase / 24;
		}
#ifdef DEBUG
		printf("Step change: wmode=%s, step=%d\n",
		       wrdstep.wmode1 ? "char" : "line", wrdstep.step_inc);
#endif /* DEBUG */
		break;
	    }
	    break;
	  case WRD_ECOMMAND:
	    arg0 = (char *)wrd_tokval + 1;
	    switch(wrd_tokval[0])
	    {
	      case WRD_eFONTM:
		num = wrd_eint(arg0);
		WRD_ADDEVENT(step_at, WRD_eFONTM, num);
		break;
	      case WRD_eFONTP:
		argc = wrd_split(arg0, args, 4);
		if(argc != 4)
		{
		    /* Error: Number of arguments miss match */
		    break;
		}
		for(i = 0; i < 3; i++)
		{
		    num = wrd_eint(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = wrd_eint(args[i]);
		WRD_ADDEVENT(step_at, WRD_eFONTP, num);
		break;
	      case WRD_eFONTR:
		argc = wrd_split(arg0, args, 17);
		if(argc != 17)
		{
		    /* Error: Number of arguments miss match */
		    break;
		}
		for(i = 0; i < 16; i++)
		{
		    num = wrd_eint(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = wrd_eint(args[i]);
		WRD_ADDEVENT(step_at, WRD_eFONTR, num);
		break;
	      case WRD_eGSC:
		num = wrd_eint(arg0);
		WRD_ADDEVENT(step_at, WRD_eGSC, num);
		break;
	      case WRD_eLINE:
		num = wrd_eint(arg0);
		WRD_ADDEVENT(step_at, WRD_eLINE, num);
		break;
	      case WRD_ePAL:
		argc = wrd_split(arg0, args, 2);
		if(argc != 2)
		{
		    /* Error: Number of arguments miss match */
		    break;
		}
		num = wrd_eint(args[0]);
		WRD_ADDEVENT(step_at, WRD_ARG, num);
		num = wrd_eint(args[1]);
		WRD_ADDEVENT(step_at, WRD_ePAL, num);
		break;
	      case WRD_eREGSAVE:
		argc = wrd_split(arg0, args, 17);
		if(argc < 2)
		{
		    /* Error: Too few arguments */
		    break;
		}
		for(i = 0; i < 16; i++)
		{
		    num = wrd_eint(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = wrd_eint(args[i]);
		WRD_ADDEVENT(step_at, WRD_eREGSAVE, num);
		break;
	      case WRD_eSCROLL:
		argc = wrd_split(arg0, args, 2);
		if(argc != 2)
		{
		    /* Error: Number of arguments miss match */
		    break;
		}
		num = wrd_eint(args[0]);
		WRD_ADDEVENT(step_at, WRD_ARG, num);
		num = wrd_eint(args[1]);
		WRD_ADDEVENT(step_at, WRD_eSCROLL, num);
		break;
	      case WRD_eTEXTDOT:
		num = wrd_eint(arg0);
		WRD_ADDEVENT(step_at, WRD_eTEXTDOT, num);
		break;
	      case WRD_eTMODE:
		num = wrd_eint(arg0);
		WRD_ADDEVENT(step_at, WRD_eTMODE, num);
		break;
	      case WRD_eTSCRL:
		num = wrd_eint(arg0);
		WRD_ADDEVENT(step_at, WRD_eTSCRL, num);
		break;
	      case WRD_eVCOPY:
		argc = wrd_split(arg0, args, 9);
		if(argc != 9)
		{
		    /* Error: Number of arguments miss match */
		    break;
		}
		for(i = 0; i < 8; i++)
		{
		    num = wrd_eint(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = wrd_eint(args[i]);
		WRD_ADDEVENT(step_at, WRD_eVCOPY, num);
		break;
	      case WRD_eVSGET:
		argc = wrd_split(arg0, args, 4);
		if(argc < 1)
		{
		    /* Error: Too few arguments */
		    break;
		}
		for(i = 0; i < 3; i++)
		{
		    num = wrd_eint(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = wrd_eint(args[i]);
		WRD_ADDEVENT(step_at, WRD_eVSGET, num);
		break;
	      case WRD_eVSRES:
		WRD_ADDEVENT(step_at, WRD_eVSRES, WRD_NOARG);
		break;
	      case WRD_eXCOPY:
		argc = wrd_split(arg0, args, 14);
		if(argc < 9)
		{
		    /* Error: Too few arguments */
		    break;
		}
		for(i = 0; i < 13; i++)
		{
		    num = wrd_eint(args[i]);
		    WRD_ADDEVENT(step_at, WRD_ARG, num);
		}
		num = wrd_eint(args[i]);
		WRD_ADDEVENT(step_at, WRD_eXCOPY, num);
		break;
	      default:
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "WRD: Unknown WRD command at line %d (Ignored)",
			  lineno);
		break;
	    }
	    break;
	  case WRD_STEP:
	    if(wrd_wmode_prev_step == 0 ||
	       wrd_wmode_prev_step == wrdstep.barstep ||
	       wrdstep.step_inc == wrdstep.barstep)
		wrdstep_inc(&wrdstep, wrdstep.step_inc);
	    else
	    {
		if(wrd_wmode_prev_step != wrdstep.step_inc)
		    WRD_BUGEMUINFO(103);
		wrdstep_inc(&wrdstep, wrd_wmode_prev_step);
	    }
	    wrd_wmode_prev_step = 0;
	    break;
	  case WRD_LYRIC:
	    if(wrdstep.wmode1 == 0)
	    {
		i = (int32)strlen((char *)wrd_tokval);
		if(i > 0 && wrd_tokval[i - 1] == ';')
		    wrd_add_lyric(step_at, (char *)wrd_tokval, i - 1);
		else
		{
		    wrd_add_lyric(step_at, (char *)wrd_tokval, i);
		    WRD_ADDEVENT(step_at, WRD_NL, WRD_NOARG);
		    wrdstep_inc(&wrdstep, wrdstep.step_inc);
		}
	    }
	    else
	    {
		unsigned char *val, *lyric;
		int barcheck;

		val = (unsigned char *)wrd_tokval;
		barcheck = 0;
		for(;;)
		{
		    if(*val == ';' && *(val + 1) == '\0')
			break;
		    if(*val == '\0')
		    {
			WRD_ADDEVENT(step_at, WRD_NL, WRD_NOARG);
			wrdstep_inc(&wrdstep, wrdstep.step_inc);
			break;
		    }

		    if(*val == '\\')
		    {
			lyric = ++val;
			wrd_add_lyric(step_at, (char *) lyric, 1);
			wrdstep_inc(&wrdstep, wrdstep.step_inc);
		    }
		    else if(*val == '|')
		    {
			lyric = ++val;
			while(*val && *val != '|')
			{
			    if( IS_MULTI_BYTE(*val) || *val == '\\')
			    {
				val++;
				if(!*val)
				    break;
			    }
			    val++;
			}
			i = val - lyric;
			if(*val == '|')
			    val++;
			wrd_add_lyric(step_at, (char *) lyric, i);

			/* Why does /^\|[^\|]+\|$/ takes only one waiting ? */
			if(mimpi_bug_emulation_level >= 2 &&
			   version == 427 &&
			   barcheck == 0 && *val == '\0')
			{
			    WRD_BUGEMUINFO(204);
			    WRD_ADDEVENT(step_at, WRD_NL, WRD_NOARG);
			    wrdstep_inc(&wrdstep, wrdstep.step_inc);
			    break;
			}

			wrdstep_inc(&wrdstep, wrdstep.step_inc);
			barcheck++;
		    }
		    else
		    {
			lyric = val;
			if( IS_MULTI_BYTE(*val) )
			    val++;
			if(*val++ == '\0')
			    break;
			i = val - lyric;
			if(*lyric != '_')
			    wrd_add_lyric(step_at, (char *) lyric, i);
			wrdstep_inc(&wrdstep, wrdstep.step_inc);
		    }
		}
	    }
	    break;
	  case WRD_EOF:
	    goto end_of_wrd;
	  default:
	    break;
	}
    }

  end_of_wrd:
    while(wrdstep.de)
	wrdstep_nextbar(&wrdstep);
    reuse_mblock(&wrdstep.pool);
    close_file(tf);
#ifdef DEBUG
    fflush(stdout);
#endif /* DEBUG */

    return WRD_TRACE_MIMPI;

#undef step_at
}

static struct wrd_delayed_event *wrd_delay_cmd(struct wrd_step_tracer *wrdstep,
					int32 waittime, int cmd, int arg)
{
    struct wrd_delayed_event *p;
    struct wrd_delayed_event *insp, *prev;

    if(wrdstep->free_de != NULL)
    {
	p = wrdstep->free_de;
	wrdstep->free_de = wrdstep->free_de->next;
    }
    else
	p = (struct wrd_delayed_event *)
	    new_segment(&wrdstep->pool, sizeof(struct wrd_delayed_event));
    p->waittime = waittime;
    p->cmd = cmd;
    p->arg = arg;

    prev = NULL;
    for(insp = wrdstep->de;
	insp != NULL && insp->waittime <= waittime;
	prev = insp, insp = insp->next)
	;
    if(prev == NULL)
    {
	p->next = wrdstep->de;
	wrdstep->de = p;
    }
    else
    {
	prev->next = p;
	p->next = insp;
    }

#ifdef DEBUG
    printf("Delay events:");
    for(insp = wrdstep->de; insp != NULL; insp = insp->next)
	printf(" @%s/%d", wrd_name_string(insp->cmd), insp->waittime);
    printf("\n");
#endif /* DEBUG */

    return p;
}

static void wrdstep_update_forward(struct wrd_step_tracer *wrdstep)
{
    int lastidx;
    lastidx = wrdstep->timeidx;
    while(wrdstep->timeidx < wrdstep->ntimesig &&
	  wrdstep->timesig[wrdstep->timeidx + 1].time <= wrdstep->at)
	wrdstep->timeidx++;
    if(lastidx != wrdstep->timeidx)
    {
	wrdstep->barstep =
	    wrdstep->timesig[wrdstep->timeidx].a * wrdstep->timebase * 4
		/ wrdstep->timesig[wrdstep->timeidx].b;
#ifdef DEBUG
	printf("Time signature is changed: %d/%d barstep=%d\n",
	       wrdstep->timesig[wrdstep->timeidx].a,
	       wrdstep->timesig[wrdstep->timeidx].b,
	       wrdstep->barstep);
#endif /* DEBUG */
	/* if(wrdstep->barstep < wrdstep->step) { WRD ERROR?? } */
    }
}

static void wrdstep_update_backward(struct wrd_step_tracer *wrdstep)
{
    int lastidx;

    lastidx = wrdstep->timeidx;
    while(wrdstep->timeidx > 0 &&
	  wrdstep->timesig[wrdstep->timeidx].time > wrdstep->at)
	wrdstep->timeidx--;
    if(lastidx != wrdstep->timeidx)
    {
	wrdstep->barstep =
	    wrdstep->timesig[wrdstep->timeidx].a * wrdstep->timebase * 4
		/ wrdstep->timesig[wrdstep->timeidx].b;
#ifdef DEBUG
	printf("Time signature is changed: %d/%d barstep=%d\n",
	       wrdstep->timesig[wrdstep->timeidx].a,
	       wrdstep->timesig[wrdstep->timeidx].b,
	       wrdstep->barstep);
#endif /* DEBUG */
	/* if(wrdstep->barstep < wrdstep->step) { WRD ERROR?? } */
    }
}

static void wrdstep_nextbar(struct wrd_step_tracer *wrdstep)
{
    wrdstep_inc(wrdstep, wrdstep->barstep - wrdstep->step);
}

static void wrdstep_prevbar(struct wrd_step_tracer *wrdstep)
{
    if(wrdstep->bar == 0)
	return;
    wrdstep_inc(wrdstep, -wrdstep->step);
    wrdstep_inc(wrdstep, -wrdstep->barstep);
}

static void wrdstep_setstep(struct wrd_step_tracer *wrdstep, int step)
{
    if(step > wrdstep->barstep) /* Over step! */
	step = wrdstep->barstep;
    wrdstep_inc(wrdstep, step - wrdstep->step);
}

static void wrdstep_inc(struct wrd_step_tracer *wrdstep, int32 inc)
{
    int inc_save = inc;

    do
    {
	if(wrdstep->de == NULL)
	{
	    wrdstep->at += inc;
	    break;
	}
	else
	{
	    struct wrd_delayed_event *p, *q, *qtail;
	    int32 w;

	    w = inc;	/* Note that: if inc < 0, w always equals inc. */
	    for(p = wrdstep->de; p; p = p->next)
		if(w > p->waittime)
		    w = p->waittime;
	    q = qtail = NULL;
	    p = wrdstep->de;
	    while(p)
	    {
		struct wrd_delayed_event *next;

		p->waittime -= w;
		next = p->next;
		if(p->waittime <= 0)
		{
		    WRD_ADDEVENT(wrdstep->at, p->cmd, p->arg);
		    p->next = wrdstep->free_de;
		    wrdstep->free_de = p;
		}
		else
		{
		    p->next = NULL;
		    if(qtail == NULL)
			q = qtail = p;
		    else
			qtail = qtail->next = p;
		}
		p = next;
	    }

	    wrdstep->de = q;
	    inc -= w;
	    wrdstep->at += w;
	}
    } while(inc > 0);

    wrdstep->step += inc_save;
    if(inc_save >= 0)
    {
	while(wrdstep->step >= wrdstep->barstep)
	{
	    wrdstep->step -= wrdstep->barstep;
	    wrdstep->bar++;
	    wrdstep_update_forward(wrdstep);
	}
    }
    else
    {
	while(wrdstep->step < 0)
	{
	    wrdstep->step += wrdstep->barstep;
	    wrdstep->bar--;
	    wrdstep_update_backward(wrdstep);
	}
    }
}

static void wrdstep_wait(struct wrd_step_tracer *wrdstep, int bar, int step)
{
    bar = bar + wrdstep->offset - 1;
    step = wrdstep->timebase * step / 48;

    if(mimpi_bug_emulation_level >= 2 && wrdstep->bar > bar)
    {
	/* ignore backward bar */
	WRD_BUGEMUINFO(213);
    }
    else
    {
	while(wrdstep->bar > bar)
	    wrdstep_prevbar(wrdstep);
    }

    while(wrdstep->bar < bar)
	wrdstep_nextbar(wrdstep);
    wrdstep_setstep(wrdstep, step);
}

static void wrdstep_rest(struct wrd_step_tracer *wrdstep, int bar, int step)
{
    while(bar-- > 0)
	wrdstep_nextbar(wrdstep);
    wrdstep_setstep(wrdstep, wrdstep->timebase * step / 48);
}

static void wrd_add_lyric(int32 at, char *lyric, int len)
{
    MBlockList pool;
    char *str;

    init_mblock(&pool);
    str = (char *)new_segment(&pool, len + 1);
    memcpy(str, lyric, len);
    str[len] = '\0';
    WRD_ADDSTREVENT(at, WRD_LYRIC, str);
    reuse_mblock(&pool);
}

static int wrd_hexval(char *hex)
{
    int val, neg;

    if(!*hex)
	return WRD_NOARG;

    if(*hex != '-')
	neg = 0;
    else
    {
	neg = 1;
	hex++;
    }
    val = 0;
    for(;;)
    {
	if('0' <= *hex && *hex <= '9')
	    val = (val << 4) | (*hex - '0');
	else if('a' <= *hex && *hex <= 'f')
	    val = (val << 4) | (*hex - 'a' + 10);
	else if('A' <= *hex && *hex <= 'F')
	    val = (val << 4) | (*hex - 'A' + 10);
	else
	    break;
	hex++;
    }
    return neg ? -val : val;
}

static int wrd_eint(char *s)
{
    if(*s == '\0')
	return WRD_NOARG;
    if(*s != '$')
	return atoi(s);
    return wrd_hexval(s + 1);
}

static int wrd_atoi(char *val, int default_value)
{
    while(*val && (*val < '0' || '9' < *val))
	val++;
    return !*val ? default_value : atoi(val);
}

static struct timidity_file *open_wrd_file(char *fn)
{
    char *wrdfile, *p;
    MBlockList pool;
    struct timidity_file *tf;

    init_mblock(&pool);
    wrdfile = (char *)new_segment(&pool, strlen(fn) + 5);
    strcpy(wrdfile, fn);
    if((p = strrchr(wrdfile, '.')) == NULL)
    {
	reuse_mblock(&pool);
	return NULL;
    }
    if('A' <= p[1] && p[1] <= 'Z')
	strcpy(p + 1, "WRD");
    else
	strcpy(p + 1, "wrd");

    tf = open_file(wrdfile, 0, OF_NORMAL);
    reuse_mblock(&pool);
    return tf;
}

static void wrd_readinit(void)
{
    wrd_nexttok(NULL);
    version = -1;
    lineno = 0;
    last_event_time = 0;
}

/* return 1 if line is modified */
static int connect_wrd_line(char *line)
{
    int len;

    len = strlen(line);
    if(len > 1 && line[len - 2] != ';')
    {
	line[len - 1] = ';';
	line[len] = '\n';
	line[len + 1] = '\0';
	return 1;
    }
    return 0;
}

static void mimpi_bug_emu(int cmd, char *linebuf)
{
    if(mimpi_bug_emulation_level >= 1 && version <= 0)
    {
	switch(wrd_bugstatus)
	{
	  case 0:  /* Normal state (0) */
	  bugstate_0:
	    if(cmd == WRD_WAIT)
	    {
		if(connect_wrd_line(linebuf))
		    WRD_BUGEMUINFO(105);
		wrd_bugstatus = 2; /* WRD_WAIT shift */
	    }
	    else if(mimpi_bug_emulation_level >= 2 &&
		    cmd == WRD_REST)
	    {
		if(connect_wrd_line(linebuf))
		    WRD_BUGEMUINFO(206);
		wrd_bugstatus = 4; /* REST shift */
	    }
	    else if(mimpi_bug_emulation_level >= 8 && /* For testing */
		    cmd == WRD_WMODE)
		wrd_bugstatus = 3; /* WMODE shift */
	    break;

	  case 2: /* WRD_WAIT shift */
	    if(mimpi_bug_emulation_level >= 2)
	    {
		if(connect_wrd_line(linebuf))
		    WRD_BUGEMUINFO(212);
	    }
	    else if(cmd == WRD_WMODE)
	    {
		if(connect_wrd_line(linebuf))
		    WRD_BUGEMUINFO(107);
	    }
	    wrd_bugstatus = 0;
	    goto bugstate_0;

	  case 3: /* Testing */
	    if(cmd > 0 && connect_wrd_line(linebuf))
		WRD_BUGEMUINFO(808);
	    wrd_bugstatus = 0;
	    goto bugstate_0;

	  case 4: /* WRD_REST shift */
	    if(connect_wrd_line(linebuf))
		WRD_BUGEMUINFO(209);
	    wrd_bugstatus = 0;
	    goto bugstate_0;
	}
    }
}

static int wrd_nexttok(struct timidity_file *tf)
{
    int c, len;
    static int waitflag;
    static uint8 linebuf[MAXTOKLEN + 16]; /* Token value */
    static int tokp;

    if(tf == NULL)
    {
	waitflag = 0;
	tokp = 0;
	linebuf[0] = '\0';
	wrd_bugstatus = 0;
	wrd_wmode_prev_step = 0;
	return 1;
    }

    if(waitflag)
    {
	waitflag = 0;
	wrd_tok = WRD_STEP;
	return 1;
    }

  retry_read:
    if(!linebuf[tokp])
    {
	tokp = 0;
	wrd_wmode_prev_step = 0;
	lineno++;
	if(tf_gets((char *)linebuf, MAXTOKLEN, tf) == NULL)
	{
	    wrd_tok = WRD_EOF;
	    return 0;
	}

	len = strlen((char *)linebuf); /* 0 < len < MAXTOKLEN */
	if(linebuf[len - 1] != '\n') /* linebuf must be terminated '\n' */
	{
	    linebuf[len] = '\n';
	    linebuf[len++] = '\0';
	}
	else if(len > 1 &&
		linebuf[len - 2] == '\r' && linebuf[len - 1] == '\n')
	{
	    /* CRLF => LF */
	    linebuf[len - 2] = '\n';
	    linebuf[len - 1] = '\0';
	    len--;
	}
    }

  retry_parse:
    if(linebuf[tokp] == WRDENDCHAR)
    {
	wrd_tok = WRD_EOF;
	return 0;
    }

    if(tokp == 0 && linebuf[tokp] != '@' && linebuf[tokp] != '^') /* Lyric */
    {
	len = 0;
	c = 0; /* Shut gcc-Wall up! */

	while(len < MAXTOKLEN)
	{
	    c = linebuf[tokp++];
	    if(c == '\n' || c == WRDENDCHAR)
		break;
	    wrd_tokval[len++] = c;
	}
	wrd_tokval[len] = '\0';
	wrd_tok = WRD_LYRIC;
	if(c == WRDENDCHAR)
	{
	    tokp = 0;
	    linebuf[0] = WRDENDCHAR;
	}
	return 1;
    }

    /* Command */

    if(tokp == 0)
    {
	int i;
	/* tab to space */
	for(i = 0; linebuf[i]; i++)
	    if(linebuf[i] == '\t')
		linebuf[i] = ' ';
    }

    /* Skip white space */
    for(;;)
    {
	if(linebuf[tokp] == ' ')
	    tokp++;
#ifdef IS_SJIS_ZENKAKU_SPACE
	else if(IS_SJIS_ZENKAKU_SPACE(linebuf + tokp))
	    tokp += 2;
#endif /* IS_SJIS_ZENKAKU_SPACE */
	else
	    break;
    }

    c = linebuf[tokp++];

    if(c == '\n')
    {
	wrd_tok = WRD_STEP;
	return 1;
    }

    if(c == ';')
    {
	if(linebuf[tokp] == '\n')
	{
	    tokp = 0;
	    linebuf[0] = '\0';
	}
	goto retry_read;
    }

    if(c == '@' || c == '^') /* command */
    {
	int cmd, save_tokp;

	wrd_tok = (c == '@' ? WRD_COMMAND : WRD_ECOMMAND);
	save_tokp = tokp;

	len = 0;
#ifdef IS_SJIS_ZENKAKU_SPACE
	if(IS_SJIS_ZENKAKU_SPACE(linebuf + tokp)) {
	    /* nop */
	    mimpi_bug_emu(-1, (char *) linebuf);
	    tokp += 2;
	    goto retry_parse;
	}
#endif /* IS_SJIS_ZENKAKU_SPACE */

	if(linebuf[tokp] == ' ' ||
	   linebuf[tokp] == '\n' ||
	   linebuf[tokp] == WRDENDCHAR ||
	   linebuf[tokp] == ';')
	{
	    /* nop */
	    mimpi_bug_emu(-1, (char *) linebuf);
	    goto retry_parse;
	}

	while(len < MAXTOKLEN)
	{
	    c = linebuf[tokp++];
	    if(!isalpha(c))
		break;
	    wrd_tokval[len++] = toupper(c);
	}
	wrd_tokval[len] = '\0';
	cmd = wrd_tokval[0] = cmdlookup(wrd_tokval);

	if(c != '(' || cmd == 0)
	{
	    len = 1;
	    if(cmd == 0) {
		/* REM */
		tokp = save_tokp;
		cmd = wrd_tokval[0] = WRD_REM;
	    } else {
		linebuf[--tokp] = c;	/* Putback advanced char */
		/* skip spaces */
		while(linebuf[tokp] == ' ')
		    tokp++;
	    }

	    if(cmd == WRD_STARTUP)
	    {
		while(len < MAXTOKLEN)
		{
		    c = linebuf[tokp++];
		    if((c == ';' && linebuf[tokp] == '\n') ||
		       c == '\n' ||
		       c == WRDENDCHAR ||
		       c == '@' ||
		       c == ' ')
			break;
		    wrd_tokval[len++] = c;
		}
	    }
	    else
	    {
		while(len < MAXTOKLEN)
		{
		    c = linebuf[tokp++];
		    if((c == ';' && linebuf[tokp] == '\n') ||
		       c == '\n' ||
		       c == WRDENDCHAR)
			break;
		    wrd_tokval[len++] = c;
		}
	    }
	    linebuf[--tokp] = c;	/* Putback advanced char */
	    wrd_tokval[len] = '\0';
	    return 1;
	}

	if(wrd_tok == WRD_ECOMMAND)
	{
	    if(cmd == WRD_PAL)
		wrd_tokval[0] = WRD_ePAL;
	    else if(cmd == WRD_SCROLL)
		wrd_tokval[0] = WRD_eSCROLL;
	}

	len = 1;
	while(len < MAXTOKLEN)
	{
	    c = linebuf[tokp++];
	    if(c == ')' || c == '\n' || c == WRDENDCHAR)
		break;
	    wrd_tokval[len++] = c;
	}
	wrd_tokval[len] = '\0';
	mimpi_bug_emu(wrd_tokval[0], (char *) linebuf);

	if(c == WRDENDCHAR)
	{
	    tokp = 0;
	    linebuf[0] = WRDENDCHAR;
	}
	return 1;
    }

    /* This is quick hack for informal WRD file */
    if(c == ':' && mimpi_bug_emulation_level >= 1)
    {
	WRD_BUGEMUINFO(111);
	goto retry_parse;
    }

    /* Convert error line to @REM format */
    linebuf[--tokp] = c;	/* Putback advanced char */
    len = 0;
    wrd_tok = WRD_COMMAND;
    wrd_tokval[len++] = WRD_REM;

    while(len < MAXTOKLEN)
    {
	c = linebuf[tokp++];
	if((c == ';' && linebuf[tokp] == '\n') ||
	   c == '\n' ||
	   c == WRDENDCHAR)
	    break;
	wrd_tokval[len++] = c;
    }
    linebuf[--tokp] = c;	/* Putback advanced char */
    wrd_tokval[len] = '\0';
    return 1;
}

static uint8 cmdlookup(uint8 *cmd)
{
    switch(cmd[0])
    {
      case 'C':
	return WRD_COLOR;
      case 'E':
	if(cmd[1] == 'N')
	    return WRD_END;
	if(cmd[1] == 'S')
	    return WRD_ESC;
	return WRD_EXEC;
      case 'F':
	if(cmd[1] == 'A')
	    return WRD_FADE;
	if(cmd[4] == 'M')
	    return WRD_eFONTM;
	if(cmd[4] == 'P')
	    return WRD_eFONTP;
	return WRD_eFONTR;
      case 'G':
	if(cmd[1] == 'O')
	    return WRD_GON;
	if(cmd[1] == 'S')
	{
	    if(cmd[3] == 'R')
		return WRD_GSCREEN;
	    return WRD_eGSC;
	}
	if(cmd[3] == 'R')
	    return WRD_GCIRCLE;
	if(cmd[3] == 'S')
	    return WRD_GCLS;
	if(cmd[3] == 'I')
	    return WRD_GINIT;
	if(cmd[3] == 'N')
	    return WRD_GLINE;
	if(cmd[3] == 'D')
	    return WRD_GMODE;
	return WRD_GMOVE;
      case 'I':
	return WRD_INKEY;
      case 'L':
	if(cmd[2] == 'C')
	    return WRD_LOCATE;
	if(cmd[2] == 'N')
	    return WRD_eLINE;
	return WRD_LOOP;
      case 'M':
	if(cmd[1] == 'A')
	    return WRD_MAG;
	return WRD_MIDI;
      case 'O':
	return WRD_OFFSET;
      case 'P':
	if(cmd[3] == '\0')
	    return WRD_PAL;
	if(cmd[1] == 'L')
	    return WRD_PLOAD;
	if(cmd[2] == 'T')
	    return WRD_PATH;
	if(cmd[3] == 'C')
	    return WRD_PALCHG;
	return WRD_PALREV;
      case 'R':
	if(cmd[2] == 'G')
	    return WRD_eREGSAVE;
	if(cmd[2] == 'S')
	    return WRD_REST;
	if(cmd[3] == 'A')
	    return WRD_REMARK;
	return WRD_REM;
      case 'S':
	if(cmd[3] == 'E')
	    return WRD_SCREEN;
	if(cmd[3] == 'O')
	    return WRD_SCROLL;
	if(cmd[3] == 'R')
	    return WRD_STARTUP;
	return WRD_STOP;
      case 'T':
	if(cmd[1] == 'C')
	    return WRD_TCLS;
	if(cmd[1] == 'E')
	    return WRD_eTEXTDOT;
	if(cmd[1] == 'M')
	    return WRD_eTMODE;
	if(cmd[1] == 'O')
	    return WRD_TON;
	return WRD_eTSCRL;
      case 'V':
	if(cmd[2] == 'O')
	    return WRD_eVCOPY;
	if(cmd[2] == 'G')
	    return WRD_eVSGET;
	return WRD_eVSRES;
      case 'W':
	if(cmd[1] == 'A')
	    return WRD_WAIT;
	return WRD_WMODE;
      case 'X':
	return WRD_eXCOPY;
    }
    return 0;
}

static int wrd_split(char* arg, char** argv, int maxarg)
{
    int i, j;

#if defined(ABORT_AT_FATAL) || defined(DEBUG)
    if(maxarg < 2) {
	fprintf(stderr,
		"wrd_read.c: wrd_split(): maxarg must be more than 1.\n");
	abort();
    }
#endif

    for(i = 0; *arg && i < maxarg; i++)
    {
	argv[i] = arg;
	while(*arg && *arg != ',' && *arg != ';')
	    arg++;
	if(!*arg)
	{
	    i++;
	    break;
	}
	*arg++ = '\0';
    }
    for(j = i; j < maxarg; j++)
	argv[j] = "";
    return i;
}

#ifdef DEBUG
static char *wrd_name_string(int cmd)
{
    switch(cmd)
    {
#define WRDCASE(cmd) case WRD_ ## cmd: return #cmd
	WRDCASE(COMMAND);
	WRDCASE(ECOMMAND);
	WRDCASE(STEP);
	WRDCASE(LYRIC);
	WRDCASE(EOF);
	WRDCASE(COLOR);
	WRDCASE(END);
	WRDCASE(ESC);
	WRDCASE(EXEC);
	WRDCASE(FADE);
	WRDCASE(GCIRCLE);
	WRDCASE(GCLS);
	WRDCASE(GINIT);
	WRDCASE(GLINE);
	WRDCASE(GMODE);
	WRDCASE(GMOVE);
	WRDCASE(GON);
	WRDCASE(GSCREEN);
	WRDCASE(INKEY);
	WRDCASE(LOCATE);
	WRDCASE(LOOP);
	WRDCASE(MAG);
	WRDCASE(MIDI);
	WRDCASE(OFFSET);
	WRDCASE(PAL);
	WRDCASE(PALCHG);
	WRDCASE(PALREV);
	WRDCASE(PATH);
	WRDCASE(PLOAD);
	WRDCASE(REM);
	WRDCASE(REMARK);
	WRDCASE(REST);
	WRDCASE(SCREEN);
	WRDCASE(SCROLL);
	WRDCASE(STARTUP);
	WRDCASE(STOP);
	WRDCASE(TCLS);
	WRDCASE(TON);
	WRDCASE(WAIT);
	WRDCASE(WMODE);
	WRDCASE(eFONTM);
	WRDCASE(eFONTP);
	WRDCASE(eFONTR);
	WRDCASE(eGSC);
	WRDCASE(eLINE);
	WRDCASE(ePAL);
	WRDCASE(eREGSAVE);
	WRDCASE(eSCROLL);
	WRDCASE(eTEXTDOT);
	WRDCASE(eTMODE);
	WRDCASE(eTSCRL);
	WRDCASE(eVCOPY);
	WRDCASE(eVSGET);
	WRDCASE(eVSRES);
	WRDCASE(eXCOPY);
	WRDCASE(ARG);
	WRDCASE(FADESTEP);
	WRDCASE(OUTKEY);
	WRDCASE(NL);
	WRDCASE(MAGPRELOAD);
	WRDCASE(PHOPRELOAD);
	WRDCASE(START_SKIP);
	WRDCASE(END_SKIP);
	WRDCASE(NOARG);
#undef WRDCASE
    }
    return "Unknown";
}
#endif /* DEBUG */

#ifdef ENABLE_SHERRY
/*******************************************************************************/
#pragma mark -

static int sherry_started;	/* 0 - before start command 0x01*/
				/* 1 - after start command 0x01*/

static int sry_timebase_mode = 0; /* 0 is default */

static int32 sry_getVariableLength(struct timidity_file	*tf)
{
  int32 value= 0;
  int tmp;
  do
    {
      tmp = tf_getc(tf);
      if(tmp == EOF)
	  return -1;
      value = (value << 7) + (tmp&0x7f);
    }while ((tmp&0x80) != 0);
  return value;
}

static int sry_check_head(struct timidity_file	*tf)
{
	char	magic[12];
	uint8	version[4];
	
	tf_read(magic, 12,1,tf);
	if( memcmp(magic, "Sherry WRD\0\0", 12) ){
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "Sherry open::Header NG." );
		return 1;
	}
	tf_read(version, 1, 4, tf);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "Sherry WRD version: %02x %02x %02x %02x",
		  version[0], version[1], version[2], version[3]);

	return 0;	/*good*/
}

struct sry_drawtext_{
	char	op;
	short	v_plane;
	char	mask;
	char	mode;
	char	fore_color;
	char	back_color;
	short	x;
	short	y;
	char	*text;
};
typedef struct sry_drawtext_ sry_drawtext;

static void sry_regist_datapacket( struct wrd_step_tracer* wrdstep,
					const sry_datapacket* packet)
{
    int a, b, c;
    int err = 0;

    if(datapacket == NULL)
    {
	datapacket = (sry_datapacket *)safe_malloc(DEFAULT_DATAPACKET_LEN *
						   sizeof(sry_datapacket));
	datapacket_len = DEFAULT_DATAPACKET_LEN;
	datapacket_cnt = 0;
    }
    else
    {
	if(datapacket_cnt == (1<<24)-1) /* Over flow */
	    err = 1;
	else
	{
	    if(datapacket_cnt >= datapacket_len)
	    {
		datapacket_len *= 2;
		datapacket = (sry_datapacket *)
		    safe_realloc(datapacket,
				 datapacket_len * sizeof(sry_datapacket));
	    }
	}
    }

    a = datapacket_cnt & 0xff;
    b = (datapacket_cnt >> 8) & 0xff;
    c = (datapacket_cnt >> 16) & 0xff;
    datapacket[datapacket_cnt] = *packet;
    MIDIEVENT(wrdstep->at, ME_SHERRY, a, b, c);

    if(err)
	datapacket[datapacket_cnt].data[0] = 0xff;
    else
	datapacket_cnt++;
}

static int sry_read_datapacket(struct timidity_file	*tf, sry_datapacket* packet)
{
	int	len;
	uint8	*data;

	do
	{
	    len = sry_getVariableLength(tf);
	    if(len < 0)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "Warning: Too shorten Sherry WRD file.");
		return 1;
	    }
	} while(len == 0);
	data = 	(uint8 *)new_segment(&sry_pool, len + 1);
	if(tf_read(data, 1, len, tf) < len)
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "Warning: Too shorten Sherry WRD file.");
	    return 1;
	}
	data[len]=0;
	packet->len= len;
	packet->data= data;
	return 0;
}

static void sry_timebase21(struct wrd_step_tracer* wrdstep, int timebase)
{
    sherry_started=0;
    memset(wrdstep, 0, sizeof(struct wrd_step_tracer));
    init_mblock(&wrdstep->pool);
    wrdstep->de = wrdstep->free_de = NULL;
    wrdstep->timebase = timebase; /* current_file_info->divisions; */
    wrdstep->ntimesig = dump_current_timesig(wrdstep->timesig, MAXTIMESIG - 1);
    if(wrdstep->ntimesig > 0)
    {
	wrdstep->timesig[wrdstep->ntimesig] =
	    wrdstep->timesig[wrdstep->ntimesig - 1];
	wrdstep->timesig[wrdstep->ntimesig].time = 0x7fffffff; /* stopper */
#ifdef DEBUG
	{
	int i;
	printf("Time signatures:\n");
	for(i = 0; i < wrdstep->ntimesig; i++)
	    printf("  %d: %d/%d\n",
		   wrdstep->timesig[i].time,
		   wrdstep->timesig[i].a,
		   wrdstep->timesig[i].b);
	}
#endif /* DEBUG */
	wrdstep->barstep =
	    wrdstep->timesig[0].a * wrdstep->timebase * 4 / wrdstep->timesig[0].b;
    }
    else
	wrdstep->barstep = 4 * wrdstep->timebase;
    wrdstep->step_inc = wrdstep->barstep;
    wrdstep->last_at = readmidi_set_track(0, 0);

    readmidi_set_track(0, 1);
    /* wrdstep.step_inc = wrdstep.timebase; */

#ifdef DEBUG
    printf("Timebase: %d, divisions:%d\n",
    		wrdstep->timebase, current_file_info->divisions);
    printf("Step: %d\n", wrdstep->step_inc);
#endif /* DEBUG */
}

static void sry_timebase22(struct wrd_step_tracer* wrdstep, int mode)
{
    sry_timebase_mode = mode;
    if(sry_timebase_mode)
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "Sherry time synchronize mode is not supported");
}

static void sry_wrdinfo(uint8 *info, int len)
{
    uint8 *info1, *info2, *desc;
    int i;

    /* "info1\0info2\0desc\0" */
    /* FIXME: Need to convert SJIS to "output_text_code" */

    i = 0;
    info1 = info;
    while(i < len && info[i])
	i++;
    i++; /* skip '\0' */
    if(i >= len)
	return;
    info2 = info + i;
    while(i < len && info[i])
	i++;
    i++; /* skip '\0' */
    if(i >= len)
	return;
    desc = info + i;

    ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
	      "Sherry WRD: %s: %s: %s", info1, info2, desc);
}

static void sry_show_debug(uint8 *data)
{
    switch(data[0])
    {
      case 0x71: /* Compiler name */
	if(data[1])
	    ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "Sherry WRD Compiler: %s", data + 1);
	break;
      case 0x72: /* Source Name */
	if(data[1])
	    ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "Sherry WRD Compiled from %s", data + 1);
	break;
      case 0x7f: /* Compiler Private */
	break;
    }
}

static void sry_read_headerblock(struct wrd_step_tracer* wrdstep,
					struct timidity_file *tf)
{
	sry_datapacket	packet;
	int err;

	packet.len = 1;
	packet.data = (uint8 *)new_segment(&sry_pool, 1);
	packet.data[0] = 0x01;
	sry_regist_datapacket(wrdstep , &packet);
	
	for(;;){
		err= sry_read_datapacket(tf, &packet);
		if( err ) break;
		sry_regist_datapacket(wrdstep , &packet);
		switch(packet.data[0])
		{
		  case 0x00: /* end of header */
		    return;
		  case 0x20:
		    if( mimpi_bug_emulation_level >= 1 ){
		      sry_timebase21(wrdstep, SRY_GET_SHORT(packet.data+1));
		    }
		    break;
		  case 0x21:
		    sry_timebase21(wrdstep, SRY_GET_SHORT(packet.data+1));
		    break;
		  case 0x22:
		    sry_timebase22(wrdstep, packet.data[1]);
		    break;
		  case 0x61:
		    sry_wrdinfo(packet.data + 1, packet.len - 1);
		    break;
		  default:
		    if((packet.data[0] & 0x70) == 0x70)
			sry_show_debug(packet.data);
		    break;
		}
	}
}

static void sry_read_datablock(struct wrd_step_tracer* wrdstep,
				  struct timidity_file	*tf)
{
    sry_datapacket  packet;
    int		    delta_time; /*, cur_time=0; */
    int		    err;
    int		    need_update;

    need_update = 0;
    for(;;){
		delta_time= sry_getVariableLength(tf);
		if(delta_time > 0 && need_update)
		{
		    WRD_ADDEVENT(wrdstep->at, WRD_SHERRY_UPDATE, WRD_NOARG);
		    need_update = 0;
		}
		err = sry_read_datapacket(tf, &packet);
		if( err ) break;
		/* cur_time =+ delta_time;*/
		/* wrdstep_wait(wrdstep, delta_time,0); */
		/* wrdstep_setstep(wrdstep, delta_time); */

		if( sherry_started && delta_time ){
		    wrdstep_inc(wrdstep,
				delta_time*current_file_info->divisions
				/wrdstep->timebase);
		}

		if( packet.data[0]==0x01 ){
			sherry_started=1;
			continue;
		} else if( (packet.data[0]&0x70) == 0x70) {
		    sry_show_debug(packet.data);
		}

		sry_regist_datapacket(wrdstep , &packet);
		if(packet.data[0] == 0x31 ||
		   packet.data[0] == 0x35 ||
		   packet.data[0] == 0x36)
		    need_update = 1;

		if( packet.data[0] == 0x00 ) break;
    }
    if(need_update)
    {
	WRD_ADDEVENT(wrdstep->at, WRD_SHERRY_UPDATE, WRD_NOARG);
	need_update = 0;
    }
}

static int import_sherrywrd_file(const char * fn)
{
	char	sry_fn[256];
	char	*cp;
	struct timidity_file	*tf;
    struct wrd_step_tracer wrdstep;
	
	strncpy(sry_fn, fn, sizeof(sry_fn));
	cp=strrchr(sry_fn, '.');
	if( cp==0 ) return 0;
	
	strncpy(cp+1, "sry", sizeof(sry_fn) - (cp - sry_fn) - 1);
	tf= open_file( sry_fn, 0, OF_NORMAL);
	if( tf==NULL ) return 0;
	if( sry_check_head(tf)!=0 ) return 0;
	ctl->cmsg(CMSG_INFO, VERB_NORMAL,
		  "%s: reading sherry data...", sry_fn);
	
	wrd_readinit();
	memset(&wrdstep, 0, sizeof(wrdstep));

/**********************/
/*    MIDIEVENT(0, ME_SHERRY_START, 0, 0, 0); */
    sry_read_headerblock( &wrdstep, tf);
    sry_read_datablock( &wrdstep, tf);

/*  end_of_wrd: */
    while(wrdstep.de)
    {
	wrdstep_nextbar(&wrdstep);
    }
    reuse_mblock(&wrdstep.pool);
    close_file(tf);
#ifdef DEBUG
    fflush(stdout);
#endif /* DEBUG */
    return 1;
}

#endif /*ENABLE_SHERRY*/
