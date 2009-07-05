/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * s3m.h - AdLib S3M Player by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_S3M
#define H_ADPLUG_S3M

#include "player.h"

class Cs3mPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  Cs3mPlayer(Copl *newopl);

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);
  float getrefresh();

  std::string gettype();
  std::string gettitle()
    { return std::string(header.name); };

  unsigned int getpatterns()
    { return header.patnum; };
  unsigned int getpattern()
    { return orders[ord]; };
  unsigned int getorders()
    { return (header.ordnum-1); };
  unsigned int getorder()
    { return ord; };
  unsigned int getrow()
    { return crow; };
  unsigned int getspeed()
    { return speed; };
  unsigned int getinstruments()
    { return header.insnum; };
  std::string getinstrument(unsigned int n)
    { return std::string(inst[n].name); };

 protected:
  struct s3mheader {
    char name[28];				// song name
    unsigned char kennung,typ,dummy[2];
    unsigned short ordnum,insnum,patnum,flags,cwtv,ffi;
    char scrm[4];
    unsigned char gv,is,it,mv,uc,dp,dummy2[8];
    unsigned short special;
    unsigned char chanset[32];
  };

  struct s3minst {
    unsigned char type;
    char filename[15];
    unsigned char d00,d01,d02,d03,d04,d05,d06,d07,d08,d09,d0a,d0b,volume,dsk,dummy[2];
    unsigned long c2spd;
    char dummy2[12], name[28],scri[4];
  } inst[99];

  struct {
    unsigned char note,oct,instrument,volume,command,info;
  } pattern[99][64][32];

  struct {
    unsigned short freq,nextfreq;
    unsigned char oct,vol,inst,fx,info,dualinfo,key,nextoct,trigger,note;
  } channel[9];

  s3mheader header;
  unsigned char orders[256];
  unsigned char crow,ord,speed,tempo,del,songend,loopstart,loopcnt;

 private:
  static const char chnresolv[];
  static const unsigned short notetable[12];
  static const unsigned char vibratotab[32];

  void load_header(binistream *f, s3mheader *h);
  void setvolume(unsigned char chan);
  void setfreq(unsigned char chan);
  void playnote(unsigned char chan);
  void slide_down(unsigned char chan, unsigned char amount);
  void slide_up(unsigned char chan, unsigned char amount);
  void vibrato(unsigned char chan, unsigned char info);
  void tone_portamento(unsigned char chan, unsigned char info);
};

#endif
