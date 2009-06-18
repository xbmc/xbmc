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
 * xsm.h - eXtra Simple Music Player, by Simon Peter <dn.tlp@gmx.net>
 */

#include "player.h"

class CxsmPlayer: public CPlayer
{
public:
  static CPlayer *factory(Copl *newopl) { return new CxsmPlayer(newopl); }

  CxsmPlayer(Copl *newopl);
  ~CxsmPlayer();

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);
  float getrefresh();

  std::string gettype() { return std::string("eXtra Simple Music"); }

private:
  unsigned short	songlen;
  char			*music;
  unsigned int		last, notenum;
  bool			songend;

  void play_note(int c, int note, int octv);
};
