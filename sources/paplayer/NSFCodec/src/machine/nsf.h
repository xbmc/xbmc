/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** nsf.h
**
** NSF loading/saving related defines / prototypes
** $Id: nsf.h,v 1.11 2000/07/04 04:59:24 matt Exp $
*/

#ifndef _NSF_H_
#define _NSF_H_

#include "../cpu/nes6502/nes6502.h"
#include "../sndhrdw/nes_apu.h"

#define  NSF_MAGIC   "NESM\x1A"

#define  NSF_DEDICATED_PAL       0x01
#define  NSF_DUAL_PAL_NTSC       0x02

#define  EXT_SOUND_NONE          0x00
#define  EXT_SOUND_VRCVI         0x01
#define  EXT_SOUND_VRCVII        0x02
#define  EXT_SOUND_FDS           0x04
#define  EXT_SOUND_MMC5          0x08
#define  EXT_SOUND_NAMCO106      0x10
#define  EXT_SOUND_SUNSOFT_FME07 0x20
/* bits 6,7: future expansion */

#define  NSF_HEADER_SIZE         0x80

/* 60 Hertz refresh (NTSC) */
#define  NES_MASTER_CLOCK     21477272.7272
#define  NTSC_REFRESH         60
#define  NTSC_SUBCARRIER_DIV  12
#define  NTSC_SCANLINES       262

#define  NES_FRAME_CYCLES     ((NES_MASTER_CLOCK / NTSC_SUBCARRIER_DIV) / NTSC_REFRESH)
#define  NES_SCANLINE_CYCLES  (NES_FRAME_CYCLES / NTSC_SCANLINES)

/* filter levels */
enum
{
   NSF_FILTER_NONE,
   NSF_FILTER_LOWPASS,
   NSF_FILTER_WEIGHTED
};

#define  __PACKED__

typedef struct nsf_s
{
   /* NESM header */
   uint8  id[5]               __PACKED__; /* NESM\x1A */
   uint8  version             __PACKED__; /* spec version */
   uint8  num_songs           __PACKED__; /* total num songs */
   uint8  start_song          __PACKED__; /* first song */
   uint16 load_addr           __PACKED__; /* loc to load code */
   uint16 init_addr           __PACKED__; /* init call address */
   uint16 play_addr           __PACKED__; /* play call address */
   uint8  song_name[32]       __PACKED__; /* name of song */
   uint8  artist_name[32]     __PACKED__; /* artist name */
   uint8  copyright[32]       __PACKED__; /* copyright info */
   uint16 ntsc_speed          __PACKED__; /* playback speed (if NTSC) */
   uint8  bankswitch_info[8]  __PACKED__; /* initial code banking */
   uint16 pal_speed           __PACKED__; /* playback speed (if PAL) */
   uint8  pal_ntsc_bits       __PACKED__; /* NTSC/PAL determination bits */
   uint8  ext_sound_type      __PACKED__; /* type of external sound gen. */
   uint8  reserved[4]         __PACKED__; /* reserved */

   /* things that the NSF player needs */
   uint8  *data;              /* actual NSF data */
   uint32 length;             /* length of data */
   uint32 playback_rate;      /* current playback rate */
   uint8  current_song;       /* current song */
   boolean bankswitched;      /* is bankswitched? */

   /* CPU and APU contexts */
   nes6502_context *cpu;
   apu_t *apu;

   /* our main processing routine, calls all external mixing routines */
   void (*process)(void *buffer, int num_samples);
} nsf_t;

/* Function prototypes */
extern void nsf_init(void);

extern nsf_t *nsf_load(char *filename, void *source, int length);
extern void nsf_free(nsf_t **nsf_info);

extern void nsf_playtrack(nsf_t *nsf, int track, int sample_rate, int sample_bits, 
                          boolean stereo);
extern void nsf_frame(nsf_t *nsf);
extern void nsf_setchan(nsf_t *nsf, int chan, boolean enabled);
extern void nsf_setfilter(nsf_t *nsf, int filter_type);

extern void apu_setcontext(apu_t* src_apu);

#endif /* _NSF_H_ */

/*
** $Log: nsf.h,v $
** Revision 1.11  2000/07/04 04:59:24  matt
** removed DOS-specific stuff
**
** Revision 1.10  2000/07/03 02:19:36  matt
** dynamic address range handlers, cleaner and faster
**
** Revision 1.9  2000/06/23 03:27:58  matt
** cleaned up external sound inteface
**
** Revision 1.8  2000/06/20 04:04:37  matt
** moved external soundchip struct to apu module
**
** Revision 1.7  2000/06/20 00:05:45  matt
** changed to driver-based external sound generation
**
** Revision 1.6  2000/06/13 03:51:54  matt
** update API to take freq/sample data on nsf_playtrack
**
** Revision 1.5  2000/06/12 01:13:00  matt
** added CPU/APU as members of the nsf struct
**
** Revision 1.4  2000/06/09 15:12:26  matt
** initial revision
**
*/

