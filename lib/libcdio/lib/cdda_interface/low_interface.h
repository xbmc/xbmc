/*
  $Id: low_interface.h,v 1.5 2005/01/07 22:15:25 rocky Exp $

  Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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
/** internal include file for cdda interface kit for Linux */

#ifndef _CDDA_LOW_INTERFACE_
#define _CDDA_LOW_INTERFACE_

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#if !defined(_XBOX) && !defined(WIN32)
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#ifdef HAVE_LINUX_VERSION_H
#include <linux/version.h>
#endif

#include <cdio/paranoia.h>
#include <cdio/cdda.h>

/* some include file locations have changed with newer kernels */

#ifndef CDROMAUDIOBUFSIZ      
#define CDROMAUDIOBUFSIZ        0x5382 /* set the audio buffer size */
#endif

#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#endif

#ifdef HAVE_LINUX_MAJOR_H
#include <linux/major.h>
#endif

#define MAX_RETRIES 8
#define MAX_BIG_BUFF_SIZE 65536
#define MIN_BIG_BUFF_SIZE 4096
#define SG_OFF sizeof(struct sg_header)

#ifndef SG_EMULATED_HOST
/* old kernel version; the check for the ioctl is still runtime, this
   is just to build */
#define SG_EMULATED_HOST 0x2203
#define SG_SET_TRANSFORM 0x2204
#define SG_GET_TRANSFORM 0x2205
#endif

extern int  cooked_init_drive (cdrom_drive_t *d);
extern unsigned char *scsi_inquiry (cdrom_drive_t *d);
extern int  scsi_init_drive (cdrom_drive_t *d);
#ifdef CDDA_TEST
extern int  test_init_drive (cdrom_drive_t *d);
#endif
#endif /*_CDDA_LOW_INTERFACE_*/

