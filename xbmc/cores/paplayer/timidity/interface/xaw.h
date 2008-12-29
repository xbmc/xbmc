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

    xaw.h: written by Yoshishige Arai (ryo2@on.rim.or.jp) 12/8/98

    */
#ifndef _XAW_H_
#define _XAW_H_
/*
 * XAW configurations
 */

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/* Define to use libXaw3d */
/* #define XAW3D */

/* Define to use Japanese and so on */
#define I18N

/* Define to use scrollable Text widget instead of Label widget */
/* #define WIDGET_IS_LABEL_WIDGET */

/*** Initial dot file name at home directory ***/
#define INITIAL_CONFIG ".xtimidity"


/*
 * CONSTANTS FOR XAW MENUS
 */
#define MAXVOLUME MAX_AMPLIFICATION
#define MAX_XAW_MIDI_CHANNELS 16

#define APP_CLASS "TiMidity"
#define APP_NAME "timidity"

#ifndef PATH_MAX
#define PATH_MAX 512
#endif
#define MAX_DIRECTORY_ENTRY BUFSIZ
#define LF 0x0a
#define SPACE 0x20
#define TAB 0x09

#define MODUL_N 0
#define PORTA_N 1
#define NRPNV_N 2
#define REVERB_N 3
#define CHPRESSURE_N 4
#define OVERLAPV_N 5
#define TXTMETA_N 6
#define MAX_OPTION_N 7

#define MODUL_BIT (1<<MODUL_N)
#define PORTA_BIT (1<<PORTA_N)
#define NRPNV_BIT (1<<NRPNV_N)
#define REVERB_BIT (1<<REVERB_N)
#define CHPRESSURE_BIT (1<<CHPRESSURE_N)
#define OVERLAPV_BIT (1<<OVERLAPV_N)
#define TXTMETA_BIT (1<<TXTMETA_N)

#include "timidity.h"

#ifdef MODULATION_WHEEL_ALLOW
#define INIT_OPTIONS0 MODUL_BIT
#else
#define INIT_OPTIONS0 0
#endif

#ifdef PORTAMENTO_ALLOW
#define INIT_OPTIONS1 PORTA_BIT
#else
#define INIT_OPTIONS1 0
#endif

#ifdef NRPN_VIBRATO_ALLOW
#define INIT_OPTIONS2 NRPNV_BIT
#else
#define INIT_OPTIONS2 0
#endif

#ifdef REVERB_CONTROL_ALLOW
#define INIT_OPTIONS3 REVERB_BIT
#else
#define INIT_OPTIONS3 0
#endif

#ifdef GM_CHANNEL_PRESSURE_ALLOW
#define INIT_OPTIONS4 CHPRESSURE_BIT
#else
#define INIT_OPTIONS4 0
#endif

#ifdef OVERLAP_VOICE_ALLOW
#define INIT_OPTIONS5 OVERLAPV_BIT
#else
#define INIT_OPTIONS5 0
#endif

#ifdef ALWAYS_TRACE_TEXT_META_EVENT
#define INIT_OPTIONS6 TXTMETA_BIT
#else
#define INIT_OPTIONS6 0
#endif

#define DEFAULT_OPTIONS (INIT_OPTIONS0+INIT_OPTIONS1+INIT_OPTIONS2+INIT_OPTIONS3+INIT_OPTIONS4+INIT_OPTIONS5+INIT_OPTIONS6)

#ifdef CHORUS_CONTROL_ALLOW
#define DEFAULT_CHORUS 1
#else
#define DEFAULT_CHORUS 0
#endif

#define XAW_UPDATE_TIME 0.1

#endif /* _XAW_H_ */
