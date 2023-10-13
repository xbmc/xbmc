/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreams.h"

#include "GameClientStreamAudio.h"
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
  m_streamManager = nullptr;
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
    return nullptr;
  }

  if (!gameStream->OpenStream(retroStream.get(), properties))
  {
    CLog::Log(LOGERROR, "GAME: Failed to open audio stream");
    return nullptr;
  }

  m_streams[gameStream.get()] = std::move(retroStream);

  return gameStream.release();
}

void CGameClientStreams::CloseStream(IGameClientStream* stream)
{
  if (stream != nullptr)
  {
    std::unique_ptr<IGameClientStream> streamHolder(stream);
    streamHolder->CloseStream();

    m_streamManager->CloseStream(std::move(m_streams[stream]));
    m_streams.erase(stream);
  }
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
