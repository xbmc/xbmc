/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AutorunMediaJob.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/builtins/Builtins.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

CAutorunMediaJob::CAutorunMediaJob(const std::string &label, const std::string &path):
  m_path(path),
  m_label(label)
{
}

bool CAutorunMediaJob::DoWork()
{
  CGUIDialogSelect* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);

  // wake up and turn off the screensaver if it's active
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->WakeUpScreenSaverAndDPMS();

  pDialog->Reset();
  if (!m_label.empty())
    pDialog->SetHeading(CVariant{m_label});
  else
    pDialog->SetHeading(CVariant{g_localizeStrings.Get(21331)});

  pDialog->Add(g_localizeStrings.Get(21332));
  pDialog->Add(g_localizeStrings.Get(21333));
  pDialog->Add(g_localizeStrings.Get(21334));
  pDialog->Add(g_localizeStrings.Get(21335));

  pDialog->Open();

  int selection = pDialog->GetSelectedItem();
  if (selection >= 0)
  {
    std::string strAction =
        StringUtils::Format("ActivateWindow({}, {})", GetWindowString(selection), m_path);
    CBuiltins::GetInstance().Execute(strAction);
  }

  return true;
}

const char *CAutorunMediaJob::GetWindowString(int selection)
{
  switch (selection)
  {
    case 0:
      return "Videos";
    case 1:
      return "Music";
    case 2:
      return "Pictures";
    case 3:
      return "FileManager";
    default:
      return "FileManager";
  }
}
