#pragma once

/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "DVDInputStream.h"

#include <string>
#include <vector>

typedef std::shared_ptr<CDVDInputStream> InputStreamPtr;
class IDVDPlayer;

class InputStreamMultiStreams : public CDVDInputStream
{
  friend class CDemuxMultiSource;

public:
  InputStreamMultiStreams(DVDStreamType type, const CFileItem& fileitem)
    : CDVDInputStream(type, fileitem) {}

  ~InputStreamMultiStreams() override = default;

protected:
  std::vector<InputStreamPtr> m_InputStreams;    // input streams for current playing file
};
