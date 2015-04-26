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
#include "AddonStatusHandler.h"
#include "AddonManager.h"
#include "threads/SingleLock.h"
#include "ApplicationMessenger.h"
#include "guilib/GUIWindowManager.h"
#include "GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

namespace ADDON
{

/**********************************************************
 * CAddonStatusHandler - AddOn Status Report Class
 *
 * Used to informate the user about occurred errors and
 * changes inside Add-on's, and ask him what to do.
 *
 */

CCriticalSection CAddonStatusHandler::m_critSection;

CAddonStatusHandler::CAddonStatusHandler(const std::string &addonID, ADDON_STATUS status, std::string message, bool sameThread)
  : CThread(("AddonStatus " + addonID).c_str())
{
  if (!CAddonMgr::Get().GetAddon(addonID, m_addon))
    return;

  CLog::Log(LOGINFO, "Called Add-on status handler for '%u' of clientName:%s, clientID:%s (same Thread=%s)", status, m_addon->Name().c_str(), m_addon->ID().c_str(), sameThread ? "yes" : "no");

  m_status  = status;
  m_message = message;

  if (sameThread)
  {
    Process();
  }
  else
  {
    Create(true, THREAD_MINSTACKSIZE);
  }
}

CAddonStatusHandler::~CAddonStatusHandler()
{
  StopThread();
}

void CAddonStatusHandler::OnStartup()
{
  SetPriority(GetMinPriority());
}

void CAddonStatusHandler::OnExit()
{
}

void CAddonStatusHandler::Process()
{
  CSingleLock lock(m_critSection);

  std::string heading = StringUtils::Format("%s: %s", TranslateType(m_addon->Type(), true).c_str(), m_addon->Name().c_str());

  /* AddOn lost connection to his backend (for ones that use Network) */
  if (m_status == ADDON_STATUS_LOST_CONNECTION)
  {
    if (m_addon->Type() == ADDON_PVRDLL)
    {
      if (!CSettings::Get().GetBool("pvrmanager.hideconnectionlostwarning"))
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, m_addon->Name().c_str(), g_localizeStrings.Get(36030)); // connection lost
      // TODO handle disconnects after the add-on's been initialised
    }
    else
    {
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (!pDialog) return;

      pDialog->SetHeading(heading);
      pDialog->SetLine(1, 24070);
      pDialog->SetLine(2, 24073);

      //send message and wait for user input
      ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
      CApplicationMessenger::Get().SendMessage(tMsg, true);

      if (pDialog->IsConfirmed())
        CAddonMgr::Get().GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, false);
    }
  }
  /* Request to restart the AddOn and data structures need updated */
  else if (m_status == ADDON_STATUS_NEED_RESTART)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 24074);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    CApplicationMessenger::Get().SendMessage(tMsg, true);

    CAddonMgr::Get().GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
  }
  /* Some required settings are missing/invalid */
  else if ((m_status == ADDON_STATUS_NEED_SETTINGS) || (m_status == ADDON_STATUS_NEED_SAVEDSETTINGS))
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 24070);
    pDialogYesNo->SetLine(2, 24072);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    CApplicationMessenger::Get().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!m_addon->HasSettings())
      return;

    if (CGUIDialogAddonSettings::ShowAndGetInput(m_addon))
    {
      //todo doesn't dialogaddonsettings save these automatically? should do
      m_addon->SaveSettings();
      CAddonMgr::Get().GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
    }
  }
  /* A unknown event has occurred */
  else if (m_status == ADDON_STATUS_UNKNOWN)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 24070);
    pDialog->SetLine(2, 24071);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    CApplicationMessenger::Get().SendMessage(tMsg, true);
  }
}


} /*namespace ADDON*/

