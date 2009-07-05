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
 * emutest.cpp - Test AdPlug emulators, by Simon Peter <dn.tlp@gmx.net>
 */

#include <stdlib.h>
#include <stdio.h>

#include "../src/emuopl.h"

/***** Local variables *****/

// String holding the relative path to the source directory
static char *srcdir;

/***** Local functions *****/

#define BUF_SIZE	1024

static bool check_emu_output(CEmuopl *emu)
  /*
   * Test if the emulator produces any output.
   */
{
  short	*buf = (short *)calloc(BUF_SIZE, sizeof(short));
  bool	nonull = false, no10k = true, nom10k = true;

  // init emulator
  emu->init();
  emu->write(1, 5 << 1);

  // set test instrument
  emu->write(0x20, 1);
  emu->write(0x40, 0x10);
  emu->write(0x60, 0xf0);
  emu->write(0x80, 0x77);
  emu->write(0xa0, 0x98);
  emu->write(0x23, 1);
  emu->write(0x43, 0);
  emu->write(0x63, 0xf0);
  emu->write(0x83, 0x77);
  emu->write(0xb0, 0x31);

  // check output from emu
  emu->update(buf, BUF_SIZE);
  for(int i = 0; i < BUF_SIZE; i++) {
    if(buf[i] != 0) nonull = true;
    if(buf[i] > 10000) no10k = false;
    if(buf[i] < -10000) nom10k = false;
  }

  free(buf);

  return (nonull && no10k && nom10k);
}

/***** Main program *****/

int main(int argc, char *argv[])
{
  bool	retval = true;

  // Set path to source directory
  srcdir = getenv("srcdir");
  if(!srcdir) srcdir = ".";

  {
    CEmuopl emu(8000, true, false);
    retval = check_emu_output(&emu);
  }

  return retval ? EXIT_SUCCESS : EXIT_FAILURE;
}
