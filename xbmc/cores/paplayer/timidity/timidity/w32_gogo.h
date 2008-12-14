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

#ifdef __W32_GOGO_H__
#define __W32_GOGO_H__

extern MERET MPGE_initializeWork(void);
extern MERET MPGE_terminateWork(void);
extern MERET MPGE_setConfigure(MPARAM mode, UPARAM dwPara1, UPARAM dwPara2 );
extern MERET MPGE_getConfigure(MPARAM mode, void *para1 );
extern MERET MPGE_detectConfigure(void);
extern MERET MPGE_processFrame(void);
extern MERET MPGE_closeCoder(void);
extern MERET MPGE_endCoder(void);
extern MERET MPGE_getVersion( unsigned long *vercode,  char *verstring );
extern MERET MPGE_getUnitStates( unsigned long *unit);
extern int MPGE_available;

extern int gogo_dll_check(void);

#endif /* __W32_GOGO_H__ */
