/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeStreamManager.h"

#include "cores/RetroPlayer/streams/IRetroPlayerStream.h"
#include "cores/RetroPlayer/streams/IStreamManager.h"
#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"
#include "smarthome/streams/SmartHomeStreamSwFramebuffer.h"
#include "utils/log.h"

#include <memory>

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeStreamManager::CSmartHomeStreamManager()
{
}

CSmartHomeStreamManager::~CSmartHomeStreamManager() = default;

void CSmartHomeStreamManager::Initialize(RETRO::IStreamManager& streamManager)
{
  m_streamManager = &streamManager;
}

void CSmartHomeStreamManager::Deinitialize()
{
  m_streamManager = nullptr;
}

std::unique_ptr<ISmartHomeStream> CSmartHomeStreamManager::OpenStream(AVPixelFormat nominalFormat,
                                                                      unsigned int nominalWidth,
                                                                      unsigned int nominalHeight)
{
  if (m_streamManager == nullptr)
    return std::unique_ptr<ISmartHomeStream>{};

  std::unique_ptr<ISmartHomeStream> smartHomeStream = CreateStream();
  if (!smartHomeStream)
  {
    CLog::Log(LOGERROR, "SMARTHOME: Failed to create stream");
    return std::unique_ptr<ISmartHomeStream>{};
  }

  RETRO::StreamPtr retroStream = m_streamManager->CreateStream(RETRO::StreamType::SW_BUFFER);
  if (!retroStream)
  {
    CLog::Log(LOGERROR, "SMARTHOME: Invalid RetroPlayer stream type: RETRO::StreamType::SW_BUFFER");
    return std::unique_ptr<ISmartHomeStream>{};
  }

  // Force 0RGB32 format
  if (!smartHomeStream->OpenStream(retroStream.get(), AV_PIX_FMT_0RGB32, nominalWidth,
                                   nominalHeight))
  {
    CLog::Log(LOGERROR, "SMARTHOME: Failed to open stream");
    return std::unique_ptr<ISmartHomeStream>{};
  }

  m_streams[smartHomeStream.get()] = std::move(retroStream);

  return smartHomeStream;
}

void CSmartHomeStreamManager::CloseStream(std::unique_ptr<ISmartHomeStream> stream)
{
  if (stream)
  {
    stream->CloseStream();

    m_streamManager->CloseStream(std::move(m_streams[stream.get()]));
    m_streams.erase(stream.get());
  }
}

std::unique_ptr<ISmartHomeStream> CSmartHomeStreamManager::CreateStream() const
{
  std::unique_ptr<ISmartHomeStream> smartHomeStream;

  smartHomeStream.reset(new CSmartHomeStreamSwFramebuffer);

  return smartHomeStream;
}
