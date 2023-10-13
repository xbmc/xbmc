/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowSplash.h"

#include "Util.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <memory>

CGUIWindowSplash::CGUIWindowSplash(void) : CGUIWindow(WINDOW_SPLASH, ""), m_image(nullptr)
{
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIWindowSplash::~CGUIWindowSplash(void) = default;

void CGUIWindowSplash::OnInitWindow()
{
  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_splashImage)
    return;

  m_image = std::make_unique<CGUIImage>(0, 0, 0, 0,
                                        CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
                                        CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight(),
                                        CTextureInfo(CUtil::GetSplashPath()));
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
