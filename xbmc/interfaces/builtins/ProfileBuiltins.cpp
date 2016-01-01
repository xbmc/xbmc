/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "ProfileBuiltins.h"

#include "addons/AddonManager.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "network/Network.h"
#include "network/NetworkServices.h"
#include "profiles/ProfilesManager.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "video/VideoLibraryQueue.h"

using namespace KODI::MESSAGING;

/*! \brief Load a profile.
 *  \param params The parameters.
 *  \details params[0] = The profile name.
 *           params[1] = "prompt" to allow unlocking dialogs (optional)
 */
static int LoadProfile(const std::vector<std::string>& params)
{
  int index = CProfilesManager::GetInstance().GetProfileIndex(params[0]);
  bool prompt = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "prompt"));
  bool bCanceled;
  if (index >= 0
      && (CProfilesManager::GetInstance().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE
        || g_passwordManager.IsProfileLockUnlocked(index,bCanceled,prompt)))
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_LOADPROFILE, index);
  }

  return 0;
}

/*! \brief Log off currently signed in profile.
 *  \param params (ignored)
 */
static int LogOff(const std::vector<std::string>& params)
{
  // there was a commit from cptspiff here which was reverted
  // for keeping the behaviour from Eden in Frodo - see
  // git rev 9ee5f0047b
  if (g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN)
    return -1;

  g_application.StopPlaying();
  if (g_application.IsMusicScanning())
    g_application.StopMusicScan();

  if (CVideoLibraryQueue::GetInstance().IsRunning())
    CVideoLibraryQueue::GetInstance().CancelAllJobs();

  ADDON::CAddonMgr::GetInstance().StopServices(true);

  g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
  CProfilesManager::GetInstance().LoadMasterProfileForLogin();
  g_passwordManager.bMasterUser = false;

  g_application.WakeUpScreenSaverAndDPMS();
  g_windowManager.ActivateWindow(WINDOW_LOGIN_SCREEN, {}, false);

  if (!CNetworkServices::GetInstance().StartEventServer()) // event server could be needed in some situations
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));

  return 0;
}

/*! \brief Toggle master mode.
 *  \param params (ignored)
 */
static int MasterMode(const std::vector<std::string>& params)
{
  if (g_passwordManager.bMasterUser)
  {
    g_passwordManager.bMasterUser = false;
    g_passwordManager.LockSources(true);
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20052),g_localizeStrings.Get(20053));
  }
  else if (g_passwordManager.IsMasterLockUnlocked(true))
  {
    g_passwordManager.LockSources(false);
    g_passwordManager.bMasterUser = true;
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20052),g_localizeStrings.Get(20054));
  }

  CUtil::DeleteVideoDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  g_windowManager.SendMessage(msg);

  return 0;
}

CBuiltins::CommandMap CProfileBuiltins::GetOperations() const
{
  return {
           {"loadprofile",   {"Load the specified profile (note; if locks are active it won't work)", 1, LoadProfile}},
           {"mastermode",    {"Control master mode", 0, MasterMode}},
           {"system.logoff", {"Log off current user", 0, LogOff}}
         };
}
