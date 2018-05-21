/*
 *      Copyright (C) 2014-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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

#pragma once
#include "IDecoder.h"

class PNGDecoder : public IDecoder
{
  public:
    ~PNGDecoder() override = default;
    bool CanDecode(const std::string &filename) override;
    bool LoadFile(const std::string &filename, DecodedFrames &frames) override;
    void FreeDecodedFrames(DecodedFrames &frames) override;
    const char* GetImageFormatName() override { return "PNG"; }
    const char* GetDecoderName() override { return "libpng"; }
  protected:
    void FillSupportedExtensions() override;
};
