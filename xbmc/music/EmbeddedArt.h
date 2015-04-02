#pragma once
/*
*      Copyright (C) 2015 Team XBMC
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

#include <stdint.h>
#include <string>
#include <vector>

#include "utils/IArchivable.h"

namespace MUSIC_INFO
{
  class EmbeddedArtInfo : public IArchivable
  {
  public:
    EmbeddedArtInfo() { }
    EmbeddedArtInfo(size_t size, const std::string &mime);
    ~EmbeddedArtInfo() { }

    // implementation of IArchivable
    virtual void Archive(CArchive& ar);

    void set(size_t size, const std::string &mime);
    void clear();
    bool empty() const;
    bool matches(const EmbeddedArtInfo &right) const;

    size_t size;
    std::string mime;
  };

  class EmbeddedArt : public EmbeddedArtInfo
  {
  public:
    EmbeddedArt() { }
    EmbeddedArt(const uint8_t *data, size_t size, const std::string &mime);
    ~EmbeddedArt() { }

    void set(const uint8_t *data, size_t size, const std::string &mime);

    std::vector<uint8_t> data;
  };
}