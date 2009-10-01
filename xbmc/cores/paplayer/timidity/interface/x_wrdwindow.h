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

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "x_mag.h"
extern void WinEvent(void);
extern void EndWin(void);
extern void WinFlush(void);
extern void AddLine(const unsigned char *,int);
extern int OpenWRDWindow(char *opt);
extern void CloseWRDWindow(void);
extern void x_RedrawControl(int flag);
extern void x_Gcls(int);
extern void x_Gline(int *,int);
extern void x_GCircle(int *,int);
extern void x_Pal(int *,int);
extern void x_Palrev(int pallet);
extern void x_Gscreen(int active,int appear);
extern void x_Fade(int *,int,int,int);
extern void x_Mag(magdata *,int32,int32,int32,int32);
extern void x_PLoad(char *filename);
extern void x_GMove(int,int,int,int,int,int,int,int,int);
extern void x_GMode(int mode);
extern void x_Ton(int param);
extern void x_Gon(int param);
extern void x_Startup(int version);
extern void x_VSget(int *,int);
extern void x_VRel(void);
extern void x_VCopy(int,int,int,int,int,int,int,int,int);
extern void x_XCopy(int sx1, int sy1, int sx2, int sy2, int tx, int ty,
		    int ss, int ts, int method, int *opts, int npots);
#endif
