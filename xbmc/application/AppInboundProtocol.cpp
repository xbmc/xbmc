/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppInboundProtocol.h"

#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"

CAppInboundProtocol::CAppInboundProtocol(CApplication &app) : m_pApp(app)
{

}

bool CAppInboundProtocol::OnEvent(XBMC_Event &event)
{
  return m_pApp.OnEvent(event);
}

void CAppInboundProtocol::SetRenderGUI(bool renderGUI)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->SetRenderGUI(renderGUI);
}
