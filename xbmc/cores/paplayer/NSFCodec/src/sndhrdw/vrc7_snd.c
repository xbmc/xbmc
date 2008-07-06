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
** vrc7_snd.c
**
** VRCVII sound hardware emulation
** Thanks to Charles MacDonald (cgfm2@hooked.net) for donating code.
** $Id: vrc7_snd.c,v 1.5 2000/07/04 04:51:02 matt Exp $
*/

#include <stdio.h>
#include "../types.h"
#include "vrc7_snd.h"
#include "fmopl.h"


static int buflen;
static int16 *buffer;

#define OPL_WRITE(opl, r, d) \
{ \
   OPLWrite((opl)->ym3812, 0, (r)); \
   OPLWrite((opl)->ym3812, 1, (d)); \
}

static vrc7_t vrc7;

/* Fixed instrument settings, from MAME's YM2413 emulation */
/* This might need some tweaking... */
unsigned char table[16][11] =
{
  /*   20    23    40    43    60    63    80    83    E0    E3    C0 */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

   /* MAME */  
   { 0x01, 0x22, 0x23, 0x07, 0xF0, 0xF0, 0x07, 0x18, 0x00, 0x00, 0x00 }, /* Violin */
   { 0x23, 0x01, 0x68, 0x05, 0xF2, 0x74, 0x6C, 0x89, 0x00, 0x00, 0x00 }, /* Acoustic Guitar(steel) */
   { 0x13, 0x11, 0x25, 0x00, 0xD2, 0xB2, 0xF4, 0xF4, 0x00, 0x00, 0x00 }, /* Acoustic Grand */
   { 0x22, 0x21, 0x1B, 0x05, 0xC0, 0xA1, 0x18, 0x08, 0x00, 0x00, 0x00 }, /* Flute */
   { 0x22, 0x21, 0x2C, 0x03, 0xD2, 0xA1, 0x18, 0x57, 0x00, 0x00, 0x00 }, /* Clarinet */
   { 0x01, 0x22, 0xBA, 0x01, 0xF1, 0xF1, 0x1E, 0x04, 0x00, 0x00, 0x00 }, /* Oboe */
   { 0x21, 0x21, 0x28, 0x06, 0xF1, 0xF1, 0x6B, 0x3E, 0x00, 0x00, 0x00 }, /* Trumpet */
   { 0x27, 0x21, 0x60, 0x00, 0xF0, 0xF0, 0x0D, 0x0F, 0x00, 0x00, 0x00 }, /* Church Organ */
   { 0x20, 0x21, 0x2B, 0x06, 0x85, 0xF1, 0x6D, 0x89, 0x00, 0x00, 0x00 }, /* French Horn */
   { 0x01, 0x21, 0xBF, 0x02, 0x53, 0x62, 0x5F, 0xAE, 0x01, 0x00, 0x00 }, /* Synth Voice */
   { 0x23, 0x21, 0x70, 0x07, 0xD4, 0xA3, 0x4E, 0x64, 0x01, 0x00, 0x00 }, /* Harpsichord */
   { 0x2B, 0x21, 0xA4, 0x07, 0xF6, 0x93, 0x5C, 0x4D, 0x00, 0x00, 0x00 }, /* Vibraphone */
   { 0x21, 0x23, 0xAD, 0x07, 0x77, 0xF1, 0x18, 0x37, 0x00, 0x00, 0x00 }, /* Synth Bass 1 */
   { 0x21, 0x21, 0x2A, 0x03, 0xF3, 0xE2, 0x29, 0x46, 0x00, 0x00, 0x00 }, /* Acoustic Bass */
   { 0x21, 0x23, 0x37, 0x03, 0xF3, 0xE2, 0x29, 0x46, 0x00, 0x00, 0x00 }, /* Electric Guitar(clean) */


#if 0
   /* Horton, try 1 */
   { 0x05, 0x03, 0x10, 0x06, 0x74, 0xA1, 0x13, 0xF4, 0x00, 0x00, 0x00 },
   { 0x05, 0x01, 0x16, 0x00, 0xF9, 0xA2, 0x15, 0xF5, 0x00, 0x00, 0x00 },
   { 0x01, 0x41, 0x11, 0x00, 0xA0, 0xA0, 0x83, 0x95, 0x00, 0x00, 0x00 },
   { 0x01, 0x41, 0x17, 0x00, 0x60, 0xF0, 0x83, 0x95, 0x00, 0x00, 0x00 },
   { 0x24, 0x41, 0x1F, 0x00, 0x50, 0xB0, 0x94, 0x94, 0x00, 0x00, 0x00 },
   { 0x05, 0x01, 0x0B, 0x04, 0x65, 0xA0, 0x54, 0x95, 0x00, 0x00, 0x00 },
   { 0x11, 0x41, 0x0E, 0x04, 0x70, 0xC7, 0x13, 0x10, 0x00, 0x00, 0x00 },
   { 0x02, 0x44, 0x16, 0x06, 0xE0, 0xE0, 0x31, 0x35, 0x00, 0x00, 0x00 },
   { 0x48, 0x22, 0x22, 0x07, 0x50, 0xA1, 0xA5, 0xF4, 0x00, 0x00, 0x00 },
   { 0x05, 0xA1, 0x18, 0x00, 0xA2, 0xA2, 0xF5, 0xF5, 0x00, 0x00, 0x00 },
   { 0x07, 0x81, 0x2B, 0x05, 0xA5, 0xA5, 0x03, 0x03, 0x00, 0x00, 0x00 },
   { 0x01, 0x41, 0x08, 0x08, 0xA0, 0xA0, 0x83, 0x95, 0x00, 0x00, 0x00 },
   { 0x21, 0x61, 0x12, 0x00, 0x93, 0x92, 0x74, 0x75, 0x00, 0x00, 0x00 },
   { 0x21, 0x62, 0x21, 0x00, 0x84, 0x85, 0x34, 0x15, 0x00, 0x00, 0x00 },
   { 0x21, 0x62, 0x0E, 0x00, 0xA1, 0xA0, 0x34, 0x15, 0x00, 0x00, 0x00 },
#endif

#if 0
   /* Horton try 2 */
   { 0x31, 0x22, 0x23, 0x07, 0xF0, 0xF0, 0xE8, 0xF7, 0x00, 0x00, 0x00 },
   { 0x03, 0x31, 0x68, 0x05, 0xF2, 0x74, 0x79, 0x9C, 0x00, 0x00, 0x00 },
   { 0x01, 0x51, 0x72, 0x04, 0xF1, 0xD3, 0x9D, 0x8B, 0x00, 0x00, 0x00 },
   { 0x22, 0x61, 0x1B, 0x05, 0xC0, 0xA1, 0xF8, 0xE8, 0x00, 0x00, 0x00 },
   { 0x22, 0x61, 0x2C, 0x03, 0xD2, 0xA1, 0xA7, 0xE8, 0x00, 0x00, 0x00 },
   { 0x31, 0x22, 0xFA, 0x01, 0xF1, 0xF1, 0xF4, 0xEE, 0x00, 0x00, 0x00 },
   { 0x21, 0x61, 0x28, 0x06, 0xF1, 0xF1, 0xCE, 0x9B, 0x00, 0x00, 0x00 },
   { 0x27, 0x61, 0x60, 0x00, 0xF0, 0xF0, 0xFF, 0xFD, 0x00, 0x00, 0x00 },
   { 0x60, 0x21, 0x2B, 0x06, 0x85, 0xF1, 0x79, 0x9D, 0x00, 0x00, 0x00 },
   { 0x31, 0xA1, 0xFF, 0x0A, 0x53, 0x62, 0x5E, 0xAF, 0x00, 0x00, 0x00 },
   { 0x03, 0xA1, 0x70, 0x0F, 0xD4, 0xA3, 0x94, 0xBE, 0x00, 0x00, 0x00 },
   { 0x2B, 0x61, 0xE4, 0x07, 0xF6, 0x93, 0xBD, 0xAC, 0x00, 0x00, 0x00 },
   { 0x21, 0x63, 0xED, 0x07, 0x77, 0xF1, 0xC7, 0xE8, 0x00, 0x00, 0x00 },
   { 0x21, 0x61, 0x2A, 0x03, 0xF3, 0xE2, 0xB6, 0xD9, 0x00, 0x00, 0x00 },
   { 0x21, 0x63, 0x37, 0x03, 0xF3, 0xE2, 0xB6, 0xD9, 0x00, 0x00, 0x00 },
#endif
};

static void vrc7_reset(void)
{
   int n;

   /* Point to current VRC7 context */
   vrc7_t *opll = &vrc7;

   /* Clear all YM3812 registers */
   for (n = 0; n < 0x100; n++)
      OPL_WRITE(opll, n, 0x00);

   /* Turn off rhythm mode and key-on bits */
   OPL_WRITE(opll, 0xBD, 0xC0);

   /* Enable waveform select */
   OPL_WRITE(opll, 0x01, 0x20);
}

static void vrc7_init(void)
{
   vrc7.ym3812 = OPLCreate(OPL_TYPE_YM3812, 3579545, apu_getcontext()->sample_rate);
   ASSERT(vrc7.ym3812);
   buflen = apu_getcontext()->num_samples;
   buffer = malloc(buflen * 2);
   ASSERT(buffer);
   vrc7_reset();
}

static void vrc7_shutdown(void)
{
   vrc7_reset();
   OPLDestroy(vrc7.ym3812);
   free(buffer);
}

/* channel (0-9), instrument (0-F), volume (0-3F, YM3812 format) */
static void load_instrument(uint8 ch, uint8 inst, uint8 vol)
{
   /* Point to current VRC7 context */
   vrc7_t *opll = &vrc7;

   /* Point to fixed instrument or user table */
   uint8 *param = (inst == 0) ? &opll->user[0] : &table[inst][0];

   /* Maps channels to operator registers */
   uint8 ch2op[] = {0, 1, 2, 8, 9, 10, 16, 17, 18};

   /* Make operator offset from requested channel */
   uint8 op = ch2op[ch];

   /* Store volume level */
   opll->channel[ch].volume = (vol & 0x3F);

   /* Store instrument number */
   opll->channel[ch].instrument = (inst & 0x0F);

   /* Update instrument settings, except frequency registers */
   OPL_WRITE(opll, 0x20 + op,  param[0]);
   OPL_WRITE(opll, 0x23 + op,  param[1]);
   OPL_WRITE(opll, 0x40 + op,  param[2]);
   OPL_WRITE(opll, 0x43 + op, (param[3] & 0xC0) | opll->channel[ch].volume);
   OPL_WRITE(opll, 0x60 + op,  param[4]);
   OPL_WRITE(opll, 0x63 + op,  param[5]);
   OPL_WRITE(opll, 0x80 + op,  param[6]);
   OPL_WRITE(opll, 0x83 + op,  param[7]);
   OPL_WRITE(opll, 0xE0 + op,  param[8]);
   OPL_WRITE(opll, 0xE3 + op,  param[9]);
   OPL_WRITE(opll, 0xC0 + ch,  param[10]);
}

static void vrc7_write(uint32 address, uint8 data)
{
   /* Point to current VRC7 context */
   vrc7_t *opll = &vrc7;

   if (address & 0x0020) /* data port */
   {
      /* Store register data */
      opll->reg[opll->latch] = data;

      switch (opll->latch & 0x30)
      {
      case 0x00: /* User instrument registers */
         switch (opll->latch & 0x0F)
         {
         case 0x00: /* Misc. ctrl. (modulator) */
         case 0x01: /* Misc. ctrl. (carrier) */
         case 0x02: /* Key scale level and total level (modulator) */
         case 0x04: /* Attack / Decay (modulator) */
         case 0x05: /* Attack / Decay (carrier) */
         case 0x06: /* Sustain / Release (modulator) */
         case 0x07: /* Sustain / Release (carrier) */
            opll->user[(opll->latch & 0x07)] = data;
            break;
    
         case 0x03: /* Key scale level, carrier/modulator waveform, feedback */
    
            /* Key scale level (carrier) */
            /* Don't touch the total level (channel volume) */
            opll->user[3] = (opll->user[3] & 0x3F) | (data & 0xC0);
    
            /* Waveform select for the modulator */
            opll->user[8] = (data >> 3) & 1;
    
            /* Waveform select for the carrier */
            opll->user[9] = (data >> 4) & 1;
    
            /* Store feedback level in YM3812 format */
            opll->user[10] = ((data & 0x07) << 1) & 0x0E;
            break;
         }
    
         /* If the user instrument registers were accessed, then
            go through each channel and update the ones that were
            currently using the user instrument. We can skip the
            last three channels in rhythm mode since they can
            only use percussion sounds anyways. */
         if (opll->latch <= 0x05)
         {
            uint8 x;

            for (x = 0; x < 6; x++)
               if (opll->channel[x].instrument == 0x00)
                  load_instrument(x, 0x00, opll->channel[x].volume);
         }
         break;
    
      case 0x10: /* Channel Frequency (LSB) */
      case 0x20: /* Channel Frequency (MSB) + key-on and sustain control */
         {
            uint8 block;
            uint16 frequency;
            uint8 ch = (opll->latch & 0x0F);

            /* Ensure proper channel range */
            if (ch > 0x05)
               break;
 
            /* Get VRC7 channel frequency */
            frequency = ((opll->reg[0x10 + ch] & 0xFF) | ((opll->reg[0x20 + ch] & 0x01) << 8));

            /* Scale 9 bit frequency to 10 bits */
            frequency = (frequency << 1) & 0x1FFF;

            /* Get VRC7 block */
            block = (opll->reg[0x20 + ch] >> 1) & 7;

            /* Add in block */
            frequency |= (block << 10);

            /* Add key-on flag */
            if (opll->reg[0x20 + ch] & 0x10)
               frequency |= 0x2000;

            /* Save current frequency/block/key-on setting */
            opll->channel[ch].frequency = (frequency & 0x3FFF);

            /* Write changes to YM3812 */
            OPL_WRITE(opll, 0xA0 + ch, (opll->channel[ch].frequency >> 0) & 0xFF);
            OPL_WRITE(opll, 0xB0 + ch, (opll->channel[ch].frequency >> 8) & 0xFF);
         }
         break;
    
      case 0x30: /* Channel Volume Level and Instrument Select */

         /* Ensure proper channel range */
         if (opll->latch > 0x35) 
            break;
         
         {
            uint8 ch = (opll->latch & 0x0F);
            uint8 inst = (data >> 4) & 0x0F;
            uint8 vol = (data & 0x0F) << 2;
            load_instrument(ch, inst, vol);
         }

         break;
      }
   }
   else /* Register latch */
   {
      opll->latch = (data & 0x3F);
   }
}

static int32 vrc7_process(void)
{
   static int sample = 0;

   /* update a large chunk at once */
   if (sample >= buflen)
   {
      sample -= buflen;
      YM3812UpdateOne(vrc7.ym3812, buffer, buflen);
   }

   return (int32) ((int16 *) buffer)[sample++];
}

static apu_memwrite vrc7_memwrite[] =
{
   { 0x9010, 0x9010, vrc7_write },
   { 0x9030, 0x9030, vrc7_write }, 
   {     -1,     -1, NULL }
};

apuext_t vrc7_ext = 
{
   vrc7_init,
   vrc7_shutdown,
   vrc7_reset,
   vrc7_process,
   NULL, /* no reads */
   vrc7_memwrite
};

/*
** $Log: vrc7_snd.c,v $
** Revision 1.5  2000/07/04 04:51:02  matt
** made data types stricter
**
** Revision 1.4  2000/07/03 02:18:53  matt
** much better external module exporting
**
** Revision 1.3  2000/06/20 20:45:09  matt
** minor cleanups
**
** Revision 1.2  2000/06/20 04:06:16  matt
** migrated external sound definition to apu module
**
** Revision 1.1  2000/06/20 00:06:47  matt
** initial revision
**
*/

