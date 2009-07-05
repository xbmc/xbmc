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
 * [xad] PSI player, by Riven the Mage <riven@ok.ru>
 */

#include "xad.h"

class CxadpsiPlayer: public CxadPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

  CxadpsiPlayer(Copl *newopl): CxadPlayer(newopl)
    { }

protected:
  struct psi_header
  {
    unsigned short  instr_ptr;
    unsigned short  seq_ptr;
  } header;

  struct
  {
    unsigned char   *instr_table;
    unsigned char   *seq_table;
    unsigned char   note_delay[9];
    unsigned char   note_curdelay[9];
    unsigned char   looping[9];
  } psi;
  //
  bool		  xadplayer_load()
    {
      if(xad.fmt == PSI)
	return true;
      else
	return false;
    }
  void            xadplayer_rewind(int subsong);
  void            xadplayer_update();
  float           xadplayer_getrefresh();
  std::string     xadplayer_gettype();
  unsigned int    xadplayer_getinstruments();

private:
  static const unsigned char psi_adlib_registers[99];
  static const unsigned short psi_notes[16];
};
