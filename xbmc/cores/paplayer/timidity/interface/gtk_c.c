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

    motif_ctl.c: written by Vincent Pagel (pagel@loria.fr) 10/4/95
   
    A motif interface for TIMIDITY : to prevent X redrawings from 
    interfering with the audio computation, I don't use the XtAppAddWorkProc

    I create a pipe between the timidity process and a Motif interface
    process forked from the 1st one

    Copied the Motif file to create a Gtk+ interface.
          - Glenn Trigg 29 Oct 1998

    Modified for TiMidity++
         - Isaku Yamahata 03 Dec 1998

    */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
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
#include "gtk_h.h"
#include "readmidi.h"

static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);

static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int secs, int voices);
static void ctl_note(int status, int channel, int note, int velocity);
static void ctl_program(int ch, int val, char *vp);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void ctl_pitch_bend(int channel, int val);
static void ctl_reset(void);
static void ctl_lyric(int);


/**********************************************/
/* export the interface functions */

#define ctl gtk_control_mode
ControlMode ctl = 
{
    "gtk+ interface", 'g',
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
/* Put controls on the pipe                                            */
/***********************************************************************/
static int
cmsg(int type, int verbosity_level, char *fmt, ...)
{
    char local[255];

    va_list ap;
    if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
	ctl.verbosity<verbosity_level)
	return 0;

    va_start(ap, fmt);
    if (!ctl.opened) {
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
    }
    else {
	vsnprintf(local, sizeof(local), fmt, ap);
	gtk_pipe_int_write(CMSG_MESSAGE);
	gtk_pipe_int_write(type);
	gtk_pipe_string_write(local);
    }
    va_end(ap);
    return 0;
}


/*
  ctl_event is stolen from ncurses_c.c
 */
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
	ctl_pitch_bend((int)e->v1, e->v2);
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
      case CTLE_SPEANA:
	break;
    }
}

static void
ctl_refresh(void)
{
    /* gtk_pipe_int_write(REFRESH_MESSAGE); */
}

static void
ctl_total_time(int tt)
{
    gtk_pipe_int_write(TOTALTIME_MESSAGE);
    gtk_pipe_int_write(tt);
}

static void
ctl_master_volume(int mv)
{
    gtk_pipe_int_write(MASTERVOL_MESSAGE);
    gtk_pipe_int_write(mv);
}

static void
ctl_file_name(char *name)
{
    gtk_pipe_int_write(FILENAME_MESSAGE);
    gtk_pipe_string_write(name);
}

static void
ctl_current_time(int secs, int voices)
{
    gtk_pipe_int_write(CURTIME_MESSAGE);
    gtk_pipe_int_write(secs);
    gtk_pipe_int_write(voices);
}

static void
ctl_note(int status, int channel, int note, int velocity)
{
    /*   int xl;
    if (!ctl.trace_playing) 
	return;
    xl=voice[v].note%(COLS-24);
    wmove(dftwin, 8+voice[v].channel,xl+3);
    switch(voice[v].status)
	{
	case VOICE_DIE:
	    waddch(dftwin, ',');
	    break;
	case VOICE_FREE: 
	    waddch(dftwin, '.');
	    break;
	case VOICE_ON:
	    wattron(dftwin, A_BOLD);
	    waddch(dftwin, '0'+(10*voice[v].velocity)/128); 
	    wattroff(dftwin, A_BOLD);
	    break;
	case VOICE_OFF:
	case VOICE_SUSTAINED:
	    waddch(dftwin, '0'+(10*voice[v].velocity)/128);
	    break;
	}
	*/
}

static void
ctl_program(int ch, int val, char *vp)
{
/*  if (!ctl.trace_playing) 
    return;
  wmove(dftwin, 8+ch, COLS-20);
  if (ISDRUMCHANNEL(ch))
    {
      wattron(dftwin, A_BOLD);
      wprintw(dftwin, "%03d", val);
      wattroff(dftwin, A_BOLD);
    }
  else
    wprintw(dftwin, "%03d", val);
    */
}

static void
ctl_volume(int channel, int val)
{
    /*
      if (!ctl.trace_playing) 
    return;
  wmove(dftwin, 8+channel, COLS-16);
  wprintw(dftwin, "%3d", (val*100)/127);
  */
}

static void
ctl_expression(int channel, int val)
{
/*  if (!ctl.trace_playing) 
    return;
  wmove(dftwin, 8+channel, COLS-12);
  wprintw(dftwin, "%3d", (val*100)/127);
  */
}

static void
ctl_panning(int channel, int val)
{
/*  if (!ctl.trace_playing) 
    return;
  
  if (val==NO_PANNING)
    waddstr(dftwin, "   ");
  else if (val<5)
    waddstr(dftwin, " L ");
  else if (val>123)
    waddstr(dftwin, " R ");
  else if (val>60 && val<68)
    waddstr(dftwin, " C ");
    */
}

static void
ctl_sustain(int channel, int val)
{
/*
  if (!ctl.trace_playing) 
    return;

  if (val) waddch(dftwin, 'S');
  else waddch(dftwin, ' ');
  */
}

static void
ctl_pitch_bend(int channel, int val)
{
/*  if (!ctl.trace_playing) 
    return;

  if (val>0x2000) waddch(dftwin, '+');
  else if (val<0x2000) waddch(dftwin, '-');
  else waddch(dftwin, ' ');
  */
}

static void
ctl_reset(void)
{
/*  int i,j;
  if (!ctl.trace_playing) 
    return;
  for (i=0; i<16; i++)
    {
	ctl_program(i, channel[i].program);
	ctl_volume(i, channel[i].volume);
	ctl_expression(i, channel[i].expression);
	ctl_panning(i, channel[i].panning);
	ctl_sustain(i, channel[i].sustain);
	ctl_pitch_bend(i, channel[i].pitchbend);
    }
  ctl_refresh();
  */
}

static void
ctl_lyric(int lyricid)
{
    char	*lyric;
    static char	lyric_buf[300];

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
	if(lyric[0] == ME_KARAOKE_LYRIC)
	{
	    if(!lyric[1])
		return;
	    if(lyric[1] == '/' || lyric[1] == '\\')
	    {
		snprintf(lyric_buf, sizeof(lyric_buf), "\n%s", lyric + 2);
		gtk_pipe_int_write(LYRIC_MESSAGE);
		gtk_pipe_string_write(lyric_buf);
	    }
	    else if(lyric[1] == '@')
	    {
		if(lyric[2] == 'L')
		    snprintf(lyric_buf, sizeof(lyric_buf), "Language: %s\n", lyric + 3);
		else if(lyric[2] == 'T')
		    snprintf(lyric_buf, sizeof(lyric_buf), "Title: %s\n", lyric + 3);
		else
		    snprintf(lyric_buf, sizeof(lyric_buf), "%s\n", lyric + 1);
		gtk_pipe_int_write(LYRIC_MESSAGE);
		gtk_pipe_string_write(lyric_buf);
	    }
	    else
	    {
		strncpy(lyric_buf, lyric + 1, sizeof(lyric_buf) - 1);
		gtk_pipe_int_write(LYRIC_MESSAGE);
		gtk_pipe_string_write(lyric_buf);
	    }
	}
	else
	{
	    strncpy(lyric_buf, lyric + 1, sizeof(lyric_buf) - 1);
	    gtk_pipe_int_write(LYRIC_MESSAGE);
	    gtk_pipe_string_write(lyric_buf);
	}
    }
}

/***********************************************************************/
/* OPEN THE CONNECTION                                                */
/***********************************************************************/
static int
ctl_open(int using_stdin, int using_stdout)
{
    ctl.opened=1;
  
    /* The child process won't come back from this call  */
    gtk_pipe_open();

    return 0;
}

/* Tells the window to disapear */
static void
ctl_close(void)
{
    if (ctl.opened) {
	gtk_pipe_int_write(CLOSE_MESSAGE);
	ctl.opened=0;
    }
}


/* 
 * Read information coming from the window in a BLOCKING way
 */
static int
ctl_blocking_read(int32 *valp)
{
    int command;
    int new_volume;
    int new_centiseconds;

    gtk_pipe_int_read(&command);
  
    while (1)    /* Loop after pause sleeping to treat other buttons! */
    {

	switch(command) {
	case GTK_CHANGE_VOLUME:
	    gtk_pipe_int_read(&new_volume);
	    *valp= new_volume - amplification ;
	    return RC_CHANGE_VOLUME;
		  
	case GTK_CHANGE_LOCATOR:
	    gtk_pipe_int_read(&new_centiseconds);
	    *valp= new_centiseconds*(play_mode->rate / 100) ;
	    return RC_JUMP;
		  
	case GTK_QUIT:
	    return RC_QUIT;
		
	case GTK_PLAY_FILE:
	    return RC_LOAD_FILE;		  
		  
	case GTK_NEXT:
	    return RC_NEXT;
		  
	case GTK_PREV:
	    return RC_REALLY_PREVIOUS;
		  
	case GTK_RESTART:
	    return RC_RESTART;
		  
	case GTK_FWD:
	    *valp=play_mode->rate;
	    return RC_FORWARD;
		  
	case GTK_RWD:
	    *valp=play_mode->rate;
	    return RC_BACK;

	case GTK_KEYUP:
	    *valp = 1;
	    return RC_KEYUP;

	case GTK_KEYDOWN:
	    *valp = -1;
	    return RC_KEYDOWN;

	case GTK_SLOWER:
	    *valp = 1;
	    return RC_SPEEDDOWN;

	case GTK_FASTER:
	    *valp = 1;
	    return RC_SPEEDUP;
	}
	  
	  
	if (command==GTK_PAUSE) {
	    gtk_pipe_int_read(&command); /* Blocking reading => Sleep ! */
	    if (command==GTK_PAUSE)
		return RC_NONE; /* Resume where we stopped */
	}
	else {
	    fprintf(stderr,"gtk UNKNOWN RC_MESSAGE %i\n",command);
	    return RC_NONE;
	}
    }
}

/* 
 * Read information coming from the window in a non blocking way
 */
static int
ctl_read(int32 *valp)
{
    int num;

    /* We don't wan't to lock on reading  */
    num = gtk_pipe_read_ready();

    if (num==0)
	return RC_NONE;
  
    return(ctl_blocking_read(valp));
#if 0
    num = ctl_blocking_read(valp);
    fprintf (stderr, "cmd=%i", num);
    return num;
#endif
}

static void
ctl_pass_playing_list(int number_of_files, char *list_of_files[])
{
    int i=0;
    char file_to_play[1000];
    int command;
    int32 val;

    if( number_of_files > 0 ) {
	/* Pass the list to the interface */
	gtk_pipe_int_write(FILE_LIST_MESSAGE);
	gtk_pipe_int_write(number_of_files);
	for (i=0;i<number_of_files;i++)
	    gtk_pipe_string_write(list_of_files[i]);
    
	/* Ask the interface for a filename to play -> begin to play automatically */
	gtk_pipe_int_write(NEXT_FILE_MESSAGE);
    }

    command = ctl_blocking_read(&val);

    /* Main Loop */
    for (;;) { 
	if (command==RC_LOAD_FILE) {
	    /* Read a LoadFile command */
	    gtk_pipe_string_read(file_to_play);
	    command=play_midi_file(file_to_play);
	}
	else {
	    if (command==RC_QUIT)
		return;
	    if (command==RC_ERROR)
		command=RC_TUNE_END; /* Launch next file */
	    

	    switch(command) {
	    case RC_NEXT:
		gtk_pipe_int_write(NEXT_FILE_MESSAGE);
		break;
	    case RC_REALLY_PREVIOUS:
		gtk_pipe_int_write(PREV_FILE_MESSAGE);
		break;
	    case RC_TUNE_END:
		gtk_pipe_int_write(TUNE_END_MESSAGE);
		break;
	    default:
		printf("PANIC !!! OTHER COMMAND ERROR ?!?! %i\n",command);
	    }
		    
	    command = ctl_blocking_read(&val);
	}
    }
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_g_loader(void)
{
    return &ctl;
}
