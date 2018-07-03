/*
*      Copyright (C) 2015 Team XBMC
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

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "IArchivable.h"

class EmbeddedArtInfo : public IArchivable
{
public:
  EmbeddedArtInfo() = default;
  EmbeddedArtInfo(size_t size, const std::string &mime, const std::string& type = "");

  // implementation of IArchivable
  void Archive(CArchive& ar) override;

  void Set(size_t size, const std::string &mime, const std::string& type = "");
  void Clear();
  bool Empty() const;
  bool Matches(const EmbeddedArtInfo &right) const;
  void SetType(const std::string& type) { m_type = type; }

  size_t m_size = 0;
  std::string m_mime;
  std::string m_type;
};

class EmbeddedArt : public EmbeddedArtInfo
{
public:
  EmbeddedArt() = default;
  EmbeddedArt(const uint8_t *data, size_t size,
              const std::string &mime, const std::string& type = "");

  void Set(const uint8_t *data, size_t size,
           const std::string &mime, const std::string& type = "");

  std::vector<uint8_t> m_data;
};
