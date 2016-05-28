/*
 *      Copyright (C) 2005-2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "AutorunMediaJob.h"
#include "Application.h"
#include "interfaces/builtins/Builtins.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogSelect.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

CAutorunMediaJob::CAutorunMediaJob(const std::string &label, const std::string &path):
  m_path(path),
  m_label(label)
{
}

bool CAutorunMediaJob::DoWork()
{
  CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

  // wake up and turn off the screensaver if it's active
  g_application.WakeUpScreenSaverAndDPMS();

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
    std::string strAction = StringUtils::Format("ActivateWindow(%s, %s)", GetWindowString(selection), m_path.c_str());
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
