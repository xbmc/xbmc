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
 * WRD Tracer for Windows console control terminal
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
#ifndef __CYGWIN32__
#include <conio.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include "wrd.h"

// #define DEBUG1
#ifndef __OLD_BORLANDC__
#define USE_ESC
#define NULL_STMT do ; while(0)
#define gotoxy(x, y) NULL_STMT
#define wherex() 1
#define wherey() 1
#define putch putchar
#define cputs puts
#define delline() NULL_STMT
#define clrscr() NULL_STMT
#define clreol() NULL_STMT
#endif /* __OLD_BORLANDC__ */

static int wrdt_open(char *dummy);
static void wrdt_apply(int cmd, int wrd_argc, int wrd_args[]);
static void wrdt_update_events(void);
static void wrdt_end(void);
static void wrdt_close(void);
#define NO_GRAPHIC_SUPPORT
#define wrdt wcon_wrdt_mode
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

#ifdef __OLD_BORLANDC__
static struct text_info text_info;
static struct conattr {
	int attr;
	int bc;
	int fc;
} iniattr, curattr, saveattr;

static void borlandc_con_set_attr(struct conattr *attr)
{
	if(attr->attr & REVERSE)
		textattr((attr->attr & ~REVERSE) + (attr->bc >> 4) + (attr->fc << 4));
	else
		textattr((attr->attr & ~REVERSE) + attr->bc + attr->fc);
}

static void borlandc_con_save_attr(void)
{
	saveattr.attr = curattr.attr;
	saveattr.bc = curattr.bc;
	saveattr.fc = curattr.fc;
}

static void borlandc_con_restore_attr(void)
{
	curattr.attr = saveattr.attr;
	curattr.bc = saveattr.bc;
	curattr.fc = saveattr.fc;
	borlandc_con_set_attr(&curattr);
}

static void borlandc_con_reset_attr(void)
{
	borlandc_con_set_attr(&iniattr);
}

static void borlandc_con_init(void)
{
	gettextinfo(&text_info);
	iniattr.attr = text_info.attribute & 0xff00;
	iniattr.bc = text_info.attribute & 0xf0;
	iniattr.fc = text_info.attribute & 0x0f;
	curattr.attr = text_info.attribute & 0xff00;
	curattr.bc = text_info.attribute & 0xf0;
	curattr.fc = text_info.attribute & 0x0f;
	borlandc_con_save_attr();
}

static void borlandc_con_reset(void)
{
	borlandc_con_reset_attr();
}
#else
struct {
    int winleft, winright;
    int wintop, winbottom;
} text_info = {
  1,25,
  1,80,
};
#endif

#define CANNOC_X(x) \
{\
	if(x < text_info.winleft) x = text_info.winleft;\
	else if(x > text_info.winright) x = text_info.winright;\
}
#define CANNOC_Y(y) \
{\
	if(y < text_info.wintop) y = text_info.wintop;\
	else if(y > text_info.winbottom) y = text_info.winbottom;\
}

static void putstring(char *str)
{
	cputs(str);
}

static void putstringn(char *str, int n)
{
	while(n--)
		putch(*(str++));
}

#ifdef __OLD_BORLANDC__
static void borlandc_con_color(int color)
{
	switch(color){
		case 0:
			borlandc_con_reset_attr();
			break;
		case 2:
			curattr.attr |= VERTICALLINE;
			borlandc_con_set_attr(&curattr);
			break;
		case 4:
			curattr.attr |= UNDERLINE;
			borlandc_con_set_attr(&curattr);
			break;
		case 5:
			curattr.attr |= BLINK;
			borlandc_con_set_attr(&curattr);
			break;
		case 8: case 16: case 30:
			curattr.attr &= ~REVERSE;
			curattr.fc = BLACK;
			borlandc_con_set_attr(&curattr);
			break;
		case 17: case 31:
			curattr.attr &= ~REVERSE;
			curattr.fc = LIGHTRED;
			borlandc_con_set_attr(&curattr);
			break;
		case 18: case 34:
			curattr.attr &= ~REVERSE;
			curattr.fc = LIGHTBLUE;
			borlandc_con_set_attr(&curattr);
			break;
		case 19: case 35:
			curattr.attr &= ~REVERSE;
			curattr.fc = LIGHTMAGENTA;
			borlandc_con_set_attr(&curattr);
			break;
		case 20: case 32:
			curattr.attr &= ~REVERSE;
			curattr.fc = LIGHTGREEN;
			borlandc_con_set_attr(&curattr);
			break;
		case 21: case 33:
			curattr.attr &= ~REVERSE;
			curattr.fc = YELLOW;
			borlandc_con_set_attr(&curattr);
			break;
		case 22: case 36:
			curattr.attr &= ~REVERSE;
			curattr.fc = LIGHTCYAN;
			borlandc_con_set_attr(&curattr);
			break;
		case 23: case 37:
			curattr.attr &= ~REVERSE;
			curattr.fc = WHITE;
			borlandc_con_set_attr(&curattr);
			break;
/*
		case 40:
			attr = REVERSE + (bc << 4) + BLACK;
			textattr(attr);
			break;
		case 41:
			attr = REVERSE + (bc << 4) + LIGHTRED;
			textattr(attr);
			break;
		case 42:
			attr = REVERSE + (bc << 4) + LIGHTGREEN;
			textattr(attr);
			break;
		case 43:
			attr = REVERSE + (bc << 4) + YELLOW;
			textattr(attr);
			break;
		case 44:
			attr = REVERSE + (bc << 4) + LIGHTBLUE;
			textattr(attr);
			break;
		case 45:
			attr = REVERSE + (bc << 4) + LIGHTMAGENTA;
			textattr(attr);
			break;
		case 46:
			attr = REVERSE + (bc << 4) + LIGHTCYAN;
			textattr(attr);
			break;
		case 47:
			attr = REVERSE + (bc << 4) + WHITE;
			textattr(attr);
			break;
*/
		case 40:
			curattr.attr |= REVERSE;
			curattr.fc = BLACK;
			borlandc_con_set_attr(&curattr);
			break;
		case 41:
			curattr.attr |= REVERSE;
			curattr.fc = LIGHTRED;
			borlandc_con_set_attr(&curattr);
			break;
		case 42:
			curattr.attr |= REVERSE;
			curattr.fc = LIGHTGREEN;
			borlandc_con_set_attr(&curattr);
			break;
		case 43:
			curattr.attr |= REVERSE;
			curattr.fc = YELLOW;
			borlandc_con_set_attr(&curattr);
			break;
		case 44:
			curattr.attr |= REVERSE;
			curattr.fc = LIGHTBLUE;
			borlandc_con_set_attr(&curattr);
			break;
		case 45:
			curattr.attr |= REVERSE;
			curattr.fc = LIGHTMAGENTA;
			borlandc_con_set_attr(&curattr);
			break;
		case 46:
			curattr.attr |= REVERSE;
			curattr.fc = LIGHTCYAN;
			borlandc_con_set_attr(&curattr);
			break;
		case 47:
			curattr.attr |= REVERSE;
			curattr.fc = WHITE;
			borlandc_con_set_attr(&curattr);
			break;
		default:
			break;
	}
	return;
}
#endif

/* Escape sequence */

static void esc_index(void)
{
	int x = wherex(), y = wherey();
	if(y<text_info.winbottom)
		gotoxy(x,y+1);
	else {
		putstring("\n");
		gotoxy(x,y);
	}
}

void esc_nextline(void)
{
	int y = wherey();
	if(y<text_info.winbottom)
		gotoxy(text_info.winleft,y+1);
	else {
		putstring("\n");
	}
}

void esc_reverseindex(void)
{
	int x = wherex(), y = wherey();
	if(y <= text_info.wintop)
		delline();
	else
		y--;
	gotoxy(x,y);
}

void esc_clearscreen(void)
{
	clrscr();
}

void esc_directcursoraddressing(int x, int y)
{
	CANNOC_X(x);
	CANNOC_Y(y);
	gotoxy(x,y);
}

void esc_cursorup(int n)
{
	int y;
	if(n < 1) n = 1;
	y = wherey() - n;
	CANNOC_Y(y);
	gotoxy(wherex(),y);
}

void esc_cursordown(int n)
{
	int y;
	if(n < 1) n = 1;
	y = wherey() + n;
	CANNOC_Y(y);
	gotoxy(wherex(),y);
}

void esc_cursorforward(int n)
{
	int x;
	if(n < 1) n = 1;
	x = wherex() - n;
	CANNOC_X(x);
	gotoxy(x,wherey());
}

void esc_cursorbackward(int n)
{
	int x;
	if(n < 1) n = 1;
	x = wherex() + n;
	CANNOC_X(x);
	gotoxy(x,wherey());
}

void esc_clearfromcursortoendofscreen(void)
{
	int oldx = wherex(), oldy = wherey();
	int y;
	clreol();
	for(y=oldy+1;y<=text_info.winbottom;y++){
		gotoxy(text_info.winleft,y);
		clreol();
	}
	gotoxy(oldx,oldy);
}

void esc_clearfrombeginningofscreentocursor(void)
{
	int oldx = wherex(), oldy = wherey();
	int y;
	for(y=oldy;y<=text_info.winbottom;y++){
		gotoxy(text_info.winleft,y);
		clreol();
	}
	gotoxy(oldx,oldy);
}

void esc_clearfromcursortoendofline(void)
{
	int oldx = wherex(), oldy = wherey();
	int x;
	for(x=oldx;x<=text_info.winright;x++)
		putstringn(" ",1);
	gotoxy(oldx,oldy);
}

void esc_clearfrombeginningoflinetocursor(void)
{
	int oldx = wherex(), oldy = wherey();
	int x;
	for(x=oldx;x>=text_info.winright;x--)
		putstringn(" ",1);
	gotoxy(oldx,oldy);
}

void esc_clearentirelinecontainingcursor(void)
{
	int oldx = wherex(), oldy = wherey();
	int x;
	for(x=text_info.winleft;x<=text_info.winright;x++)
		putstringn(" ",1);
	gotoxy(oldx,oldy);
}

void esc_deleteline(int n)
{
	int i, n_max;
	if(n < 1) n = 1;
	n_max = text_info.winbottom - wherey() + 1;
	if(n > n_max) n = n_max;
	for(i=1;i<=n;i++)
		delline();
	gotoxy(text_info.winleft,wherey());
}

void esc_insertline(int n)
{
	int i;
	if(n < 1) n = 1;
	for(i=1;i<=n;i++)
		delline();
	gotoxy(text_info.winleft,wherey());
}

void esc_savecursorposition(void)
{
	return;
}

void esc_setcursorposition(void)
{
	return;
}

void esc_enablecursordisplay(void)
{
#ifdef __OLD_BORLANDC__
	_setcursortype(_NORMALCURSOR);
#endif /* __OLD_BORLANDC__ */
}

void esc_disablecursordisplay(void)
{
#ifdef __OLD_BORLANDC__
	_setcursortype(_NOCURSOR);
#endif /* __OLD_BORLANDC__ */
}

void esc_characterattribute(int n)
{
#ifdef __OLD_BORLANDC__
	borlandc_con_color(n);
#endif
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
		putstringn(str,p-str);
		str = p;
		break;
	  }
	  if(*p == '\0')
		break;
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
				putstringn(str,p-str);
					str = p;
				break;
			}
		}

		if(n_max == 2 && (*p == 'H' || *p == 'f')){
			esc_directcursoraddressing(n[1],n[2]);
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
		putstringn(p,1);
		p++;
		str = p;
		break;
	  }
	}
  }
}

/*
static void borlandc_esc(char *str)
{
	if(!strncmp(str,"0J",2)){
		int oldx = wherex(), oldy = wherey();
		int y;
		clreol();
		for(y=oldy+1;y<=text_info.winbottom;y++){
			gotoxy(1,y);
			clreol();
		}
		gotoxy(oldx,oldy);
		return;
	}
	if(!strncmp(str,"1J",2)){
		int oldx = wherex(), oldy = wherey();
		int y;
		for(y=oldy;y<=text_info.winbottom;y++){
			gotoxy(1,y);
			clreol();
		}
		gotoxy(oldx,oldy);
		return;
	}
	if(!strncmp(str,"2J",2)){
		clrscr();
		return;
	}
}
*/

#ifdef __OLD_BORLANDC__
static void borlandc_esc(char *str)
{
	char local[201];
	local[0] = '\033';
	local[1] = '[';
	strncpy(local+2,str,sizeof(local)-3);
	local[200] = '\0';
	putstring_with_esc(local);
}
#endif /* __OLD_BORLANDC__ */

static int wrdt_open(char *dummy)
{
  
    wrdt.opened = 1;
    inkey_flag = 0;
#ifdef __OLD_BORLANDC__
//	highvideo();
	borlandc_con_init();
	esc_disablecursordisplay();
#endif
    return 0;
}

static void wrdt_update_events(void)
{
}

static void wrdt_end(void)
{
#ifdef USE_ESC
    printf("\033[0m\n");/*Restore Attributes*/
#else
	borlandc_con_reset();
	esc_enablecursordisplay();
#endif
#ifdef DEBUG1
	printf("[wrdt_end]");
#endif
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
    static txtclr_preserve=0;

    switch(cmd)
    {
      case WRD_LYRIC:
	p = wrd_event2string(wrd_args[0]);
	len = strlen(p);
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len));

	/*This must be not good thing,but as far as I know no wrd file 
	  written in EUC-JP code found*/

//	code_convert(p, text, SAFE_CONVERT_LENGTH(len), "SJIS", "JISK");
//	code_convert(p, text, SAFE_CONVERT_LENGTH(len), "SJIS", "NOCV");
	strcpy(text,p);
//	printf("%s",text);
//	cprintf("%s",text);
	putstring_with_esc(text);
	fflush(stdout);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_NL: /* Newline (Ignored) */
	putchar('\n');
	break;
      case WRD_COLOR:
/*Compatibility Hack,This remaps color(17-29 color seems 
to be ignored in kterm)*/
#ifdef USE_ESC
	txtclr_preserve=COLOR_REMAP(wrd_args[0]);
	printf("\033[%dm", txtclr_preserve);
#else
	esc_characterattribute(wrd_args[0]);
#endif
#ifdef DEBUG1
	printf("[wrd_color]");
#endif
	break;
      case WRD_END: /* Never call */
	break;
      case WRD_ESC:
#ifdef USE_ESC
	printf("\033[%s", wrd_event2string(wrd_args[0]));
#else
	borlandc_esc(wrd_event2string(wrd_args[0]));
#endif
#ifdef DEBUG1
	printf("[wrd_esc]");
#endif
	break;
      case WRD_EXEC:
	/*I don't spaun another program*/
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
	fflush(stdout);
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
#ifdef USE_ESC
	printf("\033[%d;%dH", wrd_args[1], wrd_args[0]);
#else
	{
	int x = wrd_args[0], y = wrd_args[1];
	if(x<1) x = 1;
	if(y<1) y = 1;
	gotoxy(x, y);
	}
#endif
#ifdef DEBUG1
	printf("[wrd_locate]");
#endif
	break;
      case WRD_LOOP: /* Never call */
	break;
      case WRD_MAG:
	p = (char *)new_segment(&tmpbuffer, MIN_MBLOCK_SIZE);
	strcpy(p, "@MAG(");
	strcat(p, wrd_event2string(wrd_args[0]));
	strcat(p, ",");
	for(i = 1; i < 3; i++)
	{
	    if(wrd_args[i] == WRD_NOARG)
		strcat(p, "*,");
	    else
		sprintf(p + strlen(p), "%d,", wrd_args[i]);
	}
	sprintf(p + strlen(p), "%d,%d)", wrd_args[3], wrd_args[4]);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "%s", p);
	reuse_mblock(&tmpbuffer);
	break;
      case WRD_MIDI: /* Never call */
	break;
      case WRD_OFFSET: /* Never call */
	break;
      case WRD_PAL:
	p = (char *)new_segment(&tmpbuffer, MIN_MBLOCK_SIZE);
	sprintf(p, "@PAL(%03x", wrd_args[0]);
	for(i = 1; i < 17; i++)
	    sprintf(p + strlen(p), ",%03x", wrd_args[i]);
	strcat(p, ")");
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
#ifdef USE_ESC
	printf("\033[0m\033[H\033[J");
#else
	esc_clearscreen();
#endif
#ifdef DEBUG1
	printf("[wrd_startup]");
#endif
	fflush(stdout);
	break;
      case WRD_STOP: /* Never call */
	break;

      case WRD_TCLS:
	{
	  char fillbuf[1024];
#ifdef USE_ESC
	  fillbuf[0]=0x1b;
	  fillbuf[1]='7';
	  fillbuf[2]=0;
	  printf(fillbuf);
	  i=COLOR_REMAP(wrd_args[4]);
	  printf("\033[%dm",i);
#ifdef DEBUG1
	printf("[wrd_tcls_1]");
#endif
	  memset(fillbuf,wrd_args[5],wrd_args[2]-wrd_args[0]);/*X2-X1*/
	  fillbuf[wrd_args[2]-wrd_args[0]]=0;
	  for(i=wrd_args[1];i<=wrd_args[3];i++)/*Y1 to Y2*/
	    printf("\033[%d;%dH%s",i,wrd_args[0],fillbuf);/*X1to....*/
#ifdef DEBUG1
	printf("[wrd_tcls_2]");
#endif
	  fillbuf[0]=0x1b;
	  fillbuf[1]='8';
	  fillbuf[2]=0;
	  printf(fillbuf);
	  printf("\033[%dm",txtclr_preserve);
#ifdef DEBUG1
	printf("[wrd_tcls_3]");
#endif
#else
	{
	int left = wrd_args[0], right = wrd_args[2];
	int top = wrd_args[1], bottom = wrd_args[3];
	CANNOC_X(left);
	CANNOC_X(right);
	CANNOC_Y(top);
	CANNOC_Y(bottom);
	if(left>right) right = left;
	if(top>bottom) bottom = top;
	memset(fillbuf,wrd_args[5],right-left);/*X2-X1*/
	fillbuf[right-left]=0;
	borlandc_con_save_attr();
	esc_characterattribute(wrd_args[4]);
	for(i=top;i<=bottom;i++)/*Y1 to Y2*/ {
	  gotoxy(left,i);
	  putstring(fillbuf);
	}
	borlandc_con_restore_attr();
	}
#ifdef DEBUG1
	printf("[wrd_tcls]");
#endif
#endif
	  fflush(stdout);
	}
#if 0
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		  "@TCLS(%d,%d,%d,%d,%d,%d,%d)",
		  wrd_args[0], wrd_args[1], wrd_args[2], wrd_args[3],
		  wrd_args[4], wrd_args[5]);
#endif
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

	/* Extensionals */
    }
}
