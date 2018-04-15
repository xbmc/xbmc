/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIComponent.h"
#include "GUIInfoManager.h"
#include "GUILargeTextureManager.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "StereoscopicsManager.h"
#include "TextureManager.h"
#include "URL.h"
#include "dialogs/GUIDialogYesNo.h"

CGUIComponent::CGUIComponent()
{
  m_pWindowManager.reset(new CGUIWindowManager());
  m_pTextureManager.reset(new CGUITextureManager());
  m_pLargeTextureManager.reset(new CGUILargeTextureManager());
  m_stereoscopicsManager.reset(new CStereoscopicsManager(CServiceBroker::GetSettings()));
  m_guiInfoManager.reset(new CGUIInfoManager());
}

CGUIComponent::~CGUIComponent()
{
  Deinit();
}

void CGUIComponent::Init()
{
  m_pWindowManager->Initialize();
  m_stereoscopicsManager->Initialize();
  m_guiInfoManager->Initialize();

  //! @todo This is something we need to change
  m_pWindowManager->AddMsgTarget(m_stereoscopicsManager.get());

  CServiceBroker::RegisterGUI(this);
}

void CGUIComponent::Deinit()
{
  CServiceBroker::UnregisterGUI();

  m_pWindowManager->DeInitialize();
}

CGUIWindowManager& CGUIComponent::GetWindowManager()
{
  return *m_pWindowManager;
}

CGUITextureManager& CGUIComponent::GetTextureManager()
{
  return *m_pTextureManager;
}

CGUILargeTextureManager& CGUIComponent::GetLargeTextureManager()
{
  return *m_pLargeTextureManager;
}

CStereoscopicsManager &CGUIComponent::GetStereoscopicsManager()
{
  return *m_stereoscopicsManager;
}

CGUIInfoManager &CGUIComponent::GetInfoManager()
{
  return *m_guiInfoManager;
}

bool CGUIComponent::ConfirmDelete(std::string path)
{
  CGUIDialogYesNo* pDialog = GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(CVariant{122});
    pDialog->SetLine(0, CVariant{125});
    pDialog->SetLine(1, CVariant{CURL(path).GetWithoutUserDetails()});
    pDialog->SetLine(2, CVariant{""});
    pDialog->Open();
    if (pDialog->IsConfirmed())
      return true;
  }
  return false;
}
