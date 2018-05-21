/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "RetroPlayerAutoSave.h"
#include "games/addons/GameClient.h"
#include "games/addons/playback/IGameClientPlayback.h"
#include "utils/log.h"
#include "URL.h"

using namespace KODI;
using namespace RETRO;

#define AUTOSAVE_DURATION_SECS    10 // Auto-save every 10 seconds

CRetroPlayerAutoSave::CRetroPlayerAutoSave(GAME::CGameClient &gameClient) :
  CThread("CRetroPlayerAutoSave"),
  m_gameClient(gameClient)
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

    if (m_gameClient.GetPlayback()->GetSpeed() > 0.0)
    {
      std::string savePath = m_gameClient.GetPlayback()->CreateSavestate();
      if (!savePath.empty())
        CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Saved state to %s", CURL::GetRedacted(savePath).c_str());
    }
  }

  CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Autosave thread ended");
}
