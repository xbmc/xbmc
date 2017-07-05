/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "GameClientHardware.h"
#include "GameClient.h"
#include "utils/log.h"

#include <assert.h>

using namespace KODI;
using namespace GAME;

CGameClientHardware::CGameClientHardware(CGameClient* gameClient) :
  m_gameClient(gameClient)
{
  assert(m_gameClient != nullptr);
}

void CGameClientHardware::OnResetButton(unsigned int port)
{
  CLog::Log(LOGDEBUG, "%s: Port %d sending hardware reset", m_gameClient->ID().c_str(), port);
  m_gameClient->Reset(port);
}
