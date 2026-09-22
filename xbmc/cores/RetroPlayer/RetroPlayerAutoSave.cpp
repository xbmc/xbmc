/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerAutoSave.h"

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
                                           GAME::CGameSettings& settings,
                                           std::chrono::steady_clock::time_point startTime)
  : CThread("CRetroPlayerAutoSave"),
    m_callback(callback),
    m_settings(settings),
    m_firstAutosaveTime(startTime + AUTOSAVE_DURATION_SECS)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Initializing autosave");

  Create(false);
}

CRetroPlayerAutoSave::~CRetroPlayerAutoSave()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Deinitializing autosave");

  StopThread();
}

bool CRetroPlayerAutoSave::HasInitialDelayElapsed(std::chrono::steady_clock::time_point now) const
{
  return now >= m_firstAutosaveTime;
}

void CRetroPlayerAutoSave::Process()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Autosave thread started");

  const auto now = std::chrono::steady_clock::now();
  if (now < m_firstAutosaveTime)
    CThread::Sleep(m_firstAutosaveTime - now);

  while (!m_bStop)
  {
    if (m_settings.AutosaveEnabled() && m_callback.IsAutoSaveEnabled())
      m_callback.RequestAutosave();

    CThread::Sleep(AUTOSAVE_DURATION_SECS);
  }

  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Autosave thread ended");
}
