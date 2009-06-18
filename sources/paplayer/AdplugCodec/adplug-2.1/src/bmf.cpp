/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003, 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * [xad] BMF player, by Riven the Mage <riven@ok.ru>
 */

/*
    - discovery -

  file(s) : GAMESNET.COM
     type : GamesNet advertising intro
     tune : by (?)The Brain [Razor 1911]
   player : ver.0.9b by Hammer

  file(s) : 2FAST4U.COM
     type : Ford Knox BBStro
     tune : by The Brain [Razor 1911]
   player : ver.1.1 by ?
  comment : in original player at 9th channel the feedback adlib register is not C8 but C6.

  file(s) : DATURA.COM
     type : Datura BBStro
     tune : by The Brain [Razor 1911]
   player : ver.1.2 by ?
  comment : inaccurate replaying, because constant outport; in original player it can be 380 or 382.
*/

#include <cstring>
#include "bmf.h"
#include "debug.h"

const unsigned char CxadbmfPlayer::bmf_adlib_registers[117] =
{
  0x20, 0x23, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83, 0xA0, 0xB0, 0xC0, 0xE0, 0xE3,
  0x21, 0x24, 0x41, 0x44, 0x61, 0x64, 0x81, 0x84, 0xA1, 0xB1, 0xC1, 0xE1, 0xE4,
  0x22, 0x25, 0x42, 0x45, 0x62, 0x65, 0x82, 0x85, 0xA2, 0xB2, 0xC2, 0xE2, 0xE5,
  0x28, 0x2B, 0x48, 0x4B, 0x68, 0x6B, 0x88, 0x8B, 0xA3, 0xB3, 0xC3, 0xE8, 0xEB,
  0x29, 0x2C, 0x49, 0x4C, 0x69, 0x6C, 0x89, 0x8C, 0xA4, 0xB4, 0xC4, 0xE9, 0xEC,
  0x2A, 0x2D, 0x4A, 0x4D, 0x6A, 0x6D, 0x8A, 0x8D, 0xA5, 0xB5, 0xC5, 0xEA, 0xED,
  0x30, 0x33, 0x50, 0x53, 0x70, 0x73, 0x90, 0x93, 0xA6, 0xB6, 0xC6, 0xF0, 0xF3,
  0x31, 0x34, 0x51, 0x54, 0x71, 0x74, 0x91, 0x94, 0xA7, 0xB7, 0xC7, 0xF1, 0xF4,
  0x32, 0x35, 0x52, 0x55, 0x72, 0x75, 0x92, 0x95, 0xA8, 0xB8, 0xC8, 0xF2, 0xF5
};

const unsigned short CxadbmfPlayer::bmf_notes[12] =
{
  0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA, 0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287
};

/* for 1.1 */
const unsigned short CxadbmfPlayer::bmf_notes_2[12] =
{
  0x159, 0x16D, 0x183, 0x19A, 0x1B2, 0x1CC, 0x1E8, 0x205, 0x223, 0x244, 0x267, 0x28B
};

const unsigned char CxadbmfPlayer::bmf_default_instrument[13] =
{
  0x01, 0x01, 0x3F, 0x3F, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00
};

CPlayer *CxadbmfPlayer::factory(Copl *newopl)
{
  return new CxadbmfPlayer(newopl);
}

bool CxadbmfPlayer::xadplayer_load()
{
  unsigned short ptr = 0;
  int i;

  if(xad.fmt != BMF)
    return false;

#ifdef DEBUG
  AdPlug_LogWrite("\nbmf_load():\n\n");
#endif
  if (!strncmp((char *)&tune[0],"BMF1.2",6))
  {
    bmf.version = BMF1_2;
    bmf.timer = 70.0f;
  }
  else if (!strncmp((char *)&tune[0],"BMF1.1",6))
  {
    bmf.version = BMF1_1;
    bmf.timer = 60.0f;
  }
  else
  {
    bmf.version = BMF0_9B;
    bmf.timer = 18.2f;
  }

  // copy title & author
  if (bmf.version > BMF0_9B)
  {
    ptr = 6;

    strncpy(bmf.title,(char *)&tune[ptr],36);

    while (tune[ptr]) { ptr++; }
	ptr++;

    strncpy(bmf.author,(char *)&tune[ptr],36);

    while (tune[ptr]) { ptr++; }
	ptr++;
  }
  else
  {
    strncpy(bmf.title,xad.title,36);
    strncpy(bmf.author,xad.author,36);
  }

  // speed
  if (bmf.version > BMF0_9B)
    bmf.speed = tune[ptr++];
  else
    bmf.speed = ((tune[ptr++] << 8) / 3) >> 8; // strange, yeh ?

  // load instruments
  if (bmf.version > BMF0_9B)
  {
    unsigned long iflags = (tune[ptr] << 24) | (tune[ptr+1] << 16) | (tune[ptr+2] << 8) | tune[ptr+3];
    ptr+=4;

    for(i=0;i<32;i++)
      if (iflags & (1 << (31-i)))
	  {
        strcpy(bmf.instruments[i].name, (char *)&tune[ptr]);
        memcpy(bmf.instruments[i].data, &tune[ptr+11], 13);
        ptr += 24;
	  }
      else
	  {
        bmf.instruments[i].name[0] = 0;
		
        if (bmf.version == BMF1_1)
		  for(int j=0;j<13;j++)
			bmf.instruments[i].data[j] = bmf_default_instrument[j];
        else
		  for(int j=0;j<13;j++)
			bmf.instruments[i].data[j] = 0;
	  }
  }
  else
  {
    ptr = 6;

    for(i=0;i<32;i++)
    {
      bmf.instruments[i].name[0] = 0;
      memcpy(bmf.instruments[tune[ptr]].data, &tune[ptr+2],13); // bug no.1 (no instrument-table-end detection)
      ptr+=15;
    }
  }
  
  // load streams
  if (bmf.version > BMF0_9B)
  {
    unsigned long sflags = (tune[ptr] << 24) | (tune[ptr+1] << 16) | (tune[ptr+2] << 8) | tune[ptr+3];
    ptr+=4;

    for(i=0;i<9;i++)
      if (sflags & (1 << (31-i)))
        ptr+=__bmf_convert_stream(&tune[ptr],i);
      else
        bmf.streams[i][0].cmd = 0xFF;
  }
  else
  {
    for(i=0;i<tune[5];i++)
      ptr+=__bmf_convert_stream(&tune[ptr],i);

	for(i=tune[5];i<9;i++)
      bmf.streams[i][0].cmd = 0xFF;
  }

  return true;
}

void CxadbmfPlayer::xadplayer_rewind(int subsong)
{
  int i,j;

  for(i=0; i<9; i++)
  {
    bmf.channel[i].stream_position = 0;
    bmf.channel[i].delay = 0;
    bmf.channel[i].loop_position = 0;
    bmf.channel[i].loop_counter = 0;
  }

  plr.speed = bmf.speed;
#ifdef DEBUG
  AdPlug_LogWrite("speed: %x\n",plr.speed);
#endif

  bmf.active_streams = 9;

  // OPL initialization
  if (bmf.version > BMF0_9B)
  {
    opl_write(0x01, 0x20);
    
    /* 1.1 */
    if (bmf.version == BMF1_1)
      for(i=0;i<9;i++)
        for(j=0;j<13;j++)
          opl_write(bmf_adlib_registers[13*i+j], bmf_default_instrument[j]);
    /* 1.2 */
    else if (bmf.version == BMF1_2)
      for(i=0x20; i<0x100; i++)
        opl_write(i,0xFF); // very interesting, really!
  }

  /* ALL */

  opl_write(0x08, 0x00);
  opl_write(0xBD, 0xC0);
}

void CxadbmfPlayer::xadplayer_update()
{
  for(int i=0;i<9;i++)
    if (bmf.channel[i].stream_position != 0xFFFF)
    if (bmf.channel[i].delay)
      bmf.channel[i].delay--;
	else
	{
#ifdef DEBUG
   AdPlug_LogWrite("channel %02X:\n", i);
#endif
      bmf_event event;

      // process so-called cross-events
  	  while (true)
	  {
        memcpy(&event, &bmf.streams[i][bmf.channel[i].stream_position], sizeof(bmf_event));
#ifdef DEBUG
   AdPlug_LogWrite("%02X %02X %02X %02X %02X %02X\n",
		   event.note,event.delay,event.volume,event.instrument,
		   event.cmd,event.cmd_data);
#endif

        if (event.cmd == 0xFF)
		{
          bmf.channel[i].stream_position = 0xFFFF;
          bmf.active_streams--;
          break;
		}
        else if (event.cmd == 0xFE)
		{
          bmf.channel[i].loop_position = bmf.channel[i].stream_position+1;
          bmf.channel[i].loop_counter = event.cmd_data;
		}
        else if (event.cmd == 0xFD)
		{
          if (bmf.channel[i].loop_counter)
          {
            bmf.channel[i].stream_position = bmf.channel[i].loop_position-1;
            bmf.channel[i].loop_counter--;
          }
		}
        else
          break;

        bmf.channel[i].stream_position++;
	  } // while (true)

      // process normal event
      unsigned short pos = bmf.channel[i].stream_position;

      if (pos != 0xFFFF)
      {
        bmf.channel[i].delay = bmf.streams[i][pos].delay;

        // command ?
        if (bmf.streams[i][pos].cmd)
		{
          unsigned char cmd = bmf.streams[i][pos].cmd;

          // 0x01: Set Modulator Volume
          if (cmd == 0x01)
		  {
            unsigned char reg = bmf_adlib_registers[13*i+2];

            opl_write(reg, (adlib[reg] | 0x3F) - bmf.streams[i][pos].cmd_data);
		  }
          // 0x10: Set Speed
		  else if (cmd == 0x10)
		  {
            plr.speed = bmf.streams[i][pos].cmd_data;
		    plr.speed_counter = plr.speed;
		  }
		} // if (bmf.streams[i][pos].cmd)

        // instrument ?
        if (bmf.streams[i][pos].instrument)
		{
          unsigned char ins = bmf.streams[i][pos].instrument-1;

          if (bmf.version != BMF1_1)
            opl_write(0xB0+i, adlib[0xB0+i] & 0xDF);

          for(int j=0;j<13;j++)
            opl_write(bmf_adlib_registers[i*13+j], bmf.instruments[ins].data[j]);
		} // if (bmf.streams[i][pos].instrument)

        // volume ?
        if (bmf.streams[i][pos].volume)
		{
          unsigned char vol = bmf.streams[i][pos].volume-1;
          unsigned char reg = bmf_adlib_registers[13*i+3];

          opl_write(reg, (adlib[reg] | 0x3F) - vol);
		} // if (bmf.streams[i][pos].volume)

	    // note ?
        if (bmf.streams[i][pos].note)
		{
          unsigned short note = bmf.streams[i][pos].note;
          unsigned short freq = 0;

          // mute channel
          opl_write(0xB0+i, adlib[0xB0+i] & 0xDF);

          // get frequency
          if (bmf.version == BMF1_1)
          {
            if (note <= 0x60)
              freq = bmf_notes_2[--note % 12];
          }
          else
		  {
            if (note != 0x7F)
              freq = bmf_notes[--note % 12];
		  }

          // play note
		  if (freq)
          {
            opl_write(0xB0+i, (freq >> 8) | ((note / 12) << 2) | 0x20);
            opl_write(0xA0+i, freq & 0xFF);
          }
		} // if (bmf.streams[i][pos].note)

        bmf.channel[i].stream_position++;
      } // if (pos != 0xFFFF)

	} // if (!bmf.channel[i].delay)

  // is module loop ?
  if (!bmf.active_streams)
  {
    for(int j=0;j<9;j++)
      bmf.channel[j].stream_position = 0;

	bmf.active_streams = 9;

    plr.looping = 1;
  }
}

float CxadbmfPlayer::xadplayer_getrefresh()
{
  return bmf.timer;
}

std::string CxadbmfPlayer::xadplayer_gettype()
{
  return std::string("xad: BMF Adlib Tracker");
}

std::string CxadbmfPlayer::xadplayer_gettitle()
{
  return std::string(bmf.title);
}

std::string CxadbmfPlayer::xadplayer_getauthor()
{
  return std::string(bmf.author);
}

unsigned int CxadbmfPlayer::xadplayer_getinstruments()
{
  return 32;
}

std::string CxadbmfPlayer::xadplayer_getinstrument(unsigned int i)
{
  return std::string(bmf.instruments[i].name);
}

/* -------- Internal Functions ---------------------------- */

int CxadbmfPlayer::__bmf_convert_stream(unsigned char *stream, int channel)
{
#ifdef DEBUG
  AdPlug_LogWrite("channel %02X (note,delay,volume,instrument,command,command_data):\n",channel);
  unsigned char *last = stream;
#endif
  unsigned char *stream_start = stream;

  int pos = 0;

  while (true)
  {
    memset(&bmf.streams[channel][pos], 0, sizeof(bmf_event));

    bool is_cmd = false;

    if (*stream == 0xFE)
	{
      // 0xFE -> 0xFF: End of Stream
      bmf.streams[channel][pos].cmd = 0xFF;

	  stream++;

      break;
	}
    else if (*stream == 0xFC)
	{
      // 0xFC -> 0xFE xx: Save Loop Position
      bmf.streams[channel][pos].cmd = 0xFE;
      bmf.streams[channel][pos].cmd_data = (*(stream+1) & ((bmf.version == BMF0_9B) ? 0x7F : 0x3F)) - 1;

	  stream+=2;
    }
    else if (*stream == 0x7D)
	{
      // 0x7D -> 0xFD: Loop Saved Position
      bmf.streams[channel][pos].cmd = 0xFD;

	  stream++;
	}
	else
    {
      if (*stream & 0x80)                           
      {
		if (*(stream+1) & 0x80)
        {
		  if (*(stream+1) & 0x40)
          {
            // byte0: 1aaaaaaa = NOTE
            bmf.streams[channel][pos].note = *stream & 0x7F;
            // byte1: 11bbbbbb = DELAY
            bmf.streams[channel][pos].delay = *(stream+1) & 0x3F;
            // byte2: cccccccc = COMMAND

            stream+=2;

            is_cmd = true;
          }
		  else
          {
            // byte0: 1aaaaaaa = NOTE
            bmf.streams[channel][pos].note = *stream & 0x7F;
            // byte1: 11bbbbbb = DELAY
            bmf.streams[channel][pos].delay = *(stream+1) & 0x3F;

			stream+=2;
          } // if (*(stream+1) & 0x40)
		}
        else
        {
          // byte0: 1aaaaaaa = NOTE
          bmf.streams[channel][pos].note = *stream & 0x7F;
          // byte1: 0bbbbbbb = COMMAND

          stream++;

          is_cmd = true;
        } // if (*(stream+1) & 0x80)
	  }
	  else
      {
        // byte0: 0aaaaaaa = NOTE
        bmf.streams[channel][pos].note = *stream & 0x7F;

		stream++;
      } // if (*stream & 0x80)
    } // if (*stream == 0xFE)

	// is command ?
    if (is_cmd)
    {

      /* ALL */

      if ((0x20 <= *stream) && (*stream <= 0x3F))
      {
        // 0x20 or higher; 0x3F or lower: Set Instrument 
        bmf.streams[channel][pos].instrument = *stream - 0x20 + 1;

		stream++;
      }
      else if (0x40 <= *stream)
      {
        // 0x40 or higher: Set Volume
        bmf.streams[channel][pos].volume = *stream - 0x40 + 1;

		stream++;
      }
      else
      {

        /* 0.9b */

        if (bmf.version == BMF0_9B)
        if (*stream < 0x20)
		{
          // 0x1F or lower: ?
		  stream++;
		}

        /* 1.2 */

        if (bmf.version == BMF1_2)
        if (*stream == 0x01)
		{
          // 0x01: Set Modulator Volume -> 0x01
          bmf.streams[channel][pos].cmd = 0x01;
          bmf.streams[channel][pos].cmd_data = *(stream+1);

		  stream+=2;
		}
        else if (*stream == 0x02)
		{
          // 0x02: ?
		  stream+=2;
		}
        else if (*stream == 0x03)
		{
          // 0x03: ?
		  stream+=2;
		}
        else if (*stream == 0x04)
		{
          // 0x04: Set Speed -> 0x10
          bmf.streams[channel][pos].cmd = 0x10;
          bmf.streams[channel][pos].cmd_data = *(stream+1);

          stream+=2;
		}
        else if (*stream == 0x05)
		{
          // 0x05: Set Carrier Volume (port 380)
          bmf.streams[channel][pos].volume = *(stream+1) + 1;

		  stream+=2;
		}
        else if (*stream == 0x06)
		{
          // 0x06: Set Carrier Volume (port 382)
          bmf.streams[channel][pos].volume = *(stream+1) + 1;

		  stream+=2;
		} // if (bmf.version == BMF1_2)

      } // if ((0x20 <= *stream) && (*stream <= 0x3F))

    } // if (is_cmd)

#ifdef DEBUG
   AdPlug_LogWrite("%02X %02X %02X %02X %02X %02X  <----  ", 
			bmf.streams[channel][pos].note,	
			bmf.streams[channel][pos].delay,
			bmf.streams[channel][pos].volume, 
			bmf.streams[channel][pos].instrument,
			bmf.streams[channel][pos].cmd, 
			bmf.streams[channel][pos].cmd_data
		   );
   for(int zz=0;zz<(stream-last);zz++)
     AdPlug_LogWrite("%02X ",last[zz]);
   AdPlug_LogWrite("\n");
   last=stream;
#endif
    pos++;
  } // while (true)

  return (stream - stream_start);
}
