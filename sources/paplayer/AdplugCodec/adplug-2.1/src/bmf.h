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
 * [xad] BMF player, by Riven the Mage <riven@ok.ru>
 */

#include "xad.h"

class CxadbmfPlayer: public CxadPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

  CxadbmfPlayer(Copl *newopl): CxadPlayer(newopl)
    { };
  ~CxadbmfPlayer()
    { };

protected:
  enum { BMF0_9B, BMF1_1, BMF1_2 };
  //
  struct bmf_event
  {
    unsigned char   note;
    unsigned char   delay;
    unsigned char   volume;
    unsigned char   instrument;
    unsigned char   cmd;
    unsigned char   cmd_data;
  };

  struct
  {
    unsigned char   version;
    char            title[36];
    char            author[36];
    float           timer;
    unsigned char   speed;
  
    struct
    {
      char            name[11];
      unsigned char   data[13];
    } instruments[32];

    bmf_event       streams[9][1024];

    int             active_streams;

    struct
    {
      unsigned short  stream_position;
      unsigned char   delay;
      unsigned short  loop_position;
      unsigned char   loop_counter;
    } channel[9];
  } bmf;
  //
  bool            xadplayer_load();
  void            xadplayer_rewind(int subsong);
  void            xadplayer_update();
  float           xadplayer_getrefresh();
  std::string     xadplayer_gettype();
  std::string     xadplayer_gettitle();
  std::string     xadplayer_getauthor();
  std::string     xadplayer_getinstrument(unsigned int i);
  unsigned int    xadplayer_getinstruments();
  //
private:
  static const unsigned char bmf_adlib_registers[117];
  static const unsigned short bmf_notes[12];
  static const unsigned short bmf_notes_2[12];
  static const unsigned char bmf_default_instrument[13];

  int             __bmf_convert_stream(unsigned char *stream, int channel);
};
