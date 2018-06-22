/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RPStreamManager.h"
#include "IRetroPlayerStream.h"
#include "RetroPlayerAudio.h"
//#include "RetroPlayerHardwareBuffer.h" //! @todo
//#include "RetroPlayerSoftwareBuffer.h" //! @todo
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
