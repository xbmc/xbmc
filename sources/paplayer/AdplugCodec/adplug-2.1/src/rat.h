/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * [xad] RAT player, by Riven the Mage <riven@ok.ru>
 */

#include "xad.h"

class CxadratPlayer: public CxadPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

  CxadratPlayer(Copl *newopl): CxadPlayer(newopl)
    { }

protected:
  struct rat_header
  {
    char            id[3];
    unsigned char   version;
    char            title[32];
    unsigned char   numchan;
    unsigned char   reserved_25;
    unsigned char   order_end;
    unsigned char   reserved_27;
    unsigned char   numinst;              // ?: Number of Instruments
    unsigned char   reserved_29;
    unsigned char   numpat;               // ?: Number of Patterns
    unsigned char   reserved_2B;
    unsigned char   order_start;
    unsigned char   reserved_2D;
    unsigned char   order_loop;
    unsigned char   reserved_2F;
    unsigned char   volume;
    unsigned char   speed;
    unsigned char   reserved_32[12];
    unsigned char   patseg[2];
  };

  struct rat_event
  {
    unsigned char   note;
    unsigned char   instrument;
    unsigned char   volume;
    unsigned char   fx;
    unsigned char   fxp;
  };

  struct rat_instrument
  {
    unsigned char   freq[2];
    unsigned char   reserved_2[2];
    unsigned char   mod_ctrl;
    unsigned char   car_ctrl;
    unsigned char   mod_volume;   
    unsigned char   car_volume;   
    unsigned char   mod_AD;       
    unsigned char   car_AD;       
    unsigned char   mod_SR;       
    unsigned char   car_SR;       
    unsigned char   mod_wave;
    unsigned char   car_wave;
    unsigned char   connect;
    unsigned char   reserved_F;
    unsigned char   volume;
    unsigned char   reserved_11[3];
  };

  struct
  {
    rat_header      hdr;

    unsigned char   volume;
    unsigned char   order_pos;
    unsigned char   pattern_pos;

    unsigned char   *order;

    rat_instrument  *inst;

    rat_event       tracks[256][64][9];

    struct
    {
      unsigned char   instrument;
      unsigned char   volume;
      unsigned char   fx;
      unsigned char   fxp;
    } channel[9];
  } rat;
  //
  bool            xadplayer_load();
  void            xadplayer_rewind(int subsong);
  void            xadplayer_update();
  float           xadplayer_getrefresh();
  std::string	    xadplayer_gettype();
  std::string     xadplayer_gettitle();
  unsigned int    xadplayer_getinstruments();
  //
private:
  static const unsigned char rat_adlib_bases[18];
  static const unsigned short rat_notes[16];

  unsigned char   __rat_calc_volume(unsigned char ivol, unsigned char cvol, unsigned char gvol);
};
