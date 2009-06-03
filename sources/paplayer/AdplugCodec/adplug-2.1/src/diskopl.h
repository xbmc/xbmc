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
 * diskopl.h - Disk Writer OPL, by Simon Peter <dn.tlp@gmx.net>
 */

#include <string>
#include <stdio.h>
#include "opl.h"
#include "player.h"

class CDiskopl: public Copl
{
 public:
  CDiskopl(std::string filename);
  virtual ~CDiskopl();

  void update(CPlayer *p);			// write to file
  void setnowrite(bool nw = true)		// set file write status
    { nowrite = nw; };

  void setchip(int n);

  // template methods
  void write(int reg, int val);
  void init();

 private:
  static const unsigned char	op_table[9];

  FILE		*f;
  float		old_freq;
  unsigned char	del;
  bool		nowrite;			// don't write to file, if true

  void diskwrite(int reg, int val);
};
