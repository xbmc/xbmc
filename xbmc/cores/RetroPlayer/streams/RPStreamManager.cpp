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
#include "RetroPlayerVideo.h"

using namespace KODI;
using namespace RETRO;

CRPStreamManager::CRPStreamManager(CRPRenderManager& renderManager, CRPProcessInfo& processInfo) :
  m_renderManager(renderManager),
  m_processInfo(processInfo)
{
}

void CRPStreamManager::EnableAudio(bool bEnable)
{
  if (m_audioStream != nullptr)
    m_audioStream->Enable(bEnable);
}

StreamPtr CRPStreamManager::CreateStream(StreamType streamType)
{
  switch (streamType)
  {
  case StreamType::AUDIO:
  {
    // Save pointer to audio stream
    m_audioStream = new CRetroPlayerAudio(m_processInfo);

    return StreamPtr(m_audioStream);
  }
  case StreamType::VIDEO:
  case StreamType::SW_BUFFER:
  {
    return StreamPtr(new CRetroPlayerVideo(m_renderManager, m_processInfo));
  }
  case StreamType::HW_BUFFER:
  {
    //return StreamPtr(new CRetroPlayerHardware(m_renderManager, m_processInfo)); //! @todo
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
    if (stream.get() == m_audioStream)
      m_audioStream = nullptr;

    stream->CloseStream();
  }
}
