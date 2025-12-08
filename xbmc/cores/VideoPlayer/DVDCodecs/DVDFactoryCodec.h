/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include <libavutil/pixfmt.h>
}

class CDVDVideoCodec;
class CDVDAudioCodec;
class CDVDOverlayCodec;
class IHardwareDecoder;

class CDemuxStreamVideo;
class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

using CreateHWVideoCodec =
    std::function<std::unique_ptr<CDVDVideoCodec>(CProcessInfo& processInfo)>;
using CreateHWAccel = std::function<IHardwareDecoder*(
    CDVDStreamInfo& hint, CProcessInfo& processInfo, AVPixelFormat fmt)>;
using CreateHWAudioCodec =
    std::function<std::unique_ptr<CDVDAudioCodec>(CProcessInfo& processInfo)>;

class CDVDFactoryCodec
{
public:
  static std::unique_ptr<CDVDVideoCodec> CreateVideoCodec(CDVDStreamInfo& hint,
                                                          CProcessInfo& processInfo);

  static IHardwareDecoder* CreateVideoCodecHWAccel(const std::string& id,
                                                   CDVDStreamInfo& hint,
                                                   CProcessInfo& processInfo,
                                                   AVPixelFormat fmt);

  static std::unique_ptr<CDVDAudioCodec> CreateAudioCodec(CDVDStreamInfo& hint,
                                                          CProcessInfo& processInfo,
                                                          bool allowpassthrough,
                                                          bool allowdtshddecode,
                                                          CAEStreamInfo::DataType ptStreamType);

  static std::unique_ptr<CDVDOverlayCodec> CreateOverlayCodec(CDVDStreamInfo& hint);

  static void RegisterHWVideoCodec(const std::string& id, CreateHWVideoCodec createFunc);
  static void ClearHWVideoCodecs();

  static void RegisterHWAccel(const std::string& id, CreateHWAccel createFunc);
  static std::vector<std::string> GetHWAccels();
  static void ClearHWAccels();

  static void RegisterHWAudioCodec(const std::string& id, CreateHWAudioCodec createFunc);
  static void ClearHWAudioCodecs();


protected:
  static std::unique_ptr<CDVDVideoCodec> CreateVideoCodecHW(const std::string& id,
                                                            CProcessInfo& processInfo);
  static std::unique_ptr<CDVDAudioCodec> CreateAudioCodecHW(const std::string& id,
                                                            CProcessInfo& processInfo);

  static std::map<std::string, CreateHWVideoCodec> m_hwVideoCodecs;
  static std::map<std::string, CreateHWAccel> m_hwAccels;
  static std::map<std::string, CreateHWAudioCodec> m_hwAudioCodecs;
};

