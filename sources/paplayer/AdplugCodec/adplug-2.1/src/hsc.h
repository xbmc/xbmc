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
 * hsc.h - HSC Player by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_HSCPLAYER
#define H_ADPLUG_HSCPLAYER

#include "player.h"

class ChscPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  ChscPlayer(Copl *newopl): CPlayer(newopl), mtkmode(0) {}

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);
  float getrefresh() { return 18.2f; };	// refresh rate is fixed at 18.2Hz

  std::string gettype() { return std::string("HSC Adlib Composer / HSC-Tracker"); }
  unsigned int getpatterns();
  unsigned int getpattern() { return song[songpos]; }
  unsigned int getorders();
  unsigned int getorder() { return songpos; }
  unsigned int getrow() { return pattpos; }
  unsigned int getspeed() { return speed; }
  unsigned int getinstruments();

 protected:
  struct hscnote {
    unsigned char note, effect;
  };			// note type in HSC pattern

  struct hscchan {
    unsigned char inst;			// current instrument
    signed char slide;			// used for manual slide-effects
    unsigned short freq;		// actual replaying frequency
  };			// HSC channel data

  hscchan channel[9];				// player channel-info
  unsigned char instr[128][12];	// instrument data
  unsigned char song[0x80];		// song-arrangement (MPU-401 Trakker enhanced)
  hscnote patterns[50][64*9];		// pattern data
  unsigned char pattpos,songpos,	// various bytes & flags
    pattbreak,songend,mode6,bd,fadein;
  unsigned int speed,del;
  unsigned char adl_freq[9];		// adlib frequency registers
  int mtkmode;				// flag: MPU-401 Trakker mode on/off

 private:
  void setfreq(unsigned char chan, unsigned short freq);
  void setvolume(unsigned char chan, int volc, int volm);
  void setinstr(unsigned char chan, unsigned char insnr);
};

#endif
