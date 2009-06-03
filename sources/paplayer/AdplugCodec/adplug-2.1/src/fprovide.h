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
 * fprovide.h - File provider class framework, by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_FILEPROVIDER
#define H_ADPLUG_FILEPROVIDER

#include <string>
#include <binio.h>

class CFileProvider
{
public:
  virtual ~CFileProvider()
    {
    }

  virtual binistream *open(std::string) const = 0;
  virtual void close(binistream *) const = 0;

  static bool extension(const std::string &filename,
			const std::string &extension);
  static unsigned long filesize(binistream *f);
};

class CProvider_Filesystem: public CFileProvider
{
public:
  virtual binistream *open(std::string filename) const;
  virtual void close(binistream *f) const;
};

#endif
