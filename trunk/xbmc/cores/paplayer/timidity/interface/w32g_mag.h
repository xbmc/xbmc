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

#ifndef __W32G_MAG_H__
#define __W32G_MAG_H__

typedef struct _magdata{
  char *filename;
  char machine;
  char mflag;
  char scrmode;
  int xorig;
  int yorig;
  int xend;
  int yend;
  long flagaoff;
  long flagboff;
  long flagbsize;
  long pxloff;
  long pxlsize;
  long flagapos;
  long flagbpos;
  uint8 *flagadata;
  uint8 *flagbdata;
  uint8 *pxldata;
  struct _magdata *next;
#define PALSIZE 16
  int pal[PALSIZE+1];
} magdata;

extern magdata	*mag_create ( char *file );
extern void		mag_deletetab ( void );
extern magdata	*mag_search ( char *file );
extern void		mag_load_pixel ( char *image, int width, int height, magdata *mh );
extern int pho_load_pixel ( char *image, int width, int height, char *filename );

#endif	// __W32G_MAG_H__
