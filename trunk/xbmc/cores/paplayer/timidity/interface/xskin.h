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

    xskin.h

    Oct.03.1998  Daisuke Nagano
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#ifndef __MACOS__
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#endif /* __MACOS__ */

#ifndef NO_STRING_H
#include <string.h>
#else /* NO_STRING_H */
#include <strings.h>
#endif /* NO_STRING_H */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifndef __MACOS__
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* __MACOS__ */

#define XSKIN_WINDOW_NAME "Timidity"
#define XSKIN_RES_CLASS   "timidity"
#define XSKIN_RES_NAME    "Timidity"

extern Pixmap xskin_loadBMP( Display *, Window, char *, int *, int * );
extern int xskin_loadviscolor( Display *, Window, char * );


/* text */

extern void ts_puttext( int, int, char * );

/* numbers */

extern void ts_putnum( int, int, int );

/* cbuttons */

extern void ts_prev(int);
extern void ts_play(int);
extern void ts_pause(int);
extern void ts_stop(int);
extern void ts_next(int);
extern void ts_eject(int);

/* titlebar */

extern void ts_titlebar(int);
extern void ts_exitbutton(int);
extern void ts_menubutton(int);
extern void ts_iconbutton(int);
extern void ts_minibutton(int);

/* monoster */

extern void ts_mono(int);
extern void ts_stereo(int);

/* shufrep */

extern void ts_shuf(int);
extern void ts_rep(int);
extern void ts_equ(int);
extern void ts_plist(int);

/* posbar */

extern int ts_pos(int,int);

/* volume */

extern int ts_volume(int,int);
extern int ts_pan(int,int);

/* spectrum analizer */

extern void ts_spectrum( int, unsigned char * );

/* positions */

#define OFF   0
#define ON    1
#define ONOFF 2
#define OFFON 3

#define skin_width  275
#define skin_height 116

#define TS_PREV          0
#define TS_PLAY          1
#define TS_PAUSE         2
#define TS_STOP          3
#define TS_NEXT          4
#define TS_EJECT         5
#define TS_TITLEBAR      6
#define TS_EXITBUTTON    7
#define TS_MENUBUTTON    8
#define TS_ICONBUTTON    9
#define TS_MINIBUTTON    10
#define TS_MONO          11
#define TS_STEREO        12
#define TS_SHUFON        13
#define TS_SHUFOFF       14
#define TS_REPON         15
#define TS_REPOFF        16
#define TS_EQUON         17
#define TS_EQUOFF        18
#define TS_PLISTON       19
#define TS_PLISTOFF      20
#define TS_POS           21
#define TS_VOLUME        22
#define TS_PAN           23
#define TS_SPECTRUM      24


/* text */

#define TEXT_W           5
#define TEXT_H           6

#define BITRATE_X        111
#define BITRATE_Y        43
#define SAMPLE_X         156
#define SAMPLE_Y         43
#define MESSAGE_X        112
#define MESSAGE_Y        27

/* numbers */

#define NUM_W            9
#define NUM_H            13

#define MIN_H_X          48
#define MIN_H_Y          26
#define MIN_L_X          60
#define MIN_L_Y          26
#define SEC_H_X          78
#define SEC_H_Y          26
#define SEC_L_X          90
#define SEC_L_Y          26

/* cbuttons */

#define PREV_SX(f)       0
#define PREV_SY(f)       f==OFF?0:18
#define PREV_DX          16
#define PREV_DY          88
#define PREV_W           23
#define PREV_H           18

#define PLAY_SX(f)       23
#define PLAY_SY(f)       f==OFF?0:18
#define PLAY_DX          39
#define PLAY_DY          88
#define PLAY_W           23
#define PLAY_H           18

#define PAUSE_SX(f)      46
#define PAUSE_SY(f)      f==OFF?0:18
#define PAUSE_DX         62
#define PAUSE_DY         88
#define PAUSE_W          23
#define PAUSE_H          18

#define STOP_SX(f)       69
#define STOP_SY(f)       f==OFF?0:18
#define STOP_DX          85
#define STOP_DY          88
#define STOP_W           23
#define STOP_H           18

#define NEXT_SX(f)       92
#define NEXT_SY(f)       f==OFF?0:18
#define NEXT_DX          108
#define NEXT_DY          88
#define NEXT_W           22
#define NEXT_H           18

#define EJECT_SX(f)      114
#define EJECT_SY(f)      f==OFF?0:16
#define EJECT_DX         136
#define EJECT_DY         89
#define EJECT_W          23
#define EJECT_H          16

/* titlebar */

#define TITLEBAR_SX(f)   27
#define TITLEBAR_SY(f)   f==OFF?15:0
#define TITLEBAR_DX      0
#define TITLEBAR_DY      0
#define TITLEBAR_W       275
#define TITLEBAR_H       14

#define EXITBUTTON_SX(f) 18
#define EXITBUTTON_SY(f) f==OFF?0:9
#define EXITBUTTON_DX    264
#define EXITBUTTON_DY    3
#define EXITBUTTON_W     9
#define EXITBUTTON_H     9

#define MENUBUTTON_SX(f) 0
#define MENUBUTTON_SY(f) f==OFF?0:9
#define MENUBUTTON_DX    6
#define MENUBUTTON_DY    3
#define MENUBUTTON_W     9
#define MENUBUTTON_H     9

#define ICONBUTTON_SX(f) 9
#define ICONBUTTON_SY(f) f==OFF?0:9
#define ICONBUTTON_DX    244
#define ICONBUTTON_DY    3
#define ICONBUTTON_W     9
#define ICONBUTTON_H     9

#define MINIBUTTON_SX(f) f==OFF?0:9
#define MINIBUTTON_SY(f) 18
#define MINIBUTTON_DX    254
#define MINIBUTTON_DY    3
#define MINIBUTTON_W     9
#define MINIBUTTON_H     9

/* monoster */

#define MONO_SX(f)       29
#define MONO_SY(f)       f==OFF?12:0
#define MONO_DX          212
#define MONO_DY          41
#define MONO_W           29
#define MONO_H           12

#define STEREO_SX(f)     0
#define STEREO_SY(f)     f==OFF?12:0
#define STEREO_DX        239
#define STEREO_DY        41
#define STEREO_W         29
#define STEREO_H         12

/* playpause */

#define PSTATE_STOP      0
#define PSTATE_PAUSE     1
#define PSTATE_PLAY      2

#define PSTATE1_SX(f)    (f==PSTATE_STOP?18:(f==PSTATE_PAUSE?9:0))
#define PSTATE1_SY(f)    0
#define PSTATE1_DX       26
#define PSTATE1_DY       28
#define PSTATE1_W        9
#define PSTATE1_H        9

#define PSTATE2_SX(f)    (f==PSTATE_STOP?39:(f==PSTATE_PAUSE?36:27))
#define PSTATE2_SY(f)    0
#define PSTATE2_DX       24
#define PSTATE2_DY       28
#define PSTATE2_W        3
#define PSTATE2_H        9

/* shufrep */

#define SHUF_SX(f)       28
#define SHUF_SY(f)       f==OFF?0:(f==ON?30:(f==ONOFF?45:15))
#define SHUF_DX          164
#define SHUF_DY          89
#define SHUF_W           47
#define SHUF_H           15

#define REP_SX(f)        0
#define REP_SY(f)        f==OFF?0:(f==ON?30:(f==ONOFF?45:15))
#define REP_DX           210
#define REP_DY           89
#define REP_W            28
#define REP_H            15

#define EQU_SX(f)        f==OFF?0:(f==ON?0:(f==ONOFF?46:46))
#define EQU_SY(f)        f==OFF?73:(f==ON?61:(f==ONOFF?61:73))
#define EQU_DX           219
#define EQU_DY           58
#define EQU_W            23
#define EQU_H            12

#define PLIST_SX(f)      f==OFF?23:(f==ON?23:(f==ONOFF?69:69))
#define PLIST_SY(f)      f==OFF?73:(f==ON?61:(f==ONOFF?61:73))
#define PLIST_DX         242
#define PLIST_DY         58
#define PLIST_W          23
#define PLIST_H          12

/* posbar */

#define BAR_SX           0
#define BAR_SY           0
#define BAR_W            248
#define BAR_H            10
#define BAR_DX           16
#define BAR_DY           72

#define POS_SX(f)        f==OFF?248:278
#define POS_SY(f)        0
#define POS_W            29
#define POS_H            10
#define POS_MIN_DX       16
#define POS_MAX_DX       235
#define POS_DY           72

/* volume */

#define VOLUME_SX        0
#define VOLUME_SY        0
#define VOLUME_W         68
#define VOLUME_H         15
#define VOLUME_DX        107
#define VOLUME_DY        57

#define VOL_SX(f)        f==OFF?15:0
#define VOL_SY(f)        421
#define VOL_W            15
#define VOL_H            12
#define VOL_MIN_DX       107
#define VOL_MAX_DX       160
#define VOL_DY           57

#define PANPOT_SX        9
#define PANPOT_SY        0
#define PANPOT_W         37
#define PANPOT_H         15
#define PANPOT_DX        177
#define PANPOT_DY        57

#define PAN_SX(f)        f==OFF?15:0
#define PAN_SY(f)        421
#define PAN_W            15
#define PAN_H            12
#define PAN_MIN_DX       178
#define PAN_MAX_DX       199
#define PAN_DY           57

/* spectrum analizer */

#define SPE_SX           24
#define SPE_SY           43
#define SPE_W            76
#define SPE_H            16

