/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"

#include <map>
#include <memory>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace KODI
{
namespace RETRO
{
class IStreamManager;
}

namespace SMART_HOME
{

class ISmartHomeStream;

class CSmartHomeStreamManager
{
public:
  CSmartHomeStreamManager();
  ~CSmartHomeStreamManager();

  // Rendering interface
  void Initialize(RETRO::IStreamManager& streamManager);
  void Deinitialize();

  std::unique_ptr<ISmartHomeStream> OpenStream(AVPixelFormat nominalFormat,
                                               unsigned int nominalWidth,
                                               unsigned int nominalHeight);
  void CloseStream(std::unique_ptr<ISmartHomeStream> stream);

private:
  // Utility functions
  std::unique_ptr<ISmartHomeStream> CreateStream() const;

  // Initialization parameters
  RETRO::IStreamManager* m_streamManager = nullptr;

  // Stream parameters
  std::map<ISmartHomeStream*, RETRO::StreamPtr> m_streams;
};

} // namespace SMART_HOME
} // namespace KODI
