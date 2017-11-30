/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "EmbeddedArt.h"
#include "Archive.h"

EmbeddedArtInfo::EmbeddedArtInfo(size_t siz,
                                 const std::string &mim, const std::string& t)
{
  set(siz, mim, t);
}

void EmbeddedArtInfo::set(size_t siz, const std::string &mim, const std::string& t)
{
  size = siz;
  mime = mim;
  type = t;
}

void EmbeddedArtInfo::clear()
{
  mime.clear();
  size = 0;
}

bool EmbeddedArtInfo::empty() const
{
  return size == 0;
}

bool EmbeddedArtInfo::matches(const EmbeddedArtInfo &right) const
{
  return (size == right.size &&
          mime == right.mime &&
          type == right.type);
}

void EmbeddedArtInfo::Archive(CArchive &ar)
{
  if (ar.IsStoring())
  {
    ar << size;
    ar << mime;
    ar << type;
  }
  else
  {
    ar >> size;
    ar >> mime;
    ar >> type;
  }
}

EmbeddedArt::EmbeddedArt(const uint8_t *dat, size_t siz,
                         const std::string &mim, const std::string& type)
{
  set(dat, siz, mim, type);
}

void EmbeddedArt::set(const uint8_t *dat, size_t siz,
                      const std::string &mim, const std::string& type)
{
  EmbeddedArtInfo::set(siz, mim, type);
  data.resize(siz);
  memcpy(&data[0], dat, siz);
}
