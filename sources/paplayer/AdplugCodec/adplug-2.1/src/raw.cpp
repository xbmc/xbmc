/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * raw.c - RAW Player by Simon Peter <dn.tlp@gmx.net>
 */

#include <cstring>
#include "raw.h"

/*** public methods *************************************/

CPlayer *CrawPlayer::factory(Copl *newopl)
{
  return new CrawPlayer(newopl);
}

bool CrawPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  char id[8];
  unsigned long i;

  // file validation section
  f->readString(id, 8);
  if(strncmp(id,"RAWADATA",8)) { fp.close (f); return false; }

  // load section
  clock = f->readInt(2);	// clock speed
  length = (fp.filesize(f) - 10) / 2;
  data = new Tdata [length];
  for(i = 0; i < length; i++) {
    data[i].param = f->readInt(1);
    data[i].command = f->readInt(1);
  }

  fp.close(f);
  rewind(0);
  return true;
}

bool CrawPlayer::update()
{
  bool	setspeed;

  if(pos >= length) return false;

  if(del) {
    del--;
    return !songend;
  }

  do {
    setspeed = false;
    switch(data[pos].command) {
    case 0: del = data[pos].param - 1; break;
    case 2:
      if(!data[pos].param) {
	pos++;
	speed = data[pos].param + (data[pos].command << 8);
	setspeed = true;
      } else
	opl->setchip(data[pos].param - 1);
      break;
    case 0xff:
      if(data[pos].param == 0xff) {
	rewind(0);		// auto-rewind song
	songend = true;
	return !songend;
      }
      break;
    default:
      opl->write(data[pos].command,data[pos].param);
      break;
    }
  } while(data[pos++].command || setspeed);

  return !songend;
}

void CrawPlayer::rewind(int subsong)
{
  pos = del = 0; speed = clock; songend = false;
  opl->init(); opl->write(1, 32);	// go to 9 channel mode
}

float CrawPlayer::getrefresh()
{
  return 1193180.0 / (speed ? speed : 0xffff);	// timer oscillator speed / wait register = clock frequency
}
