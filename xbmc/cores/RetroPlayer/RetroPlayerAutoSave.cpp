/*
 *  Copyright (C) 2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerAutoSave.h"
#include "games/addons/GameClient.h"
#include "games/addons/playback/IGameClientPlayback.h"
#include "games/GameSettings.h"
#include "utils/log.h"
#include "URL.h"

using namespace KODI;
using namespace RETRO;

#define AUTOSAVE_DURATION_SECS    10 // Auto-save every 10 seconds

CRetroPlayerAutoSave::CRetroPlayerAutoSave(GAME::CGameClient &gameClient,
                                           GAME::CGameSettings &settings) :
  CThread("CRetroPlayerAutoSave"),
  m_gameClient(gameClient),
  m_settings(settings)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Initializing autosave");

  Create(false);
}

CRetroPlayerAutoSave::~CRetroPlayerAutoSave()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Deinitializing autosave");

  StopThread();
}

void CRetroPlayerAutoSave::Process()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Autosave thread started");

  while (!m_bStop)
  {
    Sleep(AUTOSAVE_DURATION_SECS * 1000);

    if (m_bStop)
      break;

    if (!m_settings.AutosaveEnabled())
      continue;

    if (m_gameClient.GetPlayback()->GetSpeed() > 0.0)
    {
      std::string savePath = m_gameClient.GetPlayback()->CreateSavestate();
      if (!savePath.empty())
        CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Saved state to %s", CURL::GetRedacted(savePath).c_str());
    }
  }

  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Autosave thread ended");
}
