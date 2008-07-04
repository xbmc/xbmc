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
** mmc5_snd.h
**
** Nintendo MMC5 sound emulation header
** $Id: mmc5_snd.h,v 1.2 2000/06/20 04:06:16 matt Exp $
*/

#ifndef _MMC5_SND_H_
#define _MMC5_SND_H_

#define  MMC5_WRA0    0x5000
#define  MMC5_WRA1    0x5001
#define  MMC5_WRA2    0x5002
#define  MMC5_WRA3    0x5003
#define  MMC5_WRB0    0x5004
#define  MMC5_WRB1    0x5005
#define  MMC5_WRB2    0x5006
#define  MMC5_WRB3    0x5007
#define  MMC5_SMASK   0x5015

typedef struct mmc5rectangle_s
{
   uint8 regs[4];

   boolean enabled;
   
   int32 phaseacc;
   int32 freq;
   int32 output_vol;
   boolean fixed_envelope;
   boolean holdnote;
   uint8 volume;

   int32 env_phase;
   int32 env_delay;
   uint8 env_vol;

   int vbl_length;
   uint8 adder;
   int duty_flip;
} mmc5rectangle_t;


#include "nes_apu.h"

extern apuext_t mmc5_ext;

#endif /* !_MMC5_SND_H_ */

/*
** $Log: mmc5_snd.h,v $
** Revision 1.2  2000/06/20 04:06:16  matt
** migrated external sound definition to apu module
**
** Revision 1.1  2000/06/20 00:06:47  matt
** initial revision
**
*/

