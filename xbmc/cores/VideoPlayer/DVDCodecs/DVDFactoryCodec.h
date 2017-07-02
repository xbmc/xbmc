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

#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"

#include <map>
#include <string>
#include <vector>

extern "C" {
#include "libavutil/pixfmt.h"
}

class CDVDVideoCodec;
class CDVDAudioCodec;
class CDVDOverlayCodec;
class IHardwareDecoder;

class CDemuxStreamVideo;
class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

typedef CDVDVideoCodec* (*CreateHWVideoCodec)(CProcessInfo &processInfo);
typedef IHardwareDecoder* (*CreateHWAccel)(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt);
typedef CDVDAudioCodec* (*CreateHWAudioCodec)(CProcessInfo &processInfo);

class CDVDFactoryCodec
{
public:
  static CDVDVideoCodec* CreateVideoCodec(CDVDStreamInfo &hint,
                                          CProcessInfo &processInfo);

  static IHardwareDecoder* CreateVideoCodecHWAccel(std::string id, CDVDStreamInfo &hint,
                                          CProcessInfo &processInfo, AVPixelFormat fmt);

  static CDVDAudioCodec* CreateAudioCodec(CDVDStreamInfo &hint, CProcessInfo &processInfo,
                                          bool allowpassthrough, bool allowdtshddecode,
                                          CAEStreamInfo::DataType ptStreamType);

  static CDVDOverlayCodec* CreateOverlayCodec(CDVDStreamInfo &hint);

  static void RegisterHWVideoCodec(std::string id, CreateHWVideoCodec createFunc);
  static void ClearHWVideoCodecs();

  static void RegisterHWAccel(std::string id, CreateHWAccel createFunc);
  static std::vector<std::string> GetHWAccels();
  static void ClearHWAccels();

  static void RegisterHWAudioCodec(std::string id, CreateHWAudioCodec createFunc);
  static void ClearHWAudioCodecs();


protected:

  static CDVDVideoCodec* CreateVideoCodecHW(std::string id, CProcessInfo &processInfo);
  static CDVDAudioCodec* CreateAudioCodecHW(std::string id, CProcessInfo &processInfo);

  static std::map<std::string, CreateHWVideoCodec> m_hwVideoCodecs;
  static std::map<std::string, CreateHWAccel> m_hwAccels;
  static std::map<std::string, CreateHWAudioCodec> m_hwAudioCodecs;
};

