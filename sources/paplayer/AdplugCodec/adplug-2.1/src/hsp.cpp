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
 * hsp.cpp - HSP Loader by Simon Peter <dn.tlp@gmx.net>
 */

#include <string.h>

#include "hsp.h"

CPlayer *ChspLoader::factory(Copl *newopl)
{
  return new ChspLoader(newopl);
}

bool ChspLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream	*f = fp.open(filename); if(!f) return false;
  unsigned long	i, j, orgsize, filesize;
  unsigned char	*cmp, *org;

  // file validation section
  if(!fp.extension(filename, ".hsp")) { fp.close(f); return false; }

  filesize = fp.filesize(f);
  orgsize = f->readInt(2);
  if(orgsize > 59187) { fp.close(f); return false; }

  // load section
  cmp = new unsigned char[filesize];
  for(i = 0; i < filesize; i++) cmp[i] = f->readInt(1);
  fp.close(f);

  org = new unsigned char[orgsize];
  for(i = 0, j = 0; i < filesize; j += cmp[i], i += 2) {	// RLE decompress
    if(j >= orgsize) break;	// memory boundary check
    memset(org + j, cmp[i + 1], j + cmp[i] < orgsize ? cmp[i] : orgsize - j - 1);
  }
  delete [] cmp;

  memcpy(instr, org, 128 * 12);		// instruments
  for(i = 0; i < 128; i++) {		// correct instruments
    instr[i][2] ^= (instr[i][2] & 0x40) << 1;
    instr[i][3] ^= (instr[i][3] & 0x40) << 1;
    instr[i][11] >>= 4;		// slide
  }
  memcpy(song, org + 128 * 12, 51);	// tracklist
  memcpy(patterns, org + 128 * 12 + 51, orgsize - 128 * 12 - 51);	// patterns
  delete [] org;

  rewind(0);
  return true;
}
