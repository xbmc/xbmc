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
** nes_apu.c
**
** NES APU emulation
** $Id: nes_apu.c,v 1.19 2000/07/04 04:53:26 matt Exp $
*/
#include <string.h>
#include "../types.h"
#include "../log.h"
#include "nes_apu.h"
#include "../cpu/nes6502/nes6502.h"

#ifdef NSF_PLAYER
#include "../machine/nsf.h"
#else
#include "nes.h"
#include "nes_ppu.h"
#include "nes_mmc.h"
#include "nesinput.h"
#endif /* !NSF_PLAYER */


#define  APU_OVERSAMPLE
#define  APU_VOLUME_DECAY(x)  ((x) -= ((x) >> 7))


/* pointer to active APU */
static apu_t *apu;

/* look up table madness */
static int32 decay_lut[16];
static int vbl_lut[32];
static int trilength_lut[128];

/* noise lookups for both modes */
#ifndef REALTIME_NOISE
static int8 noise_long_lut[APU_NOISE_32K];
static int8 noise_short_lut[APU_NOISE_93];
#endif /* !REALTIME_NOISE */


/* vblank length table used for rectangles, triangle, noise */
static const uint8 vbl_length[32] =
{
    5, 127,
   10,   1,
   19,   2,
   40,   3,
   80,   4,
   30,   5,
    7,   6,
   13,   7,
    6,   8,
   12,   9,
   24,  10,
   48,  11,
   96,  12,
   36,  13,
    8,  14,
   16,  15
};

/* frequency limit of rectangle channels */
static const int freq_limit[8] =
{
   0x3FF, 0x555, 0x666, 0x71C, 0x787, 0x7C1, 0x7E0, 0x7F0
};

/* noise frequency lookup table */
static const int noise_freq[16] =
{
     4,    8,   16,   32,   64,   96,  128,  160,
   202,  254,  380,  508,  762, 1016, 2034, 4068
};

/* DMC transfer freqs */
const int dmc_clocks[16] =
{
   428, 380, 340, 320, 286, 254, 226, 214,
   190, 160, 142, 128, 106,  85,  72,  54
};

/* ratios of pos/neg pulse for rectangle waves */
static const int duty_lut[4] = { 2, 4, 8, 12 };


void apu_setcontext(apu_t *src_apu)
{
   apu = src_apu;
}


/*
** Simple queue routines
*/
#define  APU_QEMPTY()   (apu->q_head == apu->q_tail)

static void apu_enqueue(apudata_t *d)
{
   ASSERT(apu);
   apu->queue[apu->q_head] = *d;

   apu->q_head = (apu->q_head + 1) & APUQUEUE_MASK;

   if (APU_QEMPTY())
      log_printf("apu: queue overflow\n");      
}

static apudata_t *apu_dequeue(void)
{
   int loc;

   ASSERT(apu);

   if (APU_QEMPTY())
      log_printf("apu: queue empty\n");

   loc = apu->q_tail;
   apu->q_tail = (apu->q_tail + 1) & APUQUEUE_MASK;

   return &apu->queue[loc];
}

void apu_setchan(int chan, boolean enabled)
{
   ASSERT(apu);
   apu->mix_enable[chan] = enabled;
}

/* emulation of the 15-bit shift register the
** NES uses to generate pseudo-random series
** for the white noise channel
*/
#ifdef REALTIME_NOISE
INLINE int8 shift_register15(uint8 xor_tap)
{
   static int sreg = 0x4000;
   int bit0, tap, bit14;

   bit0 = sreg & 1;
   tap = (sreg & xor_tap) ? 1 : 0;
   bit14 = (bit0 ^ tap);
   sreg >>= 1;
   sreg |= (bit14 << 14);
   return (bit0 ^ 1);
}
#else
static void shift_register15(int8 *buf, int count)
{
   static int sreg = 0x4000;
   int bit0, bit1, bit6, bit14;

   if (count == APU_NOISE_93)
   {
      while (count--)
      {
         bit0 = sreg & 1;
         bit6 = (sreg & 0x40) >> 6;
         bit14 = (bit0 ^ bit6);
         sreg >>= 1;
         sreg |= (bit14 << 14);
         *buf++ = bit0 ^ 1;
      }
   }
   else /* 32K noise */
   {
      while (count--)
      {
         bit0 = sreg & 1;
         bit1 = (sreg & 2) >> 1;
         bit14 = (bit0 ^ bit1);
         sreg >>= 1;
         sreg |= (bit14 << 14);
         *buf++ = bit0 ^ 1;
      }
   }
}
#endif

/* RECTANGLE WAVE
** ==============
** reg0: 0-3=volume, 4=envelope, 5=hold, 6-7=duty cycle
** reg1: 0-2=sweep shifts, 3=sweep inc/dec, 4-6=sweep length, 7=sweep on
** reg2: 8 bits of freq
** reg3: 0-2=high freq, 7-4=vbl length counter
*/
#define  APU_RECTANGLE_OUTPUT chan->output_vol
static int32 apu_rectangle(rectangle_t *chan)
{
   int32 output;

#ifdef APU_OVERSAMPLE
   int num_times;
   int32 total;
#endif

   APU_VOLUME_DECAY(chan->output_vol);

   if (FALSE == chan->enabled || 0 == chan->vbl_length)
      return APU_RECTANGLE_OUTPUT;

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

   if ((FALSE == chan->sweep_inc && chan->freq > chan->freq_limit)
       || chan->freq < APU_TO_FIXED(4))
      return APU_RECTANGLE_OUTPUT;

   /* frequency sweeping at a rate of (sweep_delay + 1) / 120 secs */
   if (chan->sweep_on && chan->sweep_shifts)
   {
      chan->sweep_phase -= 2; /* 120/60 */
      while (chan->sweep_phase < 0)
      {
         chan->sweep_phase += chan->sweep_delay;
         if (chan->sweep_inc) /* ramp up */
            chan->freq -= chan->freq >> (chan->sweep_shifts);
         else /* ramp down */
            chan->freq += chan->freq >> (chan->sweep_shifts);
      }
   }

   chan->phaseacc -= apu->cycle_rate; /* # of cycles per sample */
   if (chan->phaseacc >= 0)
      return APU_RECTANGLE_OUTPUT;

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

   return APU_RECTANGLE_OUTPUT;
}

/* TRIANGLE WAVE
** =============
** reg0: 7=holdnote, 6-0=linear length counter
** reg2: low 8 bits of frequency
** reg3: 7-3=length counter, 2-0=high 3 bits of frequency
*/
#define  APU_TRIANGLE_OUTPUT  (chan->output_vol + (chan->output_vol >> 2))
static int32 apu_triangle(triangle_t *chan)
{
   APU_VOLUME_DECAY(chan->output_vol);

   if (FALSE == chan->enabled || 0 == chan->vbl_length)
      return APU_TRIANGLE_OUTPUT;

   if (chan->counter_started)
   {
      if (chan->linear_length > 0)
         chan->linear_length--;
      if (chan->vbl_length && FALSE == chan->holdnote)
         chan->vbl_length--;
   }
   else if (FALSE == chan->holdnote && chan->write_latency)
   {
      if (--chan->write_latency == 0)
         chan->counter_started = TRUE;
   }
/*
   if (chan->countmode == COUNTMODE_COUNT)
   {
      if (chan->linear_length > 0)
         chan->linear_length--;
      if (chan->vbl_length)
         chan->vbl_length--;
   }
*/
   if (0 == chan->linear_length || chan->freq < APU_TO_FIXED(4)) /* inaudible */
      return APU_TRIANGLE_OUTPUT;

   chan->phaseacc -= apu->cycle_rate; /* # of cycles per sample */
   while (chan->phaseacc < 0)
   {
      chan->phaseacc += chan->freq;
      chan->adder = (chan->adder + 1) & 0x1F;

      if (chan->adder & 0x10)
         chan->output_vol -= (2 << 8);
      else
         chan->output_vol += (2 << 8);
   }

   return APU_TRIANGLE_OUTPUT;
}


/* WHITE NOISE CHANNEL
** ===================
** reg0: 0-3=volume, 4=envelope, 5=hold
** reg2: 7=small(93 byte) sample,3-0=freq lookup
** reg3: 7-4=vbl length counter
*/
#define  APU_NOISE_OUTPUT  ((chan->output_vol + chan->output_vol + chan->output_vol) >> 2)

static int32 apu_noise(noise_t *chan)
{
   int32 outvol;

#if defined(APU_OVERSAMPLE) && defined(REALTIME_NOISE)
#else
   int32 noise_bit;
#endif
#ifdef APU_OVERSAMPLE
   int num_times;
   int32 total;
#endif

   APU_VOLUME_DECAY(chan->output_vol);

   if (FALSE == chan->enabled || 0 == chan->vbl_length)
      return APU_NOISE_OUTPUT;

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

   chan->phaseacc -= apu->cycle_rate; /* # of cycles per sample */
   if (chan->phaseacc >= 0)
      return APU_NOISE_OUTPUT;
   
#ifdef APU_OVERSAMPLE
   num_times = total = 0;

   if (chan->fixed_envelope)
      outvol = chan->volume << 8; /* fixed volume */
   else
      outvol = (chan->env_vol ^ 0x0F) << 8;
#endif

   while (chan->phaseacc < 0)
   {
      chan->phaseacc += chan->freq;

#ifdef REALTIME_NOISE

#ifdef APU_OVERSAMPLE
      if (shift_register15(chan->xor_tap))
         total += outvol;
      else
         total -= outvol;

      num_times++;
#else
      noise_bit = shift_register15(chan->xor_tap);
#endif

#else
      chan->cur_pos++;

      if (chan->short_sample)
      {
         if (APU_NOISE_93 == chan->cur_pos)
            chan->cur_pos = 0;
      }
      else
      {
         if (APU_NOISE_32K == chan->cur_pos)
            chan->cur_pos = 0;
      }

#ifdef APU_OVERSAMPLE
      if (chan->short_sample)
         noise_bit = noise_short_lut[chan->cur_pos];
      else
         noise_bit = noise_long_lut[chan->cur_pos];

      if (noise_bit)
         total += outvol;
      else
         total -= outvol;

      num_times++;
#endif
#endif /* REALTIME_NOISE */
   }

#ifdef APU_OVERSAMPLE
   chan->output_vol = total / num_times;
#else
   if (chan->fixed_envelope)
      outvol = chan->volume << 8; /* fixed volume */
   else
      outvol = (chan->env_vol ^ 0x0F) << 8;

#ifndef REALTIME_NOISE
   if (chan->short_sample)
      noise_bit = noise_short_lut[chan->cur_pos];
   else
      noise_bit = noise_long_lut[chan->cur_pos];
#endif /* !REALTIME_NOISE */

   if (noise_bit)
      chan->output_vol = outvol;
   else
      chan->output_vol = -outvol;
#endif

   return APU_NOISE_OUTPUT;
}


INLINE void apu_dmcreload(dmc_t *chan)
{
   chan->address = chan->cached_addr;
   chan->dma_length = chan->cached_dmalength;
   chan->irq_occurred = FALSE;
}

/* DELTA MODULATION CHANNEL
** =========================
** reg0: 7=irq gen, 6=looping, 3-0=pointer to clock table
** reg1: output dc level, 6 bits unsigned
** reg2: 8 bits of 64-byte aligned address offset : $C000 + (value * 64)
** reg3: length, (value * 16) + 1
*/
#define  APU_DMC_OUTPUT ((chan->output_vol + chan->output_vol + chan->output_vol) >> 2)
static int32 apu_dmc(dmc_t *chan)
{
   int delta_bit;

   APU_VOLUME_DECAY(chan->output_vol);

   /* only process when channel is alive */
   if (chan->dma_length)
   {
      chan->phaseacc -= apu->cycle_rate; /* # of cycles per sample */
      
      while (chan->phaseacc < 0)
      {
         chan->phaseacc += chan->freq;
         
         delta_bit = (chan->dma_length & 7) ^ 7;
         
         if (7 == delta_bit)
         {
            chan->cur_byte = nes6502_getbyte(chan->address);
            
            /* steal a cycle from CPU*/
            nes6502_setdma(1);

            if (0xFFFF == chan->address)
               chan->address = 0x8000;
            else
               chan->address++;
         }

         if (--chan->dma_length == 0)
         {
            /* if loop bit set, we're cool to retrigger sample */
            if (chan->looping)
               apu_dmcreload(chan);
            else
            {
               /* check to see if we should generate an irq */
               if (chan->irq_gen)
               {
                  chan->irq_occurred = TRUE;
                  nes6502_irq();
               }

               /* bodge for timestamp queue */
               chan->enabled = FALSE;
               break;
            }
         }

         /* positive delta */
         if (chan->cur_byte & (1 << delta_bit))
         {
            if (chan->regs[1] < 0x7D)
            {
               chan->regs[1] += 2;
               chan->output_vol += (2 << 8);
            }
/*
            if (chan->regs[1] < 0x3F)
               chan->regs[1]++;

            chan->output_vol &= ~(0x7E << 8);
            chan->output_vol |= ((chan->regs[1] << 1) << 8);
*/
         }
         /* negative delta */
         else            
         {
            if (chan->regs[1] > 1)
            {
               chan->regs[1] -= 2;
               chan->output_vol -= (2 << 8);
            }

/*
            if (chan->regs[1] > 0)
               chan->regs[1]--;

            chan->output_vol &= ~(0x7E << 8);
            chan->output_vol |= ((chan->regs[1] << 1) << 8);
*/
         }
      }
   }

   return APU_DMC_OUTPUT;
}


static void apu_regwrite(uint32 address, uint8 value)
{  
   int chan;

   ASSERT(apu);
   switch (address)
   {
   /* rectangles */
   case APU_WRA0:
   case APU_WRB0:
      chan = (address & 4) ? 1 : 0;
      apu->rectangle[chan].regs[0] = value;

      apu->rectangle[chan].volume = value & 0x0F;
      apu->rectangle[chan].env_delay = decay_lut[value & 0x0F];
      apu->rectangle[chan].holdnote = (value & 0x20) ? TRUE : FALSE;
      apu->rectangle[chan].fixed_envelope = (value & 0x10) ? TRUE : FALSE;
      apu->rectangle[chan].duty_flip = duty_lut[value >> 6];
      break;

   case APU_WRA1:
   case APU_WRB1:
      chan = (address & 4) ? 1 : 0;
      apu->rectangle[chan].regs[1] = value;
      apu->rectangle[chan].sweep_on = (value & 0x80) ? TRUE : FALSE;
      apu->rectangle[chan].sweep_shifts = value & 7;
      apu->rectangle[chan].sweep_delay = decay_lut[(value >> 4) & 7];
      
      apu->rectangle[chan].sweep_inc = (value & 0x08) ? TRUE : FALSE;
      apu->rectangle[chan].freq_limit = APU_TO_FIXED(freq_limit[value & 7]);
      break;

   case APU_WRA2:
   case APU_WRB2:
      chan = (address & 4) ? 1 : 0;
      apu->rectangle[chan].regs[2] = value;
//      if (apu->rectangle[chan].enabled)
         apu->rectangle[chan].freq = APU_TO_FIXED((((apu->rectangle[chan].regs[3] & 7) << 8) + value) + 1);
      break;

   case APU_WRA3:
   case APU_WRB3:
      chan = (address & 4) ? 1 : 0;
      apu->rectangle[chan].regs[3] = value;

//      if (apu->rectangle[chan].enabled)
      {
         apu->rectangle[chan].vbl_length = vbl_lut[value >> 3];
         apu->rectangle[chan].env_vol = 0;
         apu->rectangle[chan].freq = APU_TO_FIXED((((value & 7) << 8) + apu->rectangle[chan].regs[2]) + 1);
         apu->rectangle[chan].adder = 0;
      }
      break;

   /* triangle */
   case APU_WRC0:
/*
      if (0 == (apu->triangle.regs[0] & 0x80))
         apu->triangle.countmode = COUNTMODE_COUNT;
      else
      {
         if (apu->triangle.countmode == COUNTMODE_LOAD && apu->triangle.vbl_length)
            apu->triangle.linear_length = trilength_lut[value & 0x7F];

         if (0 == (value & 0x80))
            apu->triangle.countmode = COUNTMODE_COUNT;
      }
*/
      apu->triangle.regs[0] = value;

      apu->triangle.holdnote = (value & 0x80) ? TRUE : FALSE;


//      if (apu->triangle.enabled)
      {
         if (FALSE == apu->triangle.counter_started && apu->triangle.vbl_length)
            apu->triangle.linear_length = trilength_lut[value & 0x7F];
      }

      break;

   case APU_WRC2:

      apu->triangle.regs[1] = value;

//      if (apu->triangle.enabled)
         apu->triangle.freq = APU_TO_FIXED((((apu->triangle.regs[2] & 7) << 8) + value) + 1);
      break;

   case APU_WRC3:

      apu->triangle.regs[2] = value;
  
      /* this is somewhat of a hack.  there appears to be some latency on 
      ** the Real Thing between when trireg0 is written to and when the 
      ** linear length counter actually begins its countdown.  we want to 
      ** prevent the case where the program writes to the freq regs first, 
      ** then to reg 0, and the counter accidentally starts running because 
      ** of the sound queue's timestamp processing.
      **
      ** set latency to a couple scanlines -- should be plenty of time for 
      ** the 6502 code to do a couple of table dereferences and load up the
      ** other triregs
      */

      /* 06/13/00 MPC -- seems to work OK */
      apu->triangle.write_latency = (int) (2 * NES_SCANLINE_CYCLES / APU_FROM_FIXED(apu->cycle_rate));
/*
      apu->triangle.linear_length = trilength_lut[apu->triangle.regs[0] & 0x7F];
      if (0 == (apu->triangle.regs[0] & 0x80))
         apu->triangle.countmode = COUNTMODE_COUNT;
      else
         apu->triangle.countmode = COUNTMODE_LOAD;
*/
//      if (apu->triangle.enabled)
      {
         apu->triangle.freq = APU_TO_FIXED((((value & 7) << 8) + apu->triangle.regs[1]) + 1);
         apu->triangle.vbl_length = vbl_lut[value >> 3];
         apu->triangle.counter_started = FALSE;
         apu->triangle.linear_length = trilength_lut[apu->triangle.regs[0] & 0x7F];
      }

      break;

   /* noise */
   case APU_WRD0:
      apu->noise.regs[0] = value;
      apu->noise.env_delay = decay_lut[value & 0x0F];
      apu->noise.holdnote = (value & 0x20) ? TRUE : FALSE;
      apu->noise.fixed_envelope = (value & 0x10) ? TRUE : FALSE;
      apu->noise.volume = value & 0x0F;
      break;

   case APU_WRD2:
      apu->noise.regs[1] = value;
      apu->noise.freq = APU_TO_FIXED(noise_freq[value & 0x0F]);

#ifdef REALTIME_NOISE
      apu->noise.xor_tap = (value & 0x80) ? 0x40: 0x02;
#else
      /* detect transition from long->short sample */
      if ((value & 0x80) && FALSE == apu->noise.short_sample)
      {
         /* recalculate short noise buffer */
         shift_register15(noise_short_lut, APU_NOISE_93);
         apu->noise.cur_pos = 0;
      }
      apu->noise.short_sample = (value & 0x80) ? TRUE : FALSE;
#endif
      break;

   case APU_WRD3:
      apu->noise.regs[2] = value;

//      if (apu->noise.enabled)
      {
         apu->noise.vbl_length = vbl_lut[value >> 3];
         apu->noise.env_vol = 0; /* reset envelope */
      }
      break;

   /* DMC */
   case APU_WRE0:
      apu->dmc.regs[0] = value;

      apu->dmc.freq = APU_TO_FIXED(dmc_clocks[value & 0x0F]);
      apu->dmc.looping = (value & 0x40) ? TRUE : FALSE;

      if (value & 0x80)
         apu->dmc.irq_gen = TRUE;
      else
      {
         apu->dmc.irq_gen = FALSE;
         apu->dmc.irq_occurred = FALSE;
      }
      break;

   case APU_WRE1: /* 7-bit DAC */
      /* add the _delta_ between written value and
      ** current output level of the volume reg
      */
      value &= 0x7F; /* bit 7 ignored */
      apu->dmc.output_vol += ((value - apu->dmc.regs[1]) << 8);
      apu->dmc.regs[1] = value;
/*      
      apu->dmc.output_vol = (value & 0x7F) << 8;
      apu->dmc.regs[1] = (value & 0x7E) >> 1;
*/
      break;

   case APU_WRE2:
      apu->dmc.regs[2] = value;
      apu->dmc.cached_addr = 0xC000 + (uint16) (value << 6);
      break;

   case APU_WRE3:
      apu->dmc.regs[3] = value;
      apu->dmc.cached_dmalength = ((value << 4) + 1) << 3;
      break;

   case APU_SMASK:
      /* bodge for timestamp queue */
      apu->dmc.enabled = (value & 0x10) ? TRUE : FALSE;

      apu->enable_reg = value;

      for (chan = 0; chan < 2; chan++)
      {
         if (value & (1 << chan))
            apu->rectangle[chan].enabled = TRUE;
         else
         {
            apu->rectangle[chan].enabled = FALSE;
            apu->rectangle[chan].vbl_length = 0;
         }
      }

      if (value & 0x04)
         apu->triangle.enabled = TRUE;
      else
      {
         apu->triangle.enabled = FALSE;
         apu->triangle.vbl_length = 0;
         apu->triangle.linear_length = 0;
         apu->triangle.counter_started = FALSE;
         apu->triangle.write_latency = 0;
      }

      if (value & 0x08)
         apu->noise.enabled = TRUE;
      else
      {
         apu->noise.enabled = FALSE;
         apu->noise.vbl_length = 0;
      }

      if (value & 0x10)
      {
         if (0 == apu->dmc.dma_length)
            apu_dmcreload(&apu->dmc);
      }
      else
         apu->dmc.dma_length = 0;

      apu->dmc.irq_occurred = FALSE;
      break;

      /* unused, but they get hit in some mem-clear loops */
   case 0x4009:
   case 0x400D:
      break;
   
   default:
      break;
   }
}

/* Read from $4000-$4017 */
uint8 apu_read(uint32 address)
{
   uint8 value;

   ASSERT(apu);

   switch (address)
   {
   case APU_SMASK:
      /* seems that bit 6 denotes vblank -- return 1 for now */
      value = 0x40;

      /* Return 1 in 0-5 bit pos if a channel is playing */
      if (apu->rectangle[0].enabled && apu->rectangle[0].vbl_length)
         value |= 0x01;
      if (apu->rectangle[1].enabled && apu->rectangle[1].vbl_length)
         value |= 0x02;
      if (apu->triangle.enabled && apu->triangle.vbl_length)
         value |= 0x04;
      if (apu->noise.enabled && apu->noise.vbl_length)
         value |= 0x08;

      //if (apu->dmc.dma_length)
      /* bodge for timestamp queue */
      if (apu->dmc.enabled)
         value |= 0x10;

      if (apu->dmc.irq_occurred)
         value |= 0x80;

      break;

#ifndef NSF_PLAYER
   case APU_JOY0:
      value = input_get(INP_JOYPAD0);
      break;

   case APU_JOY1:
      value = input_get(INP_ZAPPER | INP_JOYPAD1 /*| INP_ARKANOID*/ /*| INP_POWERPAD*/);
      break;
#endif /* !NSF_PLAYER */

   default:
      value = (address >> 8); /* heavy capacitance on data bus */
      break;
   }

   return value;
}


void apu_write(uint32 address, uint8 value)
{
#ifndef NSF_PLAYER
   static uint8 last_write;
#endif /* !NSF_PLAYER */
   apudata_t d;

   switch (address)
   {
   case 0x4015:
      /* bodge for timestamp queue */
      apu->dmc.enabled = (value & 0x10) ? TRUE : FALSE;

   case 0x4000: case 0x4001: case 0x4002: case 0x4003:
   case 0x4004: case 0x4005: case 0x4006: case 0x4007:
   case 0x4008: case 0x4009: case 0x400A: case 0x400B:
   case 0x400C: case 0x400D: case 0x400E: case 0x400F:
   case 0x4010: case 0x4011: case 0x4012: case 0x4013:
      d.timestamp = nes6502_getcycles(FALSE);
      d.address = address;
      d.value = value;
      apu_enqueue(&d);
      break;

#ifndef NSF_PLAYER
   case APU_OAMDMA:
      ppu_oamdma(address, value);
      break;

   case APU_JOY0:
      /* VS system VROM switching */
      mmc_vsvrom(value & 4);

      /* see if we need to strobe them joypads */
      value &= 1;
      if ((0 == value) && last_write)
         input_strobe();
      last_write = value;
      break;

   case APU_JOY1: /* Some kind of IRQ control business */
      break;

#endif /* !NSF_PLAYER */

   default:
      break;
   }
}

void apu_getpcmdata(void **data, int *num_samples, int *sample_bits)
{
   ASSERT(apu);
   *data = apu->buffer;
   *num_samples = apu->num_samples;
   *sample_bits = apu->sample_bits;
}


void apu_process(void *buffer, int num_samples)
{
   apudata_t *d;
   uint32 elapsed_cycles;
   static int32 prev_sample = 0;
   int32 next_sample, accum;
   char* foo = (char*)buffer;

   ASSERT(apu);

   /* grab it, keep it local for speed */
   elapsed_cycles = (uint32) apu->elapsed_cycles;

   /* BLEH */
   apu->buffer = buffer;

   while (num_samples--)
   {
      while ((FALSE == APU_QEMPTY()) && (apu->queue[apu->q_tail].timestamp <= elapsed_cycles))
      {
         d = apu_dequeue();
         apu_regwrite(d->address, d->value);
      }

      elapsed_cycles += APU_FROM_FIXED(apu->cycle_rate);

      accum = 0;
      if (apu->mix_enable[0]) accum += apu_rectangle(&apu->rectangle[0]);
      if (apu->mix_enable[1]) accum += apu_rectangle(&apu->rectangle[1]);
      if (apu->mix_enable[2]) accum += apu_triangle(&apu->triangle);
      if (apu->mix_enable[3]) accum += apu_noise(&apu->noise);
      if (apu->mix_enable[4]) accum += apu_dmc(&apu->dmc);

      if (apu->ext && apu->mix_enable[5]) accum += apu->ext->process();

      /* do any filtering */
      if (APU_FILTER_NONE != apu->filter_type)
      {
         next_sample = accum;

         if (APU_FILTER_LOWPASS == apu->filter_type)
         {
            accum += prev_sample;
            accum >>= 1;
         }
         else
            accum = (accum + accum + accum + prev_sample) >> 2;

         prev_sample = next_sample;
      }

      /* little extra kick for the kids */
      accum <<= 1;

      /* prevent clipping */
      if (accum > 0x7FFF)
         accum = 0x7FFF;
      else if (accum < -0x8000)
         accum = -0x8000;

      /* signed 16-bit output, unsigned 8-bit */
      if (16 == apu->sample_bits) {
         *((int16 *) foo) = (int16) accum;
		foo += 2;	
	  }
      else {
         *((uint8 *) foo) = (accum >> 8) ^ 0x80;
		foo++;
	  }
   }

   /* resync cycle counter */
   apu->elapsed_cycles = nes6502_getcycles(FALSE);
}

/* set the filter type */
void apu_setfilter(int filter_type)
{
   ASSERT(apu);
   apu->filter_type = filter_type;
}

void apu_reset(void)
{
   uint32 address;

   ASSERT(apu);

   apu->elapsed_cycles = 0;
   memset(&apu->queue, 0, APUQUEUE_SIZE * sizeof(apudata_t));
   apu->q_head = 0;
   apu->q_tail = 0;

   /* use to avoid bugs =) */
   for (address = 0x4000; address <= 0x4013; address++)
      apu_regwrite(address, 0);

#ifdef NSF_PLAYER
   apu_regwrite(0x400C, 0x10); /* silence noise channel on NSF start */
   apu_regwrite(0x4015, 0x0F);
#else
   apu_regwrite(0x4015, 0);
#endif /* NSF_PLAYER */

   if (apu->ext)
      apu->ext->reset();
}

void apu_build_luts(int num_samples)
{
   int i;

   /* lut used for enveloping and frequency sweeps */
   for (i = 0; i < 16; i++)
      decay_lut[i] = num_samples * (i + 1);

   /* used for note length, based on vblanks and size of audio buffer */
   for (i = 0; i < 32; i++)
      vbl_lut[i] = vbl_length[i] * num_samples;

   /* triangle wave channel's linear length table */
   for (i = 0; i < 128; i++)
      trilength_lut[i] = (i * num_samples) / 4;

#ifndef REALTIME_NOISE
   /* generate noise samples */
   shift_register15(noise_long_lut, APU_NOISE_32K);
   shift_register15(noise_short_lut, APU_NOISE_93);
#endif /* !REALTIME_NOISE */
}

static void apu_setactive(apu_t *active)
{
   ASSERT(active);
   apu = active;
}

/* Initializes emulated sound hardware, creates waveforms/voices */
apu_t *apu_create(int sample_rate, int refresh_rate, int sample_bits, boolean stereo)
{
   apu_t *temp_apu;
   int channel;

   temp_apu = malloc(sizeof(apu_t));
   if (NULL == temp_apu)
      return NULL;

   temp_apu->sample_rate = sample_rate;
   temp_apu->refresh_rate = refresh_rate;
   temp_apu->sample_bits = sample_bits;

   temp_apu->num_samples = sample_rate / refresh_rate;
   /* turn into fixed point! */
   temp_apu->cycle_rate = (int32) (APU_BASEFREQ * 65536.0 / (float) sample_rate);

   /* build various lookup tables for apu */
   apu_build_luts(temp_apu->num_samples);

   /* set the update routine */
   temp_apu->process = apu_process;
   temp_apu->ext = NULL;

   apu_setactive(temp_apu);
   apu_reset();

   for (channel = 0; channel < 6; channel++)
      apu_setchan(channel, TRUE);

   apu_setfilter(APU_FILTER_LOWPASS);

   return temp_apu;
}

apu_t *apu_getcontext(void)
{
   return apu;
}

void apu_destroy(apu_t *src_apu)
{
   if (src_apu)
   {
      if (src_apu->ext)
         src_apu->ext->shutdown();
      free(src_apu);
   }
}

void apu_setext(apu_t *src_apu, apuext_t *ext)
{
   ASSERT(src_apu);

   src_apu->ext = ext;

   /* initialize it */
   if (src_apu->ext)
      src_apu->ext->init();
}

/* this exists for external mixing routines */
int32 apu_getcyclerate(void)
{
   ASSERT(apu);
   return apu->cycle_rate;
}

/*
** $Log: nes_apu.c,v $
** Revision 1.19  2000/07/04 04:53:26  matt
** minor changes, sound amplification
**
** Revision 1.18  2000/07/03 02:18:53  matt
** much better external module exporting
**
** Revision 1.17  2000/06/26 11:01:55  matt
** made triangle a tad quieter
**
** Revision 1.16  2000/06/26 05:10:33  matt
** fixed cycle rate generation accuracy
**
** Revision 1.15  2000/06/26 05:00:37  matt
** cleanups
**
** Revision 1.14  2000/06/23 11:06:24  matt
** more faithful mixing of channels
**
** Revision 1.13  2000/06/23 03:29:27  matt
** cleaned up external sound inteface
**
** Revision 1.12  2000/06/20 00:08:39  matt
** bugfix to rectangle wave
**
** Revision 1.11  2000/06/13 13:48:58  matt
** fixed triangle write latency for fixed point apu cycle rate
**
** Revision 1.10  2000/06/12 01:14:36  matt
** minor change to clipping extents
**
** Revision 1.9  2000/06/09 20:00:56  matt
** fixed noise hiccup in NSF player mode
**
** Revision 1.8  2000/06/09 16:49:02  matt
** removed all floating point from sound generation
**
** Revision 1.7  2000/06/09 15:12:28  matt
** initial revision
**
*/
