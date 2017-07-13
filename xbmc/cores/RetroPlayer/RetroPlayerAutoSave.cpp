/*
 *      Copyright (C) 2017 Team Kodi
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

#include "RetroPlayerAutoSave.h"
#include "cores/IPlayer.h"
#include "utils/log.h"
#include "URL.h"

using namespace KODI;
using namespace RETRO;

#define AUTOSAVE_DURATION_SECS    10 // Auto-save every 10 seconds

CRetroPlayerAutoSave::CRetroPlayerAutoSave(IPlayer *player) :
  CThread("CRetroPlayerAutoSave"),
  m_player(player)
{
  if (m_player != nullptr)
    Create(false);
}

CRetroPlayerAutoSave::~CRetroPlayerAutoSave() = default;

void CRetroPlayerAutoSave::Process()
{
  while (!m_bStop)
  {
    Sleep(AUTOSAVE_DURATION_SECS * 1000);

    if (m_bStop)
      break;

    if (m_player->GetSpeed() > 0.0f)
    {
      std::string savePath = m_player->GetPlayerState();
      if (!savePath.empty())
        CLog::Log(LOGDEBUG, "Saved state to %s", CURL::GetRedacted(savePath).c_str());
    }
  }
}
