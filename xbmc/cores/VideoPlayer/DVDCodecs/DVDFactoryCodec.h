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

#include "cores/VideoPlayer/VideoRenderers/RenderFormats.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"

class CDVDVideoCodec;
class CDVDAudioCodec;
class CDVDOverlayCodec;

class CDemuxStreamVideo;
class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;


class CDVDFactoryCodec
{
friend class CDVDVideoCodecFFmpeg;

public:
  static CDVDVideoCodec* CreateVideoCodec(CDVDStreamInfo &hint,
                                          CProcessInfo &processInfo,
                                          const CRenderInfo &info);

  static CDVDVideoCodec* CreateVideoCodec(CDVDStreamInfo &hint,
                                          CProcessInfo &processInfo);

  static CDVDAudioCodec* CreateAudioCodec(CDVDStreamInfo &hint, CProcessInfo &processInfo,
                                          bool allowpassthrough, bool allowdtshddecode);

  static CDVDOverlayCodec* CreateOverlayCodec(CDVDStreamInfo &hint);

protected:

  static CDVDVideoCodec* CreateVideoCodecHW(CProcessInfo &processInfo);
  static CDVDAudioCodec* CreateAudioCodecHW(CProcessInfo &processInfo);
};

