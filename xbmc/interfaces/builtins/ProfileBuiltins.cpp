/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProfileBuiltins.h"

#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

/*! \brief Load a profile.
 *  \param params The parameters.
 *  \details params[0] = The profile name.
 *           params[1] = "prompt" to allow unlocking dialogs (optional)
 */
static int LoadProfile(const std::vector<std::string>& params)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  int index = profileManager->GetProfileIndex(params[0]);
  bool prompt = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "prompt"));
  bool bCanceled;
  if (index >= 0
      && (profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE
        || g_passwordManager.IsProfileLockUnlocked(index,bCanceled,prompt)))
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_LOADPROFILE, index);
  }

  return 0;
}

/*! \brief Log off currently signed in profile.
 *  \param params (ignored)
 */
static int LogOff(const std::vector<std::string>& params)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();
  profileManager->LogOff();

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

    // master mode turned OFF => refresh favourites due to possible visibility changes
    CServiceBroker::GetFavouritesService().RefreshFavourites();

    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20052),g_localizeStrings.Get(20053));
  }
  else if (g_passwordManager.IsMasterLockUnlocked(true)) // prompt user for code
  {
    g_passwordManager.LockSources(false);
    g_passwordManager.bMasterUser = true;

    // master mode turned ON => refresh favourites due to possible visibility changes
    CServiceBroker::GetFavouritesService().RefreshFavourites();

    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20052),g_localizeStrings.Get(20054));
  }

  CUtil::DeleteVideoDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  return 0;
}


// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_13 Profile built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`LoadProfile(profilename\,[prompt])`</b>
///     ,
///     Load the specified profile. If prompt is not specified\, and a password
///     would be required for the requested profile\, this command will silently
///     fail. If prompt is specified and a password is required\, a password
///     dialog will be shown.
///     @param[in] profilename           The profile name.
///     @param[in] prompt                Add "prompt" to allow unlocking dialogs (optional)
///   }
///   \table_row2_l{
///     <b>`Mastermode`</b>
///     ,
///     Runs Kodi in master mode
///   }
///   \table_row2_l{
///     <b>`System.LogOff`</b>
///     ,
///     Log off current user.
///   }
/// \table_end
///

CBuiltins::CommandMap CProfileBuiltins::GetOperations() const
{
  return {
           {"loadprofile",   {"Load the specified profile (note; if locks are active it won't work)", 1, LoadProfile}},
           {"mastermode",    {"Control master mode", 0, MasterMode}},
           {"system.logoff", {"Log off current user", 0, LogOff}}
         };
}
