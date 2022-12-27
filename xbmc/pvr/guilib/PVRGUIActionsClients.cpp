/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsClients.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClientMenuHooks.h"
#include "pvr/addons/PVRClients.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

using namespace KODI::MESSAGING;

using namespace PVR;

bool CPVRGUIActionsClients::ProcessSettingsMenuHooks()
{
  const CPVRClientMap clients = CServiceBroker::GetPVRManager().Clients()->GetCreatedClients();

  std::vector<std::pair<std::shared_ptr<CPVRClient>, CPVRClientMenuHook>> settingsHooks;
  for (const auto& client : clients)
  {
    const auto hooks = client.second->GetMenuHooks()->GetSettingsHooks();
    std::transform(hooks.cbegin(), hooks.cend(), std::back_inserter(settingsHooks),
                   [&client](const auto& hook) { return std::make_pair(client.second, hook); });
  }

  if (settingsHooks.empty())
  {
    HELPERS::ShowOKDialogText(
        CVariant{19033}, // "Information"
        CVariant{19347}); // "None of the active PVR clients does provide client-specific settings."
    return true; // no settings hooks, no error
  }

  auto selectedHook = settingsHooks.begin();

  // if there is only one settings hook, execute it directly, otherwise let the user select
  if (settingsHooks.size() > 1)
  {
    CGUIDialogSelect* pDialog =
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
            WINDOW_DIALOG_SELECT);
    if (!pDialog)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
      return false;
    }

    pDialog->Reset();
    pDialog->SetHeading(CVariant{19196}); // "PVR client specific actions"

    for (const auto& hook : settingsHooks)
    {
      if (clients.size() == 1)
        pDialog->Add(hook.second.GetLabel());
      else
        pDialog->Add(hook.first->GetFriendlyName() + ": " + hook.second.GetLabel());
    }

    pDialog->Open();

    int selection = pDialog->GetSelectedItem();
    if (selection < 0)
      return true; // cancelled

    std::advance(selectedHook, selection);
  }
  return selectedHook->first->CallSettingsMenuHook(selectedHook->second) == PVR_ERROR_NO_ERROR;
}
