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

    wrdt_x.c - WRD Tracer for X Window

    Written by Takanori Watanabe <takawata@shidahara1.planet.kobe-u.ac.jp>
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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include "wrd.h"
#include "x_wrdwindow.h"
#include "x_mag.h"
#ifdef ENABLE_SHERRY
#include "x_sherry.h"
#endif /* ENABLE_SHERRY */


static int wrdt_open(char *dummy);
static void wrdt_apply(int cmd, int wrd_argc, int wrd_argv[]);
static void wrdt_update_events(void);
static int wrdt_start(int wrd_mode);
static void wrdt_end(void);
static void wrdt_close(void);
static int current_wrd_mode = WRD_TRACE_NOTHING;
static char *save_open_flag;

#define wrdt x_wrdt_mode

WRDTracer wrdt =
{
    "X WRD tracer", 'x',
    0,
    wrdt_open,
    wrdt_apply,
#ifdef ENABLE_SHERRY
    x_sry_wrdt_apply,
#else
    NULL,
#endif
    wrdt_update_events,
    wrdt_start,
    wrdt_end,
    wrdt_close
};

static int inkey_flag;
#define LINEBUF 1024
static char line_buf[LINEBUF];

static int wrdt_open(char *arg)
{
    save_open_flag = arg;
    wrdt.opened = 1;
    return 0;
}

static void wrdt_update_events(void)
{
    if(current_wrd_mode == WRD_TRACE_MIMPI)
	WinEvent();
#ifdef ENABLE_SHERRY
    else if(current_wrd_mode == WRD_TRACE_SHERRY)
	x_sry_event();
#endif
}

static int wrdt_start(int mode)
{
    int last_mode = current_wrd_mode;

    current_wrd_mode = mode;
    switch(mode)
    {
      case WRD_TRACE_NOTHING:
	switch(last_mode)
	{
	  case WRD_TRACE_MIMPI:
	    CloseWRDWindow();
	    break;
	  case WRD_TRACE_SHERRY:
#ifdef ENABLE_SHERRY
	    CloseSryWindow();
#endif /* ENABLE_SHERRY */
	    break;
	}
	break;

      case WRD_TRACE_MIMPI:
#ifdef ENABLE_SHERRY
	if(last_mode == WRD_TRACE_SHERRY)
	    CloseSryWindow();
#endif /* ENABLE_SHERRY */
	  if(OpenWRDWindow(save_open_flag) == -1)
	  {
	      current_wrd_mode = WRD_TRACE_NOTHING;
	      return -1; /* Error */
	  }
	break;

      case WRD_TRACE_SHERRY:
	if(last_mode == WRD_TRACE_MIMPI)
	    CloseWRDWindow();
#ifdef ENABLE_SHERRY
	if(OpenSryWindow(save_open_flag) == -1)
	{
	    current_wrd_mode = WRD_TRACE_NOTHING;
	    return -1; /* Error */
	}
#endif /* ENABLE_SHERRY */
	break;
    }

    return 0;
}

static void wrdt_end(void)
{
  inkey_flag = 0;
  mag_deletetab();
  x_VRel();
}

static void wrdt_close(void)
{
    if(wrdt.opened)
    {
	EndWin();
#ifdef ENABLE_SHERRY
	x_sry_close();
#endif /* ENABLE_SHERRY */
	wrdt.opened = 0;
	inkey_flag = 0;
    }
}

static char *wrd_event2string(int id)
{
    char *name;

    name = event2string(id);
    if(name != NULL)
	return name + 1;
    return "";
}

static void load_default_graphics(char *fn)
{
    MBlockList pool;
    char *file, *p;
    int is_caseUpper;
    magdata *m;

    init_mblock(&pool);
    file = (char *)new_segment(&pool, strlen(fn) + 5);
    strcpy(file, fn);
    if((p = strrchr(file, '.')) == NULL)
	goto done;
    is_caseUpper = ('A' <= p[1] && p[1] <= 'Z');

    /* try load default MAG file */
    strcpy(p + 1, is_caseUpper ? "MAG" : "mag");
    if((m = mag_create(file)) != NULL)
    {
	x_Mag(m, WRD_NOARG, WRD_NOARG, 0, 0);
	goto done;
    }

    /* try load default PHO file */
    strcpy(p + 1, is_caseUpper ? "PHO" : "pho");
    x_PLoad(file);

 done:
    reuse_mblock(&pool);
}

static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[])
{
    char *p;
    char *text;
    int i, len;
    static int txtclr_preserve=0;

    if(cmd == WRD_MAGPRELOAD){
	/* Load MAG file */
	magdata *m;
	char *p = wrd_event2string(wrd_args[0]);
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Loading magfile: [%s]", p);
	m = mag_create(p);
	if(m == NULL)
	    ctl->cmsg(CMSG_WARNING, VERB_NOISY, "Can't load magfile: %s", p);
	else
	{
	    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "MAG %s: [%d,%d,%d,%d]",
		      p, m->xorig, m->xend, m->yorig, m->yend);
	}
	return;
    }
    if(cmd == WRD_PHOPRELOAD){
	/* Load PHO file - Not implemented */
	return;
    }
#if 0
    if(inkey_flag)
	fprintf(file_out,"* ");
#endif
    switch(cmd)
    {
      case WRD_LYRIC:
	p = wrd_event2string(wrd_args[0]);
	len = strlen(p);
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));

	/*This must be not good thing,but as far as I know no wrd file 
	  written in EUC-JP code can be found,And Hankaku kana required 
	  for layout of charactor and ASCII art.*/

	code_convert(p, text, SAFE_CONVERT_LENGTH(len), "SJIS", "JISK");
	AddLine(text,0);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_NL: /* Newline */
	AddLine("\n",0);
	break;
      case WRD_COLOR:
	txtclr_preserve=wrd_args[0];
	/*This length is at most 20 ; this is much lesser than LINEBUF*/
	snprintf(line_buf,LINEBUF,"\033[%dm", txtclr_preserve);
	AddLine(line_buf,0);
	break;
      case WRD_END: /* Never call */
	break;
      case WRD_ESC:
	AddLine("\033[",0);
	AddLine(wrd_event2string(wrd_args[0]),0);
	break;
      case WRD_EXEC:
	/*I don't spawn another program*/
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@EXEC(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_FADE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@FADE(%d,%d,%d)", wrd_args[0], wrd_args[1], wrd_args[2]);
	x_Fade(wrd_args,wrd_argc-1,-1,-1);
	break;
      case WRD_FADESTEP:
#if 0
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@FADESTEP(%d/%d)", wrd_args[0], wrd_args[1]);
#endif
	x_Fade(NULL,0,wrd_args[0],wrd_args[1]);
	break;
      case WRD_GCIRCLE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GCIRCLE(%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
	x_GCircle(wrd_args,wrd_argc-1);
	break;
      case WRD_GCLS:
	x_Gcls(wrd_args[0]);
	break;
      case WRD_GINIT:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "@GINIT()");
	break;
      case WRD_GLINE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GLINE(%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6]);
	/*GRPH_LINE_MODE=1*/
	x_Gline(wrd_args,wrd_argc-1);
	break;
      case WRD_GMODE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GMODE(%d)", wrd_args[0]);
	x_GMode(wrd_args[0]);
	break;
      case WRD_GMOVE:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GMOVE(%d,%d,%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6], wrd_args[7], wrd_args[8]);
	wrd_args[0] &= ~0x7;
	wrd_args[4] &= ~0x7;  
	wrd_args[2] |= 0x7;
 	x_GMove(wrd_args[0], wrd_args[1], wrd_args[2],
		wrd_args[3], wrd_args[4],wrd_args[5],
		wrd_args[6], wrd_args[7], wrd_args[8]);
	break;
      case WRD_GON:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GON(%d)", wrd_args[0]);
	x_Gon(wrd_args[0]);
	break;
      case WRD_GSCREEN:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@GSCREEN(%d,%d)", wrd_args[0], wrd_args[1]);
	x_Gscreen(wrd_args[0],wrd_args[1]);
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
	/*Length is At most 40*/
	snprintf(line_buf,LINEBUF,"\033[%d;%dH", wrd_args[1], wrd_args[0]);
	AddLine(line_buf,0);
	break;
      case WRD_LOOP: /* Never call */
	break;
      case WRD_MAG:
  	{
	  magdata *m;
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
	  m=mag_search(wrd_event2string(wrd_args[0]));
	  if(m!=NULL){
	    x_Mag(m,wrd_args[1],wrd_args[2],wrd_args[3],wrd_args[4]);
	  }
	  reuse_mblock(&tmpbuffer);
	  break;
  	}
      case WRD_MIDI: /* Never call */
	break;
      case WRD_OFFSET: /* Never call */
	break;
      case WRD_PAL:
	x_Pal(wrd_args,wrd_argc-1);
	break;
      case WRD_PALCHG:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@PALCHG(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_PALREV:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@PALREV(%d)", wrd_args[0]);
	x_Palrev(wrd_args[0]);
	break;
      case WRD_PATH:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@PATH(%s)", wrd_event2string(wrd_args[0]));
	wrd_add_path(wrd_event2string(wrd_args[0]), 0);
	break;
      case WRD_PLOAD:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@PLOAD(%s)", wrd_event2string(wrd_args[0]));
	x_PLoad(wrd_event2string(wrd_args[0]));
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
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@STARTUP(%d)", wrd_args[0]);
	wrd_init_path();
	inkey_flag = 0;
	x_Startup(wrd_args[0]);
	load_default_graphics(current_file_info->filename);
	break;
      case WRD_STOP: /* Never call */
	break;
      case WRD_TCLS:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@TCLS(%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
	{
	  char fillbuf[1024];
	  int xdiff;
	  xdiff=wrd_args[2]-wrd_args[0]+1;
	  if(xdiff>80)
	    xdiff = 79-wrd_args[0]+1;
	  if(xdiff<0||xdiff>80)
	    break;
	  fillbuf[0]=0x1b;
	  fillbuf[1]='[';
	  fillbuf[2]='s';
	  fillbuf[3]=0;
	  AddLine(fillbuf,0);
	  i=wrd_args[4];
	  /*This Length is no more than 1024*/
	  sprintf(fillbuf,"\033[%dm",i);
	  AddLine(fillbuf,0);
	  memset(fillbuf,wrd_args[5],xdiff);/*X2-X1*/
	  fillbuf[xdiff]=0;
	  for(i=wrd_args[1];i<=wrd_args[3];i++){/*Y1 to Y2*/
	    snprintf(line_buf,LINEBUF,"\033[%d;%dH",i,wrd_args[0]);
/*X1to....*/
	    AddLine(line_buf,0);
	    AddLine(fillbuf,0);
	  }
	  fillbuf[0]=0x1b;
	  fillbuf[1]='[';
	  fillbuf[2]='u';
	  fillbuf[3]=0;
	  AddLine(fillbuf,0);
	}
	break;
      case WRD_TON:
	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "@TON(%d)", wrd_args[0]);
	x_Ton(wrd_args[0]);
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
	x_VCopy(wrd_args[0],wrd_args[1],wrd_args[2],wrd_args[3]
		,wrd_args[4],wrd_args[5],wrd_args[6],wrd_args[7],wrd_args[8]);
	break;
      case WRD_eVSGET:
	print_ecmd("VSGET", wrd_args, 4);
	x_VSget(wrd_args,4);
	break;
      case WRD_eVSRES:
	print_ecmd("VSRES", wrd_args, 0);
	x_VRel();
	break;
      case WRD_eXCOPY:
	if(wrd_argc < 9)
	    break;
	x_XCopy(wrd_args[0],wrd_args[1],wrd_args[2],wrd_args[3],wrd_args[4],
		wrd_args[5],wrd_args[6],wrd_args[7],wrd_args[8],
		wrd_args + 9, wrd_argc - 9);
	break;

	/* Extensionals */
      case WRD_START_SKIP:
	if(current_wrd_mode == WRD_TRACE_MIMPI)
	    x_RedrawControl(0);
#ifdef ENABLE_SHERRY
	else if(current_wrd_mode == WRD_TRACE_SHERRY)
	    x_sry_redraw_ctl(0);
#endif /* ENABLE_SHERRY */
	break;
      case WRD_END_SKIP:
	if(current_wrd_mode == WRD_TRACE_MIMPI)
	    x_RedrawControl(1);
#ifdef ENABLE_SHERRY
	else if(current_wrd_mode == WRD_TRACE_SHERRY)
	    x_sry_redraw_ctl(1);
#endif /* ENABLE_SHERRY */
	break;
#ifdef ENABLE_SHERRY
      case WRD_SHERRY_UPDATE:
	x_sry_update();
	break;
#endif /* ENABLE_SHERRY */
    }
    WinFlush();
}
