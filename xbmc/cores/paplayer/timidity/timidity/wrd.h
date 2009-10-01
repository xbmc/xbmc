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
*/

#ifndef ___WRD_H_
#define ___WRD_H_


#define WRD_MAXPARAM 32
#define WRD_MAXFADESTEP 12

#define WRD_GSCR_WIDTH 640	/* Graphics screen width in pixel */
#define WRD_GSCR_HEIGHT 400	/* Graphics screen height in pixcel */
#define WRD_TSCR_WIDTH 80	/* Text screen width in character */
#define WRD_TSCR_HEIGHT 25	/* Text screen height in character */

#define WRD_TEXT_COLOR0 "black"
#define WRD_TEXT_COLOR1 "red"
#define WRD_TEXT_COLOR2 "green"
#define WRD_TEXT_COLOR3 "yellow"
#define WRD_TEXT_COLOR4 "blue"
#define WRD_TEXT_COLOR5 "magenta"
#define WRD_TEXT_COLOR6 "cyan"
#define WRD_TEXT_COLOR7 "white"

/*sherry data is little endian*/
#define SRY_GET_SHORT(charp)  ( (charp)[0]+((charp)[1]<<8) ) 


enum wrd_token_type
{
    WRD_COMMAND,	/* Standart command */
    WRD_ECOMMAND,	/* Ensyutsukun */
    WRD_STEP,
    WRD_LYRIC,
    WRD_EOF,

    /* WRD Commands */
    WRD_COLOR, WRD_END, WRD_ESC, WRD_EXEC, WRD_FADE, WRD_GCIRCLE,
    WRD_GCLS, WRD_GINIT, WRD_GLINE, WRD_GMODE, WRD_GMOVE, WRD_GON,
    WRD_GSCREEN, WRD_INKEY, WRD_LOCATE, WRD_LOOP, WRD_MAG, WRD_MIDI,
    WRD_OFFSET, WRD_PAL, WRD_PALCHG, WRD_PALREV, WRD_PATH, WRD_PLOAD,
    WRD_REM, WRD_REMARK, WRD_REST, WRD_SCREEN, WRD_SCROLL, WRD_STARTUP,
    WRD_STOP, WRD_TCLS, WRD_TON, WRD_WAIT, WRD_WMODE,

    /* WRD Ensyutsukun Commands */
    WRD_eFONTM, WRD_eFONTP, WRD_eFONTR, WRD_eGSC, WRD_eLINE, WRD_ePAL,
    WRD_eREGSAVE, WRD_eSCROLL, WRD_eTEXTDOT, WRD_eTMODE, WRD_eTSCRL,
    WRD_eVCOPY, WRD_eVSGET, WRD_eVSRES, WRD_eXCOPY,

    /* WRD Extensionals */
    WRD_ARG,
    WRD_FADESTEP,
    WRD_OUTKEY,
    WRD_NL,
    WRD_MAGPRELOAD,
    WRD_PHOPRELOAD,
    WRD_START_SKIP,
    WRD_END_SKIP,
    WRD_SHERRY_UPDATE,		/* Update real screen of Sherry */

    WRD_NOARG = 0x7FFF
};

typedef struct _WRDTracer
{
    char *name;			/* Tracer name */
    int id;			/* ID */
    int opened;			/* 0:closed 1:opened */

    /* Initialize tracer environment
     * open() calls at first once.
     */
    int (* open)(char *wrdt_opts);

    /* apply() evaluates MIMPI WRD command. */
    /* wrd_argv[0] means WRD command, and the rests means the arguments */
    void (* apply)(int cmd, int wrd_argc, int wrd_argv[]);

    /* sherry() evaluates Sherry WRD command. */
    void (* sherry)(uint8 *data, int len);

    /* Update window events */
    void (* update_events)(void);

    /* start() calls at each end of MIDI reading.
     * If it is error, start() returns -1, otherwise return 0.
     * If start() returns -1, TiMidity strips all WRD command.
     */
    int (* start)(int wrd_mode);
#define WRD_TRACE_NOTHING	0
#define WRD_TRACE_MIMPI		1
#define WRD_TRACE_SHERRY	2

    /* end() calls at each end of playing */
    void (* end)(void);

    /* close() calls at last before exit */
    void (* close)(void);
} WRDTracer;

typedef struct _sry_datapacket
{
    int32  len;
    uint8 *data;
} sry_datapacket;

extern WRDTracer *wrdt_list[], *wrdt;
extern int wrd_color_remap[/* 8 */];
extern int wrd_plane_remap[/* 8 */];
extern sry_datapacket *datapacket;

extern int import_wrd_file(char *fn);
extern void wrd_init_path(void);
extern void wrd_add_path(char *path, int pathlen_opt);
extern void wrd_add_default_path(char *path);
extern struct timidity_file *wrd_open_file(char *filename);

extern void wrd_midi_event(int cmd, int arg);
extern void wrd_sherry_event(int addr);
extern void *wrd_sherry_data;

extern void sry_encode_bindata( char *code, const char *org, int len);
extern int sry_decode_bindata( char *data );
extern int wrd_read_sherry;

static inline void print_ecmd(char*, int*, int);
#ifdef HAVE_STRINGS_H
#include <strings.h>
#elif defined HAVE_STRING_H
#include <string.h>
#endif
#include <limits.h>
#include "mblock.h"
#include "controls.h"
static inline void print_ecmd(char *cmd, int *args, int narg)
{
    char *p;
    size_t s = MIN_MBLOCK_SIZE;

    p = (char *)new_segment(&tmpbuffer, s);
    snprintf(p, s, "^%s(", cmd);

    if(*args == WRD_NOARG)
	strncat(p, "*", s - strlen(p) - 1);
    else {
	char c[CHAR_BIT*sizeof(int)];
	snprintf(c, sizeof(c)-1, "%d", args[0]);
	strncat(p, c, s - strlen(p) - 1);
    }
    args++;
    narg--;
    while(narg > 0)
    {
	if(*args == WRD_NOARG)
	    strncat(p, ",*", s - strlen(p) - 1);
	else {
	    char c[CHAR_BIT*sizeof(int)]; /* should be enough loong */
	    snprintf(c, sizeof(c)-1, ",%d", args[0]);
	    strncat(p, c, s - strlen(p) - 1);
	}
	args++;
	narg--;
    }
    strncat(p, ")", s - strlen(p) - 1);
    ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "%s", p);
    reuse_mblock(&tmpbuffer);
}

#endif /* ___WRD_H_ */
