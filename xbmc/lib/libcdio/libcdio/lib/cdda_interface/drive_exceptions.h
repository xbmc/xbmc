/*
  $Id: drive_exceptions.h,v 1.4 2005/01/09 01:50:56 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
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

extern int scsi_enable_cdda(cdrom_drive_t *d, int);
extern long scsi_read_mmc(cdrom_drive_t *d, void *,long,long);
extern long scsi_read_D4_10(cdrom_drive_t *, void *,long,long);
extern long scsi_read_D4_12(cdrom_drive_t *, void *,long,long);
extern long scsi_read_D8(cdrom_drive_t *, void *,long,long);
extern long scsi_read_28(cdrom_drive_t *, void *,long,long);
extern long scsi_read_A8(cdrom_drive_t *, void *,long,long);

typedef struct exception {
  const char *model;
  int atapi; /* If the ioctl doesn't work */
  unsigned char density;
  int  (*enable)(cdrom_drive_t *,int);
  long (*read)(cdrom_drive_t *,void *, long, long);
  int  bigendianp;
} exception_t;

/* specific to general */

#ifdef FINISHED_DRIVE_EXCEPTIONS
extern long scsi_read_mmc2(cdrom_drive_t *d, void *,long,long);
#else 
#define scsi_read_mmc2 NULL
#endif

#if HAVE_LINUX_MAJOR_H
/* list of drives that affect autosensing in ATAPI specific portions of code 
   (force drives to detect as ATAPI or SCSI, force ATAPI read command */

static exception_t atapi_list[]={
  {"SAMSUNG SCR-830 REV 2.09 2.09 ", 1,   0,         Dummy,scsi_read_mmc2,0},
  {"Memorex CR-622",                 1,   0,         Dummy,          NULL,0},
  {"SONY CD-ROM CDU-561",            0,   0,         Dummy,          NULL,0},
  {"Chinon CD-ROM CDS-525",          0,   0,         Dummy,          NULL,0},
  {NULL,0,0,NULL,NULL,0}};
#endif /*HAVE_LINUX_MAJOR_H*/

/* list of drives that affect MMC default settings */

#ifdef NEED_MMC_LIST
static exception_t mmc_list[]={
  {"SAMSUNG SCR-830 REV 2.09 2.09 ", 1,   0,         Dummy,scsi_read_mmc2,0},
  {"Memorex CR-622",                 1,   0,         Dummy,          NULL,0},
  {"SONY CD-ROM CDU-561",            0,   0,         Dummy,          NULL,0},
  {"Chinon CD-ROM CDS-525",          0,   0,         Dummy,          NULL,0},
  {"KENWOOD CD-ROM UCR",          -1,   0,            NULL,scsi_read_D8,  0},
  {NULL,0,0,NULL,NULL,0}};
#endif /*NEED_MMC_LIST*/

/* list of drives that affect SCSI default settings */

#ifdef NEED_SCSI_LIST
static exception_t scsi_list[]={
  {"TOSHIBA",                     -1,0x82,scsi_enable_cdda,scsi_read_28,  0},
  {"IBM",                         -1,0x82,scsi_enable_cdda,scsi_read_28,  0},
  {"DEC",                         -1,0x82,scsi_enable_cdda,scsi_read_28,  0},
  
  {"IMS",                         -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"KODAK",                       -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"RICOH",                       -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"HP",                          -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"PHILIPS",                     -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"PLASMON",                     -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"GRUNDIG CDR100IPW",           -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"MITSUMI CD-R ",               -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"KENWOOD CD-ROM UCR",          -1,   0,            NULL,scsi_read_D8,  0},

  {"YAMAHA",                      -1,   0,scsi_enable_cdda,        NULL,  0},

  {"PLEXTOR",                     -1,   0,            NULL,        NULL,  0},
  {"SONY",                        -1,   0,            NULL,        NULL,  0},

  {"NEC",                         -1,   0,           NULL,scsi_read_D4_10,0},

  /* the 7501 locks up if hit with the 10 byte version from the
     autoprobe first */
  {"MATSHITA CD-R   CW-7501",     -1,   0,           NULL,scsi_read_D4_12,-1},

  {NULL,0,0,NULL,NULL,0}};

#endif /* NEED_SCSI_LIST*/
