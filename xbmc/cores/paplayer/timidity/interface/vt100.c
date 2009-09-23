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
#include "timidity.h"
#include "vt100.h"

/* #define NOT_FULLSCREEN */

/*
d0|vt100|vt100-am|vt100am|Digital VT100:\
        :cr=^M:nl=^J:bl=^G:ta=^I:\
        :do=^J:co#80:li#24:cl=50\E[;H\E[2J:sf=2*\ED:\
        :le=^H:bs:am:cm=5\E[%i%d;%dH:nd=2\E[C:up=2\E[A:\
        :ce=3\E[K:cd=50\E[J:so=2\E[7m:se=2\E[m:us=2\E[4m:ue=2\E[m:\
        :md=2\E[1m:mr=2\E[7m:mb=2\E[5m:me=2\E[m:is=\E[1;24r\E[24;1H:\
        :ct=2\E[3g:st=2\EH:\
        :rf=/usr/share/lib/tabset/vt100:\
        :rs=\E>\E[?3l\E[?4l\E[?5l\E[?7h\E[?8h:ks=\E[?1h:ke=\E[?1l:\
        :ku=\EOA:kd=\EOB:kr=\EOC:kl=\EOD:kb=^H:\
        :ho=\E[H:k1=\EOP:k2=\EOQ:k3=\EOR:k4=\EOS:pt:sr=2*\EM:vt#3:xn:\
        :sc=\E7:rc=\E8:cs=\E[%i%d;%dr:\
        :it#8:xo:
*/

#ifndef NOT_FULLSCREEN

void vt100_init_screen(void)
{
    vt100_reset_attr();
    fputs("\033[H\033[J", stdout);
}

void vt100_refresh(void)
{
    fputs("\033[H", stdout);
    fflush(stdout);
}

void vt100_clrtoeol(void)
{
    fputs("\033[K", stdout);
}

void vt100_move(int row, int col)
{
    fprintf(stdout, "\033[%02d;%02dH", row + 1, col + 1);
}

void vt100_set_attr(int attr)
{
    switch(attr)
    {
      case VT100_ATTR_UNDERLINE:
	fputs("\033[4m", stdout);
	break;
      case VT100_ATTR_REVERSE:
	fputs("\033[7m", stdout);
	break;
      case VT100_ATTR_BOLD:
	fputs("\033[1m", stdout);
	break;
    }
}

void vt100_reset_attr(void)
{
    fputs("\033[m", stdout);
}
#else
void vt100_init_screen(void) { }
void vt100_refresh(void) { fflush(stdout); }
void vt100_clrtoeol(void) { }
void vt100_move(int row, int col) { }
void vt100_set_attr(int attr) { }
void vt100_reset_attr(void) { }
#endif
