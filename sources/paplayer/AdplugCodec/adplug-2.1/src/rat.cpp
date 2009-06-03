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
 * [xad] RAT player, by Riven the Mage <riven@ok.ru>
 */

/*
    - discovery -

  file(s) : PINA.EXE
     type : Experimental Connection BBStro tune
     tune : by (?)Ratt/GRIF
   player : by (?)Ratt/GRIF
  comment : there are bug in original replayer's adlib_init(): wrong frequency registers.
*/

#include <cstring>
#include "rat.h"
#include "debug.h"

const unsigned char CxadratPlayer::rat_adlib_bases[18] =
{
  0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12,
  0x03, 0x04, 0x05, 0x0B, 0x0C, 0x0D, 0x13, 0x14, 0x15
};

const unsigned short CxadratPlayer::rat_notes[16] =
{
  0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA, 0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287,
  0x000, 0x000, 0x000, 0x000 // by riven
};

CPlayer *CxadratPlayer::factory(Copl *newopl)
{
  return new CxadratPlayer(newopl);
}

bool CxadratPlayer::xadplayer_load()
{
  if(xad.fmt != RAT)
    return false;

  // load header
  memcpy(&rat.hdr, &tune[0], sizeof(rat_header));

  // is 'RAT'-signed ?
  if (strncmp(rat.hdr.id,"RAT",3))
    return false;

  // is version 1.0 ?
  if (rat.hdr.version != 0x10)
    return false;

  // load order
  rat.order = &tune[0x40];

  // load instruments
  rat.inst = (rat_instrument *)&tune[0x140];

  // load pattern data
  unsigned short patseg = (rat.hdr.patseg[1] << 8) + rat.hdr.patseg[0];
  unsigned char *event_ptr = &tune[patseg << 4];

  for(int i=0;i<rat.hdr.numpat;i++)
    for(int j=0;j<64;j++)
      for(int k=0;k<rat.hdr.numchan;k++)
      {
        memcpy(&rat.tracks[i][j][k], event_ptr, sizeof(rat_event));

        event_ptr += sizeof(rat_event);
      }

  return true;
}

void CxadratPlayer::xadplayer_rewind(int subsong)
{
  int i;

  rat.order_pos = rat.hdr.order_start;
  rat.pattern_pos = 0;
  rat.volume = rat.hdr.volume;

  plr.speed = rat.hdr.speed;

  // clear channel data
  memset(&rat.channel, 0, sizeof(rat.channel[0])*9);

  // init OPL
  opl_write(0x01, 0x20);
  opl_write(0x08, 0x00);
  opl_write(0xBD, 0x00);

  // set default frequencies
  for(i=0;i<9;i++)
  {
    opl_write(0xA0+i, 0x00);
    opl_write(0xA3+i, 0x00);
    opl_write(0xB0+i, 0x00);
    opl_write(0xB3+i, 0x00);
  }

  // set default volumes
  for(i=0;i<0x1F;i++)
    opl_write(0x40+i, 0x3F);
}

void CxadratPlayer::xadplayer_update()
{
  int i;

  rat_event event;

  // process events
  for(i=0;i<rat.hdr.numchan;i++)
  {
    memcpy(&event,&rat.tracks[rat.order[rat.order_pos]][rat.pattern_pos][i],sizeof(rat_event));
#ifdef DEBUG
   AdPlug_LogWrite("order %02X, pattern %02X, row %02X, channel %02X, event %02X %02X %02X %02X %02X:\n",
	         rat.order_pos, rat.order[rat.order_pos], rat.pattern_pos, i, event.note, event.instrument, event.volume, event.fx, event.fxp
           );
#endif

    // is instrument ?
    if (event.instrument != 0xFF)
    {
      rat.channel[i].instrument = event.instrument - 1;
      rat.channel[i].volume = rat.inst[event.instrument - 1].volume;
    }

    // is volume ?
    if (event.volume != 0xFF)
      rat.channel[i].volume = event.volume;

    // is note ?
    if (event.note != 0xFF)
    {
      // mute channel
      opl_write(0xB0+i, 0x00);
      opl_write(0xA0+i, 0x00);

      // if note != 0xFE then play
      if (event.note != 0xFE)
      {
        unsigned char ins = rat.channel[i].instrument;

        // synthesis/feedback
        opl_write(0xC0+i, rat.inst[ins].connect);

        // controls
		opl_write(0x20+rat_adlib_bases[i], rat.inst[ins].mod_ctrl);
        opl_write(0x20+rat_adlib_bases[i+9], rat.inst[ins].car_ctrl);

        // volumes
		opl_write(0x40+rat_adlib_bases[i], __rat_calc_volume(rat.inst[ins].mod_volume,rat.channel[i].volume,rat.volume));
        opl_write(0x40+rat_adlib_bases[i+9], __rat_calc_volume(rat.inst[ins].car_volume,rat.channel[i].volume,rat.volume));

        // attack/decay
		opl_write(0x60+rat_adlib_bases[i], rat.inst[ins].mod_AD);
        opl_write(0x60+rat_adlib_bases[i+9], rat.inst[ins].car_AD);

        // sustain/release
		opl_write(0x80+rat_adlib_bases[i], rat.inst[ins].mod_SR);
        opl_write(0x80+rat_adlib_bases[i+9], rat.inst[ins].car_SR);

        // waveforms
		opl_write(0xE0+rat_adlib_bases[i], rat.inst[ins].mod_wave);
        opl_write(0xE0+rat_adlib_bases[i+9], rat.inst[ins].car_wave);

        // octave/frequency
	unsigned short insfreq = (rat.inst[ins].freq[1] << 8) + rat.inst[ins].freq[0];
        unsigned short freq = insfreq * rat_notes[event.note & 0x0F] / 0x20AB;

        opl_write(0xA0+i, freq & 0xFF);
        opl_write(0xB0+i, (freq >> 8) | ((event.note & 0xF0) >> 2) | 0x20);
      }
    }

    // is effect ?
    if (event.fx != 0xFF)
    {
      rat.channel[i].fx = event.fx;
      rat.channel[i].fxp = event.fxp;
    }
  }

  // next row
  rat.pattern_pos++;

  // process effects
  for(i=0;i<rat.hdr.numchan;i++)
  {
    unsigned char old_order_pos = rat.order_pos;

    switch (rat.channel[i].fx)
    {
      case 0x01: // 0x01: Set Speed
        plr.speed = rat.channel[i].fxp;
        break;
      case 0x02: // 0x02: Position Jump
        if (rat.channel[i].fxp < rat.hdr.order_end)
          rat.order_pos = rat.channel[i].fxp;
        else
          rat.order_pos = 0;

        // jumpback ?
        if (rat.order_pos <= old_order_pos)
          plr.looping = 1;

        rat.pattern_pos = 0;
        break;
      case 0x03: // 0x03: Pattern Break (?)
        rat.pattern_pos = 0x40;
        break;
    }

    rat.channel[i].fx = 0;
  }

  // end of pattern ?
  if (rat.pattern_pos >= 0x40)
  {
    rat.pattern_pos = 0;

    rat.order_pos++;

    // end of module ?
    if (rat.order_pos == rat.hdr.order_end)
    {
      rat.order_pos = rat.hdr.order_loop;

      plr.looping = 1;
    }
  }
}

float CxadratPlayer::xadplayer_getrefresh()
{
  return 60.0f;
}

std::string CxadratPlayer::xadplayer_gettype()
{
  return (std::string("xad: rat player"));
}

std::string CxadratPlayer::xadplayer_gettitle()
{
  return (std::string(rat.hdr.title,32));
}

unsigned int CxadratPlayer::xadplayer_getinstruments()
{
  return rat.hdr.numinst;
}

/* -------- Internal Functions ---------------------------- */

unsigned char CxadratPlayer::__rat_calc_volume(unsigned char ivol, unsigned char cvol, unsigned char gvol)
{
#ifdef DEBUG
   AdPlug_LogWrite("volumes: instrument %02X, channel %02X, global %02X:\n", ivol, cvol, gvol);
#endif
  unsigned short vol;

  vol   =  ivol;
  vol  &=  0x3F;
  vol  ^=  0x3F;
  vol  *=  cvol;
  vol >>=  6;
  vol  *=  gvol;
  vol >>=  6;
  vol  ^=  0x3F;

  vol  |=  ivol & 0xC0;

  return vol;
}
