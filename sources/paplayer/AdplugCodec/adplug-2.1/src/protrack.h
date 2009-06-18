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
 * protrack.h - Generic Protracker Player by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_PROTRACK
#define H_PROTRACK

#include "player.h"

class CmodPlayer: public CPlayer
{
public:
  CmodPlayer(Copl *newopl);
  virtual ~CmodPlayer();

  bool update();
  void rewind(int subsong);
  float getrefresh();

  unsigned int getpatterns()
    { return nop; }
  unsigned int getpattern()
    { return order[ord]; }
  unsigned int getorders()
    { return length; }
  unsigned int getorder()
    { return ord; }
  unsigned int getrow()
    { return rw; }
  unsigned int getspeed()
    { return speed; }

 protected:
  enum Flags {
    Standard = 0,
    Decimal = 1 << 0,
    Faust = 1 << 1,
    NoKeyOn = 1 << 2,
    Opl3 = 1 << 3,
    Tremolo = 1 << 4,
    Vibrato = 1 << 5,
    Percussion = 1 << 6
  };

  struct Instrument {
    unsigned char data[11],arpstart,arpspeed,arppos,arpspdcnt,misc;
    signed char slide;
  } *inst;

  struct Tracks {
    unsigned char note,command,inst,param2,param1;
  } **tracks;

  unsigned char *order, *arplist, *arpcmd, initspeed;
  unsigned short tempo, **trackord, bpm, nop;
  unsigned long length, restartpos, activechan;
  int flags, curchip;

  struct Channel {
    unsigned short freq,nextfreq;
    unsigned char oct,vol1,vol2,inst,fx,info1,info2,key,nextoct,
      note,portainfo,vibinfo1,vibinfo2,arppos,arpspdcnt;
    signed char trigger;
  } *channel;

  void init_trackord();
  bool init_specialarp();
  void init_notetable(const unsigned short *newnotetable);
  bool realloc_order(unsigned long len);
  bool realloc_patterns(unsigned long pats, unsigned long rows, unsigned long chans);
  bool realloc_instruments(unsigned long len);

  void dealloc();

 private:
  static const unsigned short sa2_notetable[12];
  static const unsigned char vibratotab[32];

  unsigned char speed, del, songend, regbd;
  unsigned short rows, notetable[12];
  unsigned long rw, ord, nrows, npats, nchans;

  void setvolume(unsigned char chan);
  void setvolume_alt(unsigned char chan);
  void setfreq(unsigned char chan);
  void playnote(unsigned char chan);
  void setnote(unsigned char chan, int note);
  void slide_down(unsigned char chan, int amount);
  void slide_up(unsigned char chan, int amount);
  void tone_portamento(unsigned char chan, unsigned char info);
  void vibrato(unsigned char chan, unsigned char speed, unsigned char depth);
  void vol_up(unsigned char chan, int amount);
  void vol_down(unsigned char chan, int amount);
  void vol_up_alt(unsigned char chan, int amount);
  void vol_down_alt(unsigned char chan, int amount);

  void dealloc_patterns();
  bool resolve_order();
  unsigned char set_opl_chip(unsigned char chan);
};

#endif
