/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    controls.h
*/

/* Return values for ControlMode.read */

#define RC_ERROR -1
#define RC_NONE 0
#define RC_QUIT 1
#define RC_NEXT 2
#define RC_PREVIOUS 3 /* Restart this song at beginning, or the previous
			 song if we're less than a second into this one. */
#define RC_FORWARD 4
#define RC_BACK 5
#define RC_JUMP 6
#define RC_TOGGLE_PAUSE 7 /* Pause/continue */
#define RC_RESTART 8 /* Restart song at beginning */

#define RC_PAUSE 9 /* Really pause playing */
#define RC_CONTINUE 10 /* Continue if paused */
#define RC_REALLY_PREVIOUS 11 /* Really go to the previous song */
#define RC_CHANGE_VOLUME 12
#define RC_LOAD_FILE 13		/* Load a new midifile */
#define RC_TUNE_END 14		/* The tune is over, play it again sam? */

#define CMSG_INFO	0
#define CMSG_WARNING	1
#define CMSG_ERROR	2
#define CMSG_FATAL	3
#define CMSG_TRACE	4
#define CMSG_TIME	5
#define CMSG_TOTAL	6
#define CMSG_FILE	7
#define CMSG_TEXT	8

#define VERB_NORMAL	0
#define VERB_VERBOSE	1
#define VERB_NOISY	2
#define VERB_DEBUG	3
#define VERB_DEBUG_SILLY	4

typedef struct {
  char *id_name, id_character;
  int verbosity, trace_playing, opened;

  int (*open)(int using_stdin, int using_stdout);
  void (*pass_playing_list)(int number_of_files, char *list_of_files[]);
  void (*close)(void);
  int (*read)(int32 *valp);
  int (*cmsg)(int type, int verbosity_level, char *fmt, ...);

  void (*refresh)(void);
  void (*reset)(void);
  void (*file_name)(char *name);
  void (*total_time)(int tt);
  void (*current_time)(int ct);

  void (*note)(int v);
  void (*master_volume)(int mv);
  void (*program)(int channel, int val); /* val<0 means drum set -val */
  void (*volume)(int channel, int val);
  void (*expression)(int channel, int val);
  void (*panning)(int channel, int val);
  void (*sustain)(int channel, int val);
  void (*pitch_bend)(int channel, int val);
  
} ControlMode;

extern ControlMode *ctl_list[], *ctl; 
extern char timidity_error[];
