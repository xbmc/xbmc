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


    xskin_spectrum.c

    Oct.08.1998  Daisuke Nagano
*/

#include "xskin.h"

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"

extern void xskin_pipe_write(char *);
extern void xskin_pipe_read_direct(int32 *, int);
extern int xskin_getcolor( Display *, int, int, int );

extern Display *xskin_d;
extern Window xskin_w;
extern GC xskin_gc;
extern Pixmap xskin_back;
extern unsigned int xskin_depth;
extern Visual *xskin_vis;

#ifdef SUPPORT_SOUNDSPEC
static void xskin_spe_ana( unsigned char * );
static void xskin_wave( unsigned char * );
#endif /* SUPPORT_SOUNDSPEC */

static int foreground;
static int background;
static int spe_pixel[16];
static int wave_pixel[5];

static XImage *spe_line;
static char *spe_background;

int readrgb( Display *d, struct timidity_file *fp ) {
  char tmp[1024];
  int r,g,b;

  if ( tf_gets( tmp, sizeof(tmp), fp ) == NULL) return -1;
  sscanf( tmp, "%d,%d,%d", &r, &g, &b );
  return xskin_getcolor( d, r*256, g*256, b*256 );
}

int xskin_loadviscolor( Display *d, Window w, char *filename ) {

  int i,j;
  int x,y;
  int sc;
  struct timidity_file *fp;

  static int r0[16] = { /* Default spectrum color (red) */
    0xcf3c, 0xb6da, 0xb6da, 0xc71b, 0xc71b, 0xefbe, 0xc71b, 0xbaea,
    0xb6da, 0x28a2, 0x28a2, 0x28a2, 0x2081, 0x2081, 0x2081, 0x2081,
  };
  static int g0[16] = { /* Default spectrum color (green) */
    0x0000, 0x69a6, 0x69a6, 0x79e7, 0x79e7, 0xa699, 0x9e79, 0xb6da,
    0xb6da, 0xbaea, 0xbaea, 0xbaea, 0xa289, 0xa289, 0xa289, 0xa289
  };
  static int b0[16] = { /* Default spectrum color (blue) */
    0x0000, 0x0000, 0x0000, 0x0820, 0x0820, 0x0000, 0x30c2, 0x4103,
    0x1040, 0x28a2, 0x28a2, 0x28a2, 0x2081, 0x2081, 0x2081, 0x2081
  };

  sc = DefaultScreen(d);
  if ( filename==NULL ) { /* Initialize */
    spe_line = XCreateImage( d, xskin_vis,
			     xskin_depth,ZPixmap, 0, NULL,
			     SPE_W, SPE_H, 8, 0 );
    spe_line->data = (char *)safe_malloc( spe_line->bytes_per_line*
					  spe_line->height );
    spe_background = (char *)safe_malloc( spe_line->bytes_per_line*
					  spe_line->height );

    foreground = xskin_getcolor( d, 0x4103, 0x4924, 0x4924 );
    background = xskin_getcolor( d, 0, 0, 0 );
    if ( background == 0 ) background = BlackPixel( d, sc );
    if ( foreground == 0 ) foreground = BlackPixel( d, sc );
    for ( i=0 ; i<16 ; i++ ) {
      spe_pixel[i] = xskin_getcolor( d, r0[i], g0[i], b0[i] );
      if ( spe_pixel[i] == 0 )
	spe_pixel[i] = spe_pixel[i-1];
    }
    for ( i=0 ; i<5 ; i++ ) {
      wave_pixel[i] = WhitePixel( d, sc );
    }

  } else {

    fp = open_file( filename, 1, OF_SILENT );
    if ( fp == NULL ) return 0;
    
    if ( (i = readrgb(d,fp))<0 ) goto end; /* spe_ana : background */
    background = i;
    if ( (i = readrgb(d,fp))<0 ) goto end; /* spe_ana : background */
    foreground = i;
    
    for ( i=0 ; i<16 ; i++ ) {
      j = readrgb(d,fp);
      if ( j<0 ) goto end;
      spe_pixel[i] = j;
    }
    for ( i=0 ; i<5 ; i++ ) {
      j = readrgb(d,fp);
      if ( j<0 ) goto end;
      wave_pixel[i] = j;
    }
    
  end:
    close_file( fp );
  }
  
  for ( y=0 ; y<SPE_H ; y++ ) {
    for ( x=0 ; x<SPE_W ; x++ ) {
      if ( (x%2)==0 && (y%2)==0 ) i=foreground;
      else i=background;
      XPutPixel( spe_line, x, y, i );
    }
  }
  memcpy( spe_background, spe_line->data,
	  spe_line->bytes_per_line * spe_line->height );

  return 1;
}

void ts_spectrum( int mode, unsigned char *buf ) {

  static int last_mode;

  switch( mode ) {

  case -1:  /* initialize */
    if ( last_mode != -1 ) {
      XCopyArea( xskin_d, xskin_back, xskin_w, xskin_gc,
		 SPE_SX, SPE_SY, SPE_W, SPE_H, SPE_SX, SPE_SY );
    }
    break;

  case 0:  /* blank */
    if ( last_mode != 0 ) {
      XCopyArea( xskin_d, xskin_back, xskin_w, xskin_gc,
		 SPE_SX, SPE_SY, SPE_W, SPE_H, SPE_SX, SPE_SY );
    }
    break;

#ifdef SUPPORT_SOUNDSPEC
  case 1:  /* spectrum analizer */
    xskin_spe_ana(buf);
    break;

  case 2:  /* wave form */
    xskin_wave(buf);
    break;
#endif /* SUPPORT_SOUNDSPEC */

  default:
    break;
  }

  last_mode=mode;
  return ;
}

#ifdef SUPPORT_SOUNDSPEC
static void xskin_spe_ana( unsigned char *buf ) {

  int x,y,i;
  int yt;

  memcpy( spe_line->data, spe_background,
	  spe_line->bytes_per_line * spe_line->height );

  if ( buf != NULL ) {
    for ( x=0 ; x<SPE_W ; x++ ) {
      yt = 16 * buf[x] / 256;
      
      for ( y=(SPE_H-yt),i=0 ; y<SPE_H ; y++,i++ ) {
	if ( i>15 ) i=15;
	XPutPixel( spe_line, x, y, spe_pixel[i] );
      }
    }
  }
  XPutImage( xskin_d, xskin_w, xskin_gc, spe_line,
	     0, 0, SPE_SX, SPE_SY, SPE_W, SPE_H );

  return;
}

static void xskin_wave( unsigned char *buf ) {

  int x,y,c;

  memcpy( spe_line->data, spe_background,
	  spe_line->bytes_per_line * spe_line->height );

  if ( buf != NULL ) {
    for ( x=0 ; x<SPE_W ; x++ ) {
      y = SPE_H - buf[x]*SPE_H/256-1;
      if ( y<4 ) c=4-y;
      else if ( y>=12 ) c=y-11;
      else c=0;
      XPutPixel( spe_line, x, y, wave_pixel[c] );
    }
  }
  XPutImage( xskin_d, xskin_w, xskin_gc, spe_line,
	     0, 0, SPE_SX, SPE_SY, SPE_W, SPE_H );

  return;
}
#endif /* SUPPORT_SOUNDSPEC */
