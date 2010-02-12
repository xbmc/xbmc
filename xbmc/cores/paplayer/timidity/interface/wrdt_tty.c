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

/*
 * WRD Tracer for vt100 control terminal
 * Written by Takanori Watanabe <takawata@shidahara1.planet.kobe-u.ac.jp>
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

static int wrdt_open(char *dummy);
static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[]);
static void wrdt_update_events(void);
static void wrdt_end(void);
static void wrdt_close(void);
#define NO_GRAPHIC_SUPPORT
#define wrdt tty_wrdt_mode

WRDTracer wrdt =
{
    "TTY WRD tracer", 't',
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

static int wrdt_open(char *dummy)
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
    printf("\033[0m\n");/*Restore Attributes*/
    inkey_flag = 0;
}

static void wrdt_close(void)
{
    printf("\033[0m");
    fflush(stdout);
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
    static int txtclr_preserve=0;

#if 0
    if(inkey_flag)
	printf("* ");
#endif
    switch(cmd)
    {
      case WRD_LYRIC:
	p = wrd_event2string(wrd_args[0]);
	len = strlen(p);
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));

	/*This must be not good thing,but as far as I know no wrd file 
	  written in EUC-JP code found*/

	code_convert(p, text, SAFE_CONVERT_LENGTH(len), "SJIS", (char *)-1);
	printf("%s",text);
	fflush(stdout);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_NL: /* Newline (Ignored) */
	putchar('\n');
	break;
      case WRD_COLOR:
	txtclr_preserve=wrd_args[0];
	if(16 <= txtclr_preserve && txtclr_preserve <= 23)
	    txtclr_preserve = wrd_color_remap[txtclr_preserve - 16] + 30;
	printf("\033[%dm", txtclr_preserve);
	break;
      case WRD_END: /* Never call */
	break;
      case WRD_ESC:
	printf("\033[%s", wrd_event2string(wrd_args[0]));
	break;
      case WRD_EXEC:
	/*I don't spaun another program*/
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@EXEC(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_FADE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@FADE(%d,%d,%d)", wrd_args[0], wrd_args[1], wrd_args[2]);
	break;
      case WRD_FADESTEP:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@FADESTEP(%d/%d)", wrd_args[0], WRD_MAXFADESTEP);
	break;
      case WRD_GCIRCLE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GCIRCLE(%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
	break;
      case WRD_GCLS:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GCLS(%d)", wrd_args[0]);
	break;
      case WRD_GINIT:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "@GINIT()");
	break;
      case WRD_GLINE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GLINE(%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6]);
	fflush(stdout);
	break;
      case WRD_GMODE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GMODE(%d)", wrd_args[0]);
	break;
      case WRD_GMOVE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GMOVE(%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6], wrd_args[7], wrd_args[8]);
	break;
      case WRD_GON:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GON(%d)", wrd_args[0]);
	break;
      case WRD_GSCREEN:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GSCREEN(%d,%d)", wrd_args[0], wrd_args[1]);
	break;
      case WRD_INKEY:
	inkey_flag = 1;
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "@INKEY - begin");
	break;
      case WRD_OUTKEY:
	inkey_flag = 0;
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "@INKEY - end");
	break;
      case WRD_LOCATE:
	printf("\033[%d;%dH", wrd_args[1], wrd_args[0]);
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
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@PALCHG(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_PALREV:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@PALREV(%d)", wrd_args[0]);
	break;
      case WRD_PATH:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@PATH(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_PLOAD:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@PLOAD(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_REM:
	p = wrd_event2string(wrd_args[0]);
	len = strlen(p);
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));
	code_convert(p, text, SAFE_CONVERT_LENGTH(len), NULL, NULL);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "@REM %s", text);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_REMARK:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@REMARK(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_REST: /* Never call */
	break;
      case WRD_SCREEN: /* Not supported */
	break;
      case WRD_SCROLL:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@SCROLL(%d,%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5], wrd_args[6]);
	break;
      case WRD_STARTUP:
	inkey_flag = 0;
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@STARTUP(%d)", wrd_args[0]);
	printf("\033[0m\033[H\033[J");
	fflush(stdout);
	break;
      case WRD_STOP: /* Never call */
	break;

      case WRD_TCLS:
	{
	  char fillbuf[1024];
	  fillbuf[0]=0x1b;
	  fillbuf[1]='7';
	  fillbuf[2]=0;
	  printf("%s", fillbuf);

	  /* 0-7: normal, 8-16: reverse */
	  if(wrd_args[4] <= 7)
	      wrd_args[4] += 30;
	  else
	      wrd_args[4] += 32;
	  printf("\033[%dm",wrd_args[4]);

	  memset(fillbuf,wrd_args[5],wrd_args[2]-wrd_args[0]);/*X2-X1*/
	  fillbuf[wrd_args[2]-wrd_args[0]]=0;
	  for(i=wrd_args[1];i<=wrd_args[3];i++)/*Y1 to Y2*/
	    printf("\033[%d;%dH%s",i,wrd_args[0],fillbuf);/*X1to....*/
	  fillbuf[0]=0x1b;
	  fillbuf[1]='8';
	  fillbuf[2]=0;
	  printf("%s", fillbuf);
	  printf("\033[%dm",txtclr_preserve);
	  fflush(stdout);
	}
#if 0
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@TCLS(%d,%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
#endif
	break;
      case WRD_TON:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@TON(%d)", wrd_args[0]);
	break;
      case WRD_WAIT: /* Never call */
	break;
      case WRD_WMODE: /* Never call */
	break;

	/* Ensyutsukun */
      case WRD_eFONTM:
	break;
      case WRD_eFONTP:
	break;
      case WRD_eFONTR:
	break;
      case WRD_eGSC:
	break;
      case WRD_eLINE:
	break;
      case WRD_ePAL:
	break;
      case WRD_eREGSAVE:
	break;
      case WRD_eSCROLL:
	break;
      case WRD_eTEXTDOT:
	break;
      case WRD_eTMODE:
	break;
      case WRD_eTSCRL:
	break;
      case WRD_eVCOPY:
	break;
      case WRD_eVSGET:
	break;
      case WRD_eVSRES:
	break;
      case WRD_eXCOPY:
	break;
      default:
	break;
    }
    wrd_argc = 0;
}
