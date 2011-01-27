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
** mmc5_snd.c
**
** Nintendo MMC5 sound emulation
** $Id: mmc5_snd.c,v 1.6 2000/07/04 04:51:41 matt Exp $
*/

#include <string.h>
#include "../types.h"
#include "mmc5_snd.h"
#include "nes_apu.h"

/* TODO: encapsulate apu/mmc5 rectangle */

#define  APU_OVERSAMPLE
#define  APU_VOLUME_DECAY(x)  ((x) -= ((x) >> 7))


typedef struct mmc5dac_s
{
   int32 output;
   boolean enabled;
} mmc5dac_t;


/* look up table madness */
static int32 decay_lut[16];
static int vbl_lut[32];

/* various sound constants for sound emulation */
/* vblank length table used for rectangles, triangle, noise */
static const uint8 vbl_length[32] =
{
   5, 127, 10, 1, 19,  2, 40,  3, 80,  4, 30,  5, 7,  6, 13,  7,
   6,   8, 12, 9, 24, 10, 48, 11, 96, 12, 36, 13, 8, 14, 16, 15
};

/* ratios of pos/neg pulse for rectangle waves
** 2/16 = 12.5%, 4/16 = 25%, 8/16 = 50%, 12/16 = 75% 
** (4-bit adder in rectangles, hence the 16)
*/
static const int duty_lut[4] =
{
   2, 4, 8, 12
};


static int32 mmc5_incsize;
static uint8 mul[2];
static mmc5rectangle_t mmc5rect[2];
static mmc5dac_t mmc5dac;

#define  MMC5_RECTANGLE_OUTPUT   chan->output_vol
static int32 mmc5_rectangle(mmc5rectangle_t *chan)
{
   int32 output;

#ifdef APU_OVERSAMPLE
   int num_times;
   int32 total;
#endif /* APU_OVERSAMPLE */

   /* reg0: 0-3=volume, 4=envelope, 5=hold, 6-7=duty cycle
   ** reg1: 0-2=sweep shifts, 3=sweep inc/dec, 4-6=sweep length, 7=sweep on
   ** reg2: 8 bits of freq
   ** reg3: 0-2=high freq, 7-4=vbl length counter
   */

   APU_VOLUME_DECAY(chan->output_vol);

   if (FALSE == chan->enabled || 0 == chan->vbl_length)
      return MMC5_RECTANGLE_OUTPUT;

   /* vbl length counter */
   if (FALSE == chan->holdnote)
      chan->vbl_length--;

   /* envelope decay at a rate of (env_delay + 1) / 240 secs */
   chan->env_phase -= 4; /* 240/60 */
   while (chan->env_phase < 0)
   {
      chan->env_phase += chan->env_delay;

      if (chan->holdnote)
         chan->env_vol = (chan->env_vol + 1) & 0x0F;
      else if (chan->env_vol < 0x0F)
         chan->env_vol++;
   }

   if (chan->freq < APU_TO_FIXED(4))
      return MMC5_RECTANGLE_OUTPUT;

   chan->phaseacc -= mmc5_incsize; /* # of cycles per sample */
   if (chan->phaseacc >= 0)
      return MMC5_RECTANGLE_OUTPUT;

#ifdef APU_OVERSAMPLE
   num_times = total = 0;

   if (chan->fixed_envelope)
      output = chan->volume << 8; /* fixed volume */
   else
      output = (chan->env_vol ^ 0x0F) << 8;
#endif

   while (chan->phaseacc < 0)
   {
      chan->phaseacc += chan->freq;
      chan->adder = (chan->adder + 1) & 0x0F;

#ifdef APU_OVERSAMPLE
      if (chan->adder < chan->duty_flip)
         total += output;
      else
         total -= output;

      num_times++;
#endif
   }

#ifdef APU_OVERSAMPLE
   chan->output_vol = total / num_times;
#else
   if (chan->fixed_envelope)
      output = chan->volume << 8; /* fixed volume */
   else
      output = (chan->env_vol ^ 0x0F) << 8;

   if (0 == chan->adder)
      chan->output_vol = output;
   else if (chan->adder == chan->duty_flip)
      chan->output_vol = -output;
#endif

   return MMC5_RECTANGLE_OUTPUT;
}

static uint8 mmc5_read(uint32 address)
{
   uint32 retval;

   retval = (uint32) (mul[0] * mul[1]);

   switch (address)
   {
   case 0x5205:
      return (uint8) retval;

   case 0x5206:
      return (uint8) (retval >> 8);

   default:
      return 0xFF;
   }
}

/* mix vrcvi sound channels together */
static int32 mmc5_process(void)
{
   int32 accum;

   accum = mmc5_rectangle(&mmc5rect[0]);
   accum += mmc5_rectangle(&mmc5rect[1]);
   if (mmc5dac.enabled)
      accum += mmc5dac.output;

   return accum;
}

/* write to registers */
static void mmc5_write(uint32 address, uint8 value)
{
   int chan;

   switch (address)
   {
   /* rectangles */
   case MMC5_WRA0:
   case MMC5_WRB0:
      chan = (address & 4) ? 1 : 0;
      mmc5rect[chan].regs[0] = value;

      mmc5rect[chan].volume = value & 0x0F;
      mmc5rect[chan].env_delay = decay_lut[value & 0x0F];
      mmc5rect[chan].holdnote = (value & 0x20) ? TRUE : FALSE;
      mmc5rect[chan].fixed_envelope = (value & 0x10) ? TRUE : FALSE;
      mmc5rect[chan].duty_flip = duty_lut[value >> 6];
      break;

   case MMC5_WRA1:
   case MMC5_WRB1:
      break;

   case MMC5_WRA2:
   case MMC5_WRB2:
      chan = (address & 4) ? 1 : 0;
      mmc5rect[chan].regs[2] = value;
      if (mmc5rect[chan].enabled)
         mmc5rect[chan].freq = APU_TO_FIXED((((mmc5rect[chan].regs[3] & 7) << 8) + value) + 1);
      break;

   case MMC5_WRA3:
   case MMC5_WRB3:
      chan = (address & 4) ? 1 : 0;
      mmc5rect[chan].regs[3] = value;

      if (mmc5rect[chan].enabled)
      {
         mmc5rect[chan].vbl_length = vbl_lut[value >> 3];
         mmc5rect[chan].env_vol = 0;
         mmc5rect[chan].freq = APU_TO_FIXED((((value & 7) << 8) + mmc5rect[chan].regs[2]) + 1);
         mmc5rect[chan].adder = 0;
      }
      break;
   
   case MMC5_SMASK:
      if (value & 0x01)
         mmc5rect[0].enabled = TRUE;
      else
      {
         mmc5rect[0].enabled = FALSE;
         mmc5rect[0].vbl_length = 0;
      }

      if (value & 0x02)
         mmc5rect[1].enabled = TRUE;
      else
      {
         mmc5rect[1].enabled = FALSE;
         mmc5rect[1].vbl_length = 0;
      }

      break;

   case 0x5010:
      if (value & 0x01)
         mmc5dac.enabled = TRUE;
      else
         mmc5dac.enabled = FALSE;
      break;

   case 0x5011:
      mmc5dac.output = (value ^ 0x80) << 8;
      break;

   case 0x5205:
      mul[0] = value;
      break;

   case 0x5206:
      mul[1] = value;
      break;

   default:
      break;
   }
}

/* reset state of vrcvi sound channels */
static void mmc5_reset(void)
{
   int i;

   /* get the phase period from the apu */
   mmc5_incsize = apu_getcyclerate();

   for (i = 0x5000; i < 0x5008; i++)
      mmc5_write(i, 0);

   mmc5_write(0x5010, 0);
   mmc5_write(0x5011, 0);
}

static void mmc5_init(void)
{
   int i;
   int num_samples = apu_getcontext()->num_samples;

   /* lut used for enveloping and frequency sweeps */
   for (i = 0; i < 16; i++)
      decay_lut[i] = num_samples * (i + 1);

   /* used for note length, based on vblanks and size of audio buffer */
   for (i = 0; i < 32; i++)
      vbl_lut[i] = vbl_length[i] * num_samples;
}

/* TODO: bleh */
static void mmc5_shutdown(void)
{
}

static apu_memread mmc5_memread[] =
{
   { 0x5205, 0x5206, mmc5_read },
   {     -1,     -1, NULL }
};

static apu_memwrite mmc5_memwrite[] =
{
   { 0x5000, 0x5015, mmc5_write },
   { 0x5205, 0x5206, mmc5_write },
   {     -1,     -1, NULL }
};

apuext_t mmc5_ext =
{
   mmc5_init,
   mmc5_shutdown,
   mmc5_reset,
   mmc5_process,
   mmc5_memread,
   mmc5_memwrite
};

/*
** $Log: mmc5_snd.c,v $
** Revision 1.6  2000/07/04 04:51:41  matt
** cleanups
**
** Revision 1.5  2000/07/03 02:18:53  matt
** much better external module exporting
**
** Revision 1.4  2000/06/28 22:03:51  matt
** fixed stupid oversight
**
** Revision 1.3  2000/06/20 20:46:58  matt
** minor cleanups
**
** Revision 1.2  2000/06/20 04:06:16  matt
** migrated external sound definition to apu module
**
** Revision 1.1  2000/06/20 00:06:47  matt
** initial revision
**
*/

