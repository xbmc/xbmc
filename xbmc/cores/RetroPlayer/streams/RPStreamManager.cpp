/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPStreamManager.h"

#include "IRetroPlayerStream.h"
#include "RetroPlayerAudio.h"
#include "RetroPlayerRendering.h"
#include "RetroPlayerVideo.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"

#include <algorithm>

using namespace KODI;
using namespace RETRO;

CRPStreamManager::CRPStreamManager(CRPRenderManager& renderManager, CRPProcessInfo& processInfo)
  : m_renderManager(renderManager),
    m_processInfo(processInfo)
{
  // Visual streams can overlap, so shared rendering resources belong to the session.
  m_renderManager.Initialize();
}

CRPStreamManager::~CRPStreamManager()
{
  m_renderManager.Deinitialize();
}

void CRPStreamManager::EnableAudio(bool bEnable)
{
  for (CRetroPlayerAudio* audioStream : m_audioStreams)
    audioStream->Enable(bEnable);
}

void CRPStreamManager::SuppressAudio(bool bSuppress)
{
  m_audioSuppressed = bSuppress;

  for (CRetroPlayerAudio* audioStream : m_audioStreams)
    audioStream->Suppress(bSuppress);
}

bool CRPStreamManager::AudioSentOffFrameThread() const
{
  return std::ranges::any_of(m_audioStreams, [](const CRetroPlayerAudio* audioStream)
                             { return audioStream->SentOffFrameThread(); });
}

void CRPStreamManager::EnableVideo(bool bEnable)
{
  m_videoEnabled = bEnable;

  for (CRetroPlayerVideo* videoStream : m_videoStreams)
    videoStream->Enable(bEnable);

  for (CRetroPlayerRendering* renderingStream : m_renderingStreams)
    renderingStream->Enable(bEnable);
}

StreamPtr CRPStreamManager::CreateStream(StreamType streamType)
{
  switch (streamType)
  {
    case StreamType::AUDIO:
    {
      auto audioStream = std::make_unique<CRetroPlayerAudio>(m_processInfo);
      audioStream->Suppress(m_audioSuppressed);

      m_audioStreams.emplace_back(audioStream.get());

      return StreamPtr{audioStream.release()};
    }
    case StreamType::VIDEO:
    case StreamType::SW_BUFFER:
    {
      auto videoStream = std::make_unique<CRetroPlayerVideo>(m_renderManager, m_processInfo);
      videoStream->Enable(m_videoEnabled);

      m_videoStreams.emplace_back(videoStream.get());

      return StreamPtr{videoStream.release()};
    }
    case StreamType::HW_BUFFER:
    {
      auto renderingStream =
          std::make_unique<CRetroPlayerRendering>(m_renderManager, m_processInfo);
      renderingStream->Enable(m_videoEnabled);

      m_renderingStreams.emplace_back(renderingStream.get());

      return StreamPtr{renderingStream.release()};
    }
    default:
      break;
  }

  return StreamPtr();
}

void CRPStreamManager::CloseStream(StreamPtr stream)
{
  if (stream)
  {
    const auto isStream = [&stream](const IRetroPlayerStream* open)
    { return open == stream.get(); };
    std::erase_if(m_audioStreams, isStream);
    std::erase_if(m_videoStreams, isStream);
    std::erase_if(m_renderingStreams, isStream);

    stream->CloseStream();
  }
}

void CRPStreamManager::SetVideoFps(float fps)
{
  m_processInfo.SetVideoFps(fps);
}

bool CRPStreamManager::BeginClientFrame()
{
  return m_renderManager.BeginClientFrame();
}

void CRPStreamManager::EndClientFrame()
{
  m_renderManager.EndClientFrame();
}

HwProcedureAddress CRPStreamManager::GetHwProcedureAddress(const char* symbol)
{
  return m_processInfo.GetHwProcedureAddress(symbol);
}

bool CRPStreamManager::HasHardwareRendering() const
{
  return m_processInfo.HasHardwareRendering();
}
