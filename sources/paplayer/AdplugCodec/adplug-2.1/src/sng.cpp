/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2002 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * sng.cpp - SNG Player by Simon Peter <dn.tlp@gmx.net>
 */

#include <cstring>
#include "sng.h"

CPlayer *CsngPlayer::factory(Copl *newopl)
{
  return new CsngPlayer(newopl);
}

bool CsngPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  int i;

  // load header
  f->readString(header.id, 4);
  header.length = f->readInt(2); header.start = f->readInt(2);
  header.loop = f->readInt(2); header.delay = f->readInt(1);
  header.compressed = f->readInt(1) ? true : false;

  // file validation section
  if(strncmp(header.id,"ObsM",4)) { fp.close(f); return false; }

  // load section
  header.length /= 2; header.start /= 2; header.loop /= 2;
  data = new Sdata [header.length];
  for(i = 0; i < header.length; i++) {
    data[i].val = f->readInt(1);
    data[i].reg = f->readInt(1);
  }

  rewind(0);
  fp.close(f);
  return true;
}

bool CsngPlayer::update()
{
  if(header.compressed && del) {
    del--;
    return !songend;
  }

  while(data[pos].reg) {
    opl->write(data[pos].reg, data[pos].val);
    pos++;
    if(pos >= header.length) {
      songend = true;
      pos = header.loop;
    }
  }

  if(!header.compressed)
    opl->write(data[pos].reg, data[pos].val);

  if(data[pos].val) del = data[pos].val - 1; pos++;
  if(pos >= header.length) { songend = true; pos = header.loop; }
  return !songend;
}

void CsngPlayer::rewind(int subsong)
{
  pos = header.start; del = header.delay; songend = false;
  opl->init(); opl->write(1,32);	// go to OPL2 mode
}
