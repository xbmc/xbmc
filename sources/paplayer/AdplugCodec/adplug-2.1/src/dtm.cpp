/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  dtm.cpp - DTM loader by Riven the Mage <riven@ok.ru>
*/
/*
  NOTE: Panning (Ex) effect is ignored.
*/

#include <cstring>
#include "dtm.h"

/* -------- Public Methods -------------------------------- */

CPlayer *CdtmLoader::factory(Copl *newopl)
{
  return new CdtmLoader(newopl);
}

bool CdtmLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  const unsigned char conv_inst[11] = { 2,1,10,9,4,3,6,5,0,8,7 };
  const unsigned short conv_note[12] = { 0x16B, 0x181, 0x198, 0x1B0, 0x1CA, 0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287, 0x2AE };
  int i,j,k,t=0;

  // read header
  f->readString(header.id, 12);
  header.version = f->readInt(1);
  f->readString(header.title, 20); f->readString(header.author, 20);
  header.numpat = f->readInt(1); header.numinst = f->readInt(1);

  // signature exists ? good version ?
  if(memcmp(header.id,"DeFy DTM ",9) || header.version != 0x10)
    { fp.close (f); return false; }

  header.numinst++;

  // load description
  memset(desc,0,80*16);

  char bufstr[80];

  for (i=0;i<16;i++)
    {
      // get line length
      unsigned char bufstr_length = f->readInt(1);

      if(bufstr_length > 80) {
	fp.close(f);
	return false;
      }

      // read line
      if (bufstr_length)
	{
	  f->readString(bufstr,bufstr_length);

	  for (j=0;j<bufstr_length;j++)
	    if (!bufstr[j])
	      bufstr[j] = 0x20;

	  bufstr[bufstr_length] = 0;

	  strcat(desc,bufstr);
	}

      strcat(desc,"\n");
    }

  // init CmodPlayer
  realloc_instruments(header.numinst);
  realloc_order(100);
  realloc_patterns(header.numpat,64,9);
  init_notetable(conv_note);
  init_trackord();

  // load instruments
  for (i=0;i<header.numinst;i++)
    {
      unsigned char name_length = f->readInt(1);

      if (name_length)
	f->readString(instruments[i].name, name_length);

      instruments[i].name[name_length] = 0;

      for(j = 0; j < 12; j++)
	instruments[i].data[j] = f->readInt(1);

      for (j=0;j<11;j++)
	inst[i].data[conv_inst[j]] = instruments[i].data[j];
    }

  // load order
  for(i = 0; i < 100; i++) order[i] = f->readInt(1);

  nop = header.numpat;

  unsigned char *pattern = new unsigned char [0x480];

  // load tracks
  for (i=0;i<nop;i++)
    {
      unsigned short packed_length;

      packed_length = f->readInt(2);

      unsigned char *packed_pattern = new unsigned char [packed_length];

      for(j = 0; j < packed_length; j++)
	packed_pattern[j] = f->readInt(1);

      long unpacked_length = unpack_pattern(packed_pattern,packed_length,pattern,0x480);

      delete [] packed_pattern;

      if (!unpacked_length)
	{
	  delete pattern;
	  fp.close(f);
	  return false;
	}

      // convert pattern
      for (j=0;j<9;j++)
	{
	  for (k=0;k<64;k++)
	    {
	      dtm_event *event = (dtm_event *)&pattern[(k*9+j)*2];

	      // instrument
	      if (event->byte0 == 0x80)
		{
		  if (event->byte1 <= 0x80)
		    tracks[t][k].inst = event->byte1 + 1;
		}

	      // note + effect
	      else
		{
		  tracks[t][k].note = event->byte0;

		  if ((event->byte0 != 0) && (event->byte0 != 127))
		    tracks[t][k].note++;

		  // convert effects
		  switch (event->byte1 >> 4)
		    {
		    case 0x0: // pattern break
		      if ((event->byte1 & 15) == 1)
			tracks[t][k].command = 13;
		      break;

		    case 0x1: // freq. slide up
		      tracks[t][k].command = 28;
		      tracks[t][k].param1 = event->byte1 & 15;
		      break;

		    case 0x2: // freq. slide down
		      tracks[t][k].command = 28;
		      tracks[t][k].param2 = event->byte1 & 15;
		      break;

		    case 0xA: // set carrier volume
		    case 0xC: // set instrument volume
		      tracks[t][k].command = 22;
		      tracks[t][k].param1 = (0x3F - (event->byte1 & 15)) >> 4;
		      tracks[t][k].param2 = (0x3F - (event->byte1 & 15)) & 15;
		      break;

		    case 0xB: // set modulator volume
		      tracks[t][k].command = 21;
		      tracks[t][k].param1 = (0x3F - (event->byte1 & 15)) >> 4;
		      tracks[t][k].param2 = (0x3F - (event->byte1 & 15)) & 15;
		      break;

		    case 0xE: // set panning
		      break;

		    case 0xF: // set speed
		      tracks[t][k].command = 13;
		      tracks[t][k].param2 = event->byte1 & 15;
		      break;
		    }
		}
	    }

	  t++;
	}
    }

  delete [] pattern;
  fp.close(f);

  // order length
  for (i=0;i<100;i++)
    {
      if (order[i] >= 0x80)
	{
	  length = i;

	  if (order[i] == 0xFF)
	    restartpos = 0;
	  else
	    restartpos = order[i] - 0x80;

	  break;
	}
    }

  // initial speed
  initspeed = 2;

  rewind(0);

  return true;
}

void CdtmLoader::rewind(int subsong)
{
  CmodPlayer::rewind(subsong);

  // default instruments
  for (int i=0;i<9;i++)
    {
      channel[i].inst = i;

      channel[i].vol1 = 63 - (inst[i].data[10] & 63);
      channel[i].vol2 = 63 - (inst[i].data[9] & 63);
    }
}

float CdtmLoader::getrefresh()
{
  return 18.2f;
}

std::string CdtmLoader::gettype()
{
  return std::string("DeFy Adlib Tracker");
}

std::string CdtmLoader::gettitle()
{
  return std::string(header.title);
}

std::string CdtmLoader::getauthor()
{
  return std::string(header.author);
}

std::string CdtmLoader::getdesc()
{
  return std::string(desc);
}

std::string CdtmLoader::getinstrument(unsigned int n)
{
  return std::string(instruments[n].name);
}

unsigned int CdtmLoader::getinstruments()
{
  return header.numinst;
}

/* -------- Private Methods ------------------------------- */

long CdtmLoader::unpack_pattern(unsigned char *ibuf, long ilen, unsigned char *obuf, long olen)
{
  unsigned char *input = ibuf;
  unsigned char *output = obuf;

  long input_length = 0;
  long output_length = 0;

  unsigned char repeat_byte, repeat_counter;

  // RLE
  while (input_length < ilen)
    {
      repeat_byte = input[input_length++];

      if ((repeat_byte & 0xF0) == 0xD0)
	{
	  repeat_counter = repeat_byte & 15;
	  repeat_byte = input[input_length++];
	}
      else
	repeat_counter = 1;

      for (int i=0;i<repeat_counter;i++)
	{
	  if (output_length < olen)
	    output[output_length++] = repeat_byte;
	}
    }

  return output_length;
}
