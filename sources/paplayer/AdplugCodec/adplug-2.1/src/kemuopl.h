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
 * kemuopl.h - Emulated OPL using Ken Silverman's emulator, by Simon Peter
 *             <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_KEMUOPL
#define H_ADPLUG_KEMUOPL

#include "opl.h"
extern "C" {
#include "adlibemu.h"
}

class CKemuopl: public Copl
{
public:
  CKemuopl(int rate, bool bit16, bool usestereo)
    : use16bit(bit16), stereo(usestereo)
    {
      adlibinit(rate, usestereo ? 2 : 1, bit16 ? 2 : 1);
      currType = TYPE_OPL2;
    };

  void update(short *buf, int samples)
    {
      if(use16bit) samples *= 2;
      if(stereo) samples *= 2;
      adlibgetsample(buf, samples);
    }

  // template methods
  void write(int reg, int val)
    {
      if(currChip == 0)
	adlib0(reg, val);
    };

  void init() {};

private:
  bool	use16bit,stereo;
};

#endif
