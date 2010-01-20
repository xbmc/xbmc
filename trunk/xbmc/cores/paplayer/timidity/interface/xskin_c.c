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

    xskin interface by Daisuke nagano <breeze_geo@geocities.co.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <sys/time.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"
#include "xskin.h"
#define MSGWINDOW
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_current_time(int secs, int v);
static void ctl_lyric(int lyricid);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]);
static void ctl_event(CtlEvent *e);
static void ctl_speana_data(double *val, int size);
static void initialize_exp_hz_table( void );

static void xskin_pipe_open(void);
void xskin_pipe_write(char *);
static int xskin_pipe_ready(void);
int xskin_pipe_read(char *,int);

static int isspeanaenabled;
static unsigned char *speana_buf;
static double exp_hz_table[SPE_W+1];
static int xskin_ready = 0;

#define FFTSIZE 1024 /* same as "soudspec.c" */
#define NCOLOR  64   /* same as "soudspec.c" */
#define DEFAULT_ZOOM (44100.0/1024.0*4.0) /* tekito---- */

#define CTL_LAST_STATUS -1

/**********************************************/
/* export the interface functions */

#define ctl xskin_control_mode

ControlMode ctl=
{
    "skin interface", 'i',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};

static char local_buf[300];
static int pipe_in_fd,pipe_out_fd=-1;

/***********************************************************************/
/* Put controls on the pipe                                            */
/***********************************************************************/
static int cmsg(int type, int verbosity_level, char *fmt, ...) {

  va_list ap;
  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;

  va_start(ap, fmt);

#ifdef MSGWINDOW
  if(!xskin_ready)
#endif
  {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, NLS);
    va_end(ap);
    return 0;
  }

  vsnprintf(local_buf+2,100,fmt,ap);
  if(pipe_out_fd==-1) {
      fputs(local_buf + 2, stderr);
      fputs(NLS, stderr);
  } else {
    local_buf[0]='L';
    local_buf[1]=' ';
    xskin_pipe_write(local_buf);
  }
  va_end(ap);
  return 0;
}

/*ARGSUSED*/
static void ctl_total_time(int tt) {

  static int previous_total_time=-1;
  int sec,min;

  sec=tt/play_mode->rate;
  min = sec/60;
  sec-= min*60;

  if ( tt!=previous_total_time ) {
    previous_total_time=tt;
    sprintf(local_buf,"A %d",min*60+sec);
    xskin_pipe_write(local_buf);
  }

  return;
}

/*ARGSUSED*/
static void ctl_master_volume(int mv) {

  static int lastvol = CTL_LAST_STATUS;

  if ( mv != lastvol ) {
    if ( mv == CTL_LAST_STATUS ) mv = lastvol;
    else lastvol = mv;
    
    sprintf( local_buf, "V %d", mv );
    xskin_pipe_write(local_buf);
  }

  return;
}

static void ctl_current_time(int sec, int v) {

  static int previous_sec=-1;
  if (sec!=previous_sec) {
    previous_sec=sec;
    sprintf(local_buf,"T %02d:%02d",sec/60,sec%60);
    xskin_pipe_write(local_buf);
  }
}

static void ctl_lyric(int lyricid)
{
    char *lyric;
	static int lyric_col = 2;
	static char lyric_buf[300];

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
	if(lyric[0] == ME_KARAOKE_LYRIC)
	{
	    if(lyric[1] == '/' || lyric[1] == '\\')
	    {
		lyric_buf[0] = 'L';
		lyric_buf[1] = ' ';
		snprintf(lyric_buf + 2, sizeof (lyric_buf) - 2, "%s", lyric + 2);
		xskin_pipe_write(lyric_buf);
		lyric_col = strlen(lyric + 2) + 2;
	    }
	    else if(lyric[1] == '@')
	    {
		lyric_buf[0] = 'L';
		lyric_buf[1] = ' ';
		if(lyric[2] == 'L')
		    snprintf(lyric_buf + 2, sizeof (lyric_buf) - 2, "Language: %s", lyric + 3);
		else if(lyric[2] == 'T')
		    snprintf(lyric_buf + 2, sizeof (lyric_buf) - 2, "Title: %s", lyric + 3);
		else
		    snprintf(lyric_buf + 2, sizeof (lyric_buf) - 2, "%s", lyric + 1);
		xskin_pipe_write(lyric_buf);
	    }
	    else
	    {
		lyric_buf[0] = 'L';
		lyric_buf[1] = ' ';
		snprintf(lyric_buf + lyric_col, sizeof (lyric_buf) - lyric_col, "%s", lyric + 1);
		xskin_pipe_write(lyric_buf);
		lyric_col += strlen(lyric + 1);
	    }
	}
	else
	{
	    if(lyric[0] == ME_CHORUS_TEXT || lyric[0] == ME_INSERT_TEXT)
		lyric_col = 0;
	    snprintf(lyric_buf + lyric_col, sizeof (lyric_buf) - lyric_col, "%s", lyric + 1);
	    xskin_pipe_write(lyric_buf);
	}
    }
}

static void ctl_speana_data(double *val, int size) {

  /* 0 <= val[n] <= (AMP*NCOLOR) */
  /* AMP and NCOLOR are defined in soundspec.c */
  /* By default, AMP*NCOLOR = 1.0*64 */

  /* size = FFTSIZE/2 = 512 */
  /* FFTSIZE is also defined in soundspec.c */

  /* val[n] is the value of FFTed audio data */

#ifdef SUPPORT_SOUNDSPEC
  int i;
  int tx,x;
  double px;
  double s,a;
  int n;

  if ( isspeanaenabled ) {

    px=0.0;
    speana_buf[0] = (unsigned char)val[0];
    for ( i=1 ; i<SPE_W-1 ; i++ ) {
      s=0.0;
      n=0;
      tx=exp_hz_table[i];
      x=(int)px;

      do {
	a=val[x];
	s += a + (tx-x)*(val[x+1]-a);
	n++;
	x++;
      } while ( x<tx );

      s/=n;
      s*=16;
      if ( s<0 ) s=0;
      if ( s>=NCOLOR ) s=NCOLOR-1;
      speana_buf[i] = (unsigned char)(256*s/NCOLOR);
      px=tx;
    }
    speana_buf[SPE_W-1] = val[FFTSIZE/2-1];

    xskin_pipe_write( "W" );

  }
#endif /* SUPPORT_SOUNDSPEC */

  return;
}

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout) {
  ctl.opened=1;
  initialize_exp_hz_table();

  /* The child process won't come back from this call  */
  xskin_pipe_open();

  return 0;
}

static void ctl_close(void)
{
  if (ctl.opened) {
    xskin_pipe_write("Q");
    ctl.opened=0;
    pipe_out_fd=-1;
    xskin_ready = 0;
  }
}

static int exitflag=0,randomflag=0,repeatflag=0,selectflag=0;

/*ARGSUSED*/
static int ctl_blocking_read(int32 *valp  /* Now, valp is not used */ ) {
  xskin_pipe_read(local_buf,sizeof(local_buf));
  for (;;) {
    switch (local_buf[0]) {
    case 'P' : return RC_LOAD_FILE;
    case 'U' : return RC_TOGGLE_PAUSE;
    case 'S' : return RC_QUIT;
    case 'N' : return RC_NEXT;
    case 'B' : return RC_REALLY_PREVIOUS;
    case 'R' : repeatflag=atoi(local_buf+2);return RC_NONE;
    case 'D' : randomflag=atoi(local_buf+2);return RC_QUIT;
    case 'L' : selectflag=atoi(local_buf+2);return RC_QUIT;
    case 'V' : *valp     =atoi(local_buf+2);return RC_CHANGE_VOLUME;
#ifdef SUPPORT_SOUNDSPEC
    case 'W' : return RC_TOGGLE_CTL_SPEANA;
#endif
    case 'Q' :
    default : exitflag=1;return RC_QUIT;
    }
  }
}

static int ctl_read(int32 *valp) {
  if (xskin_pipe_ready()<=0) return RC_NONE;
  return ctl_blocking_read(valp);
}

static void shuffle(int n,int *a) {

  int i,j,tmp;

  for (i=0;i<n;i++) {
    j=int_rand(n);
    tmp=a[i];
    a[i]=a[j];
    a[j]=tmp;
  }
}

static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]) {

  int current_no,command,i;
  int32 val;
  int *file_table;
  char **titles;
  char *p;

  /* Wait prepare 'interface' */
  xskin_pipe_read(local_buf,sizeof(local_buf));
  if (strcmp("READY",local_buf)) return;
  xskin_ready = 1;

  /* receive shared memory buffer */
  xskin_pipe_read(local_buf, sizeof(local_buf));
  if (strcmp("ERROR",local_buf)) {
    int shmid;
    isspeanaenabled=1;
    shmid = atoi(local_buf);
    speana_buf = (unsigned char *)shmat(shmid,0,0);
  } else {
    isspeanaenabled=0;
  }

  /* Make title string */
  titles=(char **)safe_malloc(number_of_files*sizeof(char *));
  for (i=0;i<number_of_files;i++) {
    p=strrchr(list_of_files[i],'/');
    if (p==NULL) {
      p=list_of_files[i];
    } else p++;
    sprintf(local_buf,"%d. %s",i+1,p);
    titles[i]=(char *)safe_malloc(strlen(local_buf)+1);
    strcpy(titles[i],local_buf);
  }

  /* Send title string */
  sprintf(local_buf,"%d",number_of_files);
  xskin_pipe_write(local_buf);
  for (i=0;i<number_of_files;i++) xskin_pipe_write(titles[i]);

  /* Make the table of play sequence */
  file_table=(int *)safe_malloc(number_of_files*sizeof(int));
  for (i=0;i<number_of_files;i++) file_table[i]=i;

  /* Draw the title of the first file */
  current_no=0;
  sprintf(local_buf,"F %s",titles[file_table[0]]);
  xskin_pipe_write(local_buf);

  command=ctl_blocking_read(&val);

  /* Main loop */
  for (;;) {
    /* Play file */
    if (command==RC_LOAD_FILE) {
      sprintf(local_buf,"F %s",titles[file_table[current_no]]);
      xskin_pipe_write(local_buf);
      command=play_midi_file(list_of_files[file_table[current_no]]);
    } else {
      /* Quit timidity*/
      if (exitflag) return;
      /* Stop playing */
      if (command==RC_QUIT) {
	sprintf(local_buf,"T 00:00");
	xskin_pipe_write(local_buf);
	/* Shuffle the table */
	if (randomflag) {
	  current_no=0;
	  if (randomflag==1) {
	    shuffle(number_of_files,file_table);
	    randomflag=0;
	    command=RC_LOAD_FILE;
	    continue;
	  }
	  randomflag=0;
	  for (i=0;i<number_of_files;i++) file_table[i]=i;
	  sprintf(local_buf,"F %s",titles[file_table[current_no]]);
	  xskin_pipe_write(local_buf);
	}
	/* Play the selected file */
	if (selectflag) {
	  for (i=0;i<number_of_files;i++)
	    if (file_table[i]==selectflag-1) break;
	  if (i!=number_of_files) current_no=i;
	  selectflag=0;
	  command=RC_LOAD_FILE;
	  continue;
	}
      /* After the all file played */
      } else if (command==RC_TUNE_END || command==RC_ERROR) {
	if (current_no+1<number_of_files) {
	  current_no++;
	  command=RC_LOAD_FILE;
	  continue;
	/* Repeat */
	} else if (repeatflag) {
	  current_no=0;
	  command=RC_LOAD_FILE;
	  continue;
	/* Off the play button */
	} else {
	  xskin_pipe_write("O");
	}
      /* Play the next */
      } else if (command==RC_NEXT) {
	if (current_no+1<number_of_files) current_no++;
	command=RC_LOAD_FILE;
	continue;
      /* Play the previous */
      } else if (command==RC_REALLY_PREVIOUS) {
	if (current_no>0) current_no--;
	command=RC_LOAD_FILE;
	continue;
      }

      command=ctl_blocking_read(&val);
    }
  }
}

/* ------ Pipe handlers ----- */

extern void xskin_start_interface(int);

static void xskin_pipe_open(void) {

  int cont_inter[2],inter_cont[2];

  if (pipe(cont_inter)<0 || pipe(inter_cont)<0) exit(1);

  if (fork()==0) {
    close(cont_inter[1]);
    close(inter_cont[0]);
    pipe_in_fd=cont_inter[0];
    pipe_out_fd=inter_cont[1];
    xskin_start_interface(pipe_in_fd);
  }
  close(cont_inter[0]);
  close(inter_cont[1]);
  pipe_in_fd=inter_cont[0];
  pipe_out_fd=cont_inter[1];
}

void xskin_pipe_write(char *buf) {
  write(pipe_out_fd,buf,strlen(buf));
  write(pipe_out_fd,"\n",1);
}

static int xskin_pipe_ready(void) {

  fd_set fds;
  static struct timeval tv;
  int cnt;

  FD_ZERO(&fds);
  FD_SET(pipe_in_fd,&fds);
  tv.tv_sec=0;
  tv.tv_usec=0;
  if((cnt=select(pipe_in_fd+1,&fds,NULL,NULL,&tv))<0)
    return -1;
  return cnt > 0 && FD_ISSET(pipe_in_fd, &fds) != 0;
}

int xskin_pipe_read(char *buf,int bufsize) {

  int i;

  bufsize--;
  for (i=0;i<bufsize;i++) {
    read(pipe_in_fd,buf+i,1);
    if (buf[i]=='\n') break;
  }
  buf[i]=0;
  return 0;
}

int xskin_pipe_read_direct(int32 *buf, int bufsize) {

  read( pipe_in_fd, buf, bufsize );

  return 0;
}

static void ctl_event(CtlEvent *e)
{
    switch(e->type)
    {
    case CTLE_PLAY_START:
      ctl_total_time((int)e->v1);
      break;
    case CTLE_CURRENT_TIME:
      ctl_current_time((int)e->v1, (int)e->v2);
      break;
    case CTLE_MASTER_VOLUME:
      ctl_master_volume((int)e->v1);
      break;
    case CTLE_LYRIC:
      ctl_lyric((int)e->v1);
      break;
#ifdef SUPPORT_SOUNDSPEC
    case CTLE_SPEANA:
      ctl_speana_data((double *)e->v1, (int)e->v2);
    break;
#endif /* SUPPORT_SOUNDSPEC */

    }
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_i_loader(void)
{
    return &ctl;
}

static void initialize_exp_hz_table( void ) {
  int i;
  double r, x, w;
  
  w = (double)play_mode->rate * 0.5 / DEFAULT_ZOOM;
  r = exp(log(w) * (1.0/SPE_W));
  w = (FFTSIZE/2.0) / (w - 1.0);

  for(i = 0, x = 1.0; i <= SPE_W; i++, x *= r)
    exp_hz_table[i] = (x - 1.0) * w;

}
