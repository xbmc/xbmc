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

    vt_100_c.c - written by Masanao Izumo <iz@onicos.co.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#ifdef __W32__
#include <windows.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"
#include "vt100.h"
#include "timer.h"
#include "bitset.h"
#include "aq.h"

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
static Bitset channel_program_flags[MAX_CHANNELS];

static void update_indicator(void);
static void reset_indicator(void);
static void indicator_chan_update(int ch);
static void indicator_set_prog(int ch, int val, char *comm);
static void display_lyric(char *lyric, int sep);
static void display_title(char *title);
static void init_lyric(char *lang);
static char *vt100_getline(void);

#define LYRIC_WORD_NOSEP	0
#define LYRIC_WORD_SEP		' '

static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct, int nv);
static const char note_name_char[12] =
{
    'c', 'C', 'd', 'D', 'e', 'f', 'F', 'g', 'G', 'a', 'A', 'b'
};

static void ctl_note(int status, int ch, int note, int vel);
static void ctl_program(int ch, int val, void *vp);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void ctl_pitch_bend(int channel, int val);
static void ctl_lyric(uint16 lyricid);

static void ctl_reset(void);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);

/**********************************************/
/* export the interface functions */

#define ctl vt100_control_mode

ControlMode ctl=
{
    "vt100 interface", 'T',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    dumb_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};

static int selected_channel = -1;
static int lyric_row = 6;
static int title_row = 6;
static int msg_row = 6;

static void ctl_refresh(void)
{
    if(ctl.opened)
	vt100_refresh();
}

static void ctl_total_time(int tt)
{
    int mins, secs=tt/play_mode->rate;
    mins=secs/60;
    secs-=mins*60;

    vt100_move(4, 6+6+3);
    vt100_set_attr(VT100_ATTR_BOLD);
    printf("%3d:%02d  ", mins, secs);
    vt100_reset_attr();
    ctl_current_time(0, 0);
}

static void ctl_master_volume(int mv)
{
    vt100_move(4, VT100_COLS-5);
    vt100_set_attr(VT100_ATTR_BOLD);
    printf("%03d %%", mv);
    vt100_reset_attr();
    ctl_refresh();
}

static void ctl_file_name(char *name)
{
    int i;

    vt100_move(3, 6);
    vt100_clrtoeol();
    vt100_set_attr(VT100_ATTR_BOLD);
    fputs(name, stdout);
    vt100_reset_attr();

    if(ctl.trace_playing)
    {
	memset(instr_comment, 0, sizeof(instr_comment));
	for(i = 0; i < MAX_CHANNELS; i++)
	    instr_comment[i].disp_cnt = 1;
	indicator_msgptr = NULL;
	for(i = 0; i < indicator_width; i++)
	    comment_indicator_buffer[i] = ' ';
    }
    ctl_refresh();
}

static void ctl_current_time(int secs, int v)
{
    int mins, bold_flag = 0;
    static int last_voices = -1, last_secs = -1;

    if(last_secs != secs)
    {
	last_secs=secs;
	mins=secs/60;
	secs-=mins*60;
	vt100_move(4, 6);
	vt100_set_attr(VT100_ATTR_BOLD);
	printf("%3d:%02d", mins, secs);
	bold_flag = 1;
    }

    if(!ctl.trace_playing || midi_trace.flush_flag)
    {
	if(bold_flag)
	    vt100_reset_attr();
	return;
    }

    vt100_move(4, 47);
    if(!bold_flag)
	vt100_set_attr(VT100_ATTR_BOLD);
    printf("%3d", v);
    vt100_reset_attr();

    if(last_voices != voices)
    {
	last_voices = voices;
	vt100_move(4, 52);
	printf("%3d", voices);
    }
}

static void ctl_note(int status, int ch, int note, int vel)
{
    int xl, n, c;
    unsigned int onoff, check, prev_check;
    Bitset *bitset;

    if(ch >= 16)
	return;

    if (!ctl.trace_playing || midi_trace.flush_flag)
	return;

    n = note_name_char[note % 12];
    c = (VT100_COLS - 24) / 12 * 12;
    if(c <= 0)
	c = 1;
    xl=note % c;
    vt100_move(8 + ch, xl + 3);
    switch(status)
    {
      case VOICE_DIE:
	putc(',', stdout);
	onoff = 0;
	break;
      case VOICE_FREE:
	putc('.', stdout);
	onoff = 0;
	break;
      case VOICE_ON:
	vt100_set_attr(VT100_ATTR_REVERSE);
	putc(n, stdout);
	vt100_reset_attr();
	indicator_chan_update(ch);
	onoff = 1;
	break;
      case VOICE_SUSTAINED:
	vt100_set_attr(VT100_ATTR_BOLD);
	putc(n, stdout);
	vt100_reset_attr();
	onoff = 0;
	break;
      case VOICE_OFF:
	putc(n, stdout);
	onoff = 0;
	break;
    }

    bitset = channel_program_flags + ch;
    prev_check = has_bitset(bitset);
    if(prev_check == onoff)
    {
	/* Not change program mark */
	onoff <<= (8 * sizeof(onoff) - 1);
	set_bitset(bitset, &onoff, note, 1);
	return;
    }
    onoff <<= (8 * sizeof(onoff) - 1);
    set_bitset(bitset, &onoff, note, 1);
    check = has_bitset(bitset);

    if(prev_check ^ check)
    {
	vt100_move(8 + ch, VT100_COLS - 21);
	if(check)
	{
	    vt100_set_attr(VT100_ATTR_BOLD);
	    putc('*', stdout);
	    vt100_reset_attr();
	}
	else
	{
	    putc(' ', stdout);
	}
    }
}

static void ctl_program(int ch, int val, void *comm)
{
    int pr;
    if(ch >= 16)
	return;
    if (!ctl.trace_playing || midi_trace.flush_flag)
	return;
    if(channel[ch].special_sample)
	pr = val = channel[ch].special_sample;
    else
	pr = val + progbase;
    vt100_move(8+ch, VT100_COLS-21);
    if (ISDRUMCHANNEL(ch))
    {
	vt100_set_attr(VT100_ATTR_BOLD);
	printf(" %03d", pr);
	vt100_reset_attr();
    }
    else
	printf(" %03d", pr);

  if(comm != NULL)
      indicator_set_prog(ch, val, (char *)comm);
}

static void ctl_volume(int ch, int val)
{
    if(ch >= 16)
	return;
    if (!ctl.trace_playing || midi_trace.flush_flag)
	return;
    vt100_move(8 + ch, VT100_COLS - 16);
    printf("%3d", (val * 100) / 127);
}

static void ctl_expression(int ch, int val)
{
    if(ch >= 16)
	return;
    if (!ctl.trace_playing || midi_trace.flush_flag)
	return;
    vt100_move(8 + ch, VT100_COLS - 12);
    printf("%3d", (val * 100) / 127);
}

static void ctl_panning(int ch, int val)
{
    if(ch >= 16)
	return;
    if (!ctl.trace_playing || midi_trace.flush_flag)
	return;
    vt100_move(8 + ch, VT100_COLS - 8);
    if (val==NO_PANNING)
	fputs("   ", stdout);
    else if (val<5)
	fputs(" L ", stdout);
    else if (val>123)
	fputs(" R ", stdout);
    else if (val>60 && val<68)
	fputs(" C ", stdout);
    else
    {
	val = (100*(val-64))/64; /* piss on curses */
	if (val<0)
	{
	    putc('-', stdout);
	    val=-val;
	}
	else
	    putc('+', stdout);
	printf("%02d", val);
    }
}

static void ctl_sustain(int ch, int val)
{
    if(ch >= 16)
	return;
    if (!ctl.trace_playing || midi_trace.flush_flag)
	return;
    vt100_move(8 + ch, VT100_COLS - 4);
    if (val) putc('S', stdout);
    else putc(' ', stdout);
}

static void ctl_pitch_bend(int ch, int val)
{
    if(ch >= 16)
	return;
    if (!ctl.trace_playing || midi_trace.flush_flag)
	return;
    vt100_move(8+ch, VT100_COLS-2);
    if (val==-1) putc('=', stdout);
    else if (val>0x2000) putc('+', stdout);
    else if (val<0x2000) putc('-', stdout);
    else putc(' ', stdout);
}

/*ARGSUSED*/
static void ctl_lyric(uint16 lyricid)
{
    char *lyric;

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
        /* EAW -- if not a true KAR lyric, ignore \r, treat \n as \r */
        if (*lyric != ME_KARAOKE_LYRIC) {
            while (strchr(lyric, '\r')) {
            	*(strchr(lyric, '\r')) = ' ';
            }
            while (strchr(lyric, '\n')) {
                *(strchr(lyric, '\n')) = '\r';
            }
        }

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

static void ctl_reset(void)
{
    int i,j,c;
    char *title;

    if (!ctl.trace_playing)
	return;
    c = (VT100_COLS - 24) / 12 * 12;
    if(c <= 0)
	c = 1;
    for (i=0; i<16; i++)
    {
	vt100_move(8+i, 3);
	for (j=0; j<c; j++)
	    putc('.', stdout);
	if(ISDRUMCHANNEL(i))
	    ctl_program(i, channel[i].bank, channel_instrum_name(i));
	else
	    ctl_program(i, channel[i].program, channel_instrum_name(i));
	ctl_volume(i, channel[i].volume);
	ctl_expression(i, channel[i].expression);
	ctl_panning(i, channel[i].panning);
	ctl_sustain(i, channel[i].sustain);
	if(channel[i].pitchbend == 0x2000 && channel[i].mod.val > 0)
	    ctl_pitch_bend(i, -1);
	else
	    ctl_pitch_bend(i, channel[i].pitchbend);
	clear_bitset(channel_program_flags + i, 0, 128);
    }

    reset_indicator();
    display_lyric(NULL, LYRIC_WORD_NOSEP);
    if((title = get_midi_title(NULL)) != NULL)
	display_lyric(title, LYRIC_WORD_NOSEP);

    ctl_refresh();
}

/***********************************************************************/

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
    int i;

    vt100_init_screen();
    ctl.opened=1;

    vt100_move(0, 0);
    fprintf(stdout, "TiMidity++ %s%s" NLS,
    		(strcmp(timidity_version, "current")) ? "v" : "",
    		timidity_version);
    vt100_move(0, VT100_COLS-45);
    fputs("(C) 1995 Tuukka Toivonen <tt@cgs.fi>", stdout);
    vt100_move(1,0);
    fputs("vt100 Interface mode - Written by Masanao Izumo <mo@goice.co.jp>", stdout);

    vt100_move(3,0);
    fputs("File:", stdout);
    vt100_move(4,0);
    if (ctl.trace_playing)
    {
	fputs("Time:", stdout);
	vt100_move(4,6+6+1);
	putc('/', stdout);
	vt100_move(4,40);
	printf("Voices:    /%3d", voices);
    }
    else
    {
	fputs("Time:", stdout);
	vt100_move(4,6+6+1);
	putc('/', stdout);
    }
    vt100_move(4,VT100_COLS-20);
    fputs("Master volume:", stdout);
    vt100_move(5,0);
    for (i=0; i<VT100_COLS; i++)
	putc('_', stdout);
    if (ctl.trace_playing)
    {
	int o;

	vt100_move(6,0);
	fputs("Ch ", stdout);
	o = (VT100_COLS - 24) / 12;
	for(i = 0; i < o; i++)
	{
	    int j, c;
	    for(j = 0; j < 12; j++)
	    {
		c = note_name_char[j];
		if(islower(c))
		    putc(c, stdout);
		else
		    putc(' ', stdout);
	    }
	}
	vt100_move(6,VT100_COLS-20);
	fputs("Prg Vol Exp Pan S B", stdout);
	vt100_move(7,0);
	for (i=0; i<VT100_COLS; i++)
	    putc('-', stdout);
	for (i=0; i<16; i++)
	{
	    vt100_move(8+i, 0);
	    printf("%02d ", i+1);
	    init_bitset(channel_program_flags + i, 128);
	}

	set_trace_loop_hook(update_indicator);
	indicator_width = VT100_COLS - 2;
	if(indicator_width < 40)
	    indicator_width = 40;
	lyric_row = 2;
	msg_row = 2;
    }
    memset(comment_indicator_buffer =
	(char *)safe_malloc(indicator_width), 0, indicator_width);
    memset(current_indicator_message =
	(char *)safe_malloc(indicator_width), 0, indicator_width);
    ctl_refresh();

    return 0;
}

static void ctl_close(void)
{
    if (ctl.opened)
    {
	ctl.opened = 0;
	vt100_move(24, 0);
	vt100_refresh();
    }
}

static int char_count(const char *s, int c)
{
    int n;

    n = 0;
    while(*s == c)
    {
	n++;
	s++;
    }
    if('0' <= *s && *s <= '9')
	n = (n - 1) + atoi(s);
    return n;
}

static void move_select_channel(int diff)
{
    if(selected_channel != -1)
    {
	/* erase the mark */
	vt100_move(8 + selected_channel, 0);
	printf("%02d", selected_channel + 1);
    }
    selected_channel += diff;
    while(selected_channel < 0)
	selected_channel += 17;
    while(selected_channel >= 16)
	selected_channel -= 17;

    if(selected_channel != -1)
    {
	vt100_move(8 + selected_channel, 0);
	vt100_set_attr(VT100_ATTR_BOLD);
	printf("%02d", selected_channel + 1);
	vt100_reset_attr();
	if(instr_comment[selected_channel].comm != NULL)
	{
	    if(indicator_mode != INDICATOR_DEFAULT)
		reset_indicator();
	    next_indicator_chan = selected_channel;
	}
    }
}

static int ctl_read(int32 *valp)
{
    char *cmd;

    if((cmd = vt100_getline()) == NULL)
	return RC_NONE;
    switch(cmd[0])
	{
	  case 'q':
	    trace_flush();
	    return RC_QUIT;
	  case 'V':
	    *valp = 10 * char_count(cmd, cmd[0]);
	    return RC_CHANGE_VOLUME;
	  case 'v':
	    *valp =- 10 * char_count(cmd, cmd[0]);
	    return RC_CHANGE_VOLUME;
#if 0
	  case '1':
	  case '2':
	  case '3':
	    *valp=cmd[0] - '2';
	    return RC_CHANGE_REV_EFFB;
	  case '4':
	  case '5':
	  case '6':
	    *valp = cmd[0] - '5';
	    return RC_CHANGE_REV_TIME;
#endif
	  case 's':
	    return RC_TOGGLE_PAUSE;
	  case 'n':
	    return RC_NEXT;
	  case 'p':
	    return RC_REALLY_PREVIOUS;
	  case 'r':
	    return RC_RESTART;
	  case 'f':
	    *valp=play_mode->rate * char_count(cmd, cmd[0]);
	    return RC_FORWARD;
	  case 'b':
	    *valp=play_mode->rate * char_count(cmd, cmd[0]);
	    return RC_BACK;
	  case '+':
	    *valp = char_count(cmd, cmd[0]);
	    return RC_KEYUP;
	  case '-':
	    *valp = -char_count(cmd, cmd[0]);
	    return RC_KEYDOWN;
	  case '>':
	    *valp = char_count(cmd, cmd[0]);
	    return RC_SPEEDUP;
	  case '<':
	    *valp = char_count(cmd, cmd[0]);
	    return RC_SPEEDDOWN;
	  case 'O':
	    *valp = char_count(cmd, cmd[0]);
	    return RC_VOICEINCR;
	  case 'o':
	    *valp = char_count(cmd, cmd[0]);
	    return RC_VOICEDECR;
	  case 'c':
	    *valp = char_count(cmd, cmd[0]);
	    move_select_channel(*valp);
	    break;
	  case 'C':
	    *valp = char_count(cmd, cmd[0]);
	    move_select_channel(-*valp);
	    break;
	  case 'd':
	    if(selected_channel != -1)
	    {
		*valp = selected_channel;
		return RC_TOGGLE_DRUMCHAN;
	    }
	    break;
	  case 'g':
	    return RC_TOGGLE_SNDSPEC;
	}

    if(cmd[0] == '\033' && cmd[1] == '[')
	{
	    switch(cmd[2])
	    {
	      case 'A':
		*valp=10;
		return RC_CHANGE_VOLUME;
	      case 'B':
		*valp=-10;
		return RC_CHANGE_VOLUME;
	      case 'C':
		*valp=play_mode->rate;
		return RC_FORWARD;
	      case 'D':
		*valp=play_mode->rate;
		return RC_BACK;
	    }
	    return RC_NONE;
	}
    return RC_NONE;
}

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
    va_list ap;
    if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
	ctl.verbosity<verbosity_level)
	return 0;
    va_start(ap, fmt);
    if (!ctl.opened)
    {
	vfprintf(stderr, fmt, ap);
	fputs(NLS, stderr);
    }
    else
    {
	char *buff;
	int i;
	MBlockList pool;

	init_mblock(&pool);
	buff = (char *)new_segment(&pool, MIN_MBLOCK_SIZE);
	vsnprintf(buff, MIN_MBLOCK_SIZE, fmt, ap);
	for(i = 0; i < VT100_COLS - 1 && buff[i]; i++)
	    if(buff[i] == '\n' || buff[i] == '\r' || buff[i] == '\t')
		buff[i] = ' ';
	buff[i] = '\0';
	if(!ctl.trace_playing){
	  msg_row++;
	  if(msg_row == VT100_ROWS)
	  {
	    int i;
	    msg_row = 6;
	    for(i = 6; i <= VT100_ROWS; i++)
	    {
	      vt100_move(i, 0);
	      vt100_clrtoeol();
	    }
	  }
	}
	vt100_move(msg_row,0);
	vt100_clrtoeol();

	switch(type)
	{
	  case CMSG_WARNING:
	  case CMSG_ERROR:
	  case CMSG_FATAL:
	    vt100_set_attr(VT100_ATTR_REVERSE);
	    fputs(buff, stdout);
	    vt100_reset_attr();
	    break;
	  default:
	    fputs(buff, stdout);
	    break;
	}
	ctl_refresh();
	if(type == CMSG_ERROR || type == CMSG_FATAL)
	    sleep(2);
	reuse_mblock(&pool);
    }

    va_end(ap);
    return 0;
}

#if !defined(__W32__) || defined(__CYGWIN32__)
/* UNIX */
static char *vt100_getline(void)
{
    static char cmd[VT100_COLS];
    fd_set fds;
    int cnt;
    struct timeval timeout;

    FD_ZERO(&fds);
    FD_SET(0, &fds);
    timeout.tv_sec = timeout.tv_usec = 0;
    if((cnt = select(1, &fds, NULL, NULL, &timeout)) < 0)
    {
	perror("select");
	return NULL;
    }

    if(cnt > 0 && FD_ISSET(0, &fds) != 0)
    {
	if(fgets(cmd, sizeof(cmd), stdin) == NULL)
	{
	    rewind(stdin);
	    return NULL;
	}
	return cmd;
    }

    return NULL;
}
#else
/* Windows */

/* Define VT100_CBREAK_MODE if you want to emulate like ncurses mode */
/* #define VT100_CBREAK_MODE */

#include <conio.h>
static char *vt100_getline(void)
{
    static char cmd[VT100_COLS];
    static int cmdlen = 0;
    int c;

    if(kbhit())
    {
	c = getch();
	if(c == 'q' || c == 3 || c == 4)
	    return "q";
	if(c == '\r')
	    c = '\n';

#ifdef VT100_CBREAK_MODE
	cmd[0] = c;
	cmd[1] = '\0';
	return cmd;
#else
	if(cmdlen < sizeof(cmd) - 1)
	    cmd[cmdlen++] = (char)c;
	if(c == '\n')
	{
	    cmd[cmdlen] = '\0';
	    cmdlen = 0;
	    return cmd;
	}
#endif /* VT100_CBREAK_MODE */
    }
    return NULL;
}
#endif


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
    vt100_move(msg_row, 0);
    fputs(comment_indicator_buffer, stdout);
    ctl_refresh();
}

static void indicator_chan_update(int ch)
{
    double t;

    t = get_current_calender_time();
    if(next_indicator_chan == -1 &&
       instr_comment[ch].last_note_on + CHECK_NOTE_SLEEP_TIME < t)
	next_indicator_chan = ch;
    instr_comment[ch].last_note_on = t;
    instr_comment[ch].disp_cnt = 0;
    if(instr_comment[ch].comm == NULL)
    {
	if((instr_comment[ch].comm = default_instrument_name) == NULL)
	{
	    if(!ISDRUMCHANNEL(ch))
		instr_comment[ch].comm = "<GrandPiano>";
	    else
		instr_comment[ch].comm = "<Drum>";
	}
    }
}

static void indicator_set_prog(int ch, int val, char *comm)
{
    instr_comment[ch].comm = comm;
    instr_comment[ch].prog = val;
    instr_comment[ch].last_note_on = 0.0;
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
	vt100_move(lyric_row, 0);
	vt100_clrtoeol();
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
	    vt100_move(lyric_row, 0);
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
		vt100_move(i, 0);
		vt100_clrtoeol();
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
	    vt100_move(lyric_row, 0);
	    vt100_clrtoeol();
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

    vt100_move(lyric_row, 0);
    fputs(comment_indicator_buffer, stdout);
    ctl_refresh();
    reuse_mblock(&tmpbuffer);
    indicator_last_update = get_current_calender_time();
}

static void display_title(char *title)
{
    vt100_move(title_row, 0);
    printf("Title:");
    vt100_move(title_row++, 7);
    vt100_set_attr(VT100_ATTR_BOLD);
    printf("%s", title);
    vt100_reset_attr();
    lyric_row = title_row + 1;
}

static void init_lyric(char *lang)
{
    int i;

    if(ctl.trace_playing)
	return;

    msg_row = 6;
    for(i = 6; i <= VT100_ROWS; i++)
    {
	vt100_move(i, 0);
	vt100_clrtoeol();
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
	update_indicator();
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
	ctl_program((int)e->v1, (int)e->v2, (char *)e->v3);
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
ControlMode *interface_T_loader(void)
{
    return &ctl;
}
