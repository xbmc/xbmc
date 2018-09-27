/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "messaging/ApplicationMessenger.h"
#include "PlayListPlayer.h"
#include "XBApplicationEx.h"
#include "utils/log.h"
#include "threads/SystemClock.h"
#include "commons/Exception.h"
#ifdef TARGET_POSIX
#include "platform/linux/XTimeUtils.h"
#endif
#include "AppParamParser.h"

CXBApplicationEx::CXBApplicationEx()
{
  // Variables to perform app timing
  m_bStop = false;
  m_AppFocused = true;
  m_ExitCode = EXITCODE_QUIT;
  m_renderGUI = false;
}

CXBApplicationEx::~CXBApplicationEx() = default;

/* Destroy the app */
void CXBApplicationEx::Destroy()
{
  CLog::Log(LOGNOTICE, "destroy");
  // Perform app-specific cleanup
  Cleanup();
}

void CXBApplicationEx::EnqueuePlayList(const CFileItemList &playlist, EnqueueOperation op)
{
  if (playlist.Size() > 0)
  {
    int currentPlayListNdx = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();

    if (currentPlayListNdx < 0)
      currentPlayListNdx = 0;

    switch (op)
    {
      case EOpReplace:
        CServiceBroker::GetPlaylistPlayer().ClearPlaylist(currentPlayListNdx);
        CServiceBroker::GetPlaylistPlayer().Add(0, playlist);
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(0);
        KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_PLAYLISTPLAYER_PLAY, -1);
        break;

      case EOpNext:
      {
        int currItemNdx = CServiceBroker::GetPlaylistPlayer().GetCurrentSong();
        bool shouldStartPlaylist = false;

        if (currItemNdx < 0)
        {
          currItemNdx = 0;
          shouldStartPlaylist = true;
        }
        else
          ++currItemNdx;
        CServiceBroker::GetPlaylistPlayer().Insert(currentPlayListNdx, playlist, currItemNdx);
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(currentPlayListNdx);
        if (shouldStartPlaylist)
          KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_PLAYLISTPLAYER_PLAY, -1);
        break;
      }

      case EOpLast:
      {
        int currItemNdx = CServiceBroker::GetPlaylistPlayer().GetCurrentSong();
        bool shouldStartPlaylist = (currItemNdx < 0);

        CServiceBroker::GetPlaylistPlayer().Add(currentPlayListNdx, playlist);
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(currentPlayListNdx);
        if (shouldStartPlaylist)
          KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_PLAYLISTPLAYER_PLAY, -1);
        break;
      }
    }
  }
}

/* Function that runs the application */
int CXBApplicationEx::Run(const CAppParamParser &params)
{
  CLog::Log(LOGNOTICE, "Running the application..." );

  unsigned int lastFrameTime = 0;
  unsigned int frameTime = 0;
  const unsigned int noRenderFrameTime = 15;  // Simulates ~66fps

  EnqueuePlayList(params.Playlist(), EOpReplace);

  // Run xbmc
  while (!m_bStop)
  {
    //-----------------------------------------
    // Animate and render a frame
    //-----------------------------------------

    lastFrameTime = XbmcThreads::SystemClockMillis();
    Process();

    if (!m_bStop)
    {
      FrameMove(true, m_renderGUI);
    }

    if (m_renderGUI && !m_bStop)
    {
      Render();
    }
    else if (!m_renderGUI)
    {
      frameTime = XbmcThreads::SystemClockMillis() - lastFrameTime;
      if(frameTime < noRenderFrameTime)
        Sleep(noRenderFrameTime - frameTime);
    }

  }
  Destroy();

  CLog::Log(LOGNOTICE, "application stopped..." );
  return m_ExitCode;
}
