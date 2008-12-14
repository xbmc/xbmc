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

    tskin_loadBMP.c

    Sep.23.1998  Daisuke Nagano
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "controls.h"
#include "output.h"
#include "arc.h"


typedef struct {
  int size;
  int offbits;

  int hsize;
  int w;
  int h;
  int bitcounts;
  int sizeimage;
  int ncolors;
  int compress;

} BMPHeader;

static int ugetc( struct timidity_file * );
static BMPHeader *loadBMPHeader( struct timidity_file * );
static Bool loadBMPColors( Display *, BMPHeader *, struct timidity_file * );
static int GetColor( Display *, int, int, int );
static int Get2bytes( struct timidity_file * );
static int Get4bytes( struct timidity_file * );
static int Draw4bit( Display *, Pixmap, GC, BMPHeader *, struct timidity_file * );
static int Draw8bit( Display *, Pixmap, GC, BMPHeader *, struct timidity_file * );
static int Draw24bit( Display *, Pixmap, GC, BMPHeader *, struct timidity_file * );
static int DrawCompressed4bit( Display *, Pixmap, GC, BMPHeader *, struct timidity_file * );
static int DrawCompressed8bit( Display *, Pixmap, GC, BMPHeader *, struct timidity_file * );
static int highbit( unsigned long ul );

extern unsigned int xskin_depth;
extern Visual *xskin_vis;

static Colormap cmap;
static int color_palletes[256];
static int sc;
long rshift,gshift,bshift;
static int cols[8][8][8];
static int iscolorinited=0;

int xskin_getcolor( Display *d, int r, int g, int b ) {

  int r0,g0,b0;

  sc   = DefaultScreen( d );
  cmap = DefaultColormap( d, sc );

  rshift = 15-highbit(xskin_vis->red_mask);
  gshift = 15-highbit(xskin_vis->green_mask);
  bshift = 15-highbit(xskin_vis->blue_mask);

  if ( iscolorinited==0 ) {
    iscolorinited=1;
    for ( r0=0 ; r0<8 ; r0++ ) {
      for ( g0=0 ; g0<8 ; g0++ ) {
	for ( b0=0 ; b0<8 ; b0++ ) {
	  cols[r0][g0][b0]=-1;
	}
      }
    }
  }

  return GetColor( d, r, g, b );
}

Pixmap xskin_loadBMP( Display *d, Window w, char *filename,
		int *width, int *height ) {

  Pixmap ret=0;
  BMPHeader *bmp;
  struct timidity_file *fp;

  GC gc;

  if ( width!=NULL )
    *width=-1;
  if ( height!=NULL )
    *height=-1;

  sc   = DefaultScreen( d );
  gc   = DefaultGC( d, sc );
  cmap = DefaultColormap( d, sc );

  rshift = 15-highbit(xskin_vis->red_mask);
  gshift = 15-highbit(xskin_vis->green_mask);
  bshift = 15-highbit(xskin_vis->blue_mask);

  fp = open_file( filename, 1, OF_SILENT );
  if ( fp == NULL ) return ret;
  if ( fp->url->url_tell == NULL ) {
    fp->url = url_buff_open( fp->url, 1 );
  }

  bmp = loadBMPHeader( fp );
  if ( bmp==NULL ) goto finish1;

  if ( !loadBMPColors( d, bmp, fp ) ) goto finish1;

  ret = XCreatePixmap( d, w, bmp->w, bmp->h, xskin_depth );
  XSetForeground( d, gc, 0 );
  XFillRectangle( d, ret, gc, 0, 0, bmp->w, bmp->h );
  XSetForeground( d, gc, WhitePixel( d, sc ));

  switch( bmp->bitcounts ) {
  case 4:
    if ( bmp->compress == 0 )
      Draw4bit( d, ret, gc, bmp, fp );
    else if ( bmp->compress == 2 )
      DrawCompressed4bit( d, ret, gc, bmp, fp );
    break;
  case 8:
    if ( bmp->compress == 0 )
      Draw8bit( d, ret, gc, bmp, fp );
    else if ( bmp->compress == 1 )
      DrawCompressed8bit( d, ret, gc, bmp, fp );
    break;
  case 24:
    Draw24bit( d, ret, gc, bmp, fp );
    break;
  default:
    break;
  }

  if ( width!=NULL )
    *width = bmp->w;
  if ( height!=NULL )
    *height = bmp->h;

finish1:
  close_file( fp );
  return ret;
}

BMPHeader *loadBMPHeader( struct timidity_file *fp ) {

  static BMPHeader h;
  int i;

  /* file header */

  if ( (ugetc(fp) != 'B') || (ugetc(fp) != 'M') ) return NULL;
  h.size = Get4bytes( fp );    /* size */
  if ( h.size < 0 ) return NULL;
  i = Get2bytes( fp );         /* reserved 1 */
  i = Get2bytes( fp );         /* reserved 2 */
  h.offbits = Get4bytes( fp ); /* offbits */
  if ( h.offbits < 0 ) return NULL;


  /* information header */

  h.hsize = Get4bytes( fp );   /* header size */
  if ( h.hsize < 0 ) return NULL;

  if ( h.hsize == 40 ) {
    h.w = Get4bytes( fp );     /* width */
    h.h = Get4bytes( fp );     /* height */
  } else {
    h.w = Get2bytes( fp );     /* width */
    h.h = Get2bytes( fp );     /* height */
  }
  if ( h.h < 0 ) return NULL;

  i = Get2bytes( fp );         /* planes */
  if ( i != 1 ) return NULL;

  h.bitcounts = Get2bytes(fp); /* bit-counts */
  if ( h.bitcounts != 4 && h.bitcounts != 8 && h.bitcounts != 24 ) 
    return NULL;

  if ( h.hsize==40 || h.hsize==64 ) {
    i = Get4bytes( fp );       /* compress */
    h.compress = i;
    h.ncolors = ( h.offbits - h.hsize - 14 ) / 4;
                               /* colors */
  } else {
    h.ncolors = 0;
    h.compress=0;
  }

  return &h;
}

Bool loadBMPColors( Display *d, BMPHeader *h, struct timidity_file *fp ) {

  int i;
  int r,g,b;

  if ( iscolorinited==0 ) {
    iscolorinited=1;
    for ( r=0 ; r<8 ; r++ ) {
      for ( g=0 ; g<8 ; g++ ) {
	for ( b=0 ; b<8 ; b++ ) {
	  cols[r][g][b]=-1;
	}
      }
    }
  }
  
  tf_seek( fp, h->hsize+14, SEEK_SET );
  if ( h->ncolors == 0 ) return True;

  if ( h->hsize==40 || h->hsize==64 ) {
    for ( i=0 ; i<h->ncolors ; i++ ) {
      b = ugetc(fp)*256;
      g = ugetc(fp)*256;
      r = ugetc(fp)*256;
      if ( ugetc(fp)==EOF ) return False;
	  
      color_palletes[i] = GetColor( d, r, g, b );
    }
  } else {
    for ( i=0 ; i<h->ncolors ; i++ ) {
      b = ugetc(fp)*256;
      g = ugetc(fp)*256;
      r = ugetc(fp)*256;
      if ( r==EOF ) return False;

      color_palletes[i] = GetColor( d, r, g, b );
    }
  }

  return True;
}

int GetColor( Display *d, int r, int g, int b ) {

  int ret;
  XColor C;
  int r2,g2,b2;

  switch ( xskin_vis->class ) {
  case TrueColor:
  case StaticColor:
  case StaticGray:
    if (rshift<0) r = r << (-rshift);
    else r = r >> rshift;
  
    if (gshift<0) g = g << (-gshift);
    else g = g >> gshift;
    
    if (bshift<0) b = b << (-bshift);
    else b = b >> bshift;
    
    r = r & xskin_vis->red_mask;
    g = g & xskin_vis->green_mask;
    b = b & xskin_vis->blue_mask;

    ret = r | g | b;
    break;

  default:
    r2=(r>>13)&7;
    g2=(g>>13)&7;
    b2=(b>>13)&7;
    if ( cols[r2][g2][b2] == -1 ) {
      C.red   = r;
      C.green = g;
      C.blue  = b;
      if ( XAllocColor( d, cmap, &C )==0 ) C.pixel=0;
      cols[r2][g2][b2] = C.pixel;
    }
    ret = cols[r2][g2][b2];
    break;
  }

  return ret;
}


int Get4bytes( struct timidity_file *fp ) {
  int ret;
  int i;

  if ( (i=ugetc(fp))==EOF ) return -1;
  ret  = i;
  if ( (i=ugetc(fp))==EOF ) return -1;
  ret += i*256;
  if ( (i=ugetc(fp))==EOF ) return -1;
  ret += i*256*256;
  if ( (i=ugetc(fp))==EOF ) return -1;
  ret += i*256*256*256;
  
  return ret;
}

int Get2bytes( struct timidity_file *fp ) {
  int ret;
  int i;

  if ( (i=ugetc(fp))==EOF ) return -1;
  ret  = i;
  if ( (i=ugetc(fp))==EOF ) return -1;
  ret += i*256;

  return ret;
}

int highbit( unsigned long ul ) {

  /*
  int i;
  int b;
  for ( i=31 ; i>=0 ; i-- ) {
    b=1<<i;
    if ( ul&i != 0 ) break;
  }
  return i;
  */
  int i;  unsigned long hb;
  hb = 0x8000;  hb = (hb<<16);  /* hb = 0x80000000UL */
  for (i=31; ((ul & hb) == 0) && i>=0;  i--, ul<<=1);
  return i;
}


int Draw4bit( Display *d, Pixmap p, GC gc, BMPHeader *bmp, struct timidity_file *fp ) {

  int x,y;
  int col,col1,col2;
  int pad;

  for ( y=bmp->h ; y>0 ; --y ) {
    pad = ((bmp->w+7)/8)*8;
    for ( x=0 ; x<pad ; x+=2 ) {
      col=ugetc(fp);
      col1=(col>>4)&0x0f;
      col2=col&0x0f;
      if ( col1 >= bmp->ncolors ) col1=0;
      if ( col2 >= bmp->ncolors ) col2=0;
      if ( x<bmp->w-1 ) {
	XSetForeground( d, gc, color_palletes[col1] );
	XDrawPoint( d, p, gc, x, y-1 );
	XSetForeground( d, gc, color_palletes[col2] );
	XDrawPoint( d, p, gc, x+1, y-1 );
      }
    }
  }

  return 0;
}

int Draw8bit( Display *d, Pixmap p, GC gc, BMPHeader *bmp, struct timidity_file *fp ) {

  int x,y;
  int col;
  int pad;

  for ( y=bmp->h ; y>0 ; --y ) {
    pad = ((bmp->w+3)/4)*4;
    for ( x=0 ; x<pad ; x++ ) {
      col = ugetc(fp);
      if ( col == EOF ) { y=0; break; }
      if ( col >= bmp->ncolors ) col=0;
      
      if ( x<bmp->w ) {
	XSetForeground( d, gc, color_palletes[col] );
	XDrawPoint( d, p, gc, x, y-1 ); 
      }
    }
  }

  return 0;
}

int Draw24bit( Display *d, Pixmap p, GC gc, BMPHeader *bmp, struct timidity_file *fp ) {

  int x,y;
  int r,g,b;
  int pad;
  pad = (4-((bmp->w*3)%4))&0x03;

  for ( y=bmp->h ; y>0 ; --y ) {
    for ( x=0 ; x<bmp->w ; x++ ) {
      b = ugetc(fp)*256;
      g = ugetc(fp)*256;
      r = ugetc(fp)*256;
      if ( r == EOF ) { y=0; break; }
      
      XSetForeground( d, gc, GetColor( d, r, g, b ) );
      XDrawPoint( d, p, gc, x, y-1 ); 
    }
    for ( x=0 ; x<pad ; x++ ) {
      ugetc(fp);
    }
  }
  return 0;
}

int DrawCompressed4bit( Display *d, Pixmap p, GC gc,
			BMPHeader *bmp, struct timidity_file *fp ) {
  int i,j;
  int a,b;
  int x,y;
  int z=1;

  x=0;
  y=bmp->h;

  while (z) {

    a=ugetc(fp);
    b=ugetc(fp);
    if ( b==EOF ) break;

    if ( a!=0 ) {
      if ( b>=bmp->ncolors ) b=0;
      for ( i=0 ; i<a ; i+=2 ) {
	if ( x<bmp->w ) {
	  XSetForeground( d, gc, color_palletes[(b>>4)&0x0f] );
	  XDrawPoint( d, p, gc, x, y-1 );
	  x++;
	  if ( i!=a-1 ) {
	    XSetForeground( d, gc, color_palletes[b&0x0f] );
	    XDrawPoint( d, p, gc, x, y-1 );
	    x++;
	  }
	}
      }

    } else {

      switch( b ) {
      case 0:
	x=0;
	y--;
	break;

      case 1:
	z=0;
	break;

      case 2:
	x+=ugetc(fp);
	i=ugetc(fp);
	if ( i==EOF ) z=0;
	else y-=i;
	break;

      default:
	for ( i=0 ; i<b/2 ; i++ ) {
	  a=ugetc(fp);
	  if ( a==EOF ) { z=0; break; }

	  j=(a>>4)&0x0f;
	  if ( j>=bmp->ncolors ) j=0;
	  if (x<bmp->w) {
	    XSetForeground( d, gc, color_palletes[j] );
	    XDrawPoint( d, p, gc, x, y-1 ); 
	    x++;
	  }
	  j=a&0x0f;
	  if ( j>=bmp->ncolors ) j=0;
	  if (x<bmp->w) {
	    XSetForeground( d, gc, color_palletes[j] );
	    XDrawPoint( d, p, gc, x, y-1 ); 
	    x++;
	  }

	}
	if (b%2==1) ugetc(fp);
      }
      break;
    } 

  }

  return 0;
}

int DrawCompressed8bit( Display *d, Pixmap p, GC gc,
			BMPHeader *bmp, struct timidity_file *fp ) {
  int i;
  int a,b;
  int x,y;
  int z=1;

  x=0;
  y=bmp->h;

  while (z) {

    a=ugetc(fp);
    b=ugetc(fp);
    if ( b==EOF ) break;

    if ( a!=0 ) {
      if ( b>=bmp->ncolors ) b=0;
      for ( i=0 ; i<a ; i++ ) {
	if ( x<bmp->w ) {
	  XSetForeground( d, gc, color_palletes[b] );
	  XDrawPoint( d, p, gc, x, y-1 ); 
	  x++;
	}
      }

    } else {

      switch( b ) {
      case 0:
	x=0;
	y--;
	break;

      case 1:
	z=0;
	break;

      case 2:
	x+=ugetc(fp);
	i=ugetc(fp);
	if ( i==EOF ) z=0;
	else y-=i;
	break;

      default:
	for ( i=0 ; i<b ; i++ ) {
	  a=ugetc(fp);
	  if ( a==EOF ) { z=0; break; }
	  if ( a>=bmp->ncolors ) a=0;

	  if (x<bmp->w) {
	    XSetForeground( d, gc, color_palletes[a] );
	    XDrawPoint( d, p, gc, x, y-1 ); 
	    x++;
	  }
	}
	if (b%2==1) ugetc(fp);
	break;
      }
    } 

  }

  return 0;
}

static int ugetc( struct timidity_file *fp ) {

  static unsigned char a[2];
  int ret;

  if ( tf_read( a, 1, 1, fp ) != 1 ) ret=EOF;
  else ret = (int)a[0];

  return ret;
}
