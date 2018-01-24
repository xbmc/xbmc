/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

EmbeddedArtInfo::EmbeddedArtInfo(size_t size,
                                 const std::string &mime, const std::string& type)
{
  Set(size, mime, type);
}

void EmbeddedArtInfo::Set(size_t size, const std::string &mime, const std::string& type)
{
  m_size = size;
  m_mime = mime;
  m_type = type;
}

void EmbeddedArtInfo::Clear()
{
  m_mime.clear();
  m_size = 0;
}

bool EmbeddedArtInfo::Empty() const
{
  return m_size == 0;
}

bool EmbeddedArtInfo::Matches(const EmbeddedArtInfo &right) const
{
  return (m_size == right.m_size &&
          m_mime == right.m_mime &&
          m_type == right.m_type);
}

void EmbeddedArtInfo::Archive(CArchive &ar)
{
  if (ar.IsStoring())
  {
    ar << m_size;
    ar << m_mime;
    ar << m_type;
  }
  else
  {
    ar >> m_size;
    ar >> m_mime;
    ar >> m_type;
  }
}

EmbeddedArt::EmbeddedArt(const uint8_t *data, size_t size,
                         const std::string &mime, const std::string& type)
{
  Set(data, size, mime, type);
}

void EmbeddedArt::Set(const uint8_t *data, size_t size,
                      const std::string &mime, const std::string& type)
{
  EmbeddedArtInfo::Set(size, mime, type);
  m_data.resize(size);
  m_data.assign(data, data+size);
}
