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
** vrcvisnd.c
**
** VRCVI sound hardware emulation
** $Id: vrcvisnd.c,v 1.9 2000/07/04 04:51:41 matt Exp $
*/

#include "../types.h"
#include "vrcvisnd.h"
#include "nes_apu.h"


static vrcvisnd_t vrcvi;
static int32 vrcvi_incsize;

/* VRCVI rectangle wave generation */
static int32 vrcvi_rectangle(vrcvirectangle_t *chan)
{
   /* reg0: 0-3=volume, 4-6=duty cycle
   ** reg1: 8 bits of freq
   ** reg2: 0-3=high freq, 7=enable
   */

   chan->phaseacc -= vrcvi_incsize; /* # of clocks per wave cycle */
   while (chan->phaseacc < 0)
   {
      chan->phaseacc += chan->freq;
      chan->adder = (chan->adder + 1) & 0x0F;
   }

   /* return if not enabled */
   if (FALSE == chan->enabled)
      return 0;

   if (chan->adder < chan->duty_flip)
      return -(chan->volume);
   else
      return chan->volume;
}

/* VRCVI sawtooth wave generation */
static int32 vrcvi_sawtooth(vrcvisawtooth_t *chan)
{
   /* reg0: 0-5=phase accumulator bits
   ** reg1: 8 bits of freq
   ** reg2: 0-3=high freq, 7=enable
   */

   chan->phaseacc -= vrcvi_incsize; /* # of clocks per wav cycle */
   while (chan->phaseacc < 0)
   {
      chan->phaseacc += chan->freq;
      chan->output_acc += chan->volume;
      
      if (7 == ++chan->adder)
      {
         chan->adder = 0;
         chan->output_acc = 0;
      }
   }

   /* return if not enabled */
   if (FALSE == chan->enabled)
      return 0;
   else
      return (chan->output_acc >> 3) << 9;
}

/* mix vrcvi sound channels together */
static int32 vrcvi_process(void)
{
   int32 output;

   output = vrcvi_rectangle(&vrcvi.rectangle[0]);
   output += vrcvi_rectangle(&vrcvi.rectangle[1]);
   output += vrcvi_sawtooth(&vrcvi.saw);

   return output;
}

/* write to registers */
static void vrcvi_write(uint32 address, uint8 value)
{
   int chan;

   switch (address & 0xB003)
   {
   case 0x9000:
   case 0xA000:
      chan = (address >> 12) - 9;
      vrcvi.rectangle[chan].reg[0] = value;
      vrcvi.rectangle[chan].volume = (value & 0x0F) << 8;
      vrcvi.rectangle[chan].duty_flip = (value >> 4) + 1;
      break;
   case 0x9001:
   case 0xA001:
      chan = (address >> 12) - 9;
      vrcvi.rectangle[chan].reg[1] = value;
      vrcvi.rectangle[chan].freq = APU_TO_FIXED(((vrcvi.rectangle[chan].reg[2] & 0x0F) << 8) + value + 1);
      break;
   case 0x9002:
   case 0xA002:
      chan = (address >> 12) - 9;
      vrcvi.rectangle[chan].reg[2] = value;
      vrcvi.rectangle[chan].freq = APU_TO_FIXED(((value & 0x0F) << 8) + vrcvi.rectangle[chan].reg[1] + 1);
      vrcvi.rectangle[chan].enabled = (value & 0x80) ? TRUE : FALSE;
      break;
   case 0xB000:
      vrcvi.saw.reg[0] = value;
      vrcvi.saw.volume = value & 0x3F;
      break;
   case 0xB001:
      vrcvi.saw.reg[1] = value;
      vrcvi.saw.freq = APU_TO_FIXED((((vrcvi.saw.reg[2] & 0x0F) << 8) + value + 1) << 1);
      break;
   case 0xB002:
      vrcvi.saw.reg[2] = value;
      vrcvi.saw.freq = APU_TO_FIXED((((value & 0x0F) << 8) + vrcvi.saw.reg[1] + 1) << 1);
      vrcvi.saw.enabled = (value & 0x80) ? TRUE : FALSE;
      break;
   default:
      break;
   }
}

/* reset state of vrcvi sound channels */
static void vrcvi_reset(void)
{
   int i;

   /* preload regs */
   for (i = 0; i < 3; i++)
   {
      vrcvi_write(0x9000 + i, 0);
      vrcvi_write(0xA000 + i, 0);
      vrcvi_write(0xB000 + i, 0);
   }

   /* get the phase period from the apu */
   vrcvi_incsize = apu_getcyclerate();
}

static void vrcvi_dummy(void)
{
}

static apu_memwrite vrcvi_memwrite[] =
{
//   { 0x4040, 0x4092, ext_write }, /* FDS sound regs */
   { 0x9000, 0x9002, vrcvi_write }, /* vrc6 */
   { 0xA000, 0xA002, vrcvi_write },
   { 0xB000, 0xB002, vrcvi_write },
   {     -1,     -1, NULL }
};

apuext_t vrcvi_ext =
{
   vrcvi_dummy, /* no init */
   vrcvi_dummy, /* no shutdown */
   vrcvi_reset,
   vrcvi_process,
   NULL, /* no reads */
   vrcvi_memwrite
};

/*
** $Log: vrcvisnd.c,v $
** Revision 1.9  2000/07/04 04:51:41  matt
** cleanups
**
** Revision 1.8  2000/07/03 02:18:53  matt
** much better external module exporting
**
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

