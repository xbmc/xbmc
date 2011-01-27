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

    controls.h
*/

#ifndef ___CONTROLS_H_
#define ___CONTROLS_H_

#define RC_IS_SKIP_FILE(rc) ((rc) == RC_QUIT || (rc) == RC_LOAD_FILE || \
			     (rc) == RC_NEXT || (rc) == RC_REALLY_PREVIOUS || \
			     (rc) == RC_ERROR || (rc) == RC_STOP || \
			     (rc) == RC_TUNE_END)

/* Return values for ControlMode.read */
#define RC_ERROR	-1
#ifdef RC_NONE
#undef RC_NONE
#endif
#define RC_NONE		0
#define RC_QUIT		1
#define RC_NEXT		2
#define RC_PREVIOUS	3 /* Restart this song at beginning, or the previous
			     song if we're less than a second into this one. */
#define RC_FORWARD	4
#define RC_BACK		5
#define RC_JUMP		6
#define RC_TOGGLE_PAUSE 7	/* Pause/continue */
#define RC_RESTART	8	/* Restart song at beginning */
#define RC_PAUSE	9	/* Really pause playing */
#define RC_CONTINUE	10	/* Continue if paused */
#define RC_REALLY_PREVIOUS 11	/* Really go to the previous song */
#define RC_CHANGE_VOLUME 12
#define RC_LOAD_FILE	13	/* Load a new midifile */
#define RC_TUNE_END	14	/* The tune is over, play it again sam? */
#define RC_KEYUP	15	/* Key up */
#define RC_KEYDOWN	16	/* Key down */
#define RC_SPEEDUP	17	/* Speed up */
#define RC_SPEEDDOWN	18	/* Speed down */
#define RC_VOICEINCR	19	/* Increase voices */
#define RC_VOICEDECR	20	/* Decrease voices */
#define RC_TOGGLE_DRUMCHAN 21	/* Toggle drum channel */
#define RC_RELOAD	22	/* Reload & Play */
#define RC_TOGGLE_SNDSPEC 23	/* Open/Close Sound Spectrogram Window */
#define RC_CHANGE_REV_EFFB 24
#define RC_CHANGE_REV_TIME 25
#define RC_SYNC_RESTART 26
#define RC_TOGGLE_CTL_SPEANA 27
#define RC_CHANGE_RATE	28
#define RC_OUTPUT_CHANGED      29
#define RC_STOP		30	/* Stop to play */
#define RC_TOGGLE_MUTE	31
#define RC_SOLO_PLAY	32
#define RC_MUTE_CLEAR	33

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
#define VERB_DEBUG_SILLY 4

enum {
    CTLE_NOW_LOADING,		/* v1:filename */
    CTLE_LOADING_DONE,		/* v1:0=success -1=error 1=terminated */
    CTLE_PLAY_START,		/* v1:nsamples */
    CTLE_PLAY_END,
    CTLE_CURRENT_TIME,		/* v1:secs, v2:voices */
    CTLE_NOTE,			/* v1:status, v2:ch, v3:note, v4:velo */
    CTLE_MASTER_VOLUME,		/* v1:amp(%) */
    CTLE_METRONOME,		/* v1:measure, v2:beat */
    CTLE_KEYSIG,		/* v1:key sig */
    CTLE_KEY_OFFSET,		/* v1:key offset */
    CTLE_TEMPO,			/* v1:tempo */
    CTLE_TIME_RATIO,		/* v1:time ratio(%) */
    CTLE_TEMPER_KEYSIG,		/* v1:tuning key sig */
    CTLE_TEMPER_TYPE,		/* v1:ch, v2:tuning type */
    CTLE_MUTE,			/* v1:ch, v2:is_mute */
    CTLE_PROGRAM,		/* v1:ch, v2:prog, v3:name, v4:bank,lsb.msb */
    CTLE_VOLUME,		/* v1:ch, v2:value */
    CTLE_EXPRESSION,		/* v1:ch, v2:value */
    CTLE_PANNING,		/* v1:ch, v2:value */
    CTLE_SUSTAIN,		/* v1:ch, v2:value */
    CTLE_PITCH_BEND,		/* v1:ch, v2:value */
    CTLE_MOD_WHEEL,		/* v1:ch, v2:value */
    CTLE_CHORUS_EFFECT,		/* v1:ch, v2:value */
    CTLE_REVERB_EFFECT,		/* v1:ch, v2:value */
    CTLE_LYRIC,			/* v1:lyric-ID */
    CTLE_REFRESH,
    CTLE_RESET,
    CTLE_SPEANA,		/* v1:double[] v2:len */
    CTLE_PAUSE,			/* v1:pause on/off v2:time of pause */
    CTLE_GSLCD,			/* GS L.C.D. */
    CTLE_MAXVOICES,		/* v1:voices, Change voices */
    CTLE_DRUMPART		/* v1:ch, v2:is_drum */
};

typedef struct _CtlEvent {
    int type;		/* See above */
    long v1, v2, v3, v4;/* Event value */
} CtlEvent;


typedef struct {
  char *id_name, id_character;
  int verbosity, trace_playing, opened;

  int32 flags;
/* ControlMode flags.
 * Some interfaces ignore these flags.
 */
#define CTLF_LIST_LOOP		(1u<<0)	/* -i?l */
#define CTLF_LIST_RANDOM	(1u<<1)	/* -i?r */
#define CTLF_LIST_SORT		(1u<<2)	/* -i?s */
#define CTLF_AUTOSTART		(1u<<3)	/* -i?a */
#define CTLF_AUTOEXIT		(1u<<4)	/* -i?x */
#define CTLF_DRAG_START		(1u<<5)	/* -i?d */
#define CTLF_AUTOUNIQ		(1u<<6)	/* -i?u */
#define CTLF_AUTOREFINE		(1u<<7)	/* -i?R */
#define CTLF_NOT_CONTINUE	(1u<<8)	/* -i?C */
#define CTLF_DAEMONIZE		(1u<<9) /* -i?D */

  int  (*open)(int using_stdin, int using_stdout);
  void (*close)(void);
  void (*pass_playing_list)(int number_of_files, char *list_of_files[]);
  int  (*read)(int32 *valp);
  int  (*cmsg)(int type, int verbosity_level, char *fmt, ...);
  void (*event)(CtlEvent *ev);	/* Control events */
} ControlMode;

extern ControlMode *ctl_list[], *ctl;
extern int dumb_error_count;

#endif /* ___CONTROLS_H_ */
