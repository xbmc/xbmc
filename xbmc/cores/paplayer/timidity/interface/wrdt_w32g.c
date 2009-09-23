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
/*
 * WRD Tracer for Win32GUI
 * modified for Windows console by Daisuke Aoki <dai@y7.net>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
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

#include <windows.h>
#include "w32g_dib.h"
#include "w32g_wrd.h"

static int wrdt_open(char *dummy);
static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[]);
static void wrdt_update_events(void);
static void wrdt_end(void);
static void wrdt_close(void);
#define NO_GRAPHIC_SUPPORT
#define wrdt w32g_wrdt_mode
#define COLOR_REMAP(k) ((k)>16)&&((k)<30)?(k)+14:k

WRDTracer wrdt =
{
    "Windows Console WRD tracer", 'w',
    0,
    wrdt_open,
    wrdt_apply,
    NULL,
    wrdt_update_events,
    NULL,
    wrdt_end,
    wrdt_close
};

static int wrd_argc;
static int wrd_args[WRD_MAXPARAM];
static int inkey_flag;

static void putstring(char *str)
{
	WrdWndPutString(str,TRUE);
}

static void putstringn(char *str, int n)
{
	WrdWndPutStringN(str,n,TRUE);
}

/* Escape sequence */

static void esc_index(void)
{
	w32g_wrd_wnd.curposy++;
	if ( w32g_wrd_wnd.curposy >= w32g_wrd_wnd.col ) {
		WrdWndScrollUp(TRUE);
		w32g_wrd_wnd.curposy = w32g_wrd_wnd.col - 1;
	}
}

void esc_nextline(void)
{
	w32g_wrd_wnd.curposx = 0;
	w32g_wrd_wnd.curposy++;
	if ( w32g_wrd_wnd.curposy >= w32g_wrd_wnd.col ) {
		WrdWndScrollUp(TRUE);
		w32g_wrd_wnd.curposy = w32g_wrd_wnd.col - 1;
	}
}

void esc_reverseindex(void)
{
	w32g_wrd_wnd.curposy--;
	if ( w32g_wrd_wnd.curposy < 0 ) {
		WrdWndScrollDown(TRUE);
	}
}

void esc_clearscreen(void)
{
	WrdWndClear(TRUE);
}

void esc_directcursoraddressing(int x, int y)
{
	WrdWndGoto( x-1, y-1 );
}

void esc_cursorup(int n)
{
	if(n < 1) n = 1;
	WrdWndGoto( w32g_wrd_wnd.curposx, w32g_wrd_wnd.curposy - n );
}

void esc_cursordown(int n)
{
	if(n < 1) n = 1;
	WrdWndGoto( w32g_wrd_wnd.curposx, w32g_wrd_wnd.curposy + n );
}

void esc_cursorforward(int n)
{
	if(n < 1) n = 1;
	WrdWndGoto( w32g_wrd_wnd.curposx - n , w32g_wrd_wnd.curposy );
}

void esc_cursorbackward(int n)
{
	if(n < 1) n = 1;
	WrdWndGoto( w32g_wrd_wnd.curposx + n , w32g_wrd_wnd.curposy );
}

void esc_clearfromcursortoendofscreen(void)
{
	WrdWndLineClearFrom(FALSE, TRUE);
	WrdWndClearLineFromTo(w32g_wrd_wnd.curposy + 1, w32g_wrd_wnd.col - 1, TRUE);
}

void esc_clearfrombeginningofscreentocursor(void)
{
	WrdWndClearLineFromTo(0,w32g_wrd_wnd.curposy - 1, TRUE);
	WrdWndLineClearFrom(TRUE, TRUE);
}

void esc_clearfromcursortoendofline(void)
{
	WrdWndLineClearFrom(FALSE, TRUE);
}

void esc_clearfrombeginningoflinetocursor(void)
{
	WrdWndLineClearFrom(TRUE, TRUE);
}

void esc_clearentirelinecontainingcursor(void)
{
	WrdWndClearLineFromTo( w32g_wrd_wnd.curposy, w32g_wrd_wnd.curposy, TRUE );
}

void esc_deleteline(int n)
{
	int i;
	if(n < 1) n = 1;
	if( w32g_wrd_wnd.curposy + n >= w32g_wrd_wnd.col )
		n = w32g_wrd_wnd.col - 1 - w32g_wrd_wnd.curposy;

	for( i = w32g_wrd_wnd.curposy; i < w32g_wrd_wnd.curposy + n; i++ ) {
		WrdWndMoveLine(i+n,i,TRUE);
	}
	WrdWndClearLineFromTo(w32g_wrd_wnd.col - n, w32g_wrd_wnd.col - 1, TRUE);
	w32g_wrd_wnd.curposx = 0;
}

void esc_insertline(int n)
{
	int i;
	if(n < 1) n = 1;
	if( w32g_wrd_wnd.curposy + n >= w32g_wrd_wnd.col )
		n = w32g_wrd_wnd.col - 1 - w32g_wrd_wnd.curposy;

	for( i = w32g_wrd_wnd.col - n - 1; i >= w32g_wrd_wnd.curposy; i-- ) {
		WrdWndMoveLine(i,i+n,TRUE);
	}
	w32g_wrd_wnd.curposx = 0;
}

static volatile int saved_x;
static volatile int saved_y;
static volatile int saved_attr;
void esc_savecursorposition(void)
{
	WrdWndCurStateSaveAndRestore(1);
	return;
}

void esc_setcursorposition(void)
{
	WrdWndCurStateSaveAndRestore(0);
	return;
}

void esc_enablecursordisplay(void)
{
	return;
}

void esc_disablecursordisplay(void)
{
	return;
}

void esc_characterattribute(int n)
{
	WrdWndSetAttr98(n);
}

/* return figures */
static int getdigit(char *str,int *num)
{
	int i;
	char local[20];
	for(i=0;i<=10;i++)
		if(str[i] < '0' || str[i] > '9'){
			if(i<1)
				return 0;
			else
				break;
		}
	strncpy(local,str,i);
	local[i] = '\0';
	*num = atoi(local);
	return i;
}

extern int gdi_lock(void);
extern int gdi_unlock(void);
static void putstring_with_esc(char *str)
{
  char *p;
  while(*str)
  {
    p = str;
	for(;;)
	{
	  if((unsigned char)*p >= 0x20){
	    p++;
		continue;
	  }
	  if(p-str > 0){
		WrdWndPutStringN(str,p-str,TRUE);
		str = p;
		break;
	  }
	  if(*p == '\0')
		break;
		if(*p == '\n') {
			esc_nextline();
			str = p + 1;
			break;
		}
		if(*p == '\r' && *(p+1) == '\n') {
			esc_nextline();
			str = p + 2;
			break;
		}
		if(*p == '\t') {
			WrdWndPutStringN ( "        ", 8, TRUE );
			str = p + 1;
			break;
		}
	  if(*p != 0x1b){
			str = p + 1;
			break;
		}
	  if(*p == 0x1b){
		int res, n[1024], n_max = 0;
		char *oldp = p;
		p++;
		if(*p == '['){
			p++;
			for(;;){
				res = getdigit(p,&(n[n_max+1]));
				if(res>0){
					n_max++;
					p += res;
				}
				if(*p != ';')
					break;
				else
					p++;
			}
		} else if(*p == 'D'){
			esc_index();
			p++;
			str = p;
			break;
		} else if(*p == 'E'){
			esc_nextline();
			p++;
			str = p;
			break;
		} else if(*p == 'M'){
			esc_reverseindex();
			p++;
			str = p;
			break;
		} else if(*p == '*'){
			esc_clearscreen();
			p++;
			str = p;
			break;
		} else {
			p = oldp;
		    if(p-str > 0){
				WrdWndPutStringN(str,p-str,TRUE);
					str = p;
				break;
			}
		}

		if(n_max == 2 && (*p == 'H' || *p == 'f')){
			esc_directcursoraddressing(n[2],n[1]);
			p++;
			str = p;
			break;
		}
		if((n_max == 1 && *p == 'A') || (n_max == 0 && *p == 'A')){
			if(n_max == 0)
				n[1] = 1;
			esc_cursorup(n[1]);
			p++;
			str = p;
			break;
		}
		if((n_max == 1 && *p == 'B') || (n_max == 0 && *p == 'B')){
			if(n_max == 0)
				n[1] = 1;
			esc_cursordown(n[1]);
			p++;
			str = p;
			break;
		}
		if((n_max == 1 && *p == 'C') || (n_max == 0 && *p == 'C')){
			if(n_max == 0)
				n[1] = 1;
			esc_cursorforward(n[1]);
			p++;
			str = p;
			break;
		}
		if((n_max == 1 && *p == 'D') || (n_max == 0 && *p == 'D')){
			if(n_max == 0)
				n[1] = 1;
			esc_cursorbackward(n[1]);
			p++;
			str = p;
			break;
		}
		if((n_max == 1 && *p == 'J') || (n_max == 0 && *p == 'J')){
			if(n_max == 0 || n[1] == 0)
				esc_clearfromcursortoendofscreen();
			else if(n[1] == 1)
				esc_clearfrombeginningofscreentocursor();
			else if(n[1] == 2)
				esc_clearscreen();
			p++;
			str = p;
			break;
		}
		if((n_max == 1 && *p == 'K') || (n_max == 0 && *p == 'K')){
			if(n_max == 0 || n[1] == 0)
				esc_clearfromcursortoendofline();
			else if(n[1] == 1)
				esc_clearfrombeginningoflinetocursor();
			else if(n[1] == 2)
				esc_clearentirelinecontainingcursor();
			p++;
			str = p;
			break;
		}
		if((n_max == 1 && *p == 'M') || (n_max == 0 && *p == 'M')){
			if(n_max == 0)
				n[1] = 1;
			esc_deleteline(n[1]);
			p++;
			str = p;
			break;
		}
		if((n_max == 1 && *p == 'L') || (n_max == 0 && *p == 'L')){
			if(n_max == 0)
				n[1] = 1;
			esc_insertline(n[1]);
			p++;
			str = p;
			break;
		}
		if(n_max == 0 && *p == 's'){
			esc_savecursorposition();
			p++;
			str = p;
			break;
		}
		if(n_max == 0 && *p == 'u'){
			esc_setcursorposition();
			p++;
			str = p;
			break;
		}
		if(!strncmp(p,">5l",3)){
			esc_enablecursordisplay();
			p += 3;
			str = p;
			break;
		}
		if(!strncmp(p,">5h",3)){
			esc_disablecursordisplay();
			p += 3;
			str = p;
			break;
		}
		if(!strncmp(p,">1h",3)){
		/* Enabel bottom line */
			p += 3;
			str = p;
			break;
		}
		if(!strncmp(p,">1l",3)){
		/* Disabel bottom line */
			p += 3;
			str = p;
			break;
		}
		if(!strncmp(p,">3h",3)){
		/* Select 31 line mode */
			p += 3;
			str = p;
			break;
		}
		if(!strncmp(p,">3l",3)){
		/* Select 25 line mode */
			p += 3;
			str = p;
			break;
		}
		if(*p == 'm'){
			int i;
			for(i=1;i<=n_max;i++)
				esc_characterattribute(n[i]);
			p++;
			str = p;
			break;
		}
		p = oldp;
		WrdWndPutStringN(p,1,TRUE);
		p++;
		str = p;
		break;
		}
	}
  }
}


static int wrdt_open(char *dummy)
{
     wrdt.opened = 1;
    inkey_flag = 0;
	WrdWndReset();
    return 0;
}

static void wrdt_update_events(void)
{
}

static void wrdt_end(void)
{
	esc_enablecursordisplay();
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

#define SEIKIX(x) { \
	if ( x < 1 ) x = 1; \
	if ( x > w32g_wrd_wnd.row ) x = w32g_wrd_wnd.row; \
}
#define SEIKIY(y) { \
	if ( y < 1 ) y = 1; \
	if ( y > w32g_wrd_wnd.col ) y = w32g_wrd_wnd.col; \
}

static void borlandc_esc(char *str)
{
	char local[201];
	local[0] = '\033';
	local[1] = '[';
	strncpy(local+2,str,sizeof(local)-3);
	local[200] = '\0';
	putstring_with_esc(local);
}

extern void wrd_graphic_ginit ( void );
extern void wrd_graphic_gcls ( int sw );
extern void wrd_graphic_gscreen ( int active, int display );
extern void wrd_graphic_gon ( int sw );
extern void wrd_graphic_gline ( int x1, int y1, int x2, int y2, int p1, int sw, int p2 );
extern void wrd_graphic_gcircle ( int x, int y, int r, int p1, int sw, int p2 );
extern void wrd_graphic_pload ( char *path );
extern void wrd_graphic_pal_g4r4b4 ( int p, int *g4r4b4, int max );
extern void wrd_graphic_palrev ( int p );
extern void wrd_graphic_apply_pal ( int p );
extern void wrd_graphic_fade ( int p1, int p2, int speed );
extern void wrd_graphic_fadestep ( int v );
extern void wrd_graphic_gmode ( int sw );
extern void wrd_graphic_gmove ( int x1, int y1, int x2, int y2, int xd, int yd, int vs, int vd, int sw );
extern void wrd_graphic_mag ( char *path, int x, int y, int s, int p );
extern void wrd_text_ton ( int sw );
extern void wrd_text_scroll ( int x1, int y1, int x2, int y2, int mode, int color, int c );
extern void wrd_start_skip ( void );
extern void wrd_end_skip ( void );
extern void wrd_graphic_xcopy ( int sx1, int sy1, int sx2, int sy2, int tx, int ty, int ss, int ts, int method,
	 int opt1, int opt2, int opt3, int opt4, int opt5 );

// #define WRD_VERBOSE
static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[])
{
    char *p;
    char *text;
    int i, len;
    static txtclr_preserve=0;

	if ( !w32g_wrd_wnd.active ) return;

    switch(cmd)
    {
      case WRD_LYRIC:
	p = wrd_event2string(wrd_args[0]);
	len = strlen(p);
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));

	/*This must be not good thing,but as far as I know no wrd file 
	  written in EUC-JP code found*/

	strcpy(text,p);
#ifdef WRD_VERBOSE
//	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
//		  "[WRD_LYRIC]\n%s", text );
#endif
	putstring_with_esc(text);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_NL: /* Newline (Ignored) */
		esc_nextline();
//			 putchar('\n');
	break;
      case WRD_COLOR:
/*Compatibility Hack,This remaps color(17-29 color seems 
to be ignored in kterm)*/
	esc_characterattribute(wrd_args[0]);
	break;
      case WRD_END: /* Never call */
	break;
      case WRD_ESC:
	borlandc_esc(wrd_event2string(wrd_args[0]));
	break;
      case WRD_EXEC:
	/*I don't spaun another program*/
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@EXEC(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_FADE:
		  wrd_graphic_fade ( wrd_args[0], wrd_args[1], wrd_args[2] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@FADE(%d,%d,%d)", wrd_args[0], wrd_args[1], wrd_args[2]);
#endif
	break;
      case WRD_FADESTEP:
		 wrd_graphic_fadestep ( wrd_args[0] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@FADESTEP(%d/%d)", wrd_args[0], WRD_MAXFADESTEP);
#endif
	break;
      case WRD_GCIRCLE:
		wrd_graphic_gcircle ( wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5] );
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GCIRCLE(%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
	break;
      case WRD_GCLS:
		wrd_graphic_gcls ( wrd_args[0] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GCLS(%d)", wrd_args[0]);
#endif
	break;
      case WRD_GINIT:
		wrd_graphic_ginit ();
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "@GINIT()");
#endif
	break;
      case WRD_GLINE:
		wrd_graphic_gline ( wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GLINE(%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6]);
#endif
	break;
      case WRD_GMODE:
		  wrd_graphic_gmode ( wrd_args[0] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GMODE(%d)", wrd_args[0]);
#endif
	break;
      case WRD_GMOVE:
		  wrd_graphic_gmove ( wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6], wrd_args[7], wrd_args[8] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GMOVE(%d,%d,%d,%d,%d,%d,%d)",
	       wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4],
	       wrd_args[5], wrd_args[6], wrd_args[7], wrd_args[8]);
#endif
	break;
      case WRD_GON:
		wrd_graphic_gon ( wrd_args[0] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GON(%d)", wrd_args[0]);
#endif
	break;
      case WRD_GSCREEN:
		wrd_graphic_gscreen ( wrd_args[0], wrd_args[1] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@GSCREEN(%d,%d)", wrd_args[0], wrd_args[1]);
#endif
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
	{
	int x = wrd_args[0], y = wrd_args[1];
	WrdWndGoto(x-1, y-1);
	}
	break;
      case WRD_LOOP: /* Never call */
	break;
      case WRD_MAG:
		wrd_graphic_mag ( wrd_event2string(wrd_args[0]), wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4]);
#ifdef WRD_VERBOSE
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
#endif
	break;
      case WRD_MIDI: /* Never call */
	break;
      case WRD_OFFSET: /* Never call */
	break;
      case WRD_PAL:
		wrd_graphic_pal_g4r4b4 (wrd_args[0], wrd_args + 1, 16 );
#ifdef WRD_VERBOSE
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
#endif
	break;
      case WRD_PALCHG:
		wrd_graphic_apply_pal ( wrd_args[0] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@PALCHG(%s)", wrd_event2string(wrd_args[0]));
#endif
	break;
      case WRD_PALREV:
		wrd_graphic_palrev ( wrd_args[0] );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@PALREV(%d)", wrd_args[0]);
#endif
	break;
      case WRD_PATH:
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@PATH(%s)", wrd_event2string(wrd_args[0]));
	break;
      case WRD_PLOAD:
		wrd_graphic_pload ( wrd_event2string(wrd_args[0]) );
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@PLOAD(%s)", wrd_event2string(wrd_args[0]));
#endif
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
		wrd_text_scroll (wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5], wrd_args[6]);
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@SCROLL(%d,%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5], wrd_args[6]);
#endif
	break;
      case WRD_STARTUP:
		  WrdWndReset ();
	inkey_flag = 0;
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@STARTUP(%d)", wrd_args[0]);
#endif
	esc_clearscreen();
	break;
      case WRD_STOP: /* Never call */
	break;

      case WRD_TCLS:
	{
	  char fillbuf[1024];
	{
	int left = wrd_args[0], right = wrd_args[2];
	int top = wrd_args[1], bottom = wrd_args[3];

	SEIKIX(left);
	SEIKIX(right);
	SEIKIY(top);
	SEIKIY(bottom);
	if(left>right) right = left;
	if(top>bottom) bottom = top;
	memset(fillbuf,wrd_args[5],right-left);/*X2-X1*/
	fillbuf[right-left]=0;
//	borlandc_con_save_attr();
	esc_characterattribute(wrd_args[4]);
	for(i=top;i<=bottom;i++)/*Y1 to Y2*/ {
	  WrdWndGoto(left-1,i-1);
	  putstring(fillbuf);
	}
//	borlandc_con_restore_attr();
	}
	}
	break;
      case WRD_TON:
		wrd_text_ton ( wrd_args[0] );
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
		wrd_graphic_xcopy ( wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3], wrd_args[4], wrd_args[5], wrd_args[6],
			 wrd_args[7], wrd_args[8], wrd_args[9], wrd_args[10], wrd_args[11], wrd_args[12], wrd_args[13] );
	print_ecmd("XCOPY", wrd_args, 14);
	break;
	case WRD_START_SKIP:
		wrd_start_skip ();
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "WRD_START_SKIP");
#endif
		break;
	case WRD_END_SKIP:
		wrd_end_skip ();
#ifdef WRD_VERBOSE
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "WRD_END_SKIP");
#endif
		break;

	/* Extensionals */
    }
}
