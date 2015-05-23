/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "AutorunMediaJob.h"
#include "Application.h"
#include "interfaces/Builtins.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogSelect.h"
#include "utils/StringUtils.h"

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
  if (m_label.size() > 0)
    pDialog->SetHeading(m_label);
  else
    pDialog->SetHeading("New media detected");

  pDialog->Add("Browse videos");
  pDialog->Add("Browse music");
  pDialog->Add("Browse pictures");
  pDialog->Add("Browse files");

  pDialog->DoModal();

  int selection = pDialog->GetSelectedLabel();
  if (selection >= 0)
  {
    std::string strAction = StringUtils::Format("ActivateWindow(%s, %s)", GetWindowString(selection), m_path.c_str());
    CBuiltins::Execute(strAction);
  }

  return true;
}

const char *CAutorunMediaJob::GetWindowString(int selection)
{
  switch (selection)
  {
    case 0:
      return "VideoFiles";
    case 1:
      return "MusicFiles";
    case 2:
      return "Pictures";
    case 3:
      return "FileManager";
    default:
      return "FileManager";
  }
}
