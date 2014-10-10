#pragma once

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

#include <vector>
#include "cores/VideoRenderers/RenderFormats.h"

class CDVDVideoCodec;
class CDVDAudioCodec;
class CDVDOverlayCodec;

class CDemuxStreamVideo;
class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

class CDVDFactoryCodec
{
public:
  static CDVDVideoCodec* CreateVideoCodec(CDVDStreamInfo &hint, unsigned int surfaces = 0, const std::vector<ERenderFormat>& formats = std::vector<ERenderFormat>());
  static CDVDAudioCodec* CreateAudioCodec(CDVDStreamInfo &hint );
  static CDVDOverlayCodec* CreateOverlayCodec(CDVDStreamInfo &hint );

  static CDVDAudioCodec* OpenCodec(CDVDAudioCodec* pCodec, CDVDStreamInfo &hint, CDVDCodecOptions &options );
  static CDVDVideoCodec* OpenCodec(CDVDVideoCodec* pCodec, CDVDStreamInfo &hint, CDVDCodecOptions &options );
  static CDVDOverlayCodec* OpenCodec(CDVDOverlayCodec* pCodec, CDVDStreamInfo &hint, CDVDCodecOptions &options );
};

