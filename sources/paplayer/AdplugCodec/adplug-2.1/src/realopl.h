/*
 * AdPlug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * realopl.h - Real hardware OPL, by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_REALOPL
#define H_ADPLUG_REALOPL

#include "opl.h"

#define DFL_ADLIBPORT	0x388		// default adlib baseport

class CRealopl: public Copl
{
 public:	
  CRealopl(unsigned short initport = DFL_ADLIBPORT);	// initport = OPL2 hardware baseport

  bool detect();			// returns true if adlib compatible board is found, else false
  void setvolume(int volume);		// set adlib master volume (0 - 63) 0 = loudest, 63 = softest
  void setquiet(bool quiet = true);	// sets the OPL2 quiet, while still writing to the registers
  void setport(unsigned short port)	// set new OPL2 hardware baseport
    {
      adlport = port;
    }
  void setnowrite(bool nw = true)	// set hardware write status
    {
      nowrite = nw;
    }

  int getvolume()			// get adlib master volume
    {
      return hardvol;
    }

  // template methods
  void write(int reg, int val);
  void init();
  void settype(ChipType type)
    {
      currType = type;
    }

 protected:
  void hardwrite(int reg, int val);		// write to OPL2 hardware registers
  bool harddetect();				// do real hardware detection

  static const unsigned char op_table[9];	// the 9 operators as expected by the OPL2

  unsigned short	adlport;		// adlib hardware baseport
  int			hardvol, oldvol;	// hardware master volume
  bool			bequiet;		// quiet status cache
  char			hardvols[2][22][2];	// volume cache
  bool			nowrite;		// don't write to hardware, if true
};

#endif
