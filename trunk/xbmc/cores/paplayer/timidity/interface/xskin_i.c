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

    xskin_i.c

    Oct.06.1998  Daisuke Nagano
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
extern int  xskin_pipe_read(char *,int);

/* text positions */

static int text_posx[] = {
/*     !   ""   #   $   %   &   '   (   )   *   +   ,   -   .   /  */
  30, 17, 26, 30, 29, 26, 25, 16, 13, 14,  4, 19, 10, 15, 10, 21,
/* 0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?  */ 
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 12, 12, 30, 28, 30,  3,
  27,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 22, 20, 23, 24, 18
};
static int text_posy[] = {
   0,  1,  0,  1,  1,  1,  1,  1,  1,  1,  2,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  0,  2,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1
};

static char local_buf[300];

static int load_skins( void );
static void xskin_jobs( int );
static void repaint( void );

static void install_sighandler( void );
static void signal_vector( int );
static void delete_shm( void );

static int fshuf,frep,fequ,fpll;
static int fplay,fpause;
static int fremain;
static int play_val, vol_val;
static char last_text[1024];
static int last_current_time;
static int total_time;

static int shmid;
static unsigned char *speana_buf;

Visual *xskin_vis;
unsigned int xskin_depth;

Display *xskin_d;
Window xskin_r,xskin_w;
GC xskin_gc;

Pixmap xskin_back,xskin_titlebar,xskin_playpaus,xskin_numbers,xskin_cbuttons;
Pixmap xskin_monoster,xskin_posbar,xskin_shufrep,xskin_text,xskin_volume;

/* text */

void ts_puttext( int x0, int y0, char *message ) {
  int i,l;
  int c;
  int x,px,py;

  if ( x0 == MESSAGE_X ) {
    px=text_posx[0]*TEXT_W;
    py=text_posy[0]*TEXT_H;
    for ( i=0 ; i<31 ; i++ ) {
      x = x0+i*TEXT_W;
      XCopyArea( xskin_d, xskin_text, xskin_w, xskin_gc,
		 px, py, TEXT_W, TEXT_H, x, y0 );
    }
  } else if ( x0 == BITRATE_X ) {
    XCopyArea( xskin_d, xskin_back, xskin_w, xskin_gc,
	       111, 43, 15, 6, 111, 43 );
  } else if ( x0 == SAMPLE_X ) {
    XCopyArea( xskin_d, xskin_back, xskin_w, xskin_gc,
	       156, 43, 10, 6, 156, 43 );
  }

  l = strlen( message );
  if ( l<=0 ) return;
  for ( i=0 ; i<l ; i++ ) {
    c = (int)(message[i]);
    if ( (c>='a') && (c<='z') ) c = c-'a'+'A';
    if ( c<' ' ) c = '.';
    if ( c>'_' ) c = '.';

    c-=' ';
    if ( c>=64 ) c=0;
    px = text_posx[c]*TEXT_W;
    py = text_posy[c]*TEXT_H;
    x = x0+i*TEXT_W;
    if (( x0 == MESSAGE_X && i<31 ) ||
	( x0 == BITRATE_X && i<3  ) ||
	( x0 == SAMPLE_X  && i<2  )) {
      XCopyArea( xskin_d, xskin_text, xskin_w, xskin_gc,
		 px, py, TEXT_W, TEXT_H, x, y0 );
    }
  }
  XSync( xskin_d, True ); /* discards any events in the queue */

  if ( x0 == MESSAGE_X )
    strncpy( last_text, message, sizeof(last_text) );

  return;
}

/* numbers */

void ts_putnum( int x, int y, int val ) {

  int x0,y0;

  if ( (val>9) || (val<0 ) ) return;
  x0=val*9;
  y0=0;

  XCopyArea( xskin_d, xskin_numbers, xskin_w, xskin_gc,
	     x0, y0, NUM_W, NUM_H, x, y );

  return ;
}

/* cbuttons */

void ts_prev( int i ) {

  XCopyArea( xskin_d, xskin_cbuttons, xskin_w, xskin_gc,
	     PREV_SX(i), PREV_SY(i), PREV_W, PREV_H, PREV_DX, PREV_DY );

  return;
}

void ts_play( int i ) {

  XCopyArea( xskin_d, xskin_cbuttons, xskin_w, xskin_gc,
	     PLAY_SX(i), PLAY_SY(i), PLAY_W, PLAY_H, PLAY_DX, PLAY_DY );

  return;
}

void ts_pause( int i ) {

  XCopyArea( xskin_d, xskin_cbuttons, xskin_w, xskin_gc,
	     PAUSE_SX(i), PAUSE_SY(i), PAUSE_W, PAUSE_H, PAUSE_DX, PAUSE_DY);

  return;
}

void ts_stop ( int i ) {

  XCopyArea( xskin_d, xskin_cbuttons, xskin_w, xskin_gc,
	     STOP_SX(i), STOP_SY(i), STOP_W, STOP_H, STOP_DX, STOP_DY );

  return;
}

void ts_next( int i ) {

  XCopyArea( xskin_d, xskin_cbuttons, xskin_w, xskin_gc,
	     NEXT_SX(i), NEXT_SY(i), NEXT_W, NEXT_H, NEXT_DX, NEXT_DY );

  return;
}

void ts_eject( int i ) {

  XCopyArea( xskin_d, xskin_cbuttons, xskin_w, xskin_gc,
	     EJECT_SX(i), EJECT_SY(i), EJECT_W, EJECT_H, EJECT_DX, EJECT_DY );
  return;
}

/* titlebar */

void ts_titlebar( int i ) {

  XCopyArea( xskin_d, xskin_titlebar, xskin_w, xskin_gc,
	     TITLEBAR_SX(i), TITLEBAR_SY(i),
	     TITLEBAR_W, TITLEBAR_H, TITLEBAR_DX, TITLEBAR_DY );
  return;
}

void ts_exitbutton( int i ) {
  
  XCopyArea( xskin_d, xskin_titlebar, xskin_w, xskin_gc,
	     EXITBUTTON_SX(i), EXITBUTTON_SY(i),
	     EXITBUTTON_W, EXITBUTTON_H, EXITBUTTON_DX, EXITBUTTON_DY );

  return;
}

void ts_menubutton( int i ) {

  XCopyArea( xskin_d, xskin_titlebar, xskin_w, xskin_gc,
	     MENUBUTTON_SX(i), MENUBUTTON_SY(i),
	     MENUBUTTON_W, MENUBUTTON_H, MENUBUTTON_DX, MENUBUTTON_DY );
  return;
}

void ts_iconbutton( int i ) {

  XCopyArea( xskin_d, xskin_titlebar, xskin_w, xskin_gc,
	     ICONBUTTON_SX(i), ICONBUTTON_SY(i),
	     ICONBUTTON_W, ICONBUTTON_H, ICONBUTTON_DX, ICONBUTTON_DY );
  return;
}

void ts_minibutton( int i ) {

  XCopyArea( xskin_d, xskin_titlebar, xskin_w, xskin_gc,
	     MINIBUTTON_SX(i), MINIBUTTON_SY(i),
	     MINIBUTTON_W, MINIBUTTON_H, MINIBUTTON_DX, MINIBUTTON_DY );
  return;
}

/* monoster */

void ts_mono( int i ) {

  XCopyArea( xskin_d, xskin_monoster, xskin_w, xskin_gc,
	     MONO_SX(i), MONO_SY(i),
	     MONO_W, MONO_H, MONO_DX, MONO_DY );
  return;
}

void ts_stereo( int i ) {

  XCopyArea( xskin_d, xskin_monoster, xskin_w, xskin_gc,
	     STEREO_SX(i), STEREO_SY(i),
	     STEREO_W, STEREO_H, STEREO_DX, STEREO_DY );
  return;
}

/* playpaus */

void ts_pstate( int i ) {

  XCopyArea( xskin_d, xskin_playpaus, xskin_w, xskin_gc,
	     PSTATE1_SX(i), PSTATE1_SY(i),
	     PSTATE1_W, PSTATE1_H, PSTATE1_DX, PSTATE1_DY );
  XCopyArea( xskin_d, xskin_playpaus, xskin_w, xskin_gc,
	     PSTATE2_SX(i), PSTATE2_SY(i),
	     PSTATE2_W, PSTATE2_H, PSTATE2_DX, PSTATE2_DY );
  return;
}

/* shufrep */

void ts_shuf( int i ) {

  XCopyArea( xskin_d, xskin_shufrep, xskin_w, xskin_gc,
	     SHUF_SX(i), SHUF_SY(i),
	     SHUF_W, SHUF_H, SHUF_DX, SHUF_DY );

  return;
}

void ts_rep( int i ) {

  XCopyArea( xskin_d, xskin_shufrep, xskin_w, xskin_gc,
	     REP_SX(i), REP_SY(i),
	     REP_W, REP_H, REP_DX, REP_DY );

  return;
}

void ts_equ( int i ) {

  XCopyArea( xskin_d, xskin_shufrep, xskin_w, xskin_gc,
	     EQU_SX(i), EQU_SY(i),
	     EQU_W, EQU_H, EQU_DX, EQU_DY );
  return;
}

void ts_plist( int i ) {

  XCopyArea( xskin_d, xskin_shufrep, xskin_w, xskin_gc,
	     PLIST_SX(i), PLIST_SY(i),
	     PLIST_W, PLIST_H, PLIST_DX, PLIST_DY );
  return;
}

/* posbar */

int ts_pos( int i, int j ) {

  int x,y;
  int p;

  if ( j<0 ) p=-j;
  else {
    if (j<POS_MIN_DX) j=POS_MIN_DX;
    if (j>POS_MAX_DX) j=POS_MAX_DX;
    p = 100*(j-POS_MIN_DX)/(POS_MAX_DX-POS_MIN_DX);
  }

  x = POS_MIN_DX + (POS_MAX_DX-POS_MIN_DX)*p/100;
  y = POS_DY;

  XCopyArea( xskin_d, xskin_posbar, xskin_w, xskin_gc,
	     BAR_SX, BAR_SY, BAR_W, BAR_H, BAR_DX, BAR_DY );
  XCopyArea( xskin_d, xskin_posbar, xskin_w, xskin_gc,
	     POS_SX(i), POS_SY(i), POS_W, POS_H, x, y );
  return p;
}

int ts_volume( int i, int j ) {

  int x,y;
  int t,p;

  if ( j<0 ) p=-j;
  else {
    if (j<VOL_MIN_DX) j=VOL_MIN_DX;
    if (j>VOL_MAX_DX) j=VOL_MAX_DX;
    p = 100*(j-VOL_MIN_DX)/(VOL_MAX_DX-VOL_MIN_DX);
  }

  x = VOL_MIN_DX + (VOL_MAX_DX-VOL_MIN_DX)*p/100;
  y = VOL_DY;
  t=27*(int)p/100;
  XCopyArea( xskin_d, xskin_volume, xskin_w, xskin_gc,
	     VOLUME_SX, t*VOLUME_H,
	     VOLUME_W, VOLUME_H-2, VOLUME_DX, VOLUME_DY );
  XCopyArea( xskin_d, xskin_volume, xskin_w, xskin_gc,
	     VOL_SX(i), VOL_SY(i), VOL_W, VOL_H, x, y );
  return p;
}

int ts_pan( int i, int j ) {

  int x,y;
  int t,p;

  if ( j<0 ) p=-j;
  else {
    if (j<PAN_MIN_DX) j=PAN_MIN_DX;
    if (j>PAN_MAX_DX) j=PAN_MAX_DX;
    p = 100*(j-PAN_MIN_DX)/(PAN_MAX_DX-PAN_MIN_DX);
  }

  x = PAN_MIN_DX + (PAN_MAX_DX-PAN_MIN_DX)*p/100;
  y = PAN_DY;
  t=27*((p>50)?((float)p-50)/50:(50-(float)p)/50);
  if ( t<2 ) t=0;
  XCopyArea( xskin_d, xskin_volume, xskin_w, xskin_gc,
	     PANPOT_SX, t*PANPOT_H,
	     PANPOT_W, PANPOT_H-2, PANPOT_DX, PANPOT_DY );
  XCopyArea( xskin_d, xskin_volume, xskin_w, xskin_gc,
	     PAN_SX(i), PAN_SY(i),
	     PAN_W, PAN_H, x, y );
  return p;
}

static void pauseOn()
{
    if(!fpause) {
	fpause = 1;
	xskin_pipe_write("U");
    }
}

static void pauseOff()
{
    if(fpause) {
	fpause = 0;
	xskin_pipe_write("U");
    }
}

/* main loop */

#define ISIN(x,y,x0,y0,w,h) ( (x>=x0)&&(x<x0+w)&&(y>=y0)&&(y<y0+h) )?1:0

void xskin_start_interface( int pipe_in ) {

  int xskin_sc;
  XEvent xskin_e;
  XSetWindowAttributes xskin_attr;

  XSizeHints xskin_hint;
  XClassHint xskin_chint;
  XTextProperty ct;
  char *namlist[2];


  /* setup window */

  xskin_d     = XOpenDisplay( NULL );
  xskin_sc    = DefaultScreen( xskin_d );
  xskin_r     = RootWindow( xskin_d, xskin_sc );
  xskin_gc    = DefaultGC( xskin_d, xskin_sc );
  xskin_vis   = DefaultVisual( xskin_d, xskin_sc );
  xskin_depth = DefaultDepth( xskin_d, xskin_sc );

  xskin_w = XCreateSimpleWindow( xskin_d, xskin_r, 0, 0,
				 skin_width, skin_height, 0,
				 WhitePixel( xskin_d, xskin_sc ),
				 BlackPixel( xskin_d, xskin_sc ) );

  xskin_attr.backing_store = True;
  xskin_attr.override_redirect = False;
  XChangeWindowAttributes( xskin_d, xskin_w,
			   CWBackingStore|CWOverrideRedirect, &xskin_attr );

  XSelectInput( xskin_d, xskin_w,
		KeyPressMask|ExposureMask|
		EnterWindowMask|LeaveWindowMask|
		ButtonPressMask|ButtonReleaseMask|
		Button1MotionMask );

  xskin_hint.flags = USSize | PMinSize | PMaxSize | USPosition;
  xskin_hint.width = xskin_hint.min_width = xskin_hint.max_width
    = skin_width;
  xskin_hint.height = xskin_hint.min_height = xskin_hint.max_height
    = skin_height;
  XSetNormalHints( xskin_d, xskin_w, &xskin_hint );

  xskin_chint.res_name  = XSKIN_RES_NAME;
  xskin_chint.res_class = XSKIN_RES_CLASS;
  XSetClassHint( xskin_d, xskin_w, &xskin_chint );

  namlist[0]=(char *)safe_malloc(strlen(XSKIN_WINDOW_NAME)+1);
  strcpy( namlist[0], XSKIN_WINDOW_NAME );
  XmbTextListToTextProperty( xskin_d, namlist, 1, XCompoundTextStyle, &ct );
  XSetWMName( xskin_d, xskin_w, &ct );
  XSetWMIconName( xskin_d, xskin_w, &ct );
  free(namlist[0]);


  /* setup pixmaps */

  if ( load_skins()!=0 ) goto finish;

  XSetWindowBackgroundPixmap( xskin_d, xskin_w, xskin_back );
  XClearWindow( xskin_d, xskin_w );

  XMapWindow( xskin_d, xskin_w );
  while( 1 ) {
    XNextEvent( xskin_d, &xskin_e );
    if ( xskin_e.type == Expose ) break; 
  }

  fshuf=0;
  frep=0;
  fequ=1;
  fpll=1;
  fplay=0;
  fpause=0;
  fremain=0;
  play_val=1;
  vol_val=50;
  last_current_time=0;
  total_time=0;
  speana_buf = NULL;
  strcpy( last_text, "welcome to timidity" );

  install_sighandler();

  repaint();
  ts_spectrum( -1, speana_buf );

  XFlush(xskin_d);

  xskin_jobs( pipe_in );   /* tskin main jobs */

finish:
  signal_vector(0);  /* finish */
}

#define ISIN(x,y,x0,y0,w,h) ( (x>=x0)&&(x<x0+w)&&(y>=y0)&&(y<y0+h) )?1:0

static void xskin_jobs( int pipe_in ) {
  XEvent e;
  int x,y;
  int window_x,window_y;
  int fspe=0;
  int pr=-1;
  int z;
  int p;
  int master_volume=0;
  char file_name[1024], tmp[1024];

  int last_puttext_time;
  int last_window_x=-1, last_window_y=-1;

  int max_files;
  int i;
  fd_set fds;
  static struct timeval tv;

  Window t_w;
  unsigned int t_width, t_height, t_border, t_depth;

  xskin_pipe_write( "READY" );

  shmid = shmget( IPC_PRIVATE, sizeof(char)*SPE_W, IPC_CREAT|0600 );
  if ( shmid<0 ) xskin_pipe_write( "ERROR" );
  else {
    sprintf( local_buf, "%d", shmid );
    xskin_pipe_write( local_buf );
    speana_buf = (unsigned char *)shmat( shmid, 0, 0 );
  }

  xskin_pipe_read( local_buf, sizeof(local_buf) );
  max_files = atoi( local_buf );
  for ( i=0 ; i<max_files ; i++ ) {
    xskin_pipe_read( local_buf, sizeof(local_buf) );
  }

  z=1;
  last_puttext_time=0;
  last_current_time=0;

  XGetGeometry( xskin_d, xskin_w, &t_w,
		&window_x, &window_y,
		&t_width, &t_height, &t_border, &t_depth );

  while( z ) {

    XFlush( xskin_d );

    FD_ZERO( &fds );
    FD_SET( pipe_in, &fds );
    tv.tv_sec=0;
    tv.tv_usec=20000L; /* 20 msec */
    i=select( pipe_in+1, &fds, NULL, NULL, &tv );

    if ( i!=0 ) {
      xskin_pipe_read( local_buf, sizeof(local_buf) );
      switch (local_buf[0]) {

      case 'A': /* total time */
	total_time=atoi( local_buf+2 );
	last_current_time=0;
	last_puttext_time=0;
	break;
	
      case 'T': /* current time */
	{
	  int min,sec;
	  sscanf( local_buf+2, "%02d:%02d", &min, &sec );
	  i=min*60+sec;
	  if ( fremain==1 ) {
	    sec =total_time-i;
	    min =sec/60;
	    sec-=min*60;
	  }
	  if ( i != last_current_time ) {
	    ts_putnum( MIN_H_X, MIN_H_Y, min/10 );
	    ts_putnum( MIN_L_X, MIN_L_Y, min%10 );
	    ts_putnum( SEC_H_X, SEC_H_Y, sec/10 );
	    ts_putnum( SEC_L_X, SEC_L_Y, sec%10 );
	    p=100*i/total_time;
	    play_val=ts_pos( OFF, -p );
	    last_current_time=i;

	    if ( last_current_time - last_puttext_time == 3 ) { /* 3 sec */
	      sprintf( tmp, "%s [%02d:%02d]",
		       file_name, total_time/60, total_time%60 );
	      ts_puttext( MESSAGE_X, MESSAGE_Y, tmp );
	    }
	  }
	}
      break;

      case 'L': /* lylics/message */
	ts_puttext( MESSAGE_X, MESSAGE_Y, local_buf+2 );
	last_puttext_time=last_current_time;
	break;

      case 'F': /* filename */
	strncpy( file_name, local_buf+2, 1023 );
	file_name[1023]=0;
	break;

      case 'O': /* off the play button */
	fplay=0;
	ts_play(OFF);
	ts_spectrum(fspe, NULL); /* erase spectrums */
	break;

      case 'V': /* master volume */
	master_volume=atoi( local_buf+2 );
	p=100*((master_volume<200)?master_volume:200)/200; /* max:200% */
	vol_val=ts_volume( OFF, -p );
	break;

      case 'Q': /* quit */
	z=1;
	break;

      case 'W': /* wave form */
	ts_spectrum(fspe, speana_buf);
	break;

      default:
	break;
      }
    }

    if ( XPending( xskin_d )==0 ) continue;
    XNextEvent( xskin_d, &e );

    switch ( e.type ) {
      /*
    case KeyPress:
      z=0;
      break;
      */
   
    case Expose:
      repaint();
      break;

    case EnterNotify:
      {
	Cursor cs;
	ts_titlebar(ON);
	cs = XCreateFontCursor( xskin_d, XC_top_left_arrow );
	XDefineCursor( xskin_d, xskin_w, cs );
      }
      break;

    case LeaveNotify:
      ts_titlebar(OFF);
      XUndefineCursor( xskin_d, xskin_w );
      break;

    case MotionNotify:
      while( XCheckMaskEvent( xskin_d, Button1MotionMask, &e ) ) {
	XNextEvent( xskin_d, &e );
      }
      x = e.xbutton.x;
      y = e.xbutton.y;
      switch( pr ) {
	
	/*
      case TS_POS:
	play_val=ts_pos( ON, x );break;
	*/
      case TS_VOLUME:
	vol_val=ts_volume( ON, x );
	i=master_volume;
	master_volume=200*vol_val/100;
	sprintf( local_buf, "V %d", master_volume-i );
	xskin_pipe_write( local_buf );

	sprintf( tmp, " volume: %d%%", vol_val );
	ts_puttext( MESSAGE_X, MESSAGE_Y, tmp );
	last_puttext_time=last_current_time;
	break;
	/*
      case TS_PAN:
	pan_val=ts_pan( ON, x );break;
	*/
	
      default:
	if ( x != last_window_x || y != last_window_y ) {
	  window_x += x-last_window_x;
	  window_y += y-last_window_y;
	  XMoveWindow( xskin_d, xskin_w,
		       window_x, window_y );
	}
	break;
      }
      break;

    case ButtonPress:
      x = e.xbutton.x;
      y = e.xbutton.y;
      last_window_x=x;
      last_window_y=y;

             if ( ISIN( x, y,EXITBUTTON_DX,EXITBUTTON_DY,
			EXITBUTTON_W,EXITBUTTON_H ) ) {
        ts_exitbutton(ON);pr=TS_EXITBUTTON;

      } else if ( ISIN( x, y, PREV_DX, PREV_DY, PREV_W, PREV_H ) ) {
	ts_prev(ON);pr=TS_PREV;
      } else if ( ISIN( x, y, PLAY_DX, PLAY_DY, PLAY_W, PLAY_H ) ) {
	ts_play(ON);pr=TS_PLAY;
      } else if ( ISIN( x, y, PAUSE_DX, PAUSE_DY, PAUSE_W, PAUSE_H ) ) {
	ts_pause(ON);pr=TS_PAUSE;
      } else if ( ISIN( x, y, STOP_DX, STOP_DY, STOP_W, STOP_H ) ) {
	ts_stop(ON);pr=TS_STOP;
      } else if ( ISIN( x, y, NEXT_DX, NEXT_DY, NEXT_W, NEXT_H ) ) {
	ts_next(ON);pr=TS_NEXT;
      } else if ( ISIN( x, y, EJECT_DX, EJECT_DY, EJECT_W, EJECT_H ) ) {
	ts_eject(ON);pr=TS_EJECT;

      } else if ( ISIN( x, y,164, 89, 47, 15 ) ) {  /* shuffle */
	if ( fshuf==0 ) {
	  ts_shuf(OFFON);pr=TS_SHUFON;
	} else { 
	  ts_shuf(ONOFF);pr=TS_SHUFOFF;
	}
      } else if ( ISIN( x, y,210, 89, 28, 15 ) ) {  /* repeat */
	if ( frep==0 ) {
	  ts_rep(OFFON);pr=TS_REPON;
	} else {
	  ts_rep(ONOFF);pr=TS_REPOFF;
	}
      } else if ( ISIN( x, y,219, 58, 23, 12 ) ) {  /* equalizer */
	if ( fequ==0 ) {
	  ts_equ(OFFON);pr=TS_EQUON;
	} else {
	  ts_equ(ONOFF);pr=TS_EQUOFF;
	}
      } else if ( ISIN( x, y,242, 58, 23, 12 ) ) {  /* playlist */
	if ( fpll==0 ) {
	  ts_plist(OFFON);pr=TS_PLISTON;
	} else {
	  ts_plist(ONOFF);pr=TS_PLISTOFF;
	}

      } else if ( ISIN( x, y, MENUBUTTON_DX, MENUBUTTON_DY,
			MENUBUTTON_W, MENUBUTTON_H ) ) {
	ts_menubutton(ON);pr=TS_MENUBUTTON;
      } else if ( ISIN( x, y, ICONBUTTON_DX, ICONBUTTON_DY,
			ICONBUTTON_W, ICONBUTTON_H ) ) {
	ts_iconbutton(ON);pr=TS_ICONBUTTON;
      } else if ( ISIN( x, y, MINIBUTTON_DX, MINIBUTTON_DY,
			MINIBUTTON_W, MINIBUTTON_H ) ) {
	ts_minibutton(ON);pr=TS_MINIBUTTON;

	/*
      }	else if ( ISIN( x, y,POS_MIN_DX+(POS_MAX_DX-POS_MIN_DX)*play_val/100,
			POS_DY, POS_W, POS_H ) ) {
	ts_pos( ON, -play_val );pr=TS_POS;
	*/
      } else if ( ISIN( x, y,VOL_MIN_DX+(VOL_MAX_DX-VOL_MIN_DX)*vol_val/100,
			VOL_DY, VOL_W, VOL_H ) ) {
	ts_volume( ON, -vol_val );pr=TS_VOLUME;
	sprintf( tmp, " volume: %d%%", vol_val );
	ts_puttext( MESSAGE_X, MESSAGE_Y, tmp );
	last_puttext_time=last_current_time;

	/*
      } else if ( ISIN( x, y,PAN_MIN_DX+(PAN_MAX_DX-PAN_MIN_DX)*pan_val/100,
			PAN_DY, PAN_W, PAN_H ) ) {
	ts_pan( ON, -pan_val );pr=TS_PAN;
	*/

      } else if ( ISIN( x, y, MIN_H_X, MIN_H_Y,
			SEC_L_X+NUM_W-MIN_H_X, NUM_H ) ) {
	int min,sec;
	fremain=(fremain==0)?1:0;
	sec=(fremain==0)?last_current_time:total_time-last_current_time;
	min =sec/60;
	sec-=min*60;
	ts_putnum( MIN_H_X, MIN_H_Y, min/10 );
	ts_putnum( MIN_L_X, MIN_L_Y, min%10 );
	ts_putnum( SEC_H_X, SEC_H_Y, sec/10 );
	ts_putnum( SEC_L_X, SEC_L_Y, sec%10 );

      } else if ( ISIN( x, y, SPE_SX, SPE_SY, SPE_W, SPE_H ) ) {
	pr=TS_SPECTRUM;
      } else {
	XRaiseWindow( xskin_d, xskin_w );
      }
	     break;  

    case ButtonRelease:

      last_window_x = -1;
      last_window_y = -1;

      switch( pr ) {

      case TS_EXITBUTTON:
	ts_exitbutton(OFF);
	xskin_pipe_write("Q");
	z=0;break;

      case TS_PREV:
	ts_prev(OFF);
	ts_spectrum( fspe, NULL );
	xskin_pipe_write("B");
	break;
      case TS_PLAY:
	xskin_pipe_write("P");
	fplay=1;
	pauseOff();
	ts_play(OFF);ts_pause(OFF);
	ts_pstate( PSTATE_PLAY );
	break;
      case TS_PAUSE:
	ts_pause(OFF);
	if ( fplay ==1 ) {
	  if ( fpause==0 ) {
	    ts_pstate( PSTATE_PAUSE );
	    ts_spectrum( fspe, NULL );
	    pauseOn();
	  } else {
	    ts_pstate( PSTATE_PLAY );
	    pauseOff();
	  }
	}
	break;
      case TS_STOP:
	pauseOff();
	fplay=0;
	ts_pause(OFF);ts_play(OFF);ts_stop(OFF);
	ts_pstate( PSTATE_STOP );
	ts_spectrum( fspe, NULL );
	xskin_pipe_write("S");
	break;
      case TS_NEXT:
	ts_next(OFF);
	ts_spectrum( fspe, NULL );
	xskin_pipe_write("N");
	break;
      case TS_EJECT:
	ts_eject(OFF);break;

      case TS_SHUFON:
	ts_shuf(ON);fshuf=1;
	fplay=1;
	pauseOff();
	ts_pstate( PSTATE_PLAY );
	xskin_pipe_write("D 1");
	break;
      case TS_SHUFOFF:
	ts_shuf(OFF);fshuf=0;
	fplay=0;
	pauseOff();
	ts_pstate( PSTATE_STOP );
	ts_spectrum( fspe, NULL );
	xskin_pipe_write("D 2");
	break;
      case TS_REPON:
	ts_rep(ON);frep=1;
	xskin_pipe_write("R 1");
	break;
      case TS_REPOFF:
	ts_rep(OFF);frep=0;
	xskin_pipe_write("R 0");
	break;

      case TS_EQUON:
	ts_equ(ON);fequ=1;break;
      case TS_EQUOFF:
	ts_equ(OFF);fequ=0;break;

      case TS_PLISTON:
	ts_plist(ON);fpll=1;break;
      case TS_PLISTOFF:
	ts_plist(OFF);fpll=0;break;

      case TS_MENUBUTTON:
	ts_menubutton(OFF);break;
      case TS_ICONBUTTON:
	ts_iconbutton(OFF);break;
      case TS_MINIBUTTON:
	ts_minibutton(OFF);break;

	/*
      case TS_POS:
	ts_pos( OFF, -play_val );break;
	*/
      case TS_VOLUME:
	ts_volume( OFF, -vol_val );break;
	/*
      case TS_PAN:
	ts_pan( OFF, -pan_val );break;
	*/

      case TS_SPECTRUM:
#ifdef SUPPORT_SOUNDSPEC
	fspe = (fspe+1)%3;
	if ( fspe==1 ) xskin_pipe_write("W");      /* on */
	else if ( fspe==0 ) {
	  xskin_pipe_write("W"); /* off */
	  ts_spectrum(0,speana_buf);
	}
#endif /* SUPPORT_SOUNDSPEC */
	break;

      default:
	break;
      }
      pr=-1;
      break;

    default:
      break;
    }
  }

  return;
}

static int load_skins( void ) {

  char **files;
  char *tmp[2];
  int nfiles;
  int i,pixmaps;
  char *p,*p0;
  char *skin_name;
  int width, height;

  skin_name = getenv( "TIMIDITY_SKIN" );
  if ( skin_name == NULL ) {
    skin_name = getenv( "timidity_skin" );
    if ( skin_name == NULL ) {
#ifdef	DEFAULT_SKIN
      skin_name = DEFAULT_SKIN;
#else
      fprintf(stderr, "Undefined environment `timidity_skin'\n");
      return -1;
#endif
    }
  }

  tmp[0]=skin_name;
  files=tmp;
  nfiles=1;
  files = expand_file_archives( files, &nfiles );

  pixmaps=0;

  xskin_loadviscolor( xskin_d, xskin_w, NULL );

  for ( i=0 ; i<nfiles ; i++ ) {

    /*printf("%s\n",files[i]);fflush(stdout);*/
    p0=strrchr( files[i], '#' );
    if ( p0==NULL ) p0=files[i];
    else p0++;
    p=strrchr( p0, PATH_SEP );
    if ( p==NULL ) p=p0;
    else p++;

           if ( strcasecmp( p, "viscolor.txt" )==0 ) {
	     xskin_loadviscolor( xskin_d, xskin_w, files[i] );

    } else if ( strcasecmp( p, "main.bmp" )==0 ) {
      xskin_back = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "titlebar.bmp" )==0 ) {
      xskin_titlebar = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "playpaus.bmp" )==0 ) {
      xskin_playpaus = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "cbuttons.bmp" )==0 ) {
      xskin_cbuttons = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "monoster.bmp" )==0 ) {
      xskin_monoster = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "posbar.bmp" )==0 ) {
      xskin_posbar = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "shufrep.bmp" )==0 ) {
      xskin_shufrep = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "text.bmp" )==0 ) {
      xskin_text = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "volume.bmp" )==0 ) {
      xskin_volume = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else if ( strcasecmp( p, "numbers.bmp" )==0 ) {
      xskin_numbers = 
	xskin_loadBMP( xskin_d, xskin_w, files[i], &width, &height );
      pixmaps++;

    } else {
      width=1;
    }
    if ( width<0 ) return -1;
  }

  if(files != tmp)
      free(files);

  if ( pixmaps<10 ) {
    fprintf(stderr, "some of bmp file might be missed.\n");
    return -1;
  }

  return 0;
}

static void repaint( void ) {

  char tmp[64];
  int min,sec;

  /* static values */

  XClearWindow( xskin_d, xskin_w );

  ts_titlebar(OFF);

  ts_prev(OFF);
  ts_play(OFF);
  ts_pause(OFF);
  ts_stop(OFF);
  ts_next(OFF);
  ts_eject(OFF);

  if ( (play_mode->encoding & PE_MONO)==0 ) {
    ts_mono(OFF);
    ts_stereo(ON);
  } else {
    ts_mono(ON);
    ts_stereo(OFF);
  }

  ts_pan(OFF,-50);
  ts_puttext( BITRATE_X, BITRATE_Y, "---" ); /* bit-rate */
  
  sprintf( tmp, "%d", (int)play_mode->rate/1000 );
  ts_puttext( SAMPLE_X,  SAMPLE_Y,  tmp  ); /* sample-rate */

  /* volatile values */

  if ( fshuf==0 ) ts_shuf(OFF);
  else ts_shuf(ON);

  if ( frep==0 ) ts_rep(OFF);
  else ts_rep(ON);

  if ( fequ==0 ) ts_equ(OFF);
  else ts_equ(ON);

  if ( fpll==0 ) ts_plist(OFF);
  else ts_plist(ON);

  if ( fplay==1 ) {
    if ( fpause==0 ) ts_pstate( PSTATE_PLAY );
    else  ts_pstate( PSTATE_PAUSE );
  } else ts_pstate( PSTATE_STOP );

  ts_volume( OFF, -vol_val );
  ts_pos( OFF, -play_val );

  ts_puttext( MESSAGE_X, MESSAGE_Y, last_text );

  if ( fremain==0 ) {
    sec=last_current_time;
  } else {
    sec=total_time-last_current_time;
  }
  min =sec/60;
  sec-=min*60;

  ts_putnum( MIN_H_X, MIN_H_Y, min/10 );
  ts_putnum( MIN_L_X, MIN_L_Y, min%10 );
  ts_putnum( SEC_H_X, SEC_H_Y, sec/10 );
  ts_putnum( SEC_L_X, SEC_L_Y, sec%10 );

  XFlush(xskin_d);
  return;
}

/* signal handler calls are ported from xmasl 
   and thanks to takawata@shidahara1.planet.kobe-u.ac.jp */

void delete_shm( void ) {
 
  if ( speana_buf != NULL ) {
    shmdt( (char *)speana_buf );
    shmctl( shmid, IPC_RMID, 0 );
  }
  return;
}

static const int signals[]={SIGHUP,SIGINT,SIGQUIT,SIGILL,SIGABRT,SIGFPE,
			    SIGBUS,SIGSEGV,SIGPIPE,SIGALRM,SIGTERM,0};

void install_sighandler( void ) {
  int i;
  for ( i=0 ; signals[i]!=0 ; i++ ) {
    signal( signals[i], signal_vector );
  }
}

void signal_vector( int sig ) {

  delete_shm();

  XUnmapWindow( xskin_d, xskin_w );
  XFlush(xskin_d);
  XDestroyWindow( xskin_d, xskin_w );

  XCloseDisplay( xskin_d );
  exit (0);
}
