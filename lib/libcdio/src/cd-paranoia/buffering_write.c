/*
   $Id: buffering_write.c,v 1.2 2004/12/19 01:43:38 rocky Exp $
 
   Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
   Copyright (C) 1998, 1999 Monty <xiphmont@mit.edu>
 
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.
 
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
 */
/* Eliminate teeny little writes.  patch submitted by 
   Rob Ross <rbross@parl.ces.clemson.edu> --Monty 19991008 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#define OUTBUFSZ 32*1024

#include "utils.h"
#include "buffering_write.h"


/* GLOBALS FOR BUFFERING CALLS */
static int  bw_fd  = -1;
static long bw_pos = 0;
static char bw_outbuf[OUTBUFSZ];



long int
blocking_write(int outf, char *buffer, long num){
  long int words=0,temp;

  while(words<num){
    temp=write(outf,buffer+words,num-words);
    if(temp==-1){
      if(errno!=EINTR && errno!=EAGAIN)
	return(-1);
      temp=0;
    }
    words+=temp;
  }
  return(0);
}

/** buffering_write() - buffers data to a specified size before writing.
 *
 * Restrictions:
 * - MUST CALL BUFFERING_CLOSE() WHEN FINISHED!!!
 *
 */
long int 
buffering_write(int fd, char *buffer, long num)
{
  if (fd != bw_fd) {
    /* clean up after buffering for some other file */
    if (bw_fd >= 0 && bw_pos > 0) {
      if (blocking_write(bw_fd, bw_outbuf, bw_pos)) {
	perror("write (in buffering_write, flushing)");
      }
    }
    bw_fd  = fd;
    bw_pos = 0;
  }
  
  if (bw_pos + num > OUTBUFSZ) {
    /* fill our buffer first, then write, then modify buffer and num */
    memcpy(&bw_outbuf[bw_pos], buffer, OUTBUFSZ - bw_pos);
    if (blocking_write(fd, bw_outbuf, OUTBUFSZ)) {
      perror("write (in buffering_write, full buffer)");
      return(-1);
    }
    num -= (OUTBUFSZ - bw_pos);
    buffer += (OUTBUFSZ - bw_pos);
    bw_pos = 0;
  }
  /* save data */
  memcpy(&bw_outbuf[bw_pos], buffer, num);
  bw_pos += num;
  
  return(0);
}

/** buffering_close() - writes out remaining buffered data before
 * closing file.
 *
 */
int 
buffering_close(int fd)
{
  if (fd == bw_fd && bw_pos > 0) {
    /* write out remaining data and clean up */
    if (blocking_write(fd, bw_outbuf, bw_pos)) {
      perror("write (in buffering_close)");
    }
    bw_fd  = -1;
    bw_pos = 0;
  }
  return(close(fd));
}
