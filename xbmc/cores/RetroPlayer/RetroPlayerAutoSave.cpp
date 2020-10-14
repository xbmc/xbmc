/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerAutoSave.h"

#include "URL.h"
#include "games/GameSettings.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;
using namespace std::chrono_literals;

namespace
{
constexpr auto AUTOSAVE_DURATION_SECS = 10s; // Auto-save every 10 seconds
}

CRetroPlayerAutoSave::CRetroPlayerAutoSave(IAutoSaveCallback& callback,
                                           GAME::CGameSettings& settings)
  : CThread("CRetroPlayerAutoSave"), m_callback(callback), m_settings(settings)
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
    CThread::Sleep(AUTOSAVE_DURATION_SECS);

    if (m_bStop)
      break;

    if (!m_settings.AutosaveEnabled())
      continue;

    if (m_callback.IsAutoSaveEnabled())
    {
      std::string savePath = m_callback.CreateAutosave();
      if (!savePath.empty())
        CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Saved state to {}", CURL::GetRedacted(savePath));
    }
  }

  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Autosave thread ended");
}
