/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * [xad] HYBRID player, by Riven the Mage <riven@ok.ru>
 */

/*
    - discovery -

  file(s) : HYBRID.EXE
     type : Hybrid cracktro for Apache Longbow CD-RIP
     tune : from 'Mig-29 Super Fulcrum' game by Domark
   player : from 'Mig-29 Super Fulcrum' game by Domark
*/

#include "hybrid.h"
#include "debug.h"

const unsigned char CxadhybridPlayer::hyb_adlib_registers[99] = 
{
  0xE0, 0x60, 0x80, 0x20, 0x40, 0xE3, 0x63, 0x83, 0x23, 0x43, 0xC0,
  0xE1, 0x61, 0x81, 0x21, 0x41, 0xE4, 0x64, 0x84, 0x24, 0x44, 0xC1,
  0xE2, 0x62, 0x82, 0x22, 0x42, 0xE5, 0x65, 0x85, 0x25, 0x45, 0xC2,
  0xE8, 0x68, 0x88, 0x28, 0x48, 0xEB, 0x6B, 0x8B, 0x2B, 0x4B, 0xC3,
  0xE9, 0x69, 0x89, 0x29, 0x49, 0xEC, 0x6C, 0x8C, 0x2C, 0x4C, 0xC4,
  0xEA, 0x6A, 0x8A, 0x2A, 0x4A, 0xED, 0x6D, 0x8D, 0x2D, 0x4D, 0xC5,
  0xF0, 0x70, 0x90, 0x30, 0x50, 0xF3, 0x73, 0x93, 0x33, 0x53, 0xC6,
  0xF1, 0x71, 0x91, 0x31, 0x51, 0xF4, 0x74, 0x94, 0x34, 0x54, 0xC7,
  0xF2, 0x72, 0x92, 0x32, 0x52, 0xF5, 0x75, 0x95, 0x35, 0x55, 0xC8
};

const unsigned short CxadhybridPlayer::hyb_notes[98] =
{
  0x0000, 0x0000,
  0x016B, 0x0181, 0x0198, 0x01B0, 0x01CA, 0x01E5, 0x0202, 0x0220, 0x0241, 0x0263, 0x0287, 0x02AE,
  0x056B, 0x0581, 0x0598, 0x05B0, 0x05CA, 0x05E5, 0x0602, 0x0620, 0x0641, 0x0663, 0x0687, 0x06AE,
  0x096B, 0x0981, 0x0998, 0x09B0, 0x09CA, 0x09E5, 0x0A02, 0x0A20, 0x0A41, 0x0A63, 0x0A87, 0x0AAE,
  0x0D6B, 0x0D81, 0x0D98, 0x0DB0, 0x0DCA, 0x0DE5, 0x0E02, 0x0E20, 0x0E41, 0x0E63, 0x0E87, 0x0EAE,
  0x116B, 0x1181, 0x1198, 0x11B0, 0x11CA, 0x11E5, 0x1202, 0x1220, 0x1241, 0x1263, 0x1287, 0x12AE,
  0x156B, 0x1581, 0x1598, 0x15B0, 0x15CA, 0x15E5, 0x1602, 0x1620, 0x1641, 0x1663, 0x1687, 0x16AE,
  0x196B, 0x1981, 0x1998, 0x19B0, 0x19CA, 0x19E5, 0x1A02, 0x1A20, 0x1A41, 0x1A63, 0x1A87, 0x1AAE,
  0x1D6B, 0x1D81, 0x1D98, 0x1DB0, 0x1DCA, 0x1DE5, 0x1E02, 0x1E20, 0x1E41, 0x1E63, 0x1E87, 0x1EAE
};

const unsigned char CxadhybridPlayer::hyb_default_instrument[11] = 
{
  0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00
};

CPlayer *CxadhybridPlayer::factory(Copl *newopl)
{
  return new CxadhybridPlayer(newopl);
}

bool CxadhybridPlayer::xadplayer_load()
{
  if(xad.fmt != HYBRID)
    return false;

  // load instruments
  hyb.inst = (hyb_instrument *)&tune[0];

  // load order
  hyb.order = &tune[0x1D4];

  return true;
}

void CxadhybridPlayer::xadplayer_rewind(int subsong)
{
  int i;

  hyb.order_pos = 0;
  hyb.pattern_pos = 0;

  hyb.speed = 6;
  hyb.speed_counter = 1;

  plr.speed = 1;

  // init channel data
  for(i=0;i<9;i++)
  {
    hyb.channel[i].freq       = 0x2000;
    hyb.channel[i].freq_slide = 0x0000;
  }

  // basic OPL init
  opl_write(0x01, 0x20);
  opl_write(0xBD, 0x40);
  opl_write(0x08, 0x00);

  // init OPL channels
  for(i=0;i<9;i++)
  {
    for(int j=0;j<11;j++)
      opl_write(hyb_adlib_registers[i*11+j], 0x00 /* hyb_default_instrument[j] */ );

	opl_write(0xA0+i, 0x00);
	opl_write(0xB0+i, 0x20);
  }
}

void CxadhybridPlayer::xadplayer_update()
{
  int i,j;
  unsigned char patpos,ordpos;

  if (--hyb.speed_counter)
    goto update_slides;

  hyb.speed_counter = hyb.speed;

  patpos = hyb.pattern_pos;
  ordpos = hyb.order_pos;

  // process channels
  for(i=0;i<9;i++)
  {
    unsigned char *pos = &tune[0xADE + (hyb.order[hyb.order_pos*9 + i] * 64 * 2) + (patpos * 2)];
    // read event
    unsigned short event = (pos[1] << 8) + pos[0];

#ifdef DEBUG
   AdPlug_LogWrite("track %02X, channel %02X, event %04X:\n", hyb.order[hyb.order_pos*9 + i], i, event );
#endif

    // calculate variables
	unsigned char  note  =   event >> 9;
	unsigned char  ins   = ((event & 0x01F0) >> 4);
    unsigned char  slide =   event & 0x000F;

    // play event
    switch(note)
    {
      case 0x7D: // 0x7D: Set Speed
        hyb.speed = event & 0xFF;
        break;
      case 0x7E: // 0x7E: Jump Position
        hyb.order_pos = event & 0xFF;
        hyb.pattern_pos = 0x3F;

        // jumpback ?
        if (hyb.order_pos <= ordpos)
          plr.looping = 1;

        break;
      case 0x7F: // 0x7F: Pattern Break
        hyb.pattern_pos = 0x3F;
        break;
      default:

        // is instrument ?
        if (ins)
          for(j=0;j<11;j++)
            opl_write(hyb_adlib_registers[i*11+j], *((unsigned char *)&hyb.inst[ins-1] + 7 + j)); // +7 = skip name...

        // is note ?
        if (note)
        {
          hyb.channel[i].freq = hyb_notes[note];
          hyb.channel[i].freq_slide = 0;
        }

        // is slide ?
        if (slide)
        {
          hyb.channel[i].freq_slide = (((slide >> 3) * -1) * (slide & 7)) << 1;
    
          if (slide & 0x80)
            slide = -(slide & 7);
        }

        // set frequency
        if (!(hyb.channel[i].freq & 0x2000))
        {
          opl_write(0xA0+i, hyb.channel[i].freq & 0xFF);
          opl_write(0xB0+i, hyb.channel[i].freq >> 8);

          hyb.channel[i].freq |= 0x2000;

          opl_write(0xA0+i, hyb.channel[i].freq & 0xFF);
          opl_write(0xB0+i, hyb.channel[i].freq >> 8);
        }

        break;
    }
  }

  hyb.pattern_pos++;

  // end of pattern ?
  if (hyb.pattern_pos >= 0x40)
  {
    hyb.pattern_pos = 0;

    hyb.order_pos++;
  }

update_slides:
#ifdef DEBUG
   AdPlug_LogWrite("slides:\n");
#endif
  // update fine frequency slides
  for(i=0;i<9;i++)
    if (hyb.channel[i].freq_slide)
    {
      hyb.channel[i].freq = (((hyb.channel[i].freq & 0x1FFF) + hyb.channel[i].freq_slide) & 0x1FFF) | 0x2000;

      opl_write(0xA0+i, hyb.channel[i].freq & 0xFF);
      opl_write(0xB0+i, hyb.channel[i].freq >> 8);
    }
}

float CxadhybridPlayer::xadplayer_getrefresh()
{
  return 50.0f;
}

std::string CxadhybridPlayer::xadplayer_gettype()
{
  return (std::string("xad: hybrid player"));
}

std::string CxadhybridPlayer::xadplayer_getinstrument(unsigned int i)
{
  return (std::string(hyb.inst[i].name,7));
}

unsigned int CxadhybridPlayer::xadplayer_getinstruments()
{
  return 26;
}
