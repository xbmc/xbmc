/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
