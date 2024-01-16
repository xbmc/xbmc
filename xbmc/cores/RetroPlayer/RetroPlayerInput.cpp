/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerInput.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "peripherals/Peripherals.h"
#include "peripherals/events/EventPollHandle.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerInput::CRetroPlayerInput(PERIPHERALS::CPeripherals& peripheralManager,
                                     CRPProcessInfo& processInfo,
                                     GAME::GameClientPtr gameClient)
  : m_peripheralManager(peripheralManager),
    m_processInfo(processInfo),
    m_gameClient(std::move(gameClient))
{
  CLog::Log(LOGDEBUG, "RetroPlayer[INPUT]: Initializing input");

  m_inputPollHandle = m_peripheralManager.RegisterEventPoller();
}

CRetroPlayerInput::~CRetroPlayerInput()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[INPUT]: Deinitializing input");

  m_inputPollHandle.reset();
}

void CRetroPlayerInput::StartAgentManager()
{
  if (!m_bAgentManagerStarted)
  {
    m_bAgentManagerStarted = true;
    m_processInfo.GetRenderContext().StartAgentInput(m_gameClient);
  }
}

void CRetroPlayerInput::StopAgentManager()
{
  if (m_bAgentManagerStarted)
  {
    m_bAgentManagerStarted = false;
    m_processInfo.GetRenderContext().StopAgentInput();
  }
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
