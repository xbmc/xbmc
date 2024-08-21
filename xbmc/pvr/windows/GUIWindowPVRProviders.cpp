/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRProviders.h"

#include "FileItemList.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "pvr/PVRManager.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "pvr/providers/PVRProvidersPath.h"
#include "utils/URIUtils.h"

#include <memory>
#include <string>

using namespace PVR;

CGUIWindowPVRProvidersBase::CGUIWindowPVRProvidersBase(bool isRadio,
                                                       int id,
                                                       const std::string& xmlFile)
  : CGUIWindowPVRBase(isRadio, id, xmlFile)
{
}

CGUIWindowPVRProvidersBase::~CGUIWindowPVRProvidersBase() = default;

bool CGUIWindowPVRProvidersBase::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_PARENT_DIR || action.GetID() == ACTION_NAV_BACK)
  {
    const CPVRProvidersPath path{m_vecItems->GetPath()};
    if (path.IsValid() && !path.IsProvidersRoot())
    {
      GoParentFolder();
      return true;
    }
  }

  return CGUIWindowPVRBase::OnAction(action);
}

void CGUIWindowPVRProvidersBase::UpdateButtons()
{
  CGUIWindowPVRBase::UpdateButtons();

  // Update window breadcrumb.
  std::string header1;
  const CPVRProvidersPath path{m_vecItems->GetPath()};
  if (path.IsProvider())
  {
    const std::shared_ptr<const CPVRProvider> provider{
        CServiceBroker::GetPVRManager().Providers()->GetByClient(path.GetClientId(),
                                                                 path.GetProviderUid())};
    if (provider)
      header1 = provider->GetName();
  }
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, header1);
}

bool CGUIWindowPVRProvidersBase::OnMessage(CGUIMessage& message)
{
  bool ret{false};
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == m_viewControl.GetCurrentControl())
      {
        const int selectedItem{m_viewControl.GetSelectedItem()};
        if (selectedItem >= 0 && selectedItem < m_vecItems->Size())
        {
          const std::shared_ptr<const CFileItem> item{m_vecItems->Get(selectedItem)};
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            {
              const CPVRProvidersPath path{m_vecItems->GetPath()};
              if (path.IsValid())
              {
                if (path.IsProvidersRoot())
                {
                  if (item->IsParentFolder())
                  {
                    // Handle .. item, which is only visible if list of providers is empty.
                    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
                    ret = true;
                    break;
                  }
                }
              }

              if (item->m_bIsFolder)
              {
                // Folders and ".." folders in subfolders are handled by base class.
                ret = false;
              }
            }
          }
        }
      }
      break;
    }
  }

  return ret || CGUIWindowPVRBase::OnMessage(message);
}

std::string CGUIWindowPVRTVProviders::GetRootPath() const
{
  return CPVRProvidersPath::PATH_TV_PROVIDERS;
}

std::string CGUIWindowPVRTVProviders::GetDirectoryPath()
{
  return URIUtils::PathHasParent(m_vecItems->GetPath(), CPVRProvidersPath::PATH_TV_PROVIDERS)
             ? m_vecItems->GetPath()
             : CPVRProvidersPath::PATH_TV_PROVIDERS;
}

std::string CGUIWindowPVRRadioProviders::GetRootPath() const
{
  return CPVRProvidersPath::PATH_RADIO_PROVIDERS;
}

std::string CGUIWindowPVRRadioProviders::GetDirectoryPath()
{
  return URIUtils::PathHasParent(m_vecItems->GetPath(), CPVRProvidersPath::PATH_RADIO_PROVIDERS)
             ? m_vecItems->GetPath()
             : CPVRProvidersPath::PATH_RADIO_PROVIDERS;
}
