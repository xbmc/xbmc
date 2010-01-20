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

#ifndef ___W32G_SUBWIN_H_
#define ___W32G_SUBWIN_H_

// Console Window
void InitConsoleWnd(HWND hParentWnd);
void PutsConsoleWnd(char *str);
void PrintfConsoleWnd(char *fmt, ...);
void ClearConsoleWnd(void);

// Tracer Window
void InitTracerWnd(HWND hParentWnd);

// List Window
void InitListWnd(HWND hParentWnd);

// Doc Window
extern int DocWndIndependent;
extern int DocWndAutoPopup;
void InitDocWnd(HWND hParentWnd);
void DocWndInfoReset(void);
void DocWndAddDocFile(char *filename);
void DocWndSetMidifile(char *filename);
void DocWndReadDoc(int num);
void DocWndReadDocNext(void);
void DocWndReadDocPrev(void);

void PutsDocWnd(char *str);
void PrintfDocWnd(char *fmt, ...);
void ClearDocWnd(void);

// Wrd Window
void InitWrdWnd(HWND hParentWnd);

// SoundSpec Window
void InitSoundSpecWnd(HWND hParentWnd);

void w32g_setup_doc(int idx);
void w32g_open_doc(int close_if_no_doc);

#endif /* ___W32G_SUBWIN_H_ */
