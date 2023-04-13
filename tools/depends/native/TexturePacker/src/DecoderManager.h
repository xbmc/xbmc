/*
 *      Copyright (C) 2014 Team Kodi
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

#include "IDecoder.h"

#include <memory>
#include <string_view>

class DecoderManager
{
  public:
    DecoderManager();
    ~DecoderManager() = default;

    bool IsSupportedGraphicsFile(std::string_view filename);
    bool LoadFile(const std::string& filename, DecodedFrames& frames);
    void EnableVerboseOutput() { verbose = true; }

  private:
    std::vector<std::unique_ptr<IDecoder>> m_decoders;

    bool verbose{false};
};
