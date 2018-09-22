/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerInput.h"
#include "peripherals/EventPollHandle.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerInput::CRetroPlayerInput(PERIPHERALS::CPeripherals &peripheralManager) :
  m_peripheralManager(peripheralManager)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[INPUT]: Initializing input");

  m_inputPollHandle = m_peripheralManager.RegisterEventPoller();
}

CRetroPlayerInput::~CRetroPlayerInput()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[INPUT]: Deinitializing input");

  m_inputPollHandle.reset();
}

void CRetroPlayerInput::SetSpeed(double speed)
{
  if (speed != 0)
    m_inputPollHandle->Activate();
  else
    m_inputPollHandle->Deactivate();
}

void CRetroPlayerInput::EnableInput(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void CRetroPlayerInput::PollInput()
{
  m_inputPollHandle->HandleEvents(true);
}
