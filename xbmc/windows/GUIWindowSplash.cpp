/*
 *      Copyright (C) 2015 Team XBMC
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

#include "GUIWindowSplash.h"

#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "Util.h"
#include "utils/log.h"

CGUIWindowSplash::CGUIWindowSplash(void) : CGUIWindow(WINDOW_SPLASH, "")
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_image = nullptr;
}

CGUIWindowSplash::~CGUIWindowSplash(void) = default;

void CGUIWindowSplash::OnInitWindow()
{
  if (!g_advancedSettings.m_splashImage)
    return;

  m_image = std::unique_ptr<CGUIImage>(new CGUIImage(0, 0, 0, 0, CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(), CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight(), CTextureInfo(CUtil::GetSplashPath())));
  m_image->SetAspectRatio(CAspectRatio::AR_SCALE);
}

void CGUIWindowSplash::Render()
{
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(), true);

  if (!m_image)
    return;

  m_image->SetWidth(CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth());
  m_image->SetHeight(CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
  m_image->AllocResources();
  m_image->Render();
  m_image->FreeResources();
}
