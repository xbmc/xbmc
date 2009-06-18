/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2007 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * rad.cpp - RAD Loader by Simon Peter <dn.tlp@gmx.net>
 *
 * BUGS:
 * some volumes are dropped out
 */

#include <cstring>
#include "rad.h"

CPlayer *CradLoader::factory(Copl *newopl)
{
  return new CradLoader(newopl);
}

bool CradLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  char id[16];
  unsigned char buf,ch,c,b,inp;
  char bufstr[2] = "\0";
  unsigned int i,j;
  unsigned short patofs[32];
  const unsigned char convfx[16] = {255,1,2,3,255,5,255,255,255,255,20,255,17,0xd,255,19};

  // file validation section
  f->readString(id, 16); version = f->readInt(1);
  if(strncmp(id,"RAD by REALiTY!!",16) || version != 0x10)
    { fp.close(f); return false; }

  // load section
  radflags = f->readInt(1);
  if(radflags & 128) {	// description
    memset(desc,0,80*22);
    while((buf = f->readInt(1)))
      if(buf == 1)
	strcat(desc,"\n");
      else
	if(buf >= 2 && buf <= 0x1f)
	  for(i=0;i<buf;i++)
	    strcat(desc," ");
	else {
	  *bufstr = buf;
	  strcat(desc,bufstr);
	}
  }
  while((buf = f->readInt(1))) {	// instruments
    buf--;
    inst[buf].data[2] = f->readInt(1); inst[buf].data[1] = f->readInt(1);
    inst[buf].data[10] = f->readInt(1); inst[buf].data[9] = f->readInt(1);
    inst[buf].data[4] = f->readInt(1); inst[buf].data[3] = f->readInt(1);
    inst[buf].data[6] = f->readInt(1); inst[buf].data[5] = f->readInt(1);
    inst[buf].data[0] = f->readInt(1);
    inst[buf].data[8] = f->readInt(1); inst[buf].data[7] = f->readInt(1);
  }
  length = f->readInt(1);
  for(i = 0; i < length; i++) order[i] = f->readInt(1);	// orderlist
  for(i = 0; i < 32; i++) patofs[i] = f->readInt(2);	// pattern offset table
  init_trackord();		// patterns
  for(i=0;i<32;i++)
    if(patofs[i]) {
      f->seek(patofs[i]);
      do {
	buf = f->readInt(1); b = buf & 127;
	do {
	  ch = f->readInt(1); c = ch & 127;
	  inp = f->readInt(1);
	  tracks[i*9+c][b].note = inp & 127;
	  tracks[i*9+c][b].inst = (inp & 128) >> 3;
	  inp = f->readInt(1);
	  tracks[i*9+c][b].inst += inp >> 4;
	  tracks[i*9+c][b].command = inp & 15;
	  if(inp & 15) {
	    inp = f->readInt(1);
	    tracks[i*9+c][b].param1 = inp / 10;
	    tracks[i*9+c][b].param2 = inp % 10;
	  }
	} while(!(ch & 128));
      } while(!(buf & 128));
    } else
      memset(trackord[i],0,9*2);
  fp.close(f);

  // convert replay data
  for(i=0;i<32*9;i++)	// convert patterns
    for(j=0;j<64;j++) {
      if(tracks[i][j].note == 15)
	tracks[i][j].note = 127;
      if(tracks[i][j].note > 16 && tracks[i][j].note < 127)
	tracks[i][j].note -= 4 * (tracks[i][j].note >> 4);
      if(tracks[i][j].note && tracks[i][j].note < 126)
	tracks[i][j].note++;
      tracks[i][j].command = convfx[tracks[i][j].command];
    }
  restartpos = 0; initspeed = radflags & 31;
  bpm = radflags & 64 ? 0 : 50; flags = Decimal;

  rewind(0);
  return true;
}

float CradLoader::getrefresh()
{
  if(tempo)
    return (float) (tempo);
  else
    return 18.2f;
}
