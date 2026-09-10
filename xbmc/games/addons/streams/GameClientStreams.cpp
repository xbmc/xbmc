/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreams.h"

#include "GameClientStreamAudio.h"
#include "GameClientStreamHwFramebuffer.h"
#include "GameClientStreamSwFramebuffer.h"
#include "GameClientStreamVideo.h"
#include "cores/RetroPlayer/streams/IRetroPlayerStream.h"
#include "cores/RetroPlayer/streams/IStreamManager.h"
#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientTranslator.h"
#include "utils/log.h"

#include <memory>

using namespace KODI;
using namespace GAME;

CGameClientStreams::CGameClientStreams(CGameClient& gameClient) : m_gameClient(gameClient)
{
}

void CGameClientStreams::Initialize(RETRO::IStreamManager& streamManager)
{
  m_streamManager = &streamManager;
}

void CGameClientStreams::Deinitialize()
{
  while (!m_streams.empty())
    CloseStream(m_streams.begin()->first);

  m_streamManager = nullptr;

  // Negotiation is per-game: left standing, the next game inherits this one's
  // context and any refusal, and is asked about or reported on the wrong terms
  m_hwProperties = {};
  m_hwRefusedWanted.clear();
  m_hwRefusedAvailable.clear();
}

IGameClientStream* CGameClientStreams::OpenStream(const game_stream_properties& properties)
{
  if (m_streamManager == nullptr)
    return nullptr;

  RETRO::StreamType retroStreamType;
  if (!CGameClientTranslator::TranslateStreamType(properties.type, retroStreamType))
  {
    CLog::Log(LOGERROR, "GAME: Invalid stream type: {}", static_cast<int>(properties.type));
    return nullptr;
  }

  std::unique_ptr<IGameClientStream> gameStream = CreateStream(properties.type);
  if (!gameStream)
  {
    CLog::Log(LOGERROR, "GAME: No stream implementation for type: {}",
              static_cast<int>(properties.type));
    return nullptr;
  }

  RETRO::StreamPtr retroStream = m_streamManager->CreateStream(retroStreamType);
  if (!retroStream)
  {
    CLog::Log(LOGERROR, "GAME:  Invalid RetroPlayer stream type: {}",
              static_cast<int>(retroStreamType));
    if (properties.type == GAME_STREAM_HW_FRAMEBUFFER)
    {
      m_hwRefusedWanted = CGameClientStreamHwFramebuffer::GetContextName(
          m_hwProperties.context_type, m_hwProperties.version_major, m_hwProperties.version_minor);
      m_hwProperties = {};
    }
    return nullptr;
  }

  if (!gameStream->OpenStream(retroStream.get(), properties))
  {
    CLog::Log(LOGERROR, "GAME: Failed to open stream");
    gameStream->CloseStream();
    m_streamManager->CloseStream(std::move(retroStream));
    if (properties.type == GAME_STREAM_HW_FRAMEBUFFER)
    {
      m_hwRefusedWanted = CGameClientStreamHwFramebuffer::GetContextName(
          m_hwProperties.context_type, m_hwProperties.version_major, m_hwProperties.version_minor);
      m_hwProperties = {};
    }
    return nullptr;
  }

  m_streams[gameStream.get()] = std::move(retroStream);

  return gameStream.release();
}

void CGameClientStreams::CloseStream(IGameClientStream* stream)
{
  const auto it = m_streams.find(stream);
  if (it == m_streams.end())
    return;

  std::unique_ptr<IGameClientStream> streamHolder(stream);
  RETRO::StreamPtr retroStream = std::move(it->second);
  m_streams.erase(it);

  streamHolder->CloseStream();
  m_streamManager->CloseStream(std::move(retroStream));
}

void CGameClientStreams::SetGameTiming(const game_system_timing& timingInfo)
{
  if (m_streamManager != nullptr)
    m_streamManager->SetVideoFps(static_cast<float>(timingInfo.fps));

  for (const auto& streamEntry : m_streams)
  {
    CGameClientStreamAudio* audioStream = dynamic_cast<CGameClientStreamAudio*>(streamEntry.first);
    if (audioStream != nullptr)
      audioStream->SetSampleRate(timingInfo.sample_rate);
  }
}

bool CGameClientStreams::EnableHardwareRendering(const game_hw_rendering_properties& properties)
{
  for (const auto& [stream, retroStream] : m_streams)
  {
    if (dynamic_cast<CGameClientStreamHwFramebuffer*>(stream) != nullptr)
      return false;
  }

  m_hwProperties = {};
  m_hwRefusedWanted.clear();
  m_hwRefusedAvailable.clear();

  if (properties.context_type == GAME_HW_CONTEXT_NONE)
    return false;

  const std::string wanted = CGameClientStreamHwFramebuffer::GetContextName(
      properties.context_type, properties.version_major, properties.version_minor);

  // Refuse before the client commits to rendering this way. It asks this long
  // before the frontend would try to build it a context, and a client told yes
  // wires itself up to callbacks it will then call regardless.
  if (m_streamManager == nullptr || !m_streamManager->HasHardwareRendering())
  {
    CLog::Log(LOGERROR, "GAME: {} is not available on this display stack", wanted);
    m_hwRefusedWanted = wanted;
    m_hwRefusedAvailable.clear();
    return false;
  }

  // The hardware pool creates contexts for the API used by this build.
#if defined(HAS_GLES) && HAS_GLES >= 3
  const bool supported = properties.context_type == GAME_HW_CONTEXT_OPENGLES2 ||
                         properties.context_type == GAME_HW_CONTEXT_OPENGLES3 ||
                         properties.context_type == GAME_HW_CONTEXT_OPENGLES_VERSION;
#elif defined(HAS_GL)
  const bool supported = properties.context_type == GAME_HW_CONTEXT_OPENGL ||
                         properties.context_type == GAME_HW_CONTEXT_OPENGL_CORE;
#else
  const bool supported = false;
#endif

  if (!supported)
  {
    // Debug, not error: a client works through the APIs it can use until one is
    // accepted, so a refusal here is the ordinary path. The refusal is recorded,
    // and if nothing is accepted the client tells the user which API it wanted.
    CLog::Log(LOGDEBUG, "GAME: Client asked for {}, which this build does not provide", wanted);
    m_hwRefusedWanted = wanted;
    m_hwRefusedAvailable.clear();
    return false;
  }

  if ((properties.context_type == GAME_HW_CONTEXT_OPENGL_CORE ||
       properties.context_type == GAME_HW_CONTEXT_OPENGLES_VERSION) &&
      properties.version_major == 0)
  {
    m_hwRefusedWanted = wanted;
    m_hwRefusedAvailable.clear();
    return false;
  }

  // The GUI context's version is not the driver's maximum; context creation validates it.
  CGameClientStreamHwFramebuffer::LogHwProperties(properties);

  // Store hardware rendering properties, and drop any earlier refusal: this
  // request was granted, so the reason a previous one was turned down no longer
  // describes the client
  m_hwProperties = properties;
  m_hwRefusedWanted.clear();
  m_hwRefusedAvailable.clear();

  return true;
}

bool CGameClientStreams::BeginClientFrame()
{
  if (m_streamManager == nullptr)
    return m_hwProperties.context_type == GAME_HW_CONTEXT_NONE;

  return m_streamManager->BeginClientFrame();
}

void CGameClientStreams::EndClientFrame()
{
  if (m_streamManager != nullptr)
    m_streamManager->EndClientFrame();
}

void CGameClientStreams::DestroyHwContext()
{
  if (m_hwProperties.context_type == GAME_HW_CONTEXT_NONE)
    return;

  for (const auto& [stream, retroStream] : m_streams)
  {
    if (auto* hwStream = dynamic_cast<CGameClientStreamHwFramebuffer*>(stream))
    {
      hwStream->DestroyHwContext();
      return;
    }
  }
}

game_proc_address_t CGameClientStreams::GetHwProcedureAddress(const char* symbol)
{
  if (symbol != nullptr && m_streamManager != nullptr)
    return m_streamManager->GetHwProcedureAddress(symbol);

  return nullptr;
}

std::unique_ptr<IGameClientStream> CGameClientStreams::CreateStream(
    GAME_STREAM_TYPE streamType) const
{
  std::unique_ptr<IGameClientStream> gameStream;

  switch (streamType)
  {
    case GAME_STREAM_AUDIO:
    {
      gameStream = std::make_unique<CGameClientStreamAudio>(m_gameClient.GetSampleRate());
      break;
    }
    case GAME_STREAM_VIDEO:
    {
      gameStream = std::make_unique<CGameClientStreamVideo>();
      break;
    }
    case GAME_STREAM_HW_FRAMEBUFFER:
    {
      if (m_hwProperties.context_type == GAME_HW_CONTEXT_NONE)
        break;

      for (const auto& [stream, retroStream] : m_streams)
      {
        if (dynamic_cast<CGameClientStreamHwFramebuffer*>(stream) != nullptr)
          return {};
      }

      gameStream = std::make_unique<CGameClientStreamHwFramebuffer>(m_gameClient, m_hwProperties);
      break;
    }
    case GAME_STREAM_SW_FRAMEBUFFER:
    {
      gameStream = std::make_unique<CGameClientStreamSwFramebuffer>();
      break;
    }
    default:
      break;
  }

  return gameStream;
}
