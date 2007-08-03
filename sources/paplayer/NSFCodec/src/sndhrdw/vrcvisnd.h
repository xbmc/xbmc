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
** vrcvisnd.h
**
** VRCVI (Konami MMC) sound hardware emulation header
** $Id: vrcvisnd.h,v 1.7 2000/06/20 04:06:16 matt Exp $
*/

#ifndef _VRCVISND_H_
#define _VRCVISND_H_

typedef struct vrcvirectangle_s
{
   uint8 reg[3];
   int32 phaseacc;
   uint8 adder;

   int32 freq;
   int32 volume;
   uint8 duty_flip;
   boolean enabled;
} vrcvirectangle_t;

typedef struct vrcvisawtooth_s
{
   uint8 reg[3];
   int32 phaseacc;
   uint8 adder;
   uint8 output_acc;

   int32 freq;
   uint8 volume;
   boolean enabled;
} vrcvisawtooth_t;

typedef struct vrcvisnd_s
{
   vrcvirectangle_t rectangle[2];
   vrcvisawtooth_t saw;
} vrcvisnd_t;

#include "nes_apu.h"

extern apuext_t vrcvi_ext;

#endif /* _VRCVISND_H_ */

/*
** $Log: vrcvisnd.h,v $
** Revision 1.7  2000/06/20 04:06:16  matt
** migrated external sound definition to apu module
**
** Revision 1.6  2000/06/20 00:08:58  matt
** changed to driver based API
**
** Revision 1.5  2000/06/09 16:49:02  matt
** removed all floating point from sound generation
**
** Revision 1.4  2000/06/09 15:12:28  matt
** initial revision
**
*/

