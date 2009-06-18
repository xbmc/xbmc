/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2007 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * player.cpp - Replayer base class, by Simon Peter <dn.tlp@gmx.net>
 */

#include "player.h"
#include "adplug.h"
#include "silentopl.h"

/***** CPlayer *****/

const unsigned short CPlayer::note_table[12] =
  {363, 385, 408, 432, 458, 485, 514, 544, 577, 611, 647, 686};

const unsigned char CPlayer::op_table[9] =
  {0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12};

CPlayer::CPlayer(Copl *newopl)
  : opl(newopl), db(CAdPlug::database)
{
}

CPlayer::~CPlayer()
{
}

unsigned long CPlayer::songlength(int subsong)
{
  CSilentopl	tempopl;
  Copl		*saveopl = opl;
  float		slength = 0.0f;

  // save original OPL from being overwritten
  opl = &tempopl;

  // get song length
  rewind(subsong);
  while(update() && slength < 600000)	// song length limit: 10 minutes
    slength += 1000.0f / getrefresh();
  rewind(subsong);

  // restore original OPL and return
  opl = saveopl;
  return (unsigned long)slength;
}

void CPlayer::seek(unsigned long ms)
{
  float pos = 0.0f;

  rewind();
  while(pos < ms && update())		// seek to new position
    pos += 1000/getrefresh();
}
