/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "AddonStatusHandler.h"
#include "AddonManager.h"
#include "SingleLock.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"
#include "log.h"

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

CAddonStatusHandler::CAddonStatusHandler(const CStdString &addonID, ADDON_STATUS status, CStdString message, bool sameThread)
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
    CStdString ThreadName;
    ThreadName.Format("Addon Status: %s", m_addon->Name().c_str());

    Create(true, THREAD_MINSTACKSIZE);
    SetName(ThreadName.c_str());
    SetPriority(-15);
  }
}

CAddonStatusHandler::~CAddonStatusHandler()
{
  StopThread();
}

void CAddonStatusHandler::OnStartup()
{
}

void CAddonStatusHandler::OnExit()
{
}

void CAddonStatusHandler::Process()
{
  CSingleLock lock(m_critSection);

  CStdString heading;
  heading.Format("%s: %s", TranslateType(m_addon->Type(), true).c_str(), m_addon->Name().c_str());

  /* AddOn lost connection to his backend (for ones that use Network) */
  if (m_status == STATUS_LOST_CONNECTION)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return;

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 24070);
    pDialog->SetLine(2, 24073);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
      CAddonMgr::Get().GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, false);
  }
  /* Request to restart the AddOn and data structures need updated */
  else if (m_status == STATUS_NEED_RESTART)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 24074);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    CAddonMgr::Get().GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
  }
  /* Some required settings are missing/invalid */
  else if (m_status == STATUS_NEED_SETTINGS)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 24070);
    pDialogYesNo->SetLine(2, 24072);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

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
  else if (m_status == STATUS_UNKNOWN)
  {
    //CAddonMgr::Get().DisableAddon(m_addon->ID());
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 24070);
    pDialog->SetLine(2, 24071);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }
}


} /*namespace ADDON*/

