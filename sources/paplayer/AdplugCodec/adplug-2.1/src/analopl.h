/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * analopl.h - Spectrum analyzing hardware OPL, by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_ANALOPL
#define H_ADPLUG_ANALOPL

#include "realopl.h"

class CAnalopl: public CRealopl
{
 public:
  CAnalopl(unsigned short initport = DFL_ADLIBPORT);	// initport = OPL2 hardware baseport

  // get carrier volume of adlib voice v on chip c
  int getcarriervol(unsigned int v, unsigned int c = 0);
  // get modulator volume of adlib voice v on chip c
  int getmodulatorvol(unsigned int v, unsigned int c = 0);
  bool getkeyon(unsigned int v, unsigned int c = 0);

  void write(int reg, int val);

 protected:
  unsigned char	keyregs[2][9][2];		// shadow key register
};

#endif
