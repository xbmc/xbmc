/*
 * AdPlug - Replayer for many OPL2/OPL3 audio file formats.
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
 * d00.h - D00 Player by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_D00
#define H_D00

#include "player.h"

class Cd00Player: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  Cd00Player(Copl *newopl)
    : CPlayer(newopl), filedata(0)
    { };
  ~Cd00Player()
    { if(filedata) delete [] filedata; };

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);
  float getrefresh();

  std::string gettype();
  std::string gettitle()
    { if(version > 1) return std::string(header->songname); else return std::string(); };
  std::string getauthor()
    { if(version > 1) return std::string(header->author); else return std::string(); };
  std::string getdesc()
    { if(*datainfo) return std::string(datainfo); else return std::string(); };
  unsigned int getsubsongs();

 protected:
#pragma pack(1)
  struct d00header {
    char id[6];
    unsigned char type,version,speed,subsongs,soundcard;
    char songname[32],author[32],dummy[32];
    unsigned short tpoin,seqptr,instptr,infoptr,spfxptr,endmark;
  };

  struct d00header1 {
    unsigned char version,speed,subsongs;
    unsigned short tpoin,seqptr,instptr,infoptr,lpulptr,endmark;
  };
#pragma pack()

  struct {
    unsigned short	*order,ordpos,pattpos,del,speed,rhcnt,key,freq,inst,
      spfx,ispfx,irhcnt;
    signed short	transpose,slide,slideval,vibspeed;
    unsigned char	seqend,vol,vibdepth,fxdel,modvol,cvol,levpuls,
      frameskip,nextnote,note,ilevpuls,trigger,fxflag;
  } channel[9];

  struct Sinsts {
    unsigned char data[11],tunelev,timer,sr,dummy[2];
  } *inst;

  struct Sspfx {
    unsigned short instnr;
    signed char halfnote;
    unsigned char modlev;
    signed char modlevadd;
    unsigned char duration;
    unsigned short ptr;
  } *spfx;

  struct Slevpuls {
    unsigned char level;
    signed char voladd;
    unsigned char duration,ptr;
  } *levpuls;

  unsigned char songend,version;
  char *datainfo;
  unsigned short *seqptr;
  d00header *header;
  d00header1 *header1;
  char *filedata;

 private:
  void setvolume(unsigned char chan);
  void setfreq(unsigned char chan);
  void setinst(unsigned char chan);
  void playnote(unsigned char chan);
  void vibrato(unsigned char chan);
};

#endif
