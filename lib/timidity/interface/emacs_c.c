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

    emacs_c.c
    Emacs control mode - written by Masanao Izumo <mo@goice.co.jp>
    */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "miditrace.h"

/*
 * miditrace functions
 *
 * ctl_current_time
 * ctl_note
 * ctl_program
 * ctl_volume
 * ctl_expression
 * ctl_panning
 * ctl_sustain
 * ctl_pitch_bend
 */

/*
 * commands:
 * LIST CMSG TIME MVOL DRUMS FILE CURT NOTE PROG VOL EXP PAN SUS PIT RESET
 */

static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct, int nv);
static void ctl_note(int status, int ch, int note, int vel);
static void ctl_program(int ch, int val);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void ctl_pitch_bend(int channel, int val);
static void ctl_reset(void);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]);
static void ctl_event(CtlEvent *e);
static int read_ready(void);
static int emacs_type = 0; /* 0:emacs, 1:mule, 2:??
			      Note that this variable not used yet.
			      */
enum emacs_type_t
{
    ETYPE_OF_EMACS,
    ETYPE_OF_MULE,
    ETYPE_OF_OTHER
};

/**********************************/
/* export the interface functions */

#define ctl emacs_control_mode

ControlMode ctl=
{
    "Emacs interface (invoked from `M-x timidity')", 'e',
    1, 0, 0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};

static FILE *outfp;

static void quote_string_out(char *str)
{
    char *s;

    s = NULL;
    if(emacs_type == ETYPE_OF_MULE)
    {
	int len;

	len = SAFE_CONVERT_LENGTH(strlen(str));
	s = (char *)new_segment(&tmpbuffer, len);
	code_convert(str, s, len, NULL, "EUC");
	str = s;
    }

    while(*str)
    {
	if(*str == '\\' || *str == '\"')
	    putc('\\', outfp);
	putc(*str, outfp);
	str++;
    }

    if(s != NULL)
	reuse_mblock(&tmpbuffer);
}

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
    if(using_stdout)
	outfp = stderr;
    else
	outfp = stdout;
    ctl.opened = 1;
    output_text_code = "NOCNV";
    fprintf(outfp, "(timidity-VERSION \"");
    quote_string_out(timidity_version);
    fprintf(outfp, "\")\n");
    ctl_refresh();
    return 0;
}

static void ctl_close(void)
{
    fflush(outfp);
    ctl.opened = 0;
}

static int ctl_read(int32 *valp)
{
    char cmd[BUFSIZ];
    int n;

    if(read_ready() <= 0)
	return RC_NONE;
    if(fgets(cmd, sizeof(cmd), stdin) == NULL)
	return RC_QUIT; /* Emacs may down */
    n = atoi(cmd + 1);
    switch(cmd[0])
    {
      case 'L':
	return RC_LOAD_FILE;
      case 'V':
	*valp = 10 * n;
	return RC_CHANGE_VOLUME;
      case 'v':
	*valp = -10 * n;
	return RC_CHANGE_VOLUME;
      case '1':
      case '2':
      case '3':
	*valp = cmd[0] - '2';
	return RC_CHANGE_REV_EFFB;
      case '4':
      case '5':
      case '6':
	*valp = cmd[0] - '5';
	return RC_CHANGE_REV_TIME;
      case 'Q':
	return RC_QUIT;
      case 'r':
	return RC_RESTART;
      case 'f':
	*valp = play_mode->rate * n;
	return RC_FORWARD;
      case 'b':
	*valp = play_mode->rate * n;
	return RC_BACK;
      case ' ':
	return RC_TOGGLE_PAUSE;
      case '+':
	*valp = n;
	return RC_KEYUP;
      case '-':
	*valp = -n;
	return RC_KEYDOWN;
      case '>':
	*valp = n;
	return RC_SPEEDUP;
      case '<':
	*valp = n;
	return RC_SPEEDDOWN;
      case 'O':
	*valp = n;
	return RC_VOICEINCR;
      case 'o':
	*valp = n;
	return RC_VOICEDECR;
      case 'd':
	*valp = n;
	return RC_TOGGLE_DRUMCHAN;
      case 'g':
	return RC_TOGGLE_SNDSPEC;
    }

    return RC_NONE;
}

static char *chomp(char *s)
{
    int len = strlen(s);

    if(len < 2)
    {
	if(len == 0)
	    return s;
	if(s[0] == '\n' || s[0] == '\r')
	    s[0] = '\0';
	return s;
    }
    if(s[len - 1] == '\n')
	s[--len] = '\0';
    if(s[len - 1] == '\r')
	s[--len] = '\0';
    return s;
}

static void ctl_pass_playing_list(int argc, char *argv[])
{
    int i;
    char cmd[BUFSIZ];

    if(argc > 0)
    {
	if(!strcmp(argv[0], "emacs"))
	{
	    emacs_type = ETYPE_OF_EMACS;
	    argc--; argv++;
	}
	else if(!strcmp(argv[0], "mule"))
	{
	    emacs_type = ETYPE_OF_MULE;
	    argc--; argv++;
	}
	else
	    emacs_type = ETYPE_OF_OTHER;
    }

    if(argc > 0 && !strcmp(argv[0], "debug"))
    {
	for(i = 1; i < argc; i++)
	    play_midi_file(argv[i]);
	return;
    }

    /* Main Loop */
    for(;;)
    {
	int rc;

	if(fgets(cmd, sizeof(cmd), stdin) == NULL)
	    return; /* Emacs may down */
	chomp(cmd);
	if(!strncmp(cmd, "PLAY", 4))
	{
	    rc = play_midi_file(cmd + 5);
	    switch(rc)
	    {
	      case RC_TUNE_END:
	      case RC_NEXT:
		fprintf(outfp, "(timidity-NEXT)\n");
		ctl_refresh();
		break;
	      case RC_QUIT:
		return;
	    } /* skipping others command */
	}
	else if(!strncmp(cmd, "QUIT", 4))
	    return;
	else
	    continue; /* skipping unknown command */
    }
    /*NOTREACHED*/
}

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
    va_list ap;
    char buff[BUFSIZ];

    if((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
       ctl.verbosity < verbosity_level)
	return 0;
    va_start(ap, fmt);

    vsprintf(buff, fmt, ap);
    fprintf(outfp, "(timidity-CMSG %d \"", type);
    quote_string_out(buff);
    fprintf(outfp, "\")\n");
    va_end(ap);
    ctl_refresh();
    return 0;
}

static void ctl_refresh(void)
{
    fflush(stdout);
}

static void ctl_total_time(int tt)
{
    int secs;
    secs = tt/play_mode->rate;
    fprintf(outfp, "(timidity-TIME %d)\n", secs);
    ctl_refresh();
}

static void ctl_master_volume(int mv)
{
    fprintf(outfp, "(timidity-MVOL %d)\n", mv);
    ctl_refresh();
}

static void ctl_file_name(char *name)
{
    fprintf(outfp, "(timidity-FILE \"");
    quote_string_out(name);
    fprintf(outfp, "\")\n");
    ctl_refresh();
}

static void ctl_current_time(int secs, int v)
{
    fprintf(outfp, "(timidity-CURT %d %d)\n", secs, v);
    ctl_refresh();
}

static int status_number(int s)
{
    switch(s)
    {
      case VOICE_FREE:
	return 0;
      case VOICE_ON:
	return 1;
      case VOICE_SUSTAINED:
	return 2;
      case VOICE_OFF:
	return 3;
      case VOICE_DIE:
	return 4;
    }
    /* dmy */
    return 3;
}

static void ctl_note(int status, int ch, int note, int vel)
{
    if(ch >= 16)
	return;
    if(midi_trace.flush_flag)
	return;
    fprintf(outfp, "(timidity-NOTE %d %d %d)\n", ch, note,
	    status_number(status));
    ctl_refresh();
}

static void ctl_program(int ch, int val)
{
    if(ch >= 16)
	return;
    if(midi_trace.flush_flag)
	return;
    if(channel[ch].special_sample)
	val = channel[ch].special_sample;
    else
	val += progbase;
    fprintf(outfp, "(timidity-PROG %d %d)\n", ch, val);
    ctl_refresh();
}

static void ctl_volume(int ch, int val)
{
    if(ch >= 16)
	return;
    if(midi_trace.flush_flag)
	return;
    fprintf(outfp, "(timidity-VOL %d %d)\n", ch, (val*100)/127);
    ctl_refresh();
}

static void ctl_expression(int ch, int val)
{
    if(ch >= 16)
	return;
    if(midi_trace.flush_flag)
	return;
    fprintf(outfp, "(timidity-EXP %d %d)\n", ch, (val*100)/127);
    ctl_refresh();
}

static void ctl_panning(int ch, int val)
{
    if(ch >= 16)
	return;
    if(midi_trace.flush_flag)
	return;
    fprintf(outfp, "(timidity-PAN %d %d)\n", ch, val);
    ctl_refresh();
}

static void ctl_sustain(int ch, int val)
{
    if(ch >= 16)
	return;
    if(midi_trace.flush_flag)
	return;
    fprintf(outfp, "(timidity-SUS %d %d)\n", ch, val);
    ctl_refresh();
}

static void ctl_pitch_bend(int ch, int val)
{
    if(ch >= 16)
	return;
    if(midi_trace.flush_flag)
	return;
    fprintf(outfp, "(timidity-PIT %d %d)\n", ch, val);
    ctl_refresh();
}

static void ctl_reset(void)
{
    int i;
    uint32 drums;

    /* Note that Emacs is 24 bit integer. */
    drums = 0;
    for(i = 0; i < 16; i++)
	if(ISDRUMCHANNEL(i))
	    drums |= (1u << i);
    fprintf(outfp, "(timidity-DRUMS %lu)\n", (unsigned long)drums);

    fprintf(outfp, "(timidity-RESET)\n");
    for(i = 0; i < 16; i++)
    {
	if(ISDRUMCHANNEL(i))
	    ctl_program(i, channel[i].bank);
	else
	    ctl_program(i, channel[i].program);
	ctl_volume(i, channel[i].volume);
	ctl_expression(i, channel[i].expression);
	ctl_panning(i, channel[i].panning);
	ctl_sustain(i, channel[i].sustain);
	ctl_pitch_bend(i, channel[i].pitchbend);
    }
    ctl_refresh();
}


#if defined(sgi)
#include <sys/time.h>
#include <bstring.h>
#endif

#if defined(SOLARIS) || defined(__FreeBSD__)
#include <unistd.h>
#include <sys/filio.h>
#endif

static int read_ready(void)
{
#if defined(sgi)
    fd_set fds;
    int cnt;
    struct timeval timeout;
    int fd;

    fd = fileno(stdin);

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeout.tv_sec = timeout.tv_usec = 0;

    if((cnt = select(fd + 1, &fds, NULL, NULL, &timeout)) < 0)
    {
	fprintf(outfp, "(error \"select system call is failed\")\n");
	ctl_refresh();
	return -1;
    }

    return cnt > 0 && FD_ISSET(fd, &fds) != 0;
#else
    int num;
    int fd;

    fd = fileno(stdin);
    if(ioctl(fd, FIONREAD, &num) < 0) /* see how many chars in buffer. */
    {
	fprintf(outfp, "(error \"ioctl system call is failed\")\n");
	ctl_refresh();
	return -1;
    }
    return num;
#endif
}

static void ctl_event(CtlEvent *e)
{
    switch(e->type)
    {
      case CTLE_NOW_LOADING:
	ctl_file_name((char *)e->v1);
	break;
      case CTLE_LOADING_DONE:
	break;
      case CTLE_PLAY_START:
	ctl_total_time((int)e->v1);
	break;
      case CTLE_PLAY_END:
	break;
      case CTLE_TEMPO:
	break;
      case CTLE_METRONOME:
	break;
      case CTLE_CURRENT_TIME:
	ctl_current_time((int)e->v1, (int)e->v2);
	break;
      case CTLE_NOTE:
	ctl_note((int)e->v1, (int)e->v2, (int)e->v3, (int)e->v4);
	break;
      case CTLE_MASTER_VOLUME:
	ctl_master_volume((int)e->v1);
	break;
      case CTLE_PROGRAM:
	ctl_program((int)e->v1, (int)e->v2);
	break;
      case CTLE_VOLUME:
	ctl_volume((int)e->v1, (int)e->v2);
	break;
      case CTLE_EXPRESSION:
	ctl_expression((int)e->v1, (int)e->v2);
	break;
      case CTLE_PANNING:
	ctl_panning((int)e->v1, (int)e->v2);
	break;
      case CTLE_SUSTAIN:
	ctl_sustain((int)e->v1, (int)e->v2);
	break;
      case CTLE_PITCH_BEND:
	ctl_pitch_bend((int)e->v1, (int)e->v2);
	break;
      case CTLE_MOD_WHEEL:
	ctl_pitch_bend((int)e->v1, e->v2 ? -1 : 0x2000);
	break;
      case CTLE_CHORUS_EFFECT:
	break;
      case CTLE_REVERB_EFFECT:
	break;
      case CTLE_LYRIC:
	default_ctl_lyric((int)e->v1);
	break;
      case CTLE_REFRESH:
	ctl_refresh();
	break;
      case CTLE_RESET:
	ctl_reset();
	break;
    }
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_e_loader(void)
{
    return &ctl;
}
