/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientHardware.h"

#include "games/addons/GameClient.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CGameClientHardware::CGameClientHardware(CGameClient& gameClient) : m_gameClient(gameClient)
{
}

void CGameClientHardware::OnResetButton()
{
  CLog::Log(LOGDEBUG, "%s: Sending hardware reset", m_gameClient.ID().c_str());
  m_gameClient.Reset();
}
