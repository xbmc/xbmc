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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include "wrd.h"

static int wrdt_open(char *wrdt_opts);
static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[]);
static void wrdt_update_events(void);
static void wrdt_end(void);
static void wrdt_close(void);

#define wrdt dumb_wrdt_mode

WRDTracer wrdt =
{
    "dumb WRD tracer", 'd',
    0,
    wrdt_open,
    wrdt_apply,
    NULL,
    wrdt_update_events,
    NULL,
    wrdt_end,
    wrdt_close
};

static int inkey_flag;

/*ARGSUSED*/
static int wrdt_open(char *wrdt_opts)
{
    wrdt.opened = 1;
    inkey_flag = 0;
    return 0;
}

static void wrdt_update_events(void)
{
}

static void wrdt_end(void)
{
    inkey_flag = 0;
}

static void wrdt_close(void)
{
    wrdt.opened = 0;
    inkey_flag = 0;
}

static char *wrd_event2string(int id)
{
    char *name;

    name = event2string(id);
    if(name != NULL)
	return name + 1;
    return "";
}

static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[])
{
    char *p;
    char *text;
    int i, len;

    if(cmd == WRD_MAGPRELOAD)
	return; /* Load MAG file */
    if(cmd == WRD_PHOPRELOAD)
	return; /* Load PHO file - Not implemented */

    if(inkey_flag)
	printf("* ");
    switch(cmd)
    {
      case WRD_LYRIC:
	p = wrd_event2string(wrd_args[0]);
	len = strlen(p);
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));
	code_convert(p, text, SAFE_CONVERT_LENGTH(len), NULL, NULL);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "%s", text);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_NL: /* Newline (Ignored) */
	break;
      case WRD_COLOR:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "@COLOR(%d)", wrd_args[0]);
	break;
      case WRD_END: /* Never call */
	break;
      case WRD_ESC:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@ESC(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_EXEC:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@EXEC(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_FADE:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@FADE(%d,%d,%d)", wrd_args[0], wrd_args[1], wrd_args[2]);
	break;
      case WRD_FADESTEP:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@FADESTEP(%d/%d)", wrd_args[0], WRD_MAXFADESTEP);
	break;
      case WRD_GCIRCLE:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GCIRCLE(%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
	break;
      case WRD_GCLS:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GCLS(%d)", wrd_args[0]);
	break;
      case WRD_GINIT:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "@GINIT()");
	break;
      case WRD_GLINE:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GLINE(%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6]);
	break;
      case WRD_GMODE:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GMODE(%d)", wrd_args[0]);
	break;
      case WRD_GMOVE:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GMOVE(%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6], wrd_args[7], wrd_args[8]);
	break;
      case WRD_GON:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GON(%d)", wrd_args[0]);
	break;
      case WRD_GSCREEN:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GSCREEN(%d,%d)", wrd_args[0], wrd_args[1]);
	break;
      case WRD_INKEY:
	inkey_flag = 1;
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "@INKEY - begin");
	break;
      case WRD_OUTKEY:
	inkey_flag = 0;
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "@INKEY - end");
	break;
      case WRD_LOCATE:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@LOCATE(%d,%d)", wrd_args[0], wrd_args[1]);
	break;
      case WRD_LOOP: /* Never call */
	break;
      case WRD_MAG:
	p = (char *)new_segment(&tmpbuffer, MIN_MBLOCK_SIZE);
        snprintf(p, MIN_MBLOCK_SIZE-1, "@MAG(%s", wrd_event2string(wrd_args[0]));
        p[MIN_MBLOCK_SIZE-1] = '\0'; /* fail safe */
	for(i = 1; i < 5; i++)
	{
	    if(wrd_args[i] == WRD_NOARG)
		strncat(p, ",*", MIN_MBLOCK_SIZE - strlen(p) - 1);
	    else {
		char q[CHAR_BIT*sizeof(int)];
		snprintf(q, sizeof(q)-1, ",%d", wrd_args[i]);
		strncat(p, q, MIN_MBLOCK_SIZE - strlen(p) - 1);
            }
	}
        strncat(p, ")", MIN_MBLOCK_SIZE - strlen(p) - 1);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "%s", p);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_MIDI: /* Never call */
	break;
      case WRD_OFFSET: /* Never call */
	break;
      case WRD_PAL:
	p = (char *)new_segment(&tmpbuffer, MIN_MBLOCK_SIZE);
	snprintf(p, MIN_MBLOCK_SIZE, "@PAL(%03x", wrd_args[0]);
	for(i = 1; i < 17; i++) {
	    char q[5];
	    snprintf(q, sizeof(q)-1, ",%03x", wrd_args[i]);
	    strncat(p, q, MIN_MBLOCK_SIZE - strlen(p) - 1);
	}
	strncat(p, ")", MIN_MBLOCK_SIZE - strlen(p) - 1);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "%s", p);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_PALCHG:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@PALCHG(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_PALREV:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@PALREV(%d)", wrd_args[0]);
	break;
      case WRD_PATH:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@PATH(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_PLOAD:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@PLOAD(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_REM:
	p = wrd_event2string(wrd_args[0]);
	len = strlen(p);
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));
	code_convert(p, text, SAFE_CONVERT_LENGTH(len), NULL, NULL);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "@REM %s", text);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_REMARK:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@REMARK(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_REST: /* Never call */
	break;
      case WRD_SCREEN: /* Not supported */
	break;
      case WRD_SCROLL:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@SCROLL(%d,%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5], wrd_args[6]);
	break;
      case WRD_STARTUP:
	inkey_flag = 0;
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@STARTUP(%d)", wrd_args[0]);
	break;
      case WRD_STOP: /* Never call */
	break;
      case WRD_TCLS:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@TCLS(%d,%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
	break;
      case WRD_TON:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@TON(%d)", wrd_args[0]);
	break;
      case WRD_WAIT: /* Never call */
	break;
      case WRD_WMODE: /* Never call */
	break;

	/* Ensyutsukun */
      case WRD_eFONTM:
	print_ecmd("FONTM", wrd_args, 1);
	break;
      case WRD_eFONTP:
	print_ecmd("FONTP", wrd_args, 4);
	break;
      case WRD_eFONTR:
	print_ecmd("FONTR", wrd_args, 17);
	break;
      case WRD_eGSC:
	print_ecmd("GSC", wrd_args, 1);
	break;
      case WRD_eLINE:
	print_ecmd("LINE", wrd_args, 1);
	break;
      case WRD_ePAL:
	print_ecmd("PAL", wrd_args, 2);
	break;
      case WRD_eREGSAVE:
	print_ecmd("REGSAVE", wrd_args, 17);
	break;
      case WRD_eSCROLL:
	print_ecmd("SCROLL",wrd_args, 2);
	break;
      case WRD_eTEXTDOT:
	print_ecmd("TEXTDOT", wrd_args, 1);
	break;
      case WRD_eTMODE:
	print_ecmd("TMODE", wrd_args, 1);
	break;
      case WRD_eTSCRL:
	print_ecmd("TSCRL", wrd_args, 0);
	break;
      case WRD_eVCOPY:
	print_ecmd("VCOPY", wrd_args, 9);
	break;
      case WRD_eVSGET:
	print_ecmd("VSGE", wrd_args, 4);
	break;
      case WRD_eVSRES:
	print_ecmd("VSRES", wrd_args, 0);
	break;
      case WRD_eXCOPY:
	print_ecmd("XCOPY", wrd_args, 14);
	break;
      default:
	break;
    }
    wrd_argc = 0;
}
