/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * analopl.cpp - Spectrum analyzing hardware OPL, by Simon Peter <dn.tlp@gmx.net>
 */

#include "analopl.h"

CAnalopl::CAnalopl(unsigned short initport)
  : CRealopl(initport)
{
  for(int i = 0; i < 9; i++) {
    keyregs[0][i][0] = 0;
    keyregs[0][i][1] = 0;
    keyregs[1][i][0] = 0;
    keyregs[1][i][1] = 0;
  }
}

void CAnalopl::write(int reg, int val)
{
  if(nowrite) return;

  if(reg >= 0xb0 && reg <= 0xb8) {
    if(!keyregs[currChip][reg - 0xb0][0] && (val & 32))
      keyregs[currChip][reg - 0xb0][1] = 1;
    else
      keyregs[currChip][reg - 0xb0][1] = 0;
    keyregs[currChip][reg - 0xb0][0] = val & 32;
  }

  CRealopl::write(reg, val);
}

int CAnalopl::getcarriervol(unsigned int v, unsigned int c)
{
  return (hardvols[c][op_table[v]+3][0] & 63);
}

int CAnalopl::getmodulatorvol(unsigned int v, unsigned int c)
{
  return (hardvols[c][op_table[v]][0] & 63);
}

bool CAnalopl::getkeyon(unsigned int v, unsigned int c)
{
  if(keyregs[c][v][1]) {
    keyregs[c][v][1] = 0;
    return true;
  } else
    return false;
}
