/*
 *      Copyright (C) 2005-2018 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AppInboundProtocol.h"
#include "Application.h"

CAppInboundProtocol::CAppInboundProtocol(CApplication &app) : m_pApp(app)
{

}

bool CAppInboundProtocol::OnEvent(XBMC_Event &event)
{
  return m_pApp.OnEvent(event);
}

void CAppInboundProtocol::SetRenderGUI(bool renderGUI)
{
  m_pApp.SetRenderGUI(renderGUI);
}
