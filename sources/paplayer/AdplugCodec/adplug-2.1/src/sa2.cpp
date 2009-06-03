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
 * sa2.cpp - SAdT2 Loader by Simon Peter <dn.tlp@gmx.net>
 *           SAdT Loader by Mamiya <mamiya@users.sourceforge.net>
 */

#include <stdio.h>

#include "sa2.h"
#include "debug.h"

CPlayer *Csa2Loader::factory(Copl *newopl)
{
  return new Csa2Loader(newopl);
}

bool Csa2Loader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  struct {
    unsigned char data[11],arpstart,arpspeed,arppos,arpspdcnt;
  } insts;
  unsigned char buf;
  int i,j, k, notedis = 0;
  const unsigned char convfx[16] = {0,1,2,3,4,5,6,255,8,255,10,11,12,13,255,15};
  unsigned char sat_type;
  enum SAT_TYPE {
    HAS_ARPEGIOLIST = (1 << 7),
    HAS_V7PATTERNS = (1 << 6),
    HAS_ACTIVECHANNELS = (1 << 5),
    HAS_TRACKORDER = (1 << 4),
    HAS_ARPEGIO = (1 << 3),
    HAS_OLDBPM = (1 << 2),
    HAS_OLDPATTERNS = (1 << 1),
    HAS_UNKNOWN127 = (1 << 0)
  };

  // read header
  f->readString(header.sadt, 4);
  header.version = f->readInt(1);

  // file validation section
  if(strncmp(header.sadt,"SAdT",4)) { fp.close(f); return false; }
  switch(header.version) {
  case 1:
    notedis = +0x18;
    sat_type = HAS_UNKNOWN127 | HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 2:
    notedis = +0x18;
    sat_type = HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 3:
    notedis = +0x0c;
    sat_type = HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 4:
    notedis = +0x0c;
    sat_type = HAS_ARPEGIO | HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 5:
    notedis = +0x0c;
    sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 6:
    sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_OLDPATTERNS | HAS_OLDBPM;
    break;
  case 7:
    sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_V7PATTERNS;
    break;
  case 8:
    sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_TRACKORDER;
    break;
  case 9:
    sat_type = HAS_ARPEGIO | HAS_ARPEGIOLIST | HAS_TRACKORDER | HAS_ACTIVECHANNELS;
    break;
  default:	/* unknown */
    fp.close(f);
    return false;
  }

  // load section
  // instruments
  for(i = 0; i < 31; i++) {
    if(sat_type & HAS_ARPEGIO) {
      for(j = 0; j < 11; j++) insts.data[j] = f->readInt(1);
      insts.arpstart = f->readInt(1);
      insts.arpspeed = f->readInt(1);
      insts.arppos = f->readInt(1);
      insts.arpspdcnt = f->readInt(1);
      inst[i].arpstart = insts.arpstart;
      inst[i].arpspeed = insts.arpspeed;
      inst[i].arppos = insts.arppos;
      inst[i].arpspdcnt = insts.arpspdcnt;
    } else {
      for(j = 0; j < 11; j++) insts.data[j] = f->readInt(1);
      inst[i].arpstart = 0;
      inst[i].arpspeed = 0;
      inst[i].arppos = 0;
      inst[i].arpspdcnt = 0;
    }
    for(j=0;j<11;j++)
      inst[i].data[j] = insts.data[j];
    inst[i].misc = 0;
    inst[i].slide = 0;
  }

  // instrument names
  for(i = 0; i < 29; i++) f->readString(instname[i], 17);

  f->ignore(3);		// dummy bytes
  for(i = 0; i < 128; i++) order[i] = f->readInt(1);	// pattern orders
  if(sat_type & HAS_UNKNOWN127) f->ignore(127);

  // infos
  nop = f->readInt(2); length = f->readInt(1); restartpos = f->readInt(1);

  // bpm
  bpm = f->readInt(2);
  if(sat_type & HAS_OLDBPM) {
    bpm = bpm * 125 / 50;		// cps -> bpm
  }

  if(sat_type & HAS_ARPEGIOLIST) {
    init_specialarp();
    for(i = 0; i < 256; i++) arplist[i] = f->readInt(1);	// arpeggio list
    for(i = 0; i < 256; i++) arpcmd[i] = f->readInt(1);	// arpeggio commands
  }

  for(i=0;i<64;i++) {				// track orders
    for(j=0;j<9;j++) {
      if(sat_type & HAS_TRACKORDER)
	trackord[i][j] = f->readInt(1);
      else
	{
	  trackord[i][j] = i * 9 + j;
	}
    }
  }

  if(sat_type & HAS_ACTIVECHANNELS)
    activechan = f->readInt(2) << 16;		// active channels

  AdPlug_LogWrite("Csa2Loader::load(\"%s\"): sat_type = %x, nop = %d, "
		  "length = %d, restartpos = %d, activechan = %x, bpm = %d\n",
		  filename.c_str(), sat_type, nop, length, restartpos, activechan, bpm);

  // track data
  if(sat_type & HAS_OLDPATTERNS) {
    i = 0;
    while(!f->ateof()) {
      for(j=0;j<64;j++) {
	for(k=0;k<9;k++) {
	  buf = f->readInt(1);
	  tracks[i+k][j].note = buf ? (buf + notedis) : 0;
	  tracks[i+k][j].inst = f->readInt(1);
	  tracks[i+k][j].command = convfx[f->readInt(1) & 0xf];
	  tracks[i+k][j].param1 = f->readInt(1);
	  tracks[i+k][j].param2 = f->readInt(1);
	}
      }
      i+=9;
    }
  } else
    if(sat_type & HAS_V7PATTERNS) {
      i = 0;
      while(!f->ateof()) {
	for(j=0;j<64;j++) {
	  for(k=0;k<9;k++) {
	    buf = f->readInt(1);
	    tracks[i+k][j].note = buf >> 1;
	    tracks[i+k][j].inst = (buf & 1) << 4;
	    buf = f->readInt(1);
	    tracks[i+k][j].inst += buf >> 4;
	    tracks[i+k][j].command = convfx[buf & 0x0f];
	    buf = f->readInt(1);
	    tracks[i+k][j].param1 = buf >> 4;
	    tracks[i+k][j].param2 = buf & 0x0f;
	  }
	}
	i+=9;
      }
    } else {
      i = 0;
      while(!f->ateof()) {
	for(j=0;j<64;j++) {
	  buf = f->readInt(1);
	  tracks[i][j].note = buf >> 1;
	  tracks[i][j].inst = (buf & 1) << 4;
	  buf = f->readInt(1);
	  tracks[i][j].inst += buf >> 4;
	  tracks[i][j].command = convfx[buf & 0x0f];
	  buf = f->readInt(1);
	  tracks[i][j].param1 = buf >> 4;
	  tracks[i][j].param2 = buf & 0x0f;
	}
	i++;
      }
    }
  fp.close(f);

  // fix instrument names
  for(i=0;i<29;i++)
    for(j=0;j<17;j++)
      if(!instname[i][j])
	instname[i][j] = ' ';

  rewind(0);		// rewind module
  return true;
}

std::string Csa2Loader::gettype()
{
  char tmpstr[40];

  sprintf(tmpstr,"Surprise! Adlib Tracker 2 (version %d)",header.version);
  return std::string(tmpstr);
}

std::string Csa2Loader::gettitle()
{
  char bufinst[29*17],buf[18];
  int i,ptr;

  // parse instrument names for song name
  memset(bufinst,'\0',29*17);
  for(i=0;i<29;i++) {
    buf[16] = ' '; buf[17] = '\0';
    memcpy(buf,instname[i]+1,16);
    for(ptr=16;ptr>0;ptr--)
      if(buf[ptr] == ' ')
	buf[ptr] = '\0';
      else {
	if(ptr<16)
	  buf[ptr+1] = ' ';
	break;
      }
    strcat(bufinst,buf);
  }

  if(strchr(bufinst,'"'))
    return std::string(bufinst,strchr(bufinst,'"')-bufinst+1,strrchr(bufinst,'"')-strchr(bufinst,'"')-1);
  else
    return std::string();
}
