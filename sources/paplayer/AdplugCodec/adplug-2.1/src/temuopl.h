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
 * temuopl.h - Tatsuyuki Satoh's OPL2 emulator, by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_TEMUOPL
#define H_ADPLUG_TEMUOPL

#include "opl.h"
extern "C" {
#include "fmopl.h"
}

class CTemuopl: public Copl
{
 public:
  CTemuopl(int rate, bool bit16, bool usestereo);	// rate = sample rate
  virtual ~CTemuopl();

  void update(short *buf, int samples);	// fill buffer

  // template methods
  void write(int reg, int val);
  void init();

 private:
  bool		use16bit,stereo;
  FM_OPL	*opl;			// holds emulator data
};

#endif
