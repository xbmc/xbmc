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

    ncurs_c.c: written by Masanao Izumo <iz@onicos.co.jp>
                      and Aoki Daisuke <dai@y7.net>.
    This version is merged with title list mode from Aoki Daisuke.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>

#if defined(__MINGW32__) && defined(USE_PDCURSES)
#define _NO_OLDNAMES 1	/* avoid type mismatch of beep() */
#ifndef sleep
extern void sleep(unsigned long);
#endif /* sleep */
#include <stdlib.h>
#undef _NO_OLDNAMES
#else /* USE_PDCURSES */
#include <stdlib.h>
#endif

#include <stdarg.h>
#include <ctype.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef __W32__
#include <windows.h>
#endif /* __W32__ */

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#else
#include <curses.h>
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
#include "bitset.h"
#include "arc.h"
#include "aq.h"

#ifdef USE_PDCURSES
int PDC_set_ctrl_break(bool setting);
#endif /* USE_PDCURSES */

#define SCREEN_BUGFIX 1 /* FIX the old ncurses bug */

/* Define WREFRESH_CACHED if wrefresh isn't clear the internal cache */
#define WREFRESH_CACHED 1

#ifdef JAPANESE
#define MULTIBUTE_CHAR_BUGFIX 1 /* Define to fix multibute overwrite bug */
#endif /* JAPANESE */

#define MIDI_TITLE
#define DISPLAY_MID_MODE
#define COMMAND_BUFFER_SIZE 4096
#define MINI_BUFF_MORE_C '$'
#define LYRIC_OUT_THRESHOLD 10.0
#define CHECK_NOTE_SLEEP_TIME 5.0
#define NCURS_MIN_LINES 8

#define CTL_STATUS_UPDATE -98
#define CTL_STATUS_INIT -99

#ifndef MIDI_TITLE
#undef DISPLAY_MID_MODE
#endif /* MIDI_TITLE */

#ifdef DISPLAY_MID_MODE
#if defined(JAPANESE) && !defined(__WATCOMC__)
#include "mid-j.defs"
#else
#include "mid.defs"
#endif /* JAPANESE */
#endif /* DISPLAY_MID_MODE */

#define MAX_U_PREFIX 256

/* GS LCD */
#define GS_LCD_MARK_ON		-1
#define GS_LCD_MARK_OFF		-2
#define GS_LCD_MARK_CLEAR	-3
#define GS_LCD_MARK_CHAR '$'
static double gslcd_last_display_time;
static int gslcd_displayed_flag = 0;
#define GS_LCD_CLEAR_TIME 10.0
#define GS_LCD_WIDTH 40

extern int set_extension_modes(char *flag);

static struct
{
    int mute, bank, bank_lsb, bank_msb, prog;
    int tt, vol, exp, pan, sus, pitch, wheel;
    int is_drum;
    int bend_mark;

    double last_note_on;
    char *comm;
} ChannelStatus[MAX_CHANNELS];

enum indicator_mode_t
{
    INDICATOR_DEFAULT,
    INDICATOR_LYRIC,
    INDICATOR_CMSG
};

static int indicator_width = 78;
static char *comment_indicator_buffer = NULL;
static char *current_indicator_message = NULL;
static char *indicator_msgptr = NULL;
static int current_indicator_chan = 0;
static double indicator_last_update;
static int indicator_mode = INDICATOR_DEFAULT;
static int display_velocity_flag = 0;
static int display_channels = 16;

static Bitset channel_program_flags[MAX_CHANNELS];
static Bitset gs_lcd_bits[MAX_CHANNELS];
static int is_display_lcd = 1;
static int scr_modified_flag = 1; /* delay flush for trace mode */


static void update_indicator(void);
static void reset_indicator(void);
static void indicator_chan_update(int ch);
static void display_lyric(char *lyric, int sep);
static void display_play_system(int mode);
static void display_intonation(int mode);
static void display_aq_ratio(void);

#define LYRIC_WORD_NOSEP	0
#define LYRIC_WORD_SEP		' '


static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);

static void ctl_refresh(void);
static void ctl_help_mode(void);
static void ctl_list_mode(int type);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_metronome(int meas, int beat);
static void ctl_keysig(int8 k, int ko);
static void ctl_tempo(int t, int tr);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct, int nv);
static const char note_name_char[12] =
{
    'c', 'C', 'd', 'D', 'e', 'f', 'F', 'g', 'G', 'a', 'A', 'b'
};

static void ctl_note(int status, int ch, int note, int vel);
static void ctl_temper_keysig(int8 tk, int ko);
static void ctl_temper_type(int ch, int8 tt);
static void ctl_mute(int ch, int mute);
static void ctl_drumpart(int ch, int is_drum);
static void ctl_program(int ch, int prog, char *vp, unsigned int banks);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void update_bend_mark(int ch);
static void ctl_pitch_bend(int channel, int val);
static void ctl_mod_wheel(int channel, int wheel);
static void ctl_lyric(int lyricid);
static void ctl_gslcd(int id);
static void ctl_reset(void);

/**********************************************/

/* define (LINE,ROW) */
#ifdef MIDI_TITLE
#define VERSION_LINE 0
#define HELP_LINE 1
#define FILE_LINE 2
#define FILE_TITLE_LINE 3
#define TIME_LINE 4
#define VOICE_LINE 4
#define SEPARATE1_LINE 5
#define TITLE_LINE 6
#define NOTE_LINE 7
#else
#define VERSION_LINE 0
#define HELP_LINE 1
#define FILE_LINE 2
#define TIME_LINE  3
#define VOICE_LINE 3
#define SEPARATE1_LINE 4
#define TITLE_LINE 5
#define SEPARATE2_LINE 6
#define NOTE_LINE 7
#endif

#define LIST_TITLE_LINES (LINES - TITLE_LINE - 1)

/**********************************************/
/* export the interface functions */

#define ctl ncurses_control_mode

ControlMode ctl=
{
    "ncurses interface", 'n',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};


/***********************************************************************/
/* foreground/background checks disabled since switching to curses */
/* static int in_foreground=1; */

enum ctl_ncurs_mode_t
{
    /* Major modes */
    NCURS_MODE_NONE,	/* None */
    NCURS_MODE_MAIN,	/* Normal mode */
    NCURS_MODE_TRACE,	/* Trace mode */
    NCURS_MODE_HELP,	/* Help mode */
    NCURS_MODE_LIST,	/* MIDI list mode */
    NCURS_MODE_DIR,	/* Directory list mode */

    /* Minor modes */
    /* Command input mode */
    NCURS_MODE_CMD_J,	/* Jump */
    NCURS_MODE_CMD_L,	/* Load file */
    NCURS_MODE_CMD_E,	/* Extensional mode */
    NCURS_MODE_CMD_FSEARCH,	/* forward search MIDI file */
    NCURS_MODE_CMD_D,	/* Change drum channel */
    NCURS_MODE_CMD_S,	/* Save as */
    NCURS_MODE_CMD_R	/* Change sample rate */
};
static int ctl_ncurs_mode = NCURS_MODE_MAIN; /* current mode */
static int ctl_ncurs_back = NCURS_MODE_MAIN; /* prev mode to back from help */
static int ctl_cmdmode = 0;
static int ctl_mode_L_dispstart = 0;
static char ctl_mode_L_lastenter[COMMAND_BUFFER_SIZE];
static char ctl_mode_SEARCH_lastenter[COMMAND_BUFFER_SIZE];

struct double_list_string
{
    char *string;
    struct double_list_string *next, *prev;
};
static struct double_list_string *ctl_mode_L_histh = NULL; /* head */
static struct double_list_string *ctl_mode_L_histc = NULL; /* current */

static void ctl_ncurs_mode_init(void);
static void init_trace_window_chan(int ch);
static void init_chan_status(void);
static void ctl_cmd_J_move(int diff);
static int ctl_cmd_J_enter(void);
static void ctl_cmd_L_dir(int move);
static int ctl_cmd_L_enter(void);

static int selected_channel = -1;

/* list_mode */
typedef struct _MFnode
{
    char *file;
#ifdef MIDI_TITLE
    char *title;
#endif /* MIDI_TITLE */
    struct midi_file_info *infop;
    struct _MFnode *next;
} MFnode;

static struct _file_list {
  int number;
  MFnode *MFnode_head;
  MFnode *MFnode_tail;
} file_list;

static MFnode *MFnode_nth_cdr(MFnode *p, int n);
static MFnode *current_MFnode = NULL;

#define NC_LIST_MAX 512
static int ctl_listmode=1;
static int ctl_listmode_max=1;	/* > 1 */
static int ctl_listmode_play=1;	/* > 1 */
static int ctl_list_select[NC_LIST_MAX];
static int ctl_list_from[NC_LIST_MAX];
static int ctl_list_to[NC_LIST_MAX];
static void ctl_list_table_init(void);
static MFnode *make_new_MFnode_entry(char *file);
static void insert_MFnode_entrys(MFnode *mfp, int pos);

#define NC_LIST_NEW 1
#define NC_LIST_NOW 2
#define NC_LIST_PLAY 3
#define NC_LIST_SELECT 4
#define NC_LIST_NEXT 5
#define NC_LIST_PREV 6
#define NC_LIST_UP 7
#define NC_LIST_DOWN 8
#define NC_LIST_UPPAGE 9
#define NC_LIST_DOWNPAGE 10

/* playing files */
static int nc_playfile=0;

typedef struct MiniBuffer
{
    char *buffer;	/* base buffer */
    int size;		/* size of base buffer */
    char *text;		/* pointer to buffer + (prompt length) */
    int maxlen;		/* max text len */
    int len;		/* [0..maxlen] */
    int cur;		/* cursor pos [0..len] */
    int uflag;		/* update flag */
    int cflag;		/* for file completion flag */
    MFnode *files;	/* completed files */
    char *lastcmpl;	/* last completed pathname */
    MBlockList pool;	/* memory pool */

    WINDOW *bufwin;	/* buffer window */
    int x, y;		/* window position */
    int w, h;		/* window size */
} MiniBuffer;

static MiniBuffer *command_buffer = NULL; /* command buffer */

static MiniBuffer *mini_buff_new(int size);
static void mini_buff_set(MiniBuffer *b,
			  WINDOW *bufwin, int line, char *prompt);
static void mini_buff_clear(MiniBuffer *b);
static void mini_buff_refresh(MiniBuffer *b);
static int mini_buff_forward(MiniBuffer *b);
static int mini_buff_backward(MiniBuffer *b);
static int mini_buff_insertc(MiniBuffer *b, int c);
static int mini_buff_inserts(MiniBuffer *b, char *s);
static int mini_buff_delc(MiniBuffer *b);
static char *mini_buff_gets(MiniBuffer *b);
static void mini_buff_sets(MiniBuffer *b, char *s);
static int mini_buff_len(MiniBuffer *b);
static int mini_buff_completion(MiniBuffer *b);

static WINDOW *dftwin=0, *msgwin=0, *listwin=0;


static void N_ctl_refresh(void)
{
  if(!ctl.opened)
    return;

  if(ctl_cmdmode)
      wmove(dftwin, command_buffer->y, command_buffer->x);
  else
      wmove(dftwin, 0,0);
  wrefresh(dftwin);
  scr_modified_flag = 0;
}

static void N_ctl_clrtoeol(int row)
{
    int i;

    wmove(dftwin, row, 0);
    for(i = 0; i < COLS; i++)
	waddch(dftwin, ' ');
    wmove(dftwin, row, 0);
    wrefresh(dftwin);
}

/* werase() is not collectly work if multibyte font is displayed. */
static void N_ctl_werase(WINDOW *w)
{
#ifdef WREFRESH_CACHED
    int x, y, xsize, ysize;
    getmaxyx(w, ysize, xsize);
    for(y = 0; y < ysize; y++)
    {
	wmove(w, y, 0);
	for(x = 0; x < xsize; x++)
	    waddch(w, ' ');
    }
#else
    werase(w);
#endif /* WREFRESH_CACHED */
    wmove(w, 0, 0);
    wrefresh(w);
}

static void N_ctl_scrinit(void)
{
    int i;

    N_ctl_werase(dftwin);
    wmove(dftwin, VERSION_LINE,0);
    waddstr(dftwin, "TiMidity++ ");
    if (strcmp(timidity_version, "current"))
    	waddch(dftwin, 'v');
    waddstr(dftwin, timidity_version);
    wmove(dftwin, VERSION_LINE,COLS-51);
    waddstr(dftwin, "(C) 1995,1999-2004 Tuukka Toivonen, Masanao Izumo");
    wmove(dftwin, FILE_LINE,0);
    waddstr(dftwin, "File:");
#ifdef MIDI_TITLE
    wmove(dftwin, FILE_TITLE_LINE,0);
    waddstr(dftwin, "Title:");
    for(i = 0; i < COLS - 6; i++)
	waddch(dftwin, ' ');
#endif
    wmove(dftwin, TIME_LINE,0);
    waddstr(dftwin, "Time:");
    wmove(dftwin, TIME_LINE,6 + 6);
    waddch(dftwin, '/');
    wmove(dftwin, VOICE_LINE,40);
    wprintw(dftwin, "Voices:     / %3d", voices);
    wmove(dftwin, VOICE_LINE, COLS-20);
    waddstr(dftwin, "Master volume:");
    wmove(dftwin, SEPARATE1_LINE, 0);
    for(i = 0; i < COLS; i++)
#ifdef MIDI_TITLE
	waddch(dftwin, '-');
#else
    waddch(dftwin, '_');
#endif
    wmove(dftwin, SEPARATE1_LINE, 0);
    waddstr(dftwin, "Meas: ");
    wmove(dftwin, SEPARATE1_LINE, 37);
    waddstr(dftwin, " Key: ");
    wmove(dftwin, SEPARATE1_LINE, 58);
    waddstr(dftwin, " Tempo: ");

    indicator_width = COLS - 2;
    if(indicator_width < 40)
	indicator_width = 40;
    if(comment_indicator_buffer != NULL)
	free(comment_indicator_buffer);
    if(current_indicator_message != NULL)
	free(current_indicator_message);
    memset(comment_indicator_buffer =
	   (char *)safe_malloc(indicator_width), 0, indicator_width);
    memset(current_indicator_message =
	   (char *)safe_malloc(indicator_width), 0, indicator_width);
    
    if(ctl.trace_playing)
    {
	int o;

	wmove(dftwin, TITLE_LINE, 0);
	waddstr(dftwin, "Ch ");
	o = (COLS - 28) / 12;
	for(i = 0; i < o; i++)
	{
	    int j;
	    for(j = 0; j < 12; j++)
	    {
		int c;
		c = note_name_char[j];
		if(islower(c))
		    waddch(dftwin, c);
		else
		    waddch(dftwin, ' ');
	    }
	}
	wmove(dftwin, TITLE_LINE, COLS - 20);
	waddstr(dftwin, "Prg Vol Exp Pan S B");
#ifndef MIDI_TITLE
	wmove(dftwin, SEPARATE2_LINE, 0);
	for(i = 0; i < COLS; i++)
	    waddch(dftwin, '-');
#endif
	for(i = 0; i < MAX_CHANNELS; i++)
	{
	    init_bitset(channel_program_flags + i, 128);
	    init_bitset(gs_lcd_bits + i, 128);
	}
    }
    N_ctl_refresh();
}

static void ctl_refresh(void)
{
  if (scr_modified_flag)
    N_ctl_refresh();
}

static void init_trace_window_chan(int ch)
{
    int i, c;

    if(ch >= display_channels)
	return;

    N_ctl_clrtoeol(NOTE_LINE + ch);
    ctl_mute(ch, CTL_STATUS_UPDATE);
    waddch(dftwin, ' ');
    if(ch != selected_channel)
    {
	c = (COLS - 28) / 12 * 12;
	if(c <= 0)
	    c = 1;
	for(i = 0; i < c; i++)
	    waddch(dftwin, '.');
	ctl_temper_type(ch, CTL_STATUS_UPDATE);
	ctl_program(ch, CTL_STATUS_UPDATE, NULL, 0);
	ctl_volume(ch, CTL_STATUS_UPDATE);
	ctl_expression(ch, CTL_STATUS_UPDATE);
	ctl_panning(ch, CTL_STATUS_UPDATE);
	ctl_sustain(ch, CTL_STATUS_UPDATE);
	update_bend_mark(ch);
	clear_bitset(channel_program_flags + ch, 0, 128);
    }
    else
    {
	ToneBankElement *prog;
	ToneBank *bank;
	int b, type, pr;

	b = ChannelStatus[ch].bank;
	pr = ChannelStatus[ch].prog;
	bank = tonebank[b];
	if(bank == NULL || bank->tone[pr].instrument == NULL)
	{
	    b = 0;
	    bank = tonebank[0];
	}

	if(ChannelStatus[ch].is_drum)
	{
	    wprintw(dftwin, "Drumset Bank %d=>%d",
		    ChannelStatus[ch].bank + progbase, b + progbase);
	}
	else
	{
	    if(IS_CURRENT_MOD_FILE)
	    {
		wprintw(dftwin, "MOD %d (%s)",
			ChannelStatus[ch].prog,
			ChannelStatus[ch].comm ? ChannelStatus[ch].comm :
			"Not installed");
	    }
	    else
	    {
		prog = &bank->tone[pr];

		if(prog->instrument != NULL &&
		   !IS_MAGIC_INSTRUMENT(prog->instrument))
		{
		    type = prog->instrument->type;
		    /* check instrument alias */
		    if(b != 0 &&
		       tonebank[0]->tone[pr].instrument == prog->instrument)
		    {
			b = 0;
			bank = tonebank[0];
			prog = &bank->tone[pr];
		    }
		}
		else
		    type = -1;

		wprintw(dftwin, "%d Bank %d/%d=>%d Prog %d",
			type,
			ChannelStatus[ch].bank_msb,
			ChannelStatus[ch].bank_lsb,
			b,
			ChannelStatus[ch].prog + progbase);

		if(type == INST_GUS)
		{
		    if(prog->name)
		    {
			waddch(dftwin, ' ');
			waddstr(dftwin, prog->name);
		    }
		    if(prog->comment != NULL)
			wprintw(dftwin, "(%s)", prog->comment);
		}
		else if(type == INST_SF2)
		{
		    char *name, *fn;

		    waddstr(dftwin, " (SF ");

		    if(prog->instype == 1)
		    {
			/* Restore original one */
			b = prog->font_bank;
			pr = prog->font_preset;
		    }

		    name = soundfont_preset_name(b, pr, -1, &fn);
		    if(name == NULL && b != 0)
		    {
			if((name = soundfont_preset_name(0, pr, -1, &fn)) != NULL)
			    b = 0;
		    }

		    wprintw(dftwin, "%d,%d", b, pr + progbase);

		    if(name != NULL)
		    {
			char *p;
			if((p = pathsep_strrchr(fn)) != NULL)
			    p++;
			else
			    p = fn;
			wprintw(dftwin, ",%s", name, p);
		    }
		    waddch(dftwin, ')');
		}
	    }
	}
    }
}

static void init_chan_status(void)
{
    int ch;

    for(ch = 0; ch < MAX_CHANNELS; ch++)
    {
	ChannelStatus[ch].mute = temper_type_mute & 1;
	ChannelStatus[ch].bank = 0;
	ChannelStatus[ch].bank_msb = 0;
	ChannelStatus[ch].bank_lsb = 0;
	ChannelStatus[ch].prog = 0;
	ChannelStatus[ch].tt = 0;
	ChannelStatus[ch].is_drum = ISDRUMCHANNEL(ch);
	ChannelStatus[ch].vol = 0;
	ChannelStatus[ch].exp = 0;
	ChannelStatus[ch].pan = NO_PANNING;
	ChannelStatus[ch].sus = 0;
	ChannelStatus[ch].pitch = 0x2000;
	ChannelStatus[ch].wheel = 0;
	ChannelStatus[ch].bend_mark = ' ';
	ChannelStatus[ch].last_note_on = 0.0;
	ChannelStatus[ch].comm = NULL;
    }
}

static void display_play_system(int mode)
{
    wmove(dftwin, TIME_LINE, 22);
    switch(mode)
    {
      case GM_SYSTEM_MODE:
	waddstr(dftwin, "[GM]");
	break;
      case GS_SYSTEM_MODE:
	waddstr(dftwin, "[GS]");
	break;
      case XG_SYSTEM_MODE:
	waddstr(dftwin, "[XG]");
	break;
      default:
	waddstr(dftwin, "    ");
	break;
    }
    scr_modified_flag = 1;
}

static void display_intonation(int mode)
{
	wmove(dftwin, TIME_LINE, 28);
	waddstr(dftwin, (mode == 1) ? "[PureInt]" : "         ");
	scr_modified_flag = 1;
}

static void ctl_ncurs_mode_init(void)
{
    int i;

    display_channels = LINES - 8;
    if(display_channels > MAX_CHANNELS)
	display_channels = MAX_CHANNELS;
    if(current_file_info != NULL && current_file_info->max_channel < 16)
	display_channels = 16;

    display_play_system(play_system_mode);
    display_intonation(opt_pure_intonation);
    switch(ctl_ncurs_mode)
    {
      case NCURS_MODE_MAIN:
	touchwin(msgwin);
	wrefresh(msgwin);
	break;
      case NCURS_MODE_TRACE:
	touchwin(dftwin);
	for(i = 0; i < MAX_CHANNELS; i++)
	    init_trace_window_chan(i);
	N_ctl_refresh();
	break;
      case NCURS_MODE_HELP:
	break;
      case NCURS_MODE_LIST:
	touchwin(listwin);
	ctl_list_mode(NC_LIST_NOW);
	break;
      case NCURS_MODE_DIR:
	ctl_cmd_L_dir(0);
	break;
    }
}

static void display_key_helpmsg(void)
{
    if(ctl_cmdmode || ctl_ncurs_mode == NCURS_MODE_HELP)
    {
	if(!ctl.trace_playing)
	{
	    wmove(dftwin, HELP_LINE, 0);
	    waddstr(dftwin, "Press 'h' for help with keys, or 'q' to quit.");
	    N_ctl_refresh();
	}
	return;
    }
    N_ctl_clrtoeol(LINES - 1);

    if(!ctl.trace_playing)
	wmove(dftwin, HELP_LINE, 0);
    waddstr(dftwin, "Press 'h' for help with keys, or 'q' to quit.");
    N_ctl_refresh();
}

static void ctl_help_mode(void)
{
    static WINDOW *helpwin;
    if(ctl_ncurs_mode == NCURS_MODE_HELP)
    {
	ctl_ncurs_mode = ctl_ncurs_back;
	touchwin(dftwin);
	delwin(helpwin);
	N_ctl_refresh();
	ctl_ncurs_mode_init();
	display_key_helpmsg();
    }
    else
    {
	int i;
	static char *help_message_list[] =
	{
"V/Up=Louder    b/Left=Skip back      n/Next=Next file      r/Home=Restart file",
"v/Down=Softer  f/Right=Skip forward  p/Prev=Previous file  q/End=Quit program",
"h/?=Help mode  s=Toggle pause        E=ExtMode-Setting",
"+=Key up       -=Key down            >=Speed up            <=Speed down",
"O=Voices up    o=Voices down         c/j/C/k=Move channel  d=Toggle drum prt.",
"J=Jump         L=Load & play (TAB: File completion)        t=Toggle trace mode",
"%=Display velocity (toggle)          D=Drum change         S=Save as",
"R=Change rate  Space=Toggle ch. mute .=Solo ch. play       /=Clear ch. mute",
#ifdef SUPPORT_SOUNDSPEC
"g=Open sound spectrogram window",
#else
"",
#endif /* SUPPORT_SOUNDSPEC */
"",
"l/INS=List mode",
"k/Up=Cursor up         j/Down=Cursor down Space=Select and play",
"p=Previous file play   n=Next file play   RollUp=Page up   RollDown=Page down",
"/=Search file",
NULL
};
	ctl_ncurs_back = ctl_ncurs_mode;
	ctl_ncurs_mode = NCURS_MODE_HELP;
	helpwin = newwin(LIST_TITLE_LINES, COLS, TITLE_LINE, 0);
	N_ctl_werase(helpwin);
	wattron(helpwin, A_BOLD);
	waddstr(helpwin, "                 ncurses interface Help");
	wattroff(helpwin, A_BOLD);

	for(i = 0; help_message_list[i]; i++)
	{
	    wmove(helpwin, i+1,0);
	    waddstr(helpwin, help_message_list[i]);
	}
	wmove(helpwin, i+2,0);
	wattron(helpwin, A_BOLD);
	waddstr(helpwin,
		"                   Type `h' to go to previous screen");
	wattroff(helpwin, A_BOLD);
	wrefresh(helpwin);
	N_ctl_clrtoeol(LINES - 1);
	N_ctl_refresh();
    }
}

static MFnode *MFnode_nth_cdr(MFnode *p, int n)
{
    while(p != NULL && n-- > 0)
	p = p->next;
    return p;
}

static void ctl_list_MFnode_files(MFnode *mfp, int select_id, int play_id)
{
    int i, mk;
#ifdef MIDI_TITLE
    char *item, *f, *title;
    int tlen, flen, mlen;
#ifdef DISPLAY_MID_MODE
    char *mname;
#endif /* DISPLAY_MID_MODE */
#endif /* MIDI_TITLE */

    N_ctl_werase(listwin);
    mk = 0;
    for(i = 0; i < LIST_TITLE_LINES && mfp; i++, mfp = mfp->next)
    {
	if(i == select_id || i == play_id)
	{
	    mk = 1;
	    wattron(listwin,A_REVERSE);
	}

	wmove(listwin, i, 0);
	wprintw(listwin,"%03d%c",
		i + ctl_list_from[ctl_listmode],
		i == play_id ? '*' : ' ');

#ifdef MIDI_TITLE

	if((f = pathsep_strrchr(mfp->file)) != NULL)
	    f++;
	else
	    f = mfp->file;
	flen = strlen(f);
	title = mfp->title;
	if(title != NULL)
	{
	    while(*title == ' ')
		title++;
	    tlen = strlen(title) + 1;
	}
	else
	    tlen = 0;

#ifdef DISPLAY_MID_MODE
	mname = mid2name(mfp->infop->mid);
	if(mname != NULL)
	    mlen = strlen(mname);
	else
	    mlen = 0;
#else
	mlen = 0;
#endif /* DISPLAY_MID_MODE */

	item = (char *)new_segment(&tmpbuffer, tlen + flen + mlen + 4);
	if(title != NULL)
	{
	    strcpy(item, title);
	    strcat(item, " ");
	}
	else
	    item[0] = '\0';
	strcat(item, "(");
	strcat(item, f);
	strcat(item, ")");

#ifdef DISPLAY_MID_MODE
	if(mlen)
	{
	    strcat(item, "/");
	    strcat(item, mname);
	}
#endif /* DISPLAY_MID_MODE */

	waddnstr(listwin, item, COLS-6);
	reuse_mblock(&tmpbuffer);
#else
	waddnstr(listwin, mfp->file, COLS-6);
#endif
	if(mk)
	{
	    mk = 0;
	    wattroff(listwin,A_REVERSE);
	}
    }
}

static void ctl_list_mode(int type)
{
  for(ctl_listmode_play=1;;ctl_listmode_play++) {
    if(ctl_list_from[ctl_listmode_play]<=nc_playfile
       &&nc_playfile<=ctl_list_to[ctl_listmode_play])
      break;
  }
  switch(type){
  case NC_LIST_PREV:
    if(ctl_listmode<=1)
      ctl_listmode=ctl_listmode_max;
    else
      ctl_listmode--;
    break;
  case NC_LIST_NEXT:
    if(ctl_listmode>=ctl_listmode_max)
      ctl_listmode=1;
    else
      ctl_listmode++;
    break;
  case NC_LIST_UP:
    if(ctl_list_select[ctl_listmode]<=ctl_list_from[ctl_listmode]){
      if(ctl_listmode<=1)
	ctl_listmode=ctl_listmode_max;
      else
	ctl_listmode--;
      ctl_list_select[ctl_listmode]=ctl_list_to[ctl_listmode];
    } else
      ctl_list_select[ctl_listmode]--;
    break;
  case NC_LIST_DOWN:
    if(ctl_list_select[ctl_listmode]>=ctl_list_to[ctl_listmode]){
      if(ctl_listmode>=ctl_listmode_max)
	ctl_listmode=1;
      else
	ctl_listmode++;
      ctl_list_select[ctl_listmode]=ctl_list_from[ctl_listmode];
    } else
	ctl_list_select[ctl_listmode]++;
    break;
  case NC_LIST_UPPAGE:
    if(ctl_listmode<=1)
      ctl_listmode=ctl_listmode_max;
    else
	ctl_listmode--;
    ctl_list_select[ctl_listmode]=ctl_list_to[ctl_listmode];
    break;
  case NC_LIST_DOWNPAGE:
    if(ctl_listmode>=ctl_listmode_max)
      ctl_listmode=1;
    else
	ctl_listmode++;
    ctl_list_select[ctl_listmode]=ctl_list_from[ctl_listmode];
    break;
  case NC_LIST_PLAY:
    if(ctl_ncurs_mode == NCURS_MODE_LIST)
    {
	/* leave list mode */
	if(ctl.trace_playing)
	    ctl_ncurs_mode = NCURS_MODE_TRACE;
	else
	    ctl_ncurs_mode = NCURS_MODE_MAIN;
	ctl_ncurs_mode_init();
    }
    else
    {
	/* enter list mode */
	ctl_ncurs_mode = NCURS_MODE_LIST;
    }
    ctl_ncurs_back = ctl_ncurs_mode;
    break;
  case NC_LIST_NEW:
    ctl_listmode=ctl_listmode_play;
    ctl_list_select[ctl_listmode]=nc_playfile;
    break;
  case NC_LIST_NOW:
    break;
  default:
    ;
  }
  if(ctl_ncurs_mode == NCURS_MODE_LIST)
    {
	int i;
	MFnode *mfp;

	i = ctl_list_from[ctl_listmode];
	mfp = MFnode_nth_cdr(file_list.MFnode_head, i);
	ctl_list_MFnode_files(mfp, ctl_list_select[ctl_listmode] - i,
			   nc_playfile - i);
	wrefresh(listwin);
	N_ctl_refresh();
    }
}

static void redraw_all(void)
{
    N_ctl_scrinit();
    ctl_total_time(CTL_STATUS_UPDATE);
    ctl_master_volume(CTL_STATUS_UPDATE);
    ctl_metronome(CTL_STATUS_UPDATE, CTL_STATUS_UPDATE);
    ctl_keysig(CTL_STATUS_UPDATE, CTL_STATUS_UPDATE);
    ctl_tempo(CTL_STATUS_UPDATE, CTL_STATUS_UPDATE);
    ctl_temper_keysig(CTL_STATUS_UPDATE, CTL_STATUS_UPDATE);
    display_key_helpmsg();
    ctl_file_name(NULL);
    ctl_ncurs_mode_init();
}

static void ctl_event(CtlEvent *e)
{
    if(midi_trace.flush_flag)
	return;
    switch(e->type)
    {
      case CTLE_NOW_LOADING:
	ctl_file_name((char *)e->v1);
	break;
      case CTLE_LOADING_DONE:
	redraw_all();
	break;
      case CTLE_PLAY_START:
	init_chan_status();
	ctl_ncurs_mode_init();
	ctl_total_time((int)e->v1);
	break;
      case CTLE_PLAY_END:
	break;
      case CTLE_CURRENT_TIME:
	ctl_current_time((int)e->v1, (int)e->v2);
	display_aq_ratio();
	break;
      case CTLE_NOTE:
	ctl_note((int)e->v1, (int)e->v2, (int)e->v3, (int)e->v4);
	break;
      case CTLE_MASTER_VOLUME:
	ctl_master_volume((int)e->v1);
	break;
	case CTLE_METRONOME:
		ctl_metronome((int) e->v1, (int) e->v2);
		update_indicator();
		break;
	case CTLE_KEYSIG:
		ctl_keysig((int8) e->v1, CTL_STATUS_UPDATE);
		break;
	case CTLE_KEY_OFFSET:
		ctl_keysig(CTL_STATUS_UPDATE, (int) e->v1);
		ctl_temper_keysig(CTL_STATUS_UPDATE, (int) e->v1);
		break;
	case CTLE_TEMPO:
		ctl_tempo((int) e->v1, CTL_STATUS_UPDATE);
		break;
	case CTLE_TIME_RATIO:
		ctl_tempo(CTL_STATUS_UPDATE, (int) e->v1);
		break;
	case CTLE_TEMPER_KEYSIG:
		ctl_temper_keysig((int8) e->v1, CTL_STATUS_UPDATE);
		break;
	case CTLE_TEMPER_TYPE:
		ctl_temper_type((int) e->v1, (int8) e->v2);
		break;
	case CTLE_MUTE:
		ctl_mute((int) e->v1, (int) e->v2);
		break;
      case CTLE_PROGRAM:
	ctl_program((int)e->v1, (int)e->v2, (char *)e->v3, (unsigned int)e->v4);
	break;
      case CTLE_DRUMPART:
	ctl_drumpart((int)e->v1, (int)e->v2);
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
	ctl_mod_wheel((int)e->v1, (int)e->v2);
	break;
      case CTLE_CHORUS_EFFECT:
	break;
      case CTLE_REVERB_EFFECT:
	break;
      case CTLE_LYRIC:
	ctl_lyric((int)e->v1);
	break;
      case CTLE_GSLCD:
	if(is_display_lcd)
	    ctl_gslcd((int)e->v1);
	break;
      case CTLE_REFRESH:
	ctl_refresh();
	break;
      case CTLE_RESET:
	ctl_reset();
	break;
      case CTLE_SPEANA:
	break;
      case CTLE_PAUSE:
	ctl_current_time((int)e->v2, 0);
	N_ctl_refresh();
	break;
    }
}

static void ctl_total_time(int tt)
{
    static int last_tt = CTL_STATUS_UPDATE;
    int mins, secs;

    if(tt == CTL_STATUS_UPDATE)
	tt = last_tt;
    else
	last_tt = tt;
    secs=tt/play_mode->rate;
    mins=secs/60;
    secs-=mins*60;

    wmove(dftwin, TIME_LINE,6+6+1);
    wattron(dftwin, A_BOLD);
    wprintw(dftwin, "%3d:%02d  ", mins, secs);
    wattroff(dftwin, A_BOLD);
    ctl_current_time(CTL_STATUS_INIT, 0); /* Init. */
    ctl_current_time(0, 0);
    N_ctl_refresh();
}

static void ctl_master_volume(int mv)
{
    static int lastvol = CTL_STATUS_UPDATE;

    if(mv == CTL_STATUS_UPDATE)
	mv = lastvol;
    else
	lastvol = mv;
    wmove(dftwin, VOICE_LINE,COLS-5);
    wattron(dftwin, A_BOLD);
    wprintw(dftwin, "%03d %%", mv);
    wattroff(dftwin, A_BOLD);
    N_ctl_refresh();
}

static void ctl_metronome(int meas, int beat)
{
	static int lastmeas = CTL_STATUS_UPDATE;
	static int lastbeat = CTL_STATUS_UPDATE;
	
	if (meas == CTL_STATUS_UPDATE)
		meas = lastmeas;
	else
		lastmeas = meas;
	if (beat == CTL_STATUS_UPDATE)
		beat = lastbeat;
	else
		lastbeat = beat;
	wmove(dftwin, SEPARATE1_LINE, 6);
	wattron(dftwin, A_BOLD);
	wprintw(dftwin, "%03d.%02d ", meas, beat);
	wattroff(dftwin, A_BOLD);
	N_ctl_refresh();
}

static void ctl_keysig(int8 k, int ko)
{
	static int8 lastkeysig = CTL_STATUS_UPDATE;
	static int lastoffset = CTL_STATUS_UPDATE;
	static const char *keysig_name[] = {
		"Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F ", "C ",
		"G ", "D ", "A ", "E ", "B ", "F#", "C#", "G#",
		"D#", "A#"
	};
	int i, j;
	
	if (k == CTL_STATUS_UPDATE)
		k = lastkeysig;
	else
		lastkeysig = k;
	if (ko == CTL_STATUS_UPDATE)
		ko = lastoffset;
	else
		lastoffset = ko;
	i = k + ((k < 8) ? 7 : -6);
	if (ko > 0)
		for (j = 0; j < ko; j++)
			i += (i > 10) ? -5 : 7;
	else
		for (j = 0; j < abs(ko); j++)
			i += (i < 7) ? 5 : -7;
	wmove(dftwin, SEPARATE1_LINE, 43);
	wattron(dftwin, A_BOLD);
	wprintw(dftwin, "%s %s (%+03d) ",
			keysig_name[i], (k < 8) ? "Maj" : "Min", ko);
	wattroff(dftwin, A_BOLD);
	N_ctl_refresh();
}

static void ctl_tempo(int t, int tr)
{
	static int lasttempo = CTL_STATUS_UPDATE;
	static int lastratio = CTL_STATUS_UPDATE;
	
	if (t == CTL_STATUS_UPDATE)
		t = lasttempo;
	else
		lasttempo = t;
	if (tr == CTL_STATUS_UPDATE)
		tr = lastratio;
	else
		lastratio = tr;
	t = (int) (500000 / (double) t * 120 * (double) tr / 100 + 0.5);
	wmove(dftwin, SEPARATE1_LINE, 66);
	wattron(dftwin, A_BOLD);
	wprintw(dftwin, "%3d (%03d %%) ", t, tr);
	wattroff(dftwin, A_BOLD);
	N_ctl_refresh();
}

static void ctl_file_name(char *name)
{
    if(name == NULL)
    {
	if(current_MFnode != NULL)
	    name = current_MFnode->file;
	else
	    return;
    }
    N_ctl_clrtoeol(FILE_LINE);
    waddstr(dftwin, "File: ");
    wattron(dftwin, A_BOLD);
    waddnstr(dftwin, name, COLS - 8);
    wattroff(dftwin, A_BOLD);

#ifdef MIDI_TITLE
    /* Display MIDI title */
    N_ctl_clrtoeol(FILE_TITLE_LINE);
    waddstr(dftwin, "Title: ");
    if(current_MFnode != NULL && current_MFnode->title != NULL)
	waddnstr(dftwin, current_MFnode->title, COLS - 9);
#endif
    N_ctl_refresh();
}

static void ctl_current_time(int secs, int v)
{
    int mins;
    static int last_voices = CTL_STATUS_INIT, last_v = CTL_STATUS_INIT;
    static int last_secs = CTL_STATUS_INIT;

    if(secs == CTL_STATUS_INIT)
    {
	last_voices = last_v = last_secs = CTL_STATUS_INIT;
	return;
    }

    if(last_secs != secs)
    {
	last_secs = secs;
	mins = secs/60;
	secs -= mins*60;
	wmove(dftwin, TIME_LINE, 5);
	wattron(dftwin, A_BOLD);
	wprintw(dftwin, "%3d:%02d", mins, secs);
	wattroff(dftwin, A_BOLD);
	scr_modified_flag = 1;
    }

    if(last_v != v)
    {
	last_v = v;
	wmove(dftwin, VOICE_LINE, 48);
	wattron(dftwin, A_BOLD);
	wprintw(dftwin, "%3d", v);
	wattroff(dftwin, A_BOLD);
	scr_modified_flag = 1;
    }

    if(last_voices != voices)
    {
	last_voices = voices;
	wmove(dftwin, VOICE_LINE, 54);
	wprintw(dftwin, "%3d", voices);
	scr_modified_flag = 1;
    }
}

static void ctl_note(int status, int ch, int note, int vel)
{
    int n, c;
    unsigned int onoff = 0, check, prev_check;
    Bitset *bitset;

    if(ch >= display_channels || ctl_ncurs_mode != NCURS_MODE_TRACE ||
       selected_channel == ch)
	return;

    scr_modified_flag = 1;

    if(display_velocity_flag)
	n = '0' + (10 * vel) / 128;
    else
	n = note_name_char[note % 12];
    c = (COLS - 28) / 12 * 12;
    if(c <= 0)
	c = 1;
    note = note % c;
    wmove(dftwin, NOTE_LINE + ch, note + 3);
    bitset = channel_program_flags + ch;

    switch(status)
    {
      case VOICE_DIE:
	waddch(dftwin, ',');
	onoff = 0;
	break;
      case VOICE_FREE:
	if(get_bitset1(gs_lcd_bits + ch, note))
	    waddch(dftwin, GS_LCD_MARK_CHAR);
	else
	    waddch(dftwin, '.');
	onoff = 0;
	break;
      case VOICE_ON:
	wattron(dftwin, A_REVERSE);
	waddch(dftwin, n);
	wattroff(dftwin, A_REVERSE);
	indicator_chan_update(ch);
	onoff = 1;
	break;
      case VOICE_SUSTAINED:
	wattron(dftwin, A_BOLD);
	waddch(dftwin, n);
	wattroff(dftwin, A_BOLD);
	onoff = 0;
	break;
      case VOICE_OFF:
	waddch(dftwin, n);
	onoff = 0;
	break;
      case GS_LCD_MARK_ON:
	set_bitset1(gs_lcd_bits + ch, note, 1);
	if(!get_bitset1(bitset, note))
	    waddch(dftwin, GS_LCD_MARK_CHAR);
	return;
      case GS_LCD_MARK_OFF:
	set_bitset1(gs_lcd_bits + ch, note, 0);
	if(!get_bitset1(bitset, note))
	    waddch(dftwin, '.');
	return;
    }

    prev_check = has_bitset(bitset);
    set_bitset1(bitset, note, onoff);
    if(prev_check == onoff)
    {
	/* Not change program mark */
	return;
    }

    check = has_bitset(bitset);
    if(prev_check ^ check)
    {
	wmove(dftwin, NOTE_LINE + ch, COLS - 21);
	if(check)
	{
	    wattron(dftwin, A_BOLD);
	    waddch(dftwin, '*');
	    wattroff(dftwin, A_BOLD);
	}
	else
	{
	    waddch(dftwin, ' ');
	}
    }
}

static void ctl_temper_keysig(int8 tk, int ko)
{
	static int8 lastkeysig = CTL_STATUS_UPDATE;
	static int lastoffset = CTL_STATUS_UPDATE;
	static const char *keysig_name[] = {
		"Cb", "Gb", "Db", "Ab", "Eb", "Bb", " F", " C",
		" G", " D", " A", " E", " B", "F#", "C#", "G#",
		"D#", "A#"
	};
	int adj, i, j;
	
	if (tk == CTL_STATUS_UPDATE)
		tk = lastkeysig;
	else
		lastkeysig = tk;
	if (ko == CTL_STATUS_UPDATE)
		ko = lastoffset;
	else
		lastoffset = ko;
	if (ctl_ncurs_mode != NCURS_MODE_TRACE)
		return;
	adj = (tk + 8) & 0x20, tk = (tk + 8) % 32 - 8;
	i = tk + ((tk < 8) ? 7 : -6);
	if (ko > 0)
		for (j = 0; j < ko; j++)
			i += (i > 10) ? -5 : 7;
	else
		for (j = 0; j < abs(ko); j++)
			i += (i < 7) ? 5 : -7;
	wmove(dftwin, TITLE_LINE, COLS - 24);
	if (adj)
		wattron(dftwin, A_BOLD);
	wprintw(dftwin, "%s%c", keysig_name[i], (tk < 8) ? ' ' : 'm');
	if (adj)
		wattroff(dftwin, A_BOLD);
	N_ctl_refresh();
}

static void ctl_temper_type(int ch, int8 tt)
{
	if (ch >= display_channels)
		return;
	if (tt != CTL_STATUS_UPDATE) {
		if (ChannelStatus[ch].tt == tt)
			return;
		ChannelStatus[ch].tt = tt;
	} else
		tt = ChannelStatus[ch].tt;
	if (ctl_ncurs_mode != NCURS_MODE_TRACE || ch == selected_channel)
		return;
	wmove(dftwin, NOTE_LINE + ch, COLS - 23);
	switch (tt) {
	case 0:
		waddch(dftwin, ' ');
		break;
	case 1:
		waddch(dftwin, 'P');
		break;
	case 2:
		waddch(dftwin, 'm');
		break;
	case 3:
		wattron(dftwin, A_BOLD);
		waddch(dftwin, 'p');
		wattroff(dftwin, A_BOLD);
		break;
	case 64:
		waddch(dftwin, '0');
		break;
	case 65:
		waddch(dftwin, '1');
		break;
	case 66:
		waddch(dftwin, '2');
		break;
	case 67:
		waddch(dftwin, '3');
		break;
	}
	scr_modified_flag = 1;
}

static void ctl_mute(int ch, int mute)
{
	if (ch >= display_channels)
		return;
	if (mute != CTL_STATUS_UPDATE) {
		if (ChannelStatus[ch].mute == mute)
			return;
		ChannelStatus[ch].mute = mute;
	} else
		mute = ChannelStatus[ch].mute;
	if (ctl_ncurs_mode != NCURS_MODE_TRACE)
		return;
	wmove(dftwin, NOTE_LINE + ch, 0);
	if (ch != selected_channel) {
		wattron(dftwin, (mute) ? A_REVERSE : 0);
		wprintw(dftwin, "%02d", ch + 1);
		wattroff(dftwin, (mute) ? A_REVERSE : 0);
	} else {
		wattron(dftwin, A_BOLD | ((mute) ? A_REVERSE : 0));
		wprintw(dftwin, "%02d", ch + 1);
		wattroff(dftwin, A_BOLD | ((mute) ? A_REVERSE : 0));
	}
	scr_modified_flag = 1;
}

static void ctl_drumpart(int ch, int is_drum)
{
    if(ch >= display_channels)
	return;
    ChannelStatus[ch].is_drum = is_drum;
}

static void ctl_program(int ch, int prog, char *comm, unsigned int banks)
{
    int val;
    int bank;

    if(ch >= display_channels)
	return;

    if(prog != CTL_STATUS_UPDATE)
    {
	bank = banks & 0xff;
	ChannelStatus[ch].prog = prog;
	ChannelStatus[ch].bank = bank;
	ChannelStatus[ch].bank_lsb = (banks >> 8) & 0xff;
	ChannelStatus[ch].bank_msb = (banks >> 16) & 0xff;
	ChannelStatus[ch].comm = (comm ? comm : "");
    } else {
	prog = ChannelStatus[ch].prog;
	bank = ChannelStatus[ch].bank;
    }
    ChannelStatus[ch].last_note_on = 0.0;	/* reset */

    if(ctl_ncurs_mode != NCURS_MODE_TRACE)
	return;

    if(selected_channel == ch)
    {
	init_trace_window_chan(ch);
	return;
    }

    if(ChannelStatus[ch].is_drum)
	val = bank;
    else
	val = prog;
    if(!IS_CURRENT_MOD_FILE)
	val += progbase;

    wmove(dftwin, NOTE_LINE + ch, COLS - 21);
    if(ChannelStatus[ch].is_drum)
    {
	wattron(dftwin, A_BOLD);
	wprintw(dftwin, " %03d", val);
	wattroff(dftwin, A_BOLD);
    }
    else
	wprintw(dftwin, " %03d", val);
    scr_modified_flag = 1;
}

static void ctl_volume(int ch, int vol)
{
    if(ch >= display_channels)
	return;

    if(vol != CTL_STATUS_UPDATE)
    {
	if(ChannelStatus[ch].vol == vol)
	    return;
	ChannelStatus[ch].vol = vol;
    }
    else
	vol = ChannelStatus[ch].vol;

    if(ctl_ncurs_mode != NCURS_MODE_TRACE || selected_channel == ch)
	return;

    wmove(dftwin, NOTE_LINE + ch, COLS - 16);
    wprintw(dftwin, "%3d", vol);
    scr_modified_flag = 1;
}

static void ctl_expression(int ch, int exp)
{
    if(ch >= display_channels)
	return;

    if(exp != CTL_STATUS_UPDATE)
    {
	if(ChannelStatus[ch].exp == exp)
	    return;
	ChannelStatus[ch].exp = exp;
    }
    else
	exp = ChannelStatus[ch].exp;

    if(ctl_ncurs_mode != NCURS_MODE_TRACE || selected_channel == ch)
	return;

    wmove(dftwin, NOTE_LINE + ch, COLS - 12);
    wprintw(dftwin, "%3d", exp);
    scr_modified_flag = 1;
}

static void ctl_panning(int ch, int pan)
{
    if(ch >= display_channels)
	return;

    if(pan != CTL_STATUS_UPDATE)
    {
	if(pan == NO_PANNING)
	    ;
	else if(pan < 5)
	    pan = 0;
	else if(pan > 123)
	    pan = 127;
	else if(pan > 60 && pan < 68)
	    pan = 64;
	if(ChannelStatus[ch].pan == pan)
	    return;
	ChannelStatus[ch].pan = pan;
    }
    else
	pan = ChannelStatus[ch].pan;

    if(ctl_ncurs_mode != NCURS_MODE_TRACE || selected_channel == ch)
	return;

    wmove(dftwin, NOTE_LINE + ch, COLS - 8);
    switch(pan)
    {
      case NO_PANNING:
	waddstr(dftwin, "   ");
	break;
      case 0:
	waddstr(dftwin, " L ");
	break;
      case 64:
	waddstr(dftwin, " C ");
	break;
      case 127:
	waddstr(dftwin, " R ");
	break;
      default:
	pan -= 64;
	if(pan < 0)
	{
	    waddch(dftwin, '-');
	    pan = -pan;
	}
	else 
	    waddch(dftwin, '+');
	wprintw(dftwin, "%02d", pan);
	break;
    }
    scr_modified_flag = 1;
}

static void ctl_sustain(int ch, int sus)
{
    if(ch >= display_channels)
	return;

    if(sus != CTL_STATUS_UPDATE)
    {
	if(ChannelStatus[ch].sus == sus)
	    return;
	ChannelStatus[ch].sus = sus;
    }
    else
	sus = ChannelStatus[ch].sus;

    if(ctl_ncurs_mode != NCURS_MODE_TRACE || selected_channel == ch)
	return;

    wmove(dftwin, NOTE_LINE + ch, COLS - 4);
    if(sus)
	waddch(dftwin, 'S');
    else
	waddch(dftwin, ' ');
    scr_modified_flag = 1;
}

static void update_bend_mark(int ch)
{
    wmove(dftwin, NOTE_LINE + ch, COLS - 2);
    waddch(dftwin, ChannelStatus[ch].bend_mark);
    scr_modified_flag = 1;
}

static void ctl_pitch_bend(int ch, int pitch)
{
    int mark;

    if(ch >= display_channels)
	return;

    ChannelStatus[ch].pitch = pitch;

    if(ctl_ncurs_mode != NCURS_MODE_TRACE || selected_channel == ch)
	return;

    if(ChannelStatus[ch].wheel)
	mark = '=';
    else if(pitch > 0x2000)
	mark = '>';
    else if(pitch < 0x2000)
	mark = '<';
    else
	mark = ' ';

    if(ChannelStatus[ch].bend_mark == mark)
	return;
    ChannelStatus[ch].bend_mark = mark;
    update_bend_mark(ch);
}

static void ctl_mod_wheel(int ch, int wheel)
{
    int mark;

    if(ch >= display_channels)
	return;

    ChannelStatus[ch].wheel = wheel;

    if(ctl_ncurs_mode != NCURS_MODE_TRACE || selected_channel == ch)
	return;

    if(wheel)
	mark = '=';
    else
    {
	/* restore pitch bend mark */
	if(ChannelStatus[ch].pitch > 0x2000)
	    mark = '>';
	else if(ChannelStatus[ch].pitch < 0x2000)
	    mark = '<';
	else
	    mark = ' ';
    }

    if(ChannelStatus[ch].bend_mark == mark)
	return;
    ChannelStatus[ch].bend_mark = mark;
    update_bend_mark(ch);
}

static void ctl_lyric(int lyricid)
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
	    if (ctl.trace_playing) {
		while (strchr(lyric, '\n')) {
		    *(strchr(lyric, '\n')) = '\r';
		}
            }
        }

	if(ctl.trace_playing)
	{
	    if(*lyric == ME_KARAOKE_LYRIC)
	    {
		if(lyric[1] == '/')
		{
		    display_lyric(" / ", LYRIC_WORD_NOSEP);
		    display_lyric(lyric + 2, LYRIC_WORD_NOSEP);
		}
		else if(lyric[1] == '\\')
		{
		    display_lyric("\r", LYRIC_WORD_NOSEP);
		    display_lyric(lyric + 2, LYRIC_WORD_NOSEP);
		}
		else if(lyric[1] == '@')
		    display_lyric(lyric + 3, LYRIC_WORD_SEP);
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
	else
	    cmsg(CMSG_INFO, VERB_NORMAL, "%s", lyric + 1);
    }
}

static void ctl_lcd_mark(int status, int x, int y)
{
    int w;

    if(!ctl.trace_playing)
    {
	waddch(msgwin, status == GS_LCD_MARK_ON ? GS_LCD_MARK_CHAR : ' ');
	return;
    }

    w = (COLS - 28) / 12 * 12;
    if(status == GS_LCD_MARK_CLEAR)
      {
	int x, y;
	for(y = 0; y < 16; y++)
	  for(x = 0; x < 40; x++)
	    ctl_note(GS_LCD_MARK_OFF, y, x + (w - 40) / 2, 0);
	return;
      }

    if(w < GS_LCD_WIDTH)
    {
	if(x < w)
	    ctl_note(status, y, x, 0);
    }
    else
    {
	ctl_note(status, y, x + (w - GS_LCD_WIDTH) / 2, 0);
    }
}

static void ctl_gslcd(int id)
{
    char *lcd;
    int i, j, k, data, mask;
    char tmp[3];

    if((lcd = event2string(id)) == NULL)
	return;
    if(lcd[0] != ME_GSLCD)
	return;
    gslcd_last_display_time = get_current_calender_time();
    gslcd_displayed_flag = 1;
    lcd++;
    for(i = 0; i < 16; i++)
    {
	for(j = 0; j < 4; j++)
	{
	    tmp[0]= lcd[2 * (j * 16 + i)];
	    tmp[1]= lcd[2 * (j * 16 + i) + 1];
	    if(sscanf(tmp, "%02X", &data) != 1)
	    {
		/* Invalid format */
		return;
	    }
	    mask = 0x10;
	    for(k = 0; k < 10; k += 2)
	    {
		if(data & mask)
		{
 		    ctl_lcd_mark(GS_LCD_MARK_ON, j * 10 + k,     i);
		    ctl_lcd_mark(GS_LCD_MARK_ON, j * 10 + k + 1, i);
		}
		else
	        {
 		    ctl_lcd_mark(GS_LCD_MARK_OFF, j * 10 + k,     i);
		    ctl_lcd_mark(GS_LCD_MARK_OFF, j * 10 + k + 1, i);
		}
		mask >>= 1;
	    }
	}
	if(!ctl.trace_playing)
	{
	    waddch(msgwin, '\n');
	    wrefresh(msgwin);
	}
    }
}

static void ctl_reset(void)
{
    if(ctl.trace_playing)
	reset_indicator();
    N_ctl_refresh();
    ctl_ncurs_mode_init();
}

/***********************************************************************/

/* #define CURSED_REDIR_HACK */

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
    static int open_init_flag = 0;
#ifdef CURSED_REDIR_HACK
    FILE *infp = stdin, *outfp = stdout;
    SCREEN *dftscr;
#endif

#ifdef USE_PDCURSES
    PDC_set_ctrl_break(1);
#endif /* USE_PDCURSES */

    if(!open_init_flag)
    {
#ifdef CURSED_REDIR_HACK
	/* This doesn't work right */
	if(using_stdin && using_stdout)
	{
	    infp = outfp = stderr;
	    fflush(stderr);
	    setvbuf(stderr, 0, _IOFBF, BUFSIZ);
	}
	else if(using_stdout)
	{
	    outfp = stderr;
	    fflush(stderr);
	    setvbuf(stderr, 0, _IOFBF, BUFSIZ);
	}
	else if(using_stdin)
	{
	    infp = stdout;
	    fflush(stdout);
	    setvbuf(stdout, 0, _IOFBF, BUFSIZ);
	}
	dftscr = newterm(0, outfp, infp);
	if (!dftscr)
	    return -1;
#else
	initscr();
#endif /* CURSED_REDIR_HACK */

	if(LINES < NCURS_MIN_LINES)
	{
	    endwin();
	    cmsg(CMSG_FATAL, VERB_NORMAL, "Error: Screen is too small.");
	    return 1;
	}

	cbreak();
	noecho();
	nonl();
	nodelay(stdscr, 1);
	scrollok(stdscr, 0);
#ifndef USE_PDCURSES
	idlok(stdscr, 1);
#endif /* USE_PDCURSES */
	keypad(stdscr, TRUE);
	ctl.opened = 1;
	init_chan_status();
    }

    open_init_flag = 1;
    dftwin=stdscr;
    if(ctl.trace_playing)
	ctl_ncurs_mode = NCURS_MODE_TRACE;
    else
	ctl_ncurs_mode = NCURS_MODE_MAIN;

    ctl_ncurs_back = ctl_ncurs_mode;
    N_ctl_scrinit();

    if(ctl.trace_playing)
    {
	if(msgwin != NULL)
	{
	    delwin(msgwin);
	    msgwin = NULL;
	}
    }
    else
    {
	set_trace_loop_hook(NULL);
	msgwin = newwin(LINES - 6 - 1, COLS, 6, 0);
	N_ctl_werase(msgwin);
	scrollok(msgwin, 1);
	wrefresh(msgwin);
    }

    if(command_buffer == NULL)
	command_buffer = mini_buff_new(COMMAND_BUFFER_SIZE);

    N_ctl_refresh();

    return 0;
}

static void ctl_close(void)
{
  if (ctl.opened)
    {
      endwin();
      ctl.opened=0;
    }
}

#if SCREEN_BUGFIX
static void re_init_screen(void)
{
    static int screen_bugfix = 0;
    if(screen_bugfix)
	return;
    screen_bugfix = 1;
    touchwin(dftwin);
    N_ctl_refresh();
    if(msgwin)
    {
	touchwin(msgwin);
	wrefresh(msgwin);
    }
}
#endif

static void move_select_channel(int diff)
{
    if(selected_channel != -1)
    {
	int prev_chan;
	prev_chan = selected_channel;
	selected_channel += diff;
	init_trace_window_chan(prev_chan);
    }
    else
	selected_channel += diff;
    while(selected_channel < 0)
	selected_channel += display_channels + 1;
    while(selected_channel >= display_channels)
	selected_channel -= display_channels + 1;

    if(selected_channel != -1)
    {
	init_trace_window_chan(selected_channel);
	current_indicator_chan = selected_channel;
    }
    N_ctl_refresh();
}

static void ctl_cmd_dir_close(void)
{
    if(ctl_ncurs_mode == NCURS_MODE_DIR)
    {
	ctl_ncurs_mode = ctl_ncurs_back;
	ctl_ncurs_mode_init();
    }
}

static void ctl_cmd_J_move(int diff)
{
    int i;
    char num[16];

    i = atoi(mini_buff_gets(command_buffer)) + diff;
    if(i < 0)
	i = 0;
    else if(i > file_list.number)
	i = file_list.number;
    sprintf(num, "%d", i);
    mini_buff_sets(command_buffer, num);
}

static int ctl_cmd_J_enter(void)
{
    int i, rc;
    char *text;

    text = mini_buff_gets(command_buffer);

    if(*text == '\0')
	rc = RC_NONE;
    else
    {
	i = atoi(text);
	if(i < 0 || i > file_list.number)
	{
	    beep();
	    rc = RC_NONE;
	}
	else
	{
	    rc = RC_LOAD_FILE;
	    ctl_listmode = 1 + i / LIST_TITLE_LINES;
	    ctl_list_select[ctl_listmode] = i;
	}
    }

    mini_buff_clear(command_buffer);
    ctl_cmdmode = 0;
    return rc;
}

static void ctl_cmd_L_dir(int move)
{
    MFnode *mfp;
    int i;

    if(ctl_ncurs_mode != NCURS_MODE_DIR)
    {
	ctl_ncurs_back = ctl_ncurs_mode;
	ctl_ncurs_mode = NCURS_MODE_DIR;
	move = 0;
    }

    N_ctl_werase(listwin);

    if(command_buffer->files == NULL)
    {
	wmove(listwin, 0, 0);
	waddstr(listwin, "No match");
	wrefresh(listwin);
	N_ctl_refresh();
	ctl_mode_L_dispstart = 0;
	return;
    }

    ctl_mode_L_dispstart += move * (LIST_TITLE_LINES-1);
    mfp = MFnode_nth_cdr(command_buffer->files, ctl_mode_L_dispstart);
    if(mfp == NULL)
    {
	mfp = command_buffer->files;
	ctl_mode_L_dispstart = 0;
    }

    N_ctl_werase(listwin);
    waddstr(listwin, "Possible completions are:");
    for(i = 0; i < LIST_TITLE_LINES - 1 && mfp; i++, mfp = mfp->next)
    {
	wmove(listwin, i + 1, 0);
	waddnstr(listwin, mfp->file, COLS - 6);
    }
    wrefresh(listwin);
    N_ctl_refresh();
}

static int ctl_cmd_L_enter(void)
{
    char *text;
    MFnode *mfp;
    int i, rc = RC_NONE;
    struct double_list_string *hist;
    int nfiles;
    char *files[1], **new_files;
    MFnode *tail, *head;

    ctl_cmd_dir_close();
    text = mini_buff_gets(command_buffer);
    if(*text == '\0')
	goto end_enter;

    strncpy(ctl_mode_L_lastenter, text, COMMAND_BUFFER_SIZE - 1);
    ctl_mode_L_lastenter[COMMAND_BUFFER_SIZE - 1] = '\0';

    hist = (struct double_list_string *)
	safe_malloc(sizeof(struct double_list_string));
    hist->string = safe_strdup(ctl_mode_L_lastenter);
    hist->prev = NULL;
    hist->next = ctl_mode_L_histh;
    if(ctl_mode_L_histh != NULL)
	ctl_mode_L_histh->prev = hist;
    ctl_mode_L_histh = hist;

    i = strlen(ctl_mode_L_lastenter);
    while(i > 0 && !IS_PATH_SEP(ctl_mode_L_lastenter[i - 1]))
	i--;
    ctl_mode_L_lastenter[i] = '\0';


    /* Add new files */
    files[0] = text;
    nfiles  = 1;
    new_files = expand_file_archives(files, &nfiles);
    if(new_files == NULL)
      {
	rc = RC_NONE;
	beep();
      }
    else
      {
	head = tail = NULL;
	for(i = 0; i < nfiles; i++)
	{
	    if((mfp = make_new_MFnode_entry(new_files[i])) != NULL)
	    {
		if(head == NULL)
		    head = tail = mfp;
		else
		    tail = tail->next = mfp;
	    }
	}
	mfp = head;
	free(new_files[0]);
	free(new_files);
	if(mfp == NULL)
	{
	    rc = RC_NONE;
	    beep();
	    goto end_enter;
	}
	insert_MFnode_entrys(mfp, nc_playfile);
	ctl_list_mode(NC_LIST_NEW);
	rc = RC_NEXT;
    }

  end_enter:
    mini_buff_clear(command_buffer);
    ctl_cmdmode = 0;
    return rc;
}

static int ctl_cmd_E_enter(int32 *val)
{
    int rc = RC_NONE;
    char *text;
    int lastb;

    *val = 1;
    text = mini_buff_gets(command_buffer);
    if(*text)
    {
	lastb = special_tonebank;
	if(set_extension_modes(text))
	    beep();
	else
	{
	    if(lastb == special_tonebank)
		rc = RC_SYNC_RESTART;
	    else
		rc = RC_RELOAD;
	}
    }
    mini_buff_clear(command_buffer);
    ctl_cmdmode = 0;
    return rc;
}

static int ctl_cmd_S_enter(void)
{
    char *file;

    ctl_cmd_dir_close();
    file = mini_buff_gets(command_buffer);
    if(*file)
    {
	if(midi_file_save_as(NULL, file) == -1)
	    beep();
    }

    mini_buff_clear(command_buffer);
    ctl_cmdmode = 0;
    return RC_NONE;
}

static int ctl_cmd_R_enter(int32 *valp)
{
    char *rateStr;
    int rc = RC_NONE;

    rateStr = mini_buff_gets(command_buffer);
    if(*rateStr)
    {
	*valp = atoi(rateStr);
	rc = RC_CHANGE_RATE;
    }
    mini_buff_clear(command_buffer);
    ctl_cmdmode = 0;
    return rc;
}

static int ctl_cmd_D_enter(int32 *val)
{
    int rc = RC_NONE, ch;
    char *text;

    text = mini_buff_gets(command_buffer);
    if(*text)
    {
	if(*text == '+')
	{
	    ch = atoi(text + 1) - 1;
	    if(ch >= 0 && ChannelStatus[ch].is_drum)
	    {
		*val = ch;
		rc = RC_TOGGLE_DRUMCHAN;
	    }
	}
	else if(*text == '-')
	{
	    ch = atoi(text + 1) - 1;
	    if(ch >= 0 && ChannelStatus[ch].is_drum)
	    {
		*val = ch;
		rc = RC_TOGGLE_DRUMCHAN;
	    }
	}
	else
	{
	    *val = atoi(text) - 1;
	    if(*val >= 0)
		rc = RC_TOGGLE_DRUMCHAN;
	}
    }
    mini_buff_clear(command_buffer);
    ctl_cmdmode = 0;
    return rc;
}

/* Previous history */
static void ctl_cmd_L_phist(void)
{
    if(ctl_mode_L_histh == NULL ||
       (ctl_mode_L_histc != NULL && ctl_mode_L_histc->next == NULL))
    {
	beep();
	return;
    }

    if(ctl_mode_L_histc != NULL)
	ctl_mode_L_histc = ctl_mode_L_histc->next;
    else
    {
	strcpy(ctl_mode_L_lastenter, mini_buff_gets(command_buffer));
	ctl_mode_L_lastenter[COMMAND_BUFFER_SIZE - 1] = '\0';
	ctl_mode_L_histc = ctl_mode_L_histh;
    }
    mini_buff_sets(command_buffer, ctl_mode_L_histc->string);
}

/* Next history */
static void ctl_cmd_L_nhist(void)
{
    if(ctl_mode_L_histc == NULL)
    {
	beep();
	return;
    }
    ctl_mode_L_histc = ctl_mode_L_histc->prev;
    if(ctl_mode_L_histc != NULL)
	mini_buff_sets(command_buffer, ctl_mode_L_histc->string);
    else
	mini_buff_sets(command_buffer, ctl_mode_L_lastenter);
}

static int ctl_cmd_forward_search(void)
{
    MFnode *mfp;
    int i, n, found;
    char *ptn, *name;

    if(mini_buff_len(command_buffer) == 0)
    {
	if(ctl_mode_SEARCH_lastenter[0] == 0)
	{
	    mini_buff_clear(command_buffer);
	    ctl_cmdmode = 0;
	    return 1;
	}
	mini_buff_sets(command_buffer, ctl_mode_SEARCH_lastenter);
    }
    else
	strcpy(ctl_mode_SEARCH_lastenter, mini_buff_gets(command_buffer));

    /* Put '*' into buffer with first and last */
    while(mini_buff_backward(command_buffer))
	;
    mini_buff_insertc(command_buffer, '*');
    while(mini_buff_forward(command_buffer))
	;
    mini_buff_insertc(command_buffer, '*');

    ptn = mini_buff_gets(command_buffer);
    n = ctl_list_select[ctl_listmode] + 1;
    mfp = MFnode_nth_cdr(file_list.MFnode_head, n);
    found = 0;
    for(i = 0; i < file_list.number; i++, n++)
    {
	if(mfp == NULL)
	{
	    mfp = file_list.MFnode_head;
	    n = 0;
	}
	if((name = pathsep_strrchr(mfp->file)) == NULL)
	    name = mfp->file;
	else
	    name++;
	if(arc_wildmat(name, ptn))
	{
	    found = 1;
	    break;
	}
	mfp = mfp->next;
    }

    mini_buff_clear(command_buffer);
    ctl_cmdmode = 0;
    if(found)
    {
	ctl_listmode = n / LIST_TITLE_LINES + 1;
	ctl_list_select[ctl_listmode] = n;
	ctl_list_mode(NC_LIST_NOW);
    }
    else
    {
	wmove(dftwin, LINES - 1, 0);
	wattron(dftwin, A_REVERSE);
	waddstr(dftwin, "Pattern not found");
	wattroff(dftwin, A_REVERSE);
    }

    return found;
}

static int ctl_read(int32 *valp)
{
  int c, i;
  static int u_prefix = 1, u_flag = 1;

  if(ctl_cmdmode)
      mini_buff_refresh(command_buffer);

  while ((c=getch())!=ERR)
    {
#if SCREEN_BUGFIX
      re_init_screen();
#endif

      if(u_flag == 0)
      {
	  u_prefix = 1;
	  u_flag = 1;
      }

      if(ctl_ncurs_mode == NCURS_MODE_HELP)
      {
	  switch(c)
	  {
	    case 'h':
	    case '?':
	    case KEY_F(1):
	      ctl_help_mode();
	      break;
	    case 'q':
	      return RC_QUIT;
	  }
	  u_prefix = 1;
	  continue;
      }

      if(ctl_cmdmode && ' ' <= c && c < 256)
      {
	  if(!mini_buff_insertc(command_buffer, c))
	      beep();
	  u_prefix = 1;
	  continue;
      }

      if(!ctl_cmdmode && c == 21)
      {
	  u_prefix <<= 1;
	  if(u_prefix > MAX_U_PREFIX)
	      u_prefix = MAX_U_PREFIX;
	  u_flag = 1;
	  continue;
      }
      else
	  u_flag = 0;

      switch(c)
	{
	case 'h':
	case '?':
	case KEY_F(1):
	  ctl_help_mode();
	  continue;

	case 'V':
 	  *valp = 10 * u_prefix;
	  return RC_CHANGE_VOLUME;
	case 'v':
	  *valp = -10 * u_prefix;
 	  return RC_CHANGE_VOLUME;

	case 16:
	case 'P':
	case KEY_UP:
	  if(ctl_cmdmode == NCURS_MODE_CMD_J)
	      ctl_cmd_J_move(1);
	  else if(ctl_cmdmode == NCURS_MODE_CMD_L)
	      ctl_cmd_L_phist();
	  else if(ctl_ncurs_mode == NCURS_MODE_LIST)
	      ctl_list_mode(NC_LIST_UP);
	  else
	  {
	      *valp = 10 * u_prefix;
	      return RC_CHANGE_VOLUME;
	  }
	  continue;

	case 14:
	case 'N':
	case KEY_DOWN:
	  if(ctl_cmdmode == NCURS_MODE_CMD_J)
	      ctl_cmd_J_move(-1);
	  else if(ctl_cmdmode == NCURS_MODE_CMD_L)
	      ctl_cmd_L_nhist();
	  else if(ctl_ncurs_mode == NCURS_MODE_LIST)
	      ctl_list_mode(NC_LIST_DOWN);
	  else
	  {
	      *valp = -10 * u_prefix;
	      return RC_CHANGE_VOLUME;
	  }
	  continue;

	case KEY_PPAGE:
	  if(ctl_ncurs_mode == NCURS_MODE_LIST)
	  {
	      ctl_list_mode(NC_LIST_UPPAGE);
	      continue;
	  }
	  else
	      return RC_REALLY_PREVIOUS;

	case 22: /* ^V */
	case KEY_NPAGE:
	  if(ctl_ncurs_mode == NCURS_MODE_LIST)
	  {
	      ctl_list_mode(NC_LIST_DOWNPAGE);
	      continue;
	  }
	  else
	      return RC_NEXT;
#if 0
	case '1':
	case '2':
	case '3':
	  *valp = c - '2';
	  return RC_CHANGE_REV_EFFB;
	case '4':
	case '5':
	case '6':
	  *valp = c - '5';
	  return RC_CHANGE_REV_TIME;
#endif
	case 'q':
	case 3: /* ^C */
	case KEY_END:
	  trace_flush();
	  sleep(1);
	  return RC_QUIT;
	case 'n':
	  return RC_NEXT;
	case 'p':
	  return RC_REALLY_PREVIOUS;
	case 'r':
	case KEY_HOME:
	  return RC_RESTART;
	case 'f':
	case KEY_RIGHT:
	case 6: /* ^F */
	  if(ctl_cmdmode)
	  {
	      if(!mini_buff_forward(command_buffer))
		  beep();
	      continue;
	  }
	  *valp = play_mode->rate * u_prefix;
	  return RC_FORWARD;
	case 'b':
	case KEY_LEFT:
	case 2: /* ^B */
	  if(ctl_cmdmode)
	  {
	      if(!mini_buff_backward(command_buffer))
		  beep();
	      continue;
	  }
	  *valp = play_mode->rate * u_prefix;
	  return RC_BACK;
	case 's':
	  return RC_TOGGLE_PAUSE;
	case 'l':
	  display_key_helpmsg();
	  ctl_list_mode(NC_LIST_PLAY);
	  continue;
	case ' ':
	case KEY_ENTER:
	case '\r':
	case '\n':
	  if(ctl_cmdmode == NCURS_MODE_CMD_J)
	      return ctl_cmd_J_enter();
	  if(ctl_cmdmode == NCURS_MODE_CMD_L)
	      return ctl_cmd_L_enter();
	  if(ctl_cmdmode == NCURS_MODE_CMD_D)
	      return ctl_cmd_D_enter(valp);
	  if(ctl_cmdmode == NCURS_MODE_CMD_E)
	      return ctl_cmd_E_enter(valp);
	  if(ctl_cmdmode == NCURS_MODE_CMD_S)
	      return ctl_cmd_S_enter();
	  if(ctl_cmdmode == NCURS_MODE_CMD_R)
	      return ctl_cmd_R_enter(valp);
	  if(ctl_cmdmode == NCURS_MODE_CMD_FSEARCH)
	  {
	      if(!ctl_cmd_forward_search())
		  beep();
	      continue;
	  }
	  if(ctl_ncurs_mode == NCURS_MODE_LIST)
	  {
	      /* ctl_list_mode(NC_LIST_SELECT); */
	      return RC_LOAD_FILE;
	  }
		if (ctl_ncurs_mode == NCURS_MODE_TRACE && selected_channel != -1) {
			*valp = selected_channel;
			return RC_TOGGLE_MUTE;
		}
	  continue;
	case '+':
	  *valp = u_prefix;
	  return RC_KEYUP;
	case '-':
	  *valp = -u_prefix;
	  return RC_KEYDOWN;
	case '>':
	  *valp = u_prefix;
	  return RC_SPEEDUP;
	case '<':
	  *valp = u_prefix;
	  return RC_SPEEDDOWN;
	case 'O':
	  *valp = u_prefix;
	  return RC_VOICEINCR;
	case 'o':
	  *valp = u_prefix;
	  return RC_VOICEDECR;
	case 'c':
	  if(ctl_ncurs_mode == NCURS_MODE_TRACE)
	  {
	      move_select_channel(u_prefix);
	      continue;
	  }
	  break;
	case 'j':
		if (ctl_ncurs_mode == NCURS_MODE_TRACE)
			move_select_channel(u_prefix);
		else if (ctl_ncurs_mode == NCURS_MODE_LIST)
			ctl_list_mode(NC_LIST_DOWN);
		continue;
	case 'C':
	  if(ctl_ncurs_mode == NCURS_MODE_TRACE)
	  {
	      move_select_channel(-u_prefix);
	      continue;
	  }
	  break;
	case 'k':
		if (ctl_ncurs_mode == NCURS_MODE_TRACE)
			move_select_channel(-u_prefix);
		else if (ctl_ncurs_mode == NCURS_MODE_LIST)
			ctl_list_mode(NC_LIST_UP);
		continue;
	case 'd':
	  if(ctl_ncurs_mode == NCURS_MODE_TRACE && selected_channel != -1)
	  {
	      *valp = selected_channel;
	      return RC_TOGGLE_DRUMCHAN;
	  }
	  break;
	case '.':
		if (ctl_ncurs_mode == NCURS_MODE_TRACE && selected_channel != -1) {
			*valp = selected_channel;
			return RC_SOLO_PLAY;
		}
		break;
	case 'g':
	  return RC_TOGGLE_SNDSPEC;
	case 'G':
	  return RC_TOGGLE_CTL_SPEANA;
	case 't': /* toggle trace */
	  if(ctl.trace_playing)
	      trace_flush();
	  ctl.trace_playing = (ctl.trace_playing) ? 0 : 1;
	  if(ctl_open(0, 0))
	      return RC_QUIT; /* Error */
	  ctl_total_time(CTL_STATUS_UPDATE);
	  ctl_master_volume(CTL_STATUS_UPDATE);
	  ctl_metronome(CTL_STATUS_UPDATE, CTL_STATUS_UPDATE);
	  ctl_keysig(CTL_STATUS_UPDATE, CTL_STATUS_UPDATE);
	  ctl_tempo(CTL_STATUS_UPDATE, CTL_STATUS_UPDATE);
	  ctl_file_name(NULL);
	  display_key_helpmsg();
	  if(ctl.trace_playing)
	  {
	      *valp = 0;
	      return RC_SYNC_RESTART;
	  }
	  return RC_NONE;
	case 7: /* ^G */
	case 27: /* cancel */
	  if(ctl_cmdmode)
	  {
	      mini_buff_clear(command_buffer);
	      beep();
	      ctl_cmdmode = 0;
	      ctl_cmd_dir_close();
	  }
	  continue;
	case 1: /* ^A */
	  if(ctl_cmdmode)
	  {
	      while(mini_buff_backward(command_buffer))
		  ;
	  }
	  continue;
	case 4: /* ^D */
	  if(ctl_cmdmode)
	  {
	      if(!mini_buff_delc(command_buffer))
		  beep();
	  }
	  continue;
	case 5: /* ^E */
	  if(ctl_cmdmode)
	  {
	      while(mini_buff_forward(command_buffer))
		  ;
	  }
	  continue;
	case 9: /* TAB: file completion */
	  if(ctl_cmdmode == NCURS_MODE_CMD_L ||
	     ctl_cmdmode == NCURS_MODE_CMD_S)
	  {
	      if(!mini_buff_completion(command_buffer))
	      {
		  /* Completion failure */
		  beep();
		  ctl_cmd_L_dir(0);
	      }
	      if(command_buffer->cflag == 1)
	      {
		  ctl_mode_L_dispstart = 0;
		  ctl_cmd_L_dir(0);
	      }
	      else if(command_buffer->cflag > 1)
		  ctl_cmd_L_dir(1);
	  }
	  continue;
	case 11: /* ^K */
	  if(ctl_cmdmode)
	  {
	      while(mini_buff_delc(command_buffer))
		  ;
	  }
	  continue;
	case KEY_BACKSPACE:
	case 8: /* ^H */
	case 127: /* del */
	  if(ctl_cmdmode)
	  {
	      if(mini_buff_backward(command_buffer))
		  mini_buff_delc(command_buffer);
	      else
		  beep();
	  }
	  continue;
	case 21: /* ^U */
	  if(ctl_cmdmode)
	  {
	      while(mini_buff_backward(command_buffer))
		  mini_buff_delc(command_buffer);
	  }
	  continue;
	case'J':
	  ctl_cmdmode = NCURS_MODE_CMD_J;
	  mini_buff_set(command_buffer, dftwin, LINES - 1, "Jump: ");
	  continue;
	case'L':
	  ctl_cmdmode = NCURS_MODE_CMD_L;
	  mini_buff_set(command_buffer, dftwin, LINES - 1, "MIDI File: ");
	  if(*ctl_mode_L_lastenter == '\0' && current_MFnode != NULL)
	  {
	      char *p;
	      strncpy(ctl_mode_L_lastenter, current_MFnode->file,
		      COMMAND_BUFFER_SIZE - 1);
	      ctl_mode_L_lastenter[COMMAND_BUFFER_SIZE - 1] = '\0';
	      if((p = strrchr(ctl_mode_L_lastenter, '#')) != NULL)
		  i = p - ctl_mode_L_lastenter;
	      else
		  i = strlen(ctl_mode_L_lastenter);
	      while(i > 0 && !IS_PATH_SEP(ctl_mode_L_lastenter[i - 1]))
		  i--;
	      ctl_mode_L_lastenter[i] = '\0';
	  }
	  mini_buff_sets(command_buffer, ctl_mode_L_lastenter);
	  ctl_mode_L_histc = NULL;
	  continue;
	case 'D':
	  ctl_cmdmode = NCURS_MODE_CMD_D;
	  mini_buff_set(command_buffer, dftwin, LINES - 1, "DrumCh> ");
	  continue;
	case 'E':
	  ctl_cmdmode = NCURS_MODE_CMD_E;
	  mini_buff_set(command_buffer, dftwin, LINES - 1, "ExtMode> ");
	  continue;
	case 'S':
	  ctl_cmdmode = NCURS_MODE_CMD_S;
	  mini_buff_set(command_buffer, dftwin, LINES - 1, "SaveAs> ");
	  if(*ctl_mode_L_lastenter == '\0' && current_MFnode != NULL)
	  {
	      int i;
	      strncpy(ctl_mode_L_lastenter, current_MFnode->file,
		      COMMAND_BUFFER_SIZE - 1);
	      ctl_mode_L_lastenter[COMMAND_BUFFER_SIZE - 1] = '\0';
	      i = strlen(ctl_mode_L_lastenter);
	      while(i > 0 && !IS_PATH_SEP(ctl_mode_L_lastenter[i - 1]))
		  i--;
	      ctl_mode_L_lastenter[i] = '\0';
	  }
	  mini_buff_sets(command_buffer, ctl_mode_L_lastenter);
	  continue;
	case 'R': {
	    char currentRate[16];
	    ctl_cmdmode = NCURS_MODE_CMD_R;
	    mini_buff_set(command_buffer, dftwin, LINES - 1, "Sample rate> ");
	    sprintf(currentRate, "%d", (int)play_mode->rate);
	    mini_buff_sets(command_buffer, currentRate);
	    continue;
	  }
	case '%':
	  display_velocity_flag = !display_velocity_flag;
	  continue;
	case '/':
	  if(ctl_ncurs_mode == NCURS_MODE_LIST)
	  {
	      ctl_cmdmode = NCURS_MODE_CMD_FSEARCH;
	      mini_buff_set(command_buffer, dftwin, LINES - 1, "/");
	  }
		if (ctl_ncurs_mode == NCURS_MODE_TRACE)
			return RC_MUTE_CLEAR;
	  continue;
	case 12: /* ^L */
	  redraw_all();
	  continue;
	default:
	  beep();
	  continue;
	}
    }

#if SCREEN_BUGFIX
  re_init_screen();
#endif

  return RC_NONE;
}

#ifdef USE_PDCURSES
static void vwprintw(WINDOW *w, char *fmt, va_list ap)
{
    char *buff;
    MBlockList pool;

    init_mblock(&pool);
    buff = (char *)new_segment(&pool, MIN_MBLOCK_SIZE);
    vsnprintf(buff, MIN_MBLOCK_SIZE, fmt, ap);
    waddstr(w, buff);
    reuse_mblock(&pool);
}
#endif /* USE_PDCURSES */

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
    va_list ap;

    if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
	ctl.verbosity<verbosity_level)
	return 0;
    indicator_mode = INDICATOR_CMSG;
    va_start(ap, fmt);
    if(!ctl.opened)
    {
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, NLS);
    }
    else
    {
#if defined( __BORLANDC__) || (defined(__W32__) && !defined(__CYGWIN32__))
	nl();
#endif
	if(ctl.trace_playing)
	{
	    char *buff;
	    int i;
	    MBlockList pool;

	    init_mblock(&pool);
	    buff = (char *)new_segment(&pool, MIN_MBLOCK_SIZE);
	    vsnprintf(buff, MIN_MBLOCK_SIZE, fmt, ap);
	    for(i = 0; i < COLS - 1 && buff[i]; i++)
		if(buff[i] == '\n' || buff[i] == '\r' || buff[i] == '\t')
		    buff[i] = ' ';
	    buff[i] = '\0';
	    N_ctl_clrtoeol(HELP_LINE);

	    switch(type)
	    {
		/* Pretty pointless to only have one line for messages, but... */
	      case CMSG_WARNING:
	      case CMSG_ERROR:
	      case CMSG_FATAL:
		wattron(dftwin, A_REVERSE);
		waddstr(dftwin, buff);
		wattroff(dftwin, A_REVERSE);
		N_ctl_refresh();
		if(type != CMSG_WARNING)
		    sleep(2);
		break;
	      default:
		waddstr(dftwin, buff);
		N_ctl_refresh();
		break;
	    }
	    reuse_mblock(&pool);
	}
	else
	{
	    switch(type)
	    {
	      default:
		vwprintw(msgwin, fmt, ap);
		wprintw(msgwin, "\n");
		if(ctl_ncurs_mode == NCURS_MODE_MAIN)
		    wrefresh(msgwin);
		break;

	      case CMSG_WARNING:
		wattron(msgwin, A_BOLD);
		vwprintw(msgwin, fmt, ap);
		wprintw(msgwin, "\n");
		wattroff(msgwin, A_BOLD);
		if(ctl_ncurs_mode == NCURS_MODE_MAIN)
		    wrefresh(msgwin);
		break;

	      case CMSG_ERROR:
	      case CMSG_FATAL:
		wattron(msgwin, A_REVERSE);
		vwprintw(msgwin, fmt, ap);
		wprintw(msgwin, "\n");
		wattroff(msgwin, A_REVERSE);
		if(ctl_ncurs_mode == NCURS_MODE_MAIN)
		{
		    wrefresh(msgwin);
		    if(type==CMSG_FATAL)
			sleep(2);
		}
		break;
	    }
	}
#ifdef __BORLANDC__
	nonl();
#endif /* __BORLANDC__ */
    }
    va_end(ap);
    return 0;
}

static void insert_MFnode_entrys(MFnode *mfp, int pos)
{
    MFnode *q; /* tail pointer of mfp */
    int len;

    q = mfp;
    len = 1;
    while(q->next)
    {
	q = q->next;
	len++;
    }

    if(pos < 0) /* head */
    {
	q->next = file_list.MFnode_head;
	file_list.MFnode_head = mfp;
    }
    else
    {
	MFnode *p;
	p = MFnode_nth_cdr(file_list.MFnode_head, pos);

	if(p == NULL)
	    file_list.MFnode_tail = file_list.MFnode_tail->next = mfp;
	else
	{
	    q->next = p->next;
	    p->next = mfp;
	}
    }
    file_list.number += len;
    ctl_list_table_init();
}

static void ctl_list_table_init(void)
{
    for(;;) {
      ctl_list_from[ctl_listmode_max]=LIST_TITLE_LINES*(ctl_listmode_max-1);
      ctl_list_select[ctl_listmode_max]=ctl_list_from[ctl_listmode_max];
      ctl_list_to[ctl_listmode_max]=LIST_TITLE_LINES*ctl_listmode_max-1;
      if(ctl_list_to[ctl_listmode_max]>=file_list.number){
	ctl_list_to[ctl_listmode_max]=file_list.number;
	break;
      }
      ctl_listmode_max++;
    }
}

static MFnode *make_new_MFnode_entry(char *file)
{
    struct midi_file_info *infop;
#ifdef MIDI_TITLE
    char *title = NULL;
#endif

    if(!strcmp(file, "-"))
	infop = get_midi_file_info("-", 1);
    else
    {
#ifdef MIDI_TITLE
	title = get_midi_title(file);
#else
	if(check_midi_file(file) < 0)
	    return NULL;
#endif /* MIDI_TITLE */
	infop = get_midi_file_info(file, 0);
    }

    if(!strcmp(file, "-") || (infop && infop->format >= 0))
    {
	MFnode *mfp;
	mfp = (MFnode *)safe_malloc(sizeof(MFnode));
	memset(mfp, 0, sizeof(MFnode));
#ifdef MIDI_TITLE
	mfp->title = title;
#endif /* MIDI_TITLE */
	mfp->file = safe_strdup(url_unexpand_home_dir(file));
	mfp->infop = infop;
	return mfp;
    }

    cmsg(CMSG_WARNING, VERB_NORMAL, "%s: Not a midi file (Ignored)",
	 url_unexpand_home_dir(file));
    return NULL;
}

static void shuffle_list(void)
{
    MFnode **nodeList;
    int i, j, n;

    n = file_list.number + 1;
    /* Move MFnode into nodeList */
    nodeList = (MFnode **)new_segment(&tmpbuffer, n * sizeof(MFnode));
    for(i = 0; i < n; i++)
    {
	nodeList[i] = file_list.MFnode_head;
	file_list.MFnode_head = file_list.MFnode_head->next;
    }

    /* Simple validate check */
    if(file_list.MFnode_head != NULL)
	ctl.cmsg(CMSG_ERROR, VERB_NORMAL, "BUG: MFnode_head is corrupted");

    /* Construct randamized chain */
    file_list.MFnode_head = file_list.MFnode_tail = NULL;
    for(i = 0; i < n; i++)
    {
	MFnode *tmp;

	j = int_rand(n - i);
	if(file_list.MFnode_head == NULL)
	    file_list.MFnode_head = file_list.MFnode_tail = nodeList[j];
	else
	    file_list.MFnode_tail = file_list.MFnode_tail->next = nodeList[j];

	/* nodeList[j] is used.  Swap out it */
	tmp = nodeList[j];
	nodeList[j] = nodeList[n - i - 1];
	nodeList[n - i - 1] = tmp;
    }
    file_list.MFnode_tail->next = NULL;
    reuse_mblock(&tmpbuffer);
}

static void ctl_pass_playing_list(int number_of_files, char *list_of_files[])
{
    int i;
    int act_number_of_files;
    int stdin_check;

    listwin=newwin(LIST_TITLE_LINES,COLS,TITLE_LINE,0);
    stdin_check = 0;
    act_number_of_files=0;
    for(i=0;i<number_of_files;i++){
	MFnode *mfp;
	if(!strcmp(list_of_files[i], "-"))
	    stdin_check = 1;
	mfp = make_new_MFnode_entry(list_of_files[i]);
	if(mfp != NULL)
	{
	    if(file_list.MFnode_head == NULL)
		file_list.MFnode_head = file_list.MFnode_tail = mfp;
	    else
		file_list.MFnode_tail = file_list.MFnode_tail->next = mfp;
	    act_number_of_files++;
	}
    }

    file_list.number=act_number_of_files-1;

    if (file_list.number<0) {
      cmsg(CMSG_FATAL, VERB_NORMAL, "No MIDI file to play!");
      return;
    }

    ctl_listmode_max=1;
    ctl_list_table_init();
    i=0;
    for (;;)
	{
	  int rc;
	  current_MFnode = MFnode_nth_cdr(file_list.MFnode_head, i);
	  display_key_helpmsg();
	  switch((rc=play_midi_file(current_MFnode->file)))
	    {
	    case RC_REALLY_PREVIOUS:
		if (i>0)
		    i--;
		else
		{
		    if(ctl.flags & CTLF_LIST_LOOP)
			i = file_list.number;
		    else
		    {
			ctl_reset();
			break;
		    }
		    sleep(1);
		}
		nc_playfile=i;
		ctl_list_mode(NC_LIST_NEW);
		break;

	    default: /* An error or something */
	    case RC_TUNE_END:
	    case RC_NEXT:
		if (i<file_list.number)
		    i++;
		else
		{
		    if(!(ctl.flags & CTLF_LIST_LOOP) || stdin_check)
		    {
			aq_flush(0);
			return;
		    }
		    i = 0;
		    if(rc == RC_TUNE_END)
			sleep(2);
		    if(ctl.flags & CTLF_LIST_RANDOM)
			shuffle_list();
		}
		nc_playfile=i;
		ctl_list_mode(NC_LIST_NEW);
		break;
	    case RC_LOAD_FILE:
		i=ctl_list_select[ctl_listmode];
		nc_playfile=i;
		break;

		/* else fall through */
	    case RC_QUIT:
		return;
	    }
	  ctl_reset();
	}
}

static void reset_indicator(void)
{
    int i;

    memset(comment_indicator_buffer, ' ', indicator_width - 1);
    comment_indicator_buffer[indicator_width - 1] = '\0';

    indicator_last_update = get_current_calender_time();
    indicator_mode = INDICATOR_DEFAULT;
    indicator_msgptr = NULL;

    for(i = 0; i < MAX_CHANNELS; i++)
    {
	ChannelStatus[i].last_note_on = 0.0;
	ChannelStatus[i].comm = channel_instrum_name(i);
    }
}

static void display_aq_ratio(void)
{
    static int last_rate = -1;
    int rate, devsiz;

    if((devsiz = aq_get_dev_queuesize()) <= 0)
	return;
    rate = (int)(((double)(aq_filled() + aq_soft_filled()) /
		  devsiz) * 100 + 0.5);
    if(rate > 9999)
	rate = 10000;

    if(last_rate != rate)
    {
	last_rate = rate;
	wmove(dftwin, VOICE_LINE + 1, 15);
	if(rate > 9999)
	    wprintw(dftwin, " Audio queue: ****%% ");
	else
	    wprintw(dftwin, " Audio queue: %4d%% ", rate);
	scr_modified_flag = 1;
    }
}

static void update_indicator(void)
{
    double t;
    int i;
    char c;
    static int play_modeflag = 1;

#if 0
    play_modeflag = 1;
    display_play_system(play_system_mode);
    display_intonation(opt_pure_intonation);
#else
    if(midi_trace.flush_flag)
    {
	play_modeflag = 1;
	return;
    }

    if(gslcd_displayed_flag)
    {
	t = get_current_calender_time();
	if(t - gslcd_last_display_time > GS_LCD_CLEAR_TIME)
	{
	    ctl_lcd_mark(GS_LCD_MARK_CLEAR, 0, 0);
	    gslcd_displayed_flag = 0;
	}
    }

	if (play_modeflag) {
		display_play_system(play_system_mode);
		display_intonation(opt_pure_intonation);
	} else {
		display_play_system(-1);
		display_intonation(-1);
	}
    play_modeflag = !play_modeflag;
#endif /* __W32__ */

    t = get_current_calender_time();
    if(indicator_mode != INDICATOR_DEFAULT)
    {
	if(indicator_last_update + LYRIC_OUT_THRESHOLD > t)
	    return;
	reset_indicator();
    }
    indicator_last_update = t;

    if(indicator_msgptr != NULL && *indicator_msgptr == '\0')
	indicator_msgptr = NULL;

    if(indicator_msgptr == NULL)
    {
	int i, prog, first_ch;

	first_ch = -1;
	prog = ChannelStatus[current_indicator_chan].prog;
	/* Find next message */
	for(i = 0; i < MAX_CHANNELS; i++,
	    current_indicator_chan = (current_indicator_chan + 1) % MAX_CHANNELS)
	{
	    if(ChannelStatus[current_indicator_chan].is_drum ||
	       ChannelStatus[current_indicator_chan].comm == NULL ||
	       *ChannelStatus[current_indicator_chan].comm == '\0')
		continue;

	    if(first_ch == -1 && 
	       ChannelStatus[current_indicator_chan].last_note_on > 0)
		first_ch = current_indicator_chan;
	    if(ChannelStatus[current_indicator_chan].prog != prog &&
	       (ChannelStatus[current_indicator_chan].last_note_on
		+ CHECK_NOTE_SLEEP_TIME > t))
		break;
	}

	if(i == MAX_CHANNELS)
	{
	    if(first_ch == -1)
		first_ch = 0;
	    if(ChannelStatus[first_ch].comm == NULL ||
	       *ChannelStatus[first_ch].comm == '\0')
		return;
	    current_indicator_chan = first_ch;
	}

	snprintf(current_indicator_message, indicator_width, "%03d:%s   ",
		 ChannelStatus[current_indicator_chan].prog,
		 ChannelStatus[current_indicator_chan].comm);
	indicator_msgptr = current_indicator_message;
    }

    c = *indicator_msgptr++;

    for(i = 0; i < indicator_width - 2; i++)
	comment_indicator_buffer[i] = comment_indicator_buffer[i + 1];
    comment_indicator_buffer[indicator_width - 2] = c;
    wmove(dftwin, HELP_LINE, 0);
    waddstr(dftwin, comment_indicator_buffer);
    scr_modified_flag = 1;
    N_ctl_refresh();
}

static void indicator_chan_update(int ch)
{
    ChannelStatus[ch].last_note_on = get_current_calender_time();
    if(ChannelStatus[ch].comm == NULL)
    {
	if((ChannelStatus[ch].comm = default_instrument_name) == NULL)
	{
	    if(ChannelStatus[ch].is_drum)
		ChannelStatus[ch].comm = "<Drum>";
	    else
		ChannelStatus[ch].comm = "<GrandPiano>";
	}
    }
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
	N_ctl_clrtoeol(HELP_LINE);
	N_ctl_refresh();
	indicator_mode = INDICATOR_LYRIC;
	crflag = 0;
    }

    if(*lyric == '\0')
    {
	indicator_last_update = get_current_calender_time();
	return;
    }

    if(strchr(lyric, '\r') != NULL)
    {
	crflag = 1;
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
	    N_ctl_clrtoeol(HELP_LINE);
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

    wmove(dftwin, HELP_LINE, 0);
    waddstr(dftwin, comment_indicator_buffer);
    N_ctl_refresh();
    reuse_mblock(&tmpbuffer);
    indicator_last_update = get_current_calender_time();
}


/*
 * MiniBuffer functions
  */
#include <sys/types.h>
#include <sys/stat.h>

#ifndef S_ISDIR
#define S_ISDIR(mode)   (((mode)&0xF000) == 0x4000)
#endif /* S_ISDIR */

/* Allocate new buffer */
static MiniBuffer *mini_buff_new(int size)
{
    MiniBuffer *b;

    b = (MiniBuffer *)safe_malloc(sizeof(MiniBuffer) + size + 1);
    memset(b, 0, sizeof(MiniBuffer) + size + 1);
    b->buffer = (char *)b + sizeof(MiniBuffer);
    b->size = size;
    mini_buff_set(b, NULL, 0, NULL);
    return b;
}

/* Initialize buffer */
static void mini_buff_set(MiniBuffer *b, WINDOW *bufwin, int line,
			  char *prompt)
{
    int plen = 0;

    memset(b->buffer, 0, b->size);

    b->len = 0;
    b->cur = 0;
    b->bufwin = bufwin;
    b->cflag = 0;
    b->uflag = 0;
    reuse_mblock(&b->pool);
    b->files = NULL;
    b->lastcmpl = NULL;

    if(prompt)
    {
	plen = strlen(prompt);
	b->text = b->buffer + plen;
	b->maxlen = b->size - plen;
	memcpy(b->buffer, prompt, plen);
    }
    else
    {
	b->text = b->buffer;
	b->maxlen = b->size;
    }

    if(bufwin)
    {
	b->x = 0;
	b->y = line;
	getmaxyx(bufwin, b->h, b->w);
	N_ctl_clrtoeol(line);
	if(prompt)
	{
	    waddstr(bufwin, prompt);
	    b->x = plen;
	}
	wrefresh(b->bufwin);
    }
}

/* Clear buffer */
static void mini_buff_clear(MiniBuffer *b)
{
    reuse_mblock(&b->pool);
    mini_buff_set(b, b->bufwin, b->y, NULL);
}

/* Refresh buffer window if modified */
static void mini_buff_refresh(MiniBuffer *b)
{
    if(b->uflag && b->bufwin)
    {
	wmove(b->bufwin, b->y, b->x);
	wrefresh(b->bufwin);
	b->uflag = 0;
    }
}

static void mb_disp_line(MiniBuffer *b, int offset, int view_start)
{
    int rlen;
    int tlen;

    if(b->bufwin == NULL)
	return;

    /* Note that: -prompt_length <= view_start <= b->maxlen */

    wmove(b->bufwin, b->y, offset);
    wclrtoeol(b->bufwin);

    rlen = b->w - offset;
    tlen = b->len - view_start - offset;

    if(tlen < rlen)
	waddnstr(b->bufwin, b->text + view_start + offset, tlen);
    else
    {
	waddnstr(b->bufwin, b->text + view_start + offset, rlen - 1);
	waddch(b->bufwin, MINI_BUFF_MORE_C);
    }
}

/* Forward one character */
static int mini_buff_forward(MiniBuffer *b)
{
    if(b->cur == b->len)
	return 0;
    b->cur++;
    b->x++;

    if(b->cur == b->len && b->x == b->w)
    {
	/* turn the line */
	mb_disp_line(b, 0, b->cur - 1);
	b->x = 0;
    }
    else if(b->x == b->w - 1)
    {
	/* turn the line */
	mb_disp_line(b, 0, b->cur);
	b->x = 0;
    }
    b->uflag = 1;
    return 1;
}

/* Forward one character */
static int mini_buff_backward(MiniBuffer *b)
{
    if(b->cur == 0)
	return 0;
    b->cur--;
    b->x--;

    if(b->x < 0)
    {
	/* restore the prev line */
	b->x = b->w - 2;
	mb_disp_line(b, 0, b->cur - b->x);
    }
    b->uflag = 1;
    return 1;
}

/* Insert a character */
static int mini_buff_insertc(MiniBuffer *b, int c)
{
    if(b->cur == b->maxlen || c == 0)
	return 0;

    /* insert */
    if(b->cur == b->len)
    {
	/* end of buffer */
	b->text[b->cur] = c;
	b->cur++;
	b->len++;
	b->x++;
	if(b->x == b->w)
	{
	    mb_disp_line(b, 0, b->cur - 1);
	    b->x = 1;
	}
	else
	{
	    if(b->bufwin)
	    {
		wmove(b->bufwin, b->y, b->x - 1);
		waddch(b->bufwin, c);
	    }
	}
    }
    else
    {
	/* not end of buffer */
	int i;
	for(i = b->len; i > b->cur; i--)
	    b->text[i] = b->text[i - 1];
	b->text[i] = c;
	b->cur++;
	b->len++;
	b->x++;
	if(b->x == b->w - 1)
	{
	    mb_disp_line(b, 0, b->cur);
	    b->x = 0;
	}
	else
	{
	    mb_disp_line(b, b->x - 1, b->cur - b->x);
	}
    }
    b->uflag = 1;
    return 1;
}

/* Insert a string */
static int mini_buff_inserts(MiniBuffer *b, char *s)
{
    unsigned char *c = (unsigned char *)s;

    while(*c)
	if(!mini_buff_insertc(b, *c++))
	    return 0;
    return 1;
}

/* Delete a character */
static int mini_buff_delc(MiniBuffer *b)
{
    int i, c;

    if(b->cur == b->len)
	return 0;

    c = (int)(unsigned char)b->text[b->cur];
    for(i = b->cur; i < b->len - 1; i++)
	b->text[i] = b->text[i + 1];
    b->len--;
    if(b->x > 0 || b->cur != b->len || b->cur == 0)
	mb_disp_line(b, b->x, b->cur - b->x);
    else
    {
	mb_disp_line(b, 0, b->cur - b->w + 1);
	b->x = b->w - 1;
    }
    b->uflag = 1;
    return c;
}

/* Get buffer string */
static char *mini_buff_gets(MiniBuffer *b)
{
    b->text[b->len] = '\0';
    return b->text;
}

/* Set buffer string */
static void mini_buff_sets(MiniBuffer *b, char *s)
{
    while(mini_buff_backward(b))
	;
    while(mini_buff_delc(b))
	;
    mini_buff_inserts(b, s);
}

static int mini_buff_len(MiniBuffer *b)
{
    return b->len;
}

static int is_directory(char *pathname)
{
    struct stat stb;

    pathname = url_expand_home_dir(pathname);
    if(stat(pathname, &stb) < 0)
	return 0;
    return S_ISDIR(stb.st_mode);
}

static MFnode *MFnode_insert_node(MFnode *list, MFnode *node)
{
    MFnode *cur, *prev;

    prev = NULL;
    for(cur = list; cur; prev = cur, cur = cur->next)
	if(strcmp(cur->file, node->file) >= 0)
	    break;
    if(cur == list)
    {
	node->next = list;
	return node;
    }
    prev->next = node;
    node->next = cur;
    return list;
}

/* Completion as file name */
static int mini_buff_completion(MiniBuffer *b)
{
    char *text, *dir, *file, *pr;
    URL url;
    char buff[BUFSIZ];
    int dirlen, prefix;

    text = mini_buff_gets(b);
    if(b->lastcmpl != NULL && strcmp(b->lastcmpl, text) == 0)
    {
	/* same */
	b->cflag++;
	return 1;
    }

    /* make new completion list */

    /* fix the path */
    pr = text;
    for(;;)
    {
	pr = pathsep_strchr(pr);
	if(pr == NULL)
	    break;
	pr++;
#ifdef TILD_SCHEME_ENABLE
	if(*pr == '~')
	    break;
#endif /* TILD_SCHEME_ENABLE */
	if(IS_PATH_SEP(*pr))
	{
	    do
		pr++;
	    while(IS_PATH_SEP(*pr))
		;
	    pr--;
	    break;
	}
    }
    if(pr != NULL)
    {
	int pos;

	pos = pr - text;
	/* goto pos */
	while(b->cur < pos)
	    mini_buff_forward(b);
	while(b->cur > pos)
	    mini_buff_backward(b);
	/* del */
	while(mini_buff_backward(b))
	    mini_buff_delc(b);
    }
    text = mini_buff_gets(b);

    reuse_mblock(&b->pool);
    b->lastcmpl = NULL;
    b->files = NULL;
    b->cflag = 0;

    /* split dir and file name */
    if((file = pathsep_strrchr(text)) != NULL)
    {
	file++;
	dirlen = file - text;
	dir = (char *)new_segment(&b->pool, dirlen + 1);
	memcpy(dir, text, dirlen);
	dir[dirlen] = '\0';
    }
    else
    {
	file = text;
	dir = ""; /* "" means current directory */
	dirlen = 0;
    }

    /* open directory */
    url = url_dir_open(dir);

    if(url == NULL) /* No completion */
    {
	reuse_mblock(&b->pool);
	return 0;
    }

    /* scan and match each files */
    prefix = -1;
    pr = NULL;
    while(url_gets(url, buff, sizeof(buff)))
    {
	char *path;
	MFnode *mfp;
	int i;

	if(!strcmp(buff, ".") || !strcmp(buff, "..") ||
	   (*buff == '.' && *file != '.'))
	    continue;

	/* check prefix */
	for(i = 0; file[i]; i++)
	    if(file[i] != buff[i])
		break;

	if(file[i] == '\0') /* matched */
	{
	    int flen;
	    flen = strlen(buff);
	    path = (char *)new_segment(&b->pool, dirlen + flen + 1);
	    memcpy(path, dir, dirlen);
	    memcpy(path + dirlen, buff, flen + 1);
	    mfp = (MFnode *)new_segment(&b->pool, sizeof(MFnode));
	    mfp->file = path;
	    b->files = MFnode_insert_node(b->files, mfp);

	    if(prefix == -1)
	    {
		prefix = flen;
		pr = path + dirlen;
	    }
	    else
	    {
		int j;
		for(j = i; j < prefix && pr[j]; j++)
		    if(pr[j] != buff[j])
			break;
		prefix = j;
	    }
	}
    }
    url_close(url);

    prefix -= strlen(file);

    if(b->files == NULL)
    {
	reuse_mblock(&b->pool);
	b->files = NULL;
	return 0;
    }

    /* go to end of buffer */
    while(mini_buff_forward(b))
	;

    if(b->files->next == NULL) /* Sole completed */
    {
	char *p;
	p = b->files->file + strlen(text);
	while(*p)
	    mini_buff_insertc(b, *p++);
	if(is_directory(mini_buff_gets(b)))
	{
	    /* Enter in the new directory. */
	    mini_buff_insertc(b, PATH_SEP);
	    reuse_mblock(&b->pool);
	    b->lastcmpl = NULL;
	    b->files = NULL;
	}
	else
	    b->lastcmpl = strdup_mblock(&b->pool, mini_buff_gets(b));
    }
    else if(prefix > 0) /* partial completed */
    {
	char *p;
	int i;

	p = b->files->file + strlen(text);
	for(i = 0; i < prefix; i++)
	    mini_buff_insertc(b, p[i]);
	b->lastcmpl = strdup_mblock(&b->pool, mini_buff_gets(b));
    }
    else
    {
	b->cflag++;
	b->lastcmpl = strdup_mblock(&b->pool, mini_buff_gets(b));
    }

    return 1;
}


/*
 * interface_<id>_loader();
 */
ControlMode *interface_n_loader(void)
{
    return &ctl;
}
