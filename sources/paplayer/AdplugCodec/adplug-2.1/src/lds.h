/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2004 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * lds.h - LOUDNESS Player by Simon Peter <dn.tlp@gmx.net>
 */

#include "player.h"

class CldsPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl) { return new CldsPlayer(newopl); }

  CldsPlayer(Copl *newopl);
  virtual ~CldsPlayer();

  bool load(const std::string &filename, const CFileProvider &fp);
  virtual bool update();
  virtual void rewind(int subsong = -1);
  float getrefresh() { return 70.0f; }

  std::string gettype() { return std::string("LOUDNESS Sound System"); }
  unsigned int getorders() { return numposi; }
  unsigned int getorder() { return posplay; }
  unsigned int getrow() { return pattplay; }
  unsigned int getspeed() { return speed; }
  unsigned int getinstruments() { return numpatch; }

 private:
  typedef struct {
    unsigned char	mod_misc, mod_vol, mod_ad, mod_sr, mod_wave,
      car_misc, car_vol, car_ad, car_sr, car_wave, feedback, keyoff,
      portamento, glide, finetune, vibrato, vibdelay, mod_trem, car_trem,
      tremwait, arpeggio, arp_tab[12];
    unsigned short	start, size;
    unsigned char	fms;
    unsigned short	transp;
    unsigned char	midinst, midvelo, midkey, midtrans, middum1, middum2;
  } SoundBank;

  typedef struct {
    unsigned short	gototune, lasttune, packpos;
    unsigned char	finetune, glideto, portspeed, nextvol, volmod, volcar,
      vibwait, vibspeed, vibrate, trmstay, trmwait, trmspeed, trmrate, trmcount,
      trcwait, trcspeed, trcrate, trccount, arp_size, arp_speed, keycount,
      vibcount, arp_pos, arp_count, packwait, arp_tab[12];

    struct {
      unsigned char	chandelay, sound;
      unsigned short	high;
    } chancheat;
  } Channel;

  typedef struct {
    unsigned short	patnum;
    unsigned char	transpose;
  } Position;

  static const unsigned short	frequency[];
  static const unsigned char	vibtab[], tremtab[];
  static const unsigned short	maxsound, maxpos;

  SoundBank		*soundbank;
  Channel		channel[9];
  Position		*positions;
  unsigned char		fmchip[0xff], jumping, fadeonoff, allvolume, hardfade,
    tempo_now, pattplay, tempo, regbd, chandelay[9], mode, pattlen;
  unsigned short	posplay, jumppos, *patterns, speed;
  bool			playing, songlooped;
  unsigned int		numpatch, numposi, patterns_size, mainvolume;

  void		playsound(int inst_number, int channel_number, int tunehigh);
  inline void	setregs(unsigned char reg, unsigned char val);
  inline void	setregs_adv(unsigned char reg, unsigned char mask,
			    unsigned char val);
};
