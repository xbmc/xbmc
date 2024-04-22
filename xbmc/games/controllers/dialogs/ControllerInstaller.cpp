/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerInstaller.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CControllerInstaller::CControllerInstaller() : CThread("ControllerInstaller")
{
}

void CControllerInstaller::Process()
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return;

  CGUIWindowManager& windowManager = gui->GetWindowManager();

  auto pSelectDialog = windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (pSelectDialog == nullptr)
    return;

  auto pProgressDialog = windowManager.GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
  if (pProgressDialog == nullptr)
    return;

  ADDON::VECADDONS installableAddons;
  CServiceBroker::GetAddonMgr().GetInstallableAddons(installableAddons,
                                                     ADDON::AddonType::GAME_CONTROLLER);
  if (installableAddons.empty())
  {
    // "Controller profiles"
    // "All available controller profiles are installed."
    MESSAGING::HELPERS::ShowOKDialogText(CVariant{35050}, CVariant{35062});
    return;
  }

  CLog::Log(LOGDEBUG, "Controller installer: Found {} controller add-ons",
            installableAddons.size());

  CFileItemList items;
  for (const auto& addon : installableAddons)
  {
    CFileItemPtr item(new CFileItem(addon->Name()));
    item->SetArt("icon", addon->Icon());
    items.Add(std::move(item));
  }

  pSelectDialog->Reset();
  pSelectDialog->SetHeading(39020); // "The following additional add-ons will be installed"
  pSelectDialog->SetUseDetails(true);
  pSelectDialog->EnableButton(true, 186); // "OK""
  for (const auto& it : items)
    pSelectDialog->Add(*it);
  pSelectDialog->Open();

  if (!pSelectDialog->IsButtonPressed())
  {
    CLog::Log(LOGDEBUG, "Controller installer: User cancelled installation dialog");
    return;
  }

  CLog::Log(LOGDEBUG, "Controller installer: Installing {} controller add-ons",
            installableAddons.size());

  pProgressDialog->SetHeading(CVariant{24086}); // "Installing add-on..."
  pProgressDialog->SetLine(0, CVariant{""});
  pProgressDialog->SetLine(1, CVariant{""});
  pProgressDialog->SetLine(2, CVariant{""});

  pProgressDialog->Open();

  unsigned int installedCount = 0;
  while (installedCount < installableAddons.size())
  {
    const auto& addon = installableAddons[installedCount];

    // Set dialog text
    const std::string& progressTemplate = g_localizeStrings.Get(24057); // "Installing {0:s}..."
    const std::string progressText = StringUtils::Format(progressTemplate, addon->Name());
    pProgressDialog->SetLine(0, CVariant{progressText});

    // Set dialog percentage
    const unsigned int percentage =
        100 * (installedCount + 1) / static_cast<unsigned int>(installableAddons.size());
    pProgressDialog->SetPercentage(percentage);

    if (!ADDON::CAddonInstaller::GetInstance().InstallOrUpdate(
            addon->ID(), ADDON::BackgroundJob::CHOICE_NO, ADDON::ModalJob::CHOICE_NO))
    {
      CLog::Log(LOGERROR, "Controller installer: Failed to install {}", addon->ID());
      // "Error"
      // "Failed to install add-on."
      MESSAGING::HELPERS::ShowOKDialogText(257, 35256);
      break;
    }

    if (pProgressDialog->IsCanceled())
    {
      CLog::Log(LOGDEBUG, "Controller installer: User cancelled add-on installation");
      break;
    }

    if (windowManager.GetActiveWindowOrDialog() != WINDOW_DIALOG_PROGRESS)
    {
      CLog::Log(LOGDEBUG, "Controller installer: Progress dialog is hidden, cancelling");
      break;
    }

    installedCount++;
  }

  CLog::Log(LOGDEBUG, "Controller window: Installed {} controller add-ons", installedCount);
  pProgressDialog->Close();
}
