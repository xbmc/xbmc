/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    slang_c.c, Riccardo Facchetti (riccardo@cdc8g5.cdc.polimi.it)

      based on ncurses_ctl.c
      slang library is more efficient than ncurses one.

    04/04/1995
      - Initial, working version.

    15/04/1995
      - Works with no-trace playing too; not the best way, but
        it is the only way: slang 0.99.1 don't have a window management interface
        and I don't want write one for it! :)
        The problem is that I have set the no-scroll slang option so
        when there are too much messages, the last ones are not displayed at
        all. Tipically the last messages are warning for instruments not found
        so this is no real problem (I hope :)
      - Get the real size of screen we are running on

    TiMidity++ release: Masanao Izumo <iz@onicos.co.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <termios.h>
#include <sys/ioctl.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef HAVE_SLANG_SLANG_H
#include <slang/slang.h>
#else
#include <slang.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"
#include "timer.h"

/*
 * For the set_color pairs (so called 'objects')
 * see the ctl_open()
 */
#define SLsmg_normal()                SLsmg_set_color(20)
#define SLsmg_bold()                  SLsmg_set_color(21)
#define SLsmg_reverse()               SLsmg_set_color(22)

#define SCRMODE_OUT_THRESHOLD 10.0
#define CHECK_NOTE_SLEEP_TIME 5.0
#define INDICATOR_UPDATE_TIME 0.2

static struct
{
    int prog;
    int disp_cnt;
    double last_note_on;
    char *comm;
} instr_comment[MAX_CHANNELS];

enum indicator_mode_t
{
    INDICATOR_DEFAULT,
    INDICATOR_LYRIC
};

static int indicator_width = 78;
static char *comment_indicator_buffer = NULL;
static char *current_indicator_message = NULL;
static char *indicator_msgptr = NULL;
static int current_indicator_chan = 0;
static int next_indicator_chan = -1;
static double indicator_last_update;
static int indicator_mode = INDICATOR_DEFAULT;

static void update_indicator(void);
static void reset_indicator(void);
static void display_lyric(char *lyric, int sep);
static void display_title(char *title);
static void init_lyric(char *lang);

#define LYRIC_WORD_NOSEP	0
#define LYRIC_WORD_SEP		' '

static void ctl_refresh(void);
static void ctl_help_mode(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int secs, int v);
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
static void ctl_lyric(int valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);

/**********************************************/
/* export the interface functions */

#define ctl slang_control_mode

ControlMode ctl=
{
    "slang interface", 's',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    dumb_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};

/***********************************************************************/
/* foreground/background checks disabled since switching to curses */
/* static int in_foreground=1; */
static int ctl_helpmode=0;
static int lyric_row = 6;
static int title_row = 6;
static int msg_row = 0;

static void _ctl_refresh(void)
{
  SLsmg_gotorc(0,0);
  SLsmg_refresh();
}

static void SLsmg_printfrc( int r, int c, char *fmt, ...) {
  char p[1000];
  va_list ap;

  SLsmg_gotorc( r, c);
  va_start(ap, fmt);
  vsnprintf(p, sizeof(p), fmt, ap);
  va_end(ap);

  SLsmg_write_string (p);
}

static void ctl_head(void)
{
  SLsmg_printfrc(0, 0, "TiMidity++ %s%s",
  		(strcmp(timidity_version, "current")) ? "v" : "", timidity_version);
  SLsmg_printfrc(0,SLtt_Screen_Cols-45, "(C) 1995 Tuukka Toivonen <toivonen@clinet.fi>");
  SLsmg_printfrc(1,0, "Press 'h' for help with keys, or 'q' to quit.");
}
static void ctl_refresh(void)
{
  if (ctl.trace_playing)
    _ctl_refresh();
}

static void ctl_help_mode(void)
{
  if (ctl_helpmode)
    {
      ctl_helpmode=0;

/*
 * Clear the head zone and reprint head message.
 */
      SLsmg_gotorc(0,0);
      SLsmg_erase_eol();
      SLsmg_gotorc(1,0);
      SLsmg_erase_eol();
        ctl_head();
      _ctl_refresh();
    }
  else
    {
      ctl_helpmode=1;
        SLsmg_reverse();
      SLsmg_gotorc(0,0);
      SLsmg_erase_eol();
      SLsmg_write_string(
            "V=Louder    b=Skip back      "
            "n=Next file      r=Restart file");

      SLsmg_gotorc(1,0);
      SLsmg_erase_eol();
      SLsmg_write_string(
            "v=Softer    f=Skip forward   "
            "p=Previous file  q=Quit program");
        SLsmg_normal();
      _ctl_refresh();
    }
}

static void ctl_total_time(int tt)
{
  int mins, secs=tt/play_mode->rate;
  mins=secs/60;
  secs-=mins*60;

  SLsmg_gotorc(4,6+6+3);
  SLsmg_bold();
  SLsmg_printf("%3d:%02d", mins, secs);
  SLsmg_normal();
  _ctl_refresh();
}

static void ctl_master_volume(int mv)
{
  SLsmg_gotorc(4,SLtt_Screen_Cols-5);
  SLsmg_bold();
  SLsmg_printf("%03d %%", mv);
  SLsmg_normal();
  _ctl_refresh();
}

static void ctl_file_name(char *name)
{
  SLsmg_gotorc(3,6);
  SLsmg_erase_eol();
  SLsmg_bold();
  SLsmg_write_string(name);
  SLsmg_normal();
  _ctl_refresh();
}

static void ctl_current_time(int secs, int v)
{
  int mins;
  static int last_voices=-1,last_secs=-1;

  if(1/*last_secs!=secs*/)
  {
    last_secs=secs;
    mins=secs/60;
    secs-=mins*60;
    SLsmg_gotorc(4,6);
    SLsmg_bold();
    SLsmg_printf("%3d:%02d", mins, secs);
    _ctl_refresh();
  }
  if(!ctl.trace_playing||midi_trace.flush_flag)
  {
    SLsmg_normal();
    return;
  }

  if(last_voices!=voices)
  {
    last_voices=voices;
    SLsmg_gotorc(4,48);
    SLsmg_printf("%2d", v);
    SLsmg_normal();
    _ctl_refresh();
  }
}

static void ctl_note(int status, int channel, int note, int velocity)
{
  int xl;
  if(channel >= 16)
      return;
  if (!ctl.trace_playing)
    return;
  xl=note%(SLtt_Screen_Cols-24);
  SLsmg_gotorc(8+channel,xl+3);
  switch(status)
    {
    case VOICE_DIE:
      SLsmg_write_char(',');
      break;
    case VOICE_FREE:
      SLsmg_write_char('.');
      break;
    case VOICE_ON:
        SLsmg_bold();
      SLsmg_write_char('0'+(10*velocity)/128);
      SLsmg_normal();
      break;
    case VOICE_OFF:
    case VOICE_SUSTAINED:
      SLsmg_write_char('0'+(10*velocity)/128);
      break;
    }
}

static void ctl_program(int ch, int val)
{
  if(ch >= 16)
    return;
  if (!ctl.trace_playing)
    return;
  if(channel[ch].special_sample)
      val = channel[ch].special_sample;
  else
      val += progbase;
  SLsmg_gotorc(8+ch, SLtt_Screen_Cols-20);
  if (ISDRUMCHANNEL(ch))
    {
        SLsmg_bold();
      SLsmg_printf("%03d", val);
        SLsmg_normal();
    }
  else
    SLsmg_printf("%03d", val);
}

static void ctl_volume(int ch, int val)
{
  if(ch >= 16)
    return;
  if (!ctl.trace_playing)
    return;
  SLsmg_gotorc(8+ch, SLtt_Screen_Cols-16);
  SLsmg_printf("%3d", (val*100)/127);
}

static void ctl_expression(int ch, int val)
{
  if(ch >= 16)
    return;
  if (!ctl.trace_playing)
    return;
  SLsmg_gotorc(8+ch, SLtt_Screen_Cols-12);
  SLsmg_printf("%3d", (val*100)/127);
}

static void ctl_panning(int ch, int val)
{
  if(ch >= 16)
    return;
  if (!ctl.trace_playing)
    return;
  SLsmg_gotorc(8+ch, SLtt_Screen_Cols-8);
  if (val==NO_PANNING)
    SLsmg_write_string("   ");
  else if (val<5)
    SLsmg_write_string(" L ");
  else if (val>123)
    SLsmg_write_string(" R ");
  else if (val>60 && val<68)
    SLsmg_write_string(" C ");
  else
    {
      /* wprintw(dftwin, "%+02d", (100*(val-64))/64); */
      val = (100*(val-64))/64; /* piss on curses */
      if (val<0)
      {
        SLsmg_write_char('-');
        val=-val;
      }
      else SLsmg_write_char('+');
      SLsmg_printf("%02d", val);
    }
}

static void ctl_sustain(int ch, int val)
{
  if(ch >= 16)
    return;
  if (!ctl.trace_playing)
    return;
  SLsmg_gotorc(8+ch, SLtt_Screen_Cols-4);
  if (val) SLsmg_write_char('S');
  else SLsmg_write_char(' ');
}

static void ctl_pitch_bend(int ch, int val)
{
  if(ch >= 16)
    return;
  if (!ctl.trace_playing)
    return;
  SLsmg_gotorc(8+ch, SLtt_Screen_Cols-2);
  if (val==-1) SLsmg_write_char('=');
  else if (val>0x2000) SLsmg_write_char('+');
  else if (val<0x2000) SLsmg_write_char('-');
  else SLsmg_write_char(' ');
}

static void ctl_reset(void)
{
  int i,j;
  if (!ctl.trace_playing)
    return;
  for (i=0; i<16; i++)
    {
      SLsmg_gotorc(8+i, 3);
      for (j=0; j<SLtt_Screen_Cols-24; j++)
	  SLsmg_write_char('.');
      if(ISDRUMCHANNEL(i))
	  ctl_program(i, channel[i].bank);
      else
	  ctl_program(i, channel[i].program);
      ctl_volume(i, channel[i].volume);
      ctl_expression(i, channel[i].expression);
      ctl_panning(i, channel[i].panning);
      ctl_sustain(i, channel[i].sustain);
      if(channel[i].pitchbend == 0x2000 && channel[i].mod.val > 0)
	  ctl_pitch_bend(i, -1);
      else
	  ctl_pitch_bend(i, channel[i].pitchbend);
    }
  _ctl_refresh();
}

/***********************************************************************/

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
#ifdef TIOCGWINSZ
  struct winsize size;
#endif
  int i;
  int save_lines, save_cols;

  SLtt_get_terminfo();
/*
 * Save the terminfo values for lines and cols
 * then detect the real values.
 */
  save_lines = SLtt_Screen_Rows;
  save_cols = SLtt_Screen_Cols;
#ifdef TIOCGWINSZ
  if (!ioctl(0, TIOCGWINSZ, &size)) {
    SLtt_Screen_Cols=size.ws_col;
    SLtt_Screen_Rows=size.ws_row;
  } else
#endif
  {
    SLtt_Screen_Cols=atoi(getenv("COLUMNS"));
    SLtt_Screen_Rows=atoi(getenv("LINES"));
  }
  if (!SLtt_Screen_Cols || !SLtt_Screen_Rows) {
    SLtt_Screen_Rows = save_lines;
      SLtt_Screen_Cols = save_cols;
  }
  SLang_init_tty(7, 0, 0);
  SLsmg_init_smg();
  SLtt_set_color (20, "Normal", "lightgray", "black");
  SLtt_set_color (21, "HighLight", "white", "black");
  SLtt_set_color (22, "Reverse", "black", "white");
  SLtt_Use_Ansi_Colors = 1;
  SLtt_Term_Cannot_Scroll = 1;

  ctl.opened=1;

  SLsmg_cls();

  ctl_head();

  SLsmg_printfrc(3,0, "File:");
  if (ctl.trace_playing)
    {
      SLsmg_printfrc(4,0, "Time:");
      SLsmg_gotorc(4,6+6+1);
      SLsmg_write_char('/');
      SLsmg_gotorc(4,40);
      SLsmg_printf("Voices:    / %d", voices);
    }
  else
    {
      SLsmg_printfrc(4,0, "Time:");
      SLsmg_printfrc(4,13, "/");
    }
  SLsmg_printfrc(4,SLtt_Screen_Cols-20, "Master volume:");
  SLsmg_gotorc(5,0);
  for (i=0; i<SLtt_Screen_Cols; i++)
    SLsmg_write_char('_');
  if (ctl.trace_playing)
    {
      SLsmg_printfrc(6,0, "Ch");
      SLsmg_printfrc(6,SLtt_Screen_Cols-20, "Prg Vol Exp Pan S B");
      SLsmg_gotorc(7,0);
      for (i=0; i<SLtt_Screen_Cols; i++)
      SLsmg_write_char('-');
      for (i=0; i<16; i++)
      {
        SLsmg_printfrc(8+i, 0, "%02d", i+1);
      }
      set_trace_loop_hook(update_indicator);
      indicator_width=SLtt_Screen_Cols-2;
      if(indicator_width<40)
	indicator_width=40;
      lyric_row=2;
    }
  else
    msg_row = 6;
  memset(comment_indicator_buffer =
    (char *)safe_malloc(indicator_width), 0, indicator_width);
  memset(current_indicator_message =
    (char *)safe_malloc(indicator_width), 0, indicator_width);
  _ctl_refresh();

  return 0;
}

static void ctl_close(void)
{
  if (ctl.opened)
    {
        SLsmg_normal();
        SLsmg_gotorc(SLtt_Screen_Rows - 1, 0);
        SLsmg_refresh();
        SLsmg_reset_smg();
        SLang_reset_tty();
      ctl.opened=0;
    }
}

static int ctl_read(int32 *valp)
{
  int c;

  if (!SLang_input_pending(0))
    return RC_NONE;

  c=SLang_getkey();
    switch(c)
      {
      case 'h':
      case '?':
        ctl_help_mode();
        return RC_NONE;

      case 'V':
        *valp=10;
        return RC_CHANGE_VOLUME;
      case 'v':
        *valp=-10;
        return RC_CHANGE_VOLUME;
      case 'q':
        return RC_QUIT;
      case 'n':
        return RC_NEXT;
      case 'p':
        return RC_REALLY_PREVIOUS;
      case 'r':
        return RC_RESTART;

      case 'f':
        *valp=play_mode->rate;
        return RC_FORWARD;
      case 'b':
        *valp=play_mode->rate;
        return RC_BACK;
      case 's':
	return RC_TOGGLE_PAUSE;
      }
  return RC_NONE;
}

/*ARGSUSED*/
static void ctl_lyric(int lyricid)
{
    char *lyric;

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
	if(*lyric == ME_KARAOKE_LYRIC)
	{
	    if(lyric[1] == '/')
	    {
		display_lyric("\n", LYRIC_WORD_NOSEP);
		display_lyric(lyric + 2, LYRIC_WORD_NOSEP);
	    }
	    else if(lyric[1] == '\\')
	    {
		display_lyric("\r", LYRIC_WORD_NOSEP);
		display_lyric(lyric + 2, LYRIC_WORD_NOSEP);
	    }
	    else if(lyric[1] == '@' && lyric[2] == 'T')
	    {
		if(ctl.trace_playing)
		{
		    display_lyric("\n", LYRIC_WORD_NOSEP);
		    display_lyric(lyric + 3, LYRIC_WORD_SEP);
		}
		else
		    display_title(lyric + 3);
	    }
	    else if(lyric[1] == '@' && lyric[2] == 'L')
	    {
		init_lyric(lyric + 3);
	    }
	    else
		display_lyric(lyric + 1, LYRIC_WORD_NOSEP);
	}
	else
	{
	    if(*lyric == ME_CHORUS_TEXT || *lyric == ME_INSERT_TEXT)
		display_lyric("\r", LYRIC_WORD_SEP);
	    display_lyric(lyric + 1, LYRIC_WORD_SEP);
	}
    }
}

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
  va_list ap;
  char p[1000];
  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
  va_start(ap, fmt);
  if (!ctl.opened)
    {
      vfprintf(stderr, fmt, ap);
      fprintf(stderr, "\n");
    }
  else if (ctl.trace_playing)
    {
      switch(type)
      {
        /* Pretty pointless to only have one line for messages, but... */
      case CMSG_WARNING:
      case CMSG_ERROR:
      case CMSG_FATAL:
        SLsmg_gotorc(2,0);
      SLsmg_erase_eol();
      SLsmg_bold();
        vsnprintf(p, sizeof(p), fmt, ap);
        SLsmg_write_string(p);
      SLsmg_normal();
        _ctl_refresh();
        if (type==CMSG_WARNING)
          sleep(1); /* Don't you just _HATE_ it when programs do this... */
        else
          sleep(2);
        SLsmg_gotorc(2,0);
      SLsmg_erase_eol();
        _ctl_refresh();
        break;
      }
    }
  else
    {
      SLsmg_gotorc(msg_row++,0);
      if(msg_row==SLtt_Screen_Rows){
	int i;
        msg_row=6;
	for(i=6;i<=SLtt_Screen_Rows;i++){
          SLsmg_gotorc(i,0);
          SLsmg_erase_eol();
	}
      }
      switch(type)
      {
      default:
        vsnprintf(p, sizeof(p), fmt, ap);
        SLsmg_write_string(p);
        _ctl_refresh();
        break;

      case CMSG_WARNING:
      SLsmg_bold();
        vsnprintf(p, sizeof(p), fmt, ap);
        SLsmg_write_string(p);
      SLsmg_normal();
        _ctl_refresh();
        break;

      case CMSG_ERROR:
      case CMSG_FATAL:
      SLsmg_bold();
        vsnprintf(p, sizeof(p), fmt, ap);
        SLsmg_write_string(p);
      SLsmg_normal();
        _ctl_refresh();
        if (type==CMSG_FATAL)
          sleep(2);
        break;
      }
    }

  va_end(ap);
  return 0;
}

/* Indicator */

static void reset_indicator(void)
{
    int i;

    memset(comment_indicator_buffer, ' ', indicator_width - 1);
    comment_indicator_buffer[indicator_width - 1] = '\0';

    next_indicator_chan = -1;
    indicator_last_update = get_current_calender_time();
    indicator_mode = INDICATOR_DEFAULT;
    indicator_msgptr = NULL;

    for(i = 0; i < MAX_CHANNELS; i++)
    {
	instr_comment[i].last_note_on = 0.0;
	instr_comment[i].comm = channel_instrum_name(i);
    }
}

static void update_indicator(void)
{
    double t;
    int i;
    char c;

    t = get_current_calender_time();
    if(indicator_mode != INDICATOR_DEFAULT)
    {
	int save_chan;
	if(indicator_last_update + SCRMODE_OUT_THRESHOLD > t)
	    return;
	save_chan = next_indicator_chan;
	reset_indicator();
	next_indicator_chan = save_chan;
    }
    else
    {
	if(indicator_last_update + INDICATOR_UPDATE_TIME > t)
	    return;
    }
    indicator_last_update = t;

    if(indicator_msgptr != NULL && *indicator_msgptr == '\0')
	indicator_msgptr = NULL;

    if(indicator_msgptr == NULL)
    {
	if(next_indicator_chan >= 0 &&
	   instr_comment[next_indicator_chan].comm != NULL &&
	   *instr_comment[next_indicator_chan].comm)
	{
	    current_indicator_chan = next_indicator_chan;
	}
	else
	{
	    int prog;

	    prog = instr_comment[current_indicator_chan].prog;
	    for(i = 0; i < MAX_CHANNELS; i++)
	    {
		current_indicator_chan++;
		if(current_indicator_chan == MAX_CHANNELS)
		    current_indicator_chan = 0;


		if(instr_comment[current_indicator_chan].comm != NULL &&
		   *instr_comment[current_indicator_chan].comm &&
		   instr_comment[current_indicator_chan].prog != prog &&
		   (instr_comment[current_indicator_chan].last_note_on + CHECK_NOTE_SLEEP_TIME > t ||
		    instr_comment[current_indicator_chan].disp_cnt == 0))
		    break;
	    }

	    if(i == MAX_CHANNELS)
		return;
	}
	next_indicator_chan = -1;

	if(instr_comment[current_indicator_chan].comm == NULL ||
	   *instr_comment[current_indicator_chan].comm == '\0')
	    return;

	snprintf(current_indicator_message, indicator_width, "%03d:%s   ",
		instr_comment[current_indicator_chan].prog,
		instr_comment[current_indicator_chan].comm);
	instr_comment[current_indicator_chan].disp_cnt++;
	indicator_msgptr = current_indicator_message;
    }

    c = *indicator_msgptr++;

    for(i = 0; i < indicator_width - 2; i++)
	comment_indicator_buffer[i] = comment_indicator_buffer[i + 1];
    comment_indicator_buffer[indicator_width - 2] = c;
    SLsmg_printfrc(2,0,comment_indicator_buffer);
    ctl_refresh();
}

static void display_lyric(char *lyric, int sep)
{
    char *p;
    int len, idlen, sepoffset;
    static int crflag = 0;

    if(lyric == NULL)
    {
	indicator_last_update = get_current_calender_time();
	crflag = 0;
	return;
    }

    if(indicator_mode != INDICATOR_LYRIC || crflag)
    {
	memset(comment_indicator_buffer, 0, indicator_width);
	SLsmg_gotorc(lyric_row,0);
        SLsmg_erase_eol();
	ctl_refresh();
	indicator_mode = INDICATOR_LYRIC;
	crflag = 0;
    }

    if(*lyric == '\0')
    {
	indicator_last_update = get_current_calender_time();
	return;
    }
    else if(*lyric == '\n')
    {
	if(!ctl.trace_playing)
	{
	    crflag = 1;
	    lyric_row++;
	    SLsmg_gotorc(0,lyric_row);
	    return;
	}
	else
	    lyric = " / ";
    }

    if(strchr(lyric, '\r') != NULL)
    {
	crflag = 1;
	if(!ctl.trace_playing)
	{
	    int i;
	    for(i = title_row+1; i <= lyric_row; i++)
	    {
		SLsmg_gotorc(i,0);
		SLsmg_erase_eol();
	    }
	    lyric_row = title_row+1;
	}
	if(lyric[0] == '\r' && lyric[1] == '\0')
	{
	    indicator_last_update = get_current_calender_time();
	    return;
	}
    }

    idlen = strlen(comment_indicator_buffer);
    len = strlen(lyric);

    if(sep)
    {
	while(idlen > 0 && comment_indicator_buffer[idlen - 1] == ' ')
	    comment_indicator_buffer[--idlen] = '\0';
	while(len > 0 && lyric[len - 1] == ' ')
	    len--;
    }

    if(len == 0)
    {
	/* update time stamp */
	indicator_last_update = get_current_calender_time();
	reuse_mblock(&tmpbuffer);
	return;
    }

    sepoffset = (sep != 0);

    if(len >= indicator_width - 2)
    {
	memcpy(comment_indicator_buffer, lyric, indicator_width - 1);
	comment_indicator_buffer[indicator_width - 1] = '\0';
    }
    else if(idlen == 0)
    {
	memcpy(comment_indicator_buffer, lyric, len);
	comment_indicator_buffer[len] = '\0';
    }
    else if(len + idlen + 2 < indicator_width)
    {
	if(sep)
	    comment_indicator_buffer[idlen] = sep;
	memcpy(comment_indicator_buffer + idlen + sepoffset, lyric, len);
	comment_indicator_buffer[idlen + sepoffset + len] = '\0';
    }
    else
    {
	int spaces;
	p = comment_indicator_buffer;
	spaces = indicator_width - idlen - 2;

	while(spaces < len)
	{
	    char *q;

	    /* skip one word */
	    if((q = strchr(p, ' ')) == NULL)
	    {
		p = NULL;
		break;
	    }

	    do q++; while(*q == ' ');
	    spaces += (q - p);
	    p = q;
	}

	if(p == NULL)
	{
	    SLsmg_gotorc(lyric_row,0);
	    SLsmg_erase_eol();
	    memcpy(comment_indicator_buffer, lyric, len);
	    comment_indicator_buffer[len] = '\0';
	}
	else
	{
	    int d, l, r, i, j;

	    d = (p - comment_indicator_buffer);
	    l = strlen(p);
	    r = len - (indicator_width - 2 - l - d);

	    j = d - r;
	    for(i = 0; i < j; i++)
		comment_indicator_buffer[i] = ' ';
	    for(i = 0; i < l; i++)
		comment_indicator_buffer[j + i] =
		    comment_indicator_buffer[d + i];
	    if(sep)
		comment_indicator_buffer[j + i] = sep;
	    memcpy(comment_indicator_buffer + j + i + sepoffset, lyric, len);
	    comment_indicator_buffer[j + i + sepoffset + len] = '\0';
	}
    }

    SLsmg_printfrc(lyric_row,0,"%s",comment_indicator_buffer);
    ctl_refresh();
    reuse_mblock(&tmpbuffer);
    indicator_last_update = get_current_calender_time();
}

static void display_title(char *title)
{
    SLsmg_printfrc(title_row,0,"Title:");
    SLsmg_bold();
    SLsmg_printfrc(title_row++,7,"%s", title);
    SLsmg_normal();
    lyric_row = title_row + 1;
}

static void init_lyric(char *lang)
{
    int i;

    if(ctl.trace_playing)
      return;
    for(i=6;i<=SLtt_Screen_Rows;i++){
      SLsmg_gotorc(i,0);
      SLsmg_erase_eol();
    }
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
	/* update_indicator(); */
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
	ctl_lyric((int)e->v1);
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
ControlMode *interface_s_loader(void)
{
    return &ctl;
}
