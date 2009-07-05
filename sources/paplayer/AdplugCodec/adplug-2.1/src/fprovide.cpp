/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2002 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * fprovide.cpp - File provider class framework, by Simon Peter <dn.tlp@gmx.net>
 */

#include <string.h>
#include <binio.h>
#include <binfile.h>

#include "fprovide.h"

/***** CFileProvider *****/

bool CFileProvider::extension(const std::string &filename,
			      const std::string &extension)
{
  const char *fname = filename.c_str(), *ext = extension.c_str();

  if(strlen(fname) < strlen(ext) ||
     stricmp(fname + strlen(fname) - strlen(ext), ext))
    return false;
  else
    return true;
}

unsigned long CFileProvider::filesize(binistream *f)
{
  unsigned long oldpos = f->pos(), size;

  f->seek(0, binio::End);
  size = f->pos();
  f->seek(oldpos, binio::Set);

  return size;
}

/***** CProvider_Filesystem *****/

binistream *CProvider_Filesystem::open(std::string filename) const
{
  binifstream *f = new binifstream(filename);

  if(!f) return 0;
  if(f->error()) { delete f; return 0; }

  // Open all files as little endian with IEEE floats by default
  f->setFlag(binio::BigEndian, false); f->setFlag(binio::FloatIEEE);

  return f;
}

void CProvider_Filesystem::close(binistream *f) const
{
  binifstream *ff = (binifstream *)f;

  if(f) {
    ff->close();
    delete ff;
  }
}
