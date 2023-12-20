/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowHome.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/JobManager.h"
#include "utils/RecentlyAddedJob.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <mutex>

CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(WINDOW_HOME, "Home.xml")
{
  m_updateRA = (Audio | Video | Totals);
  m_loadType = KEEP_IN_MEMORY;

  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
}

CGUIWindowHome::~CGUIWindowHome(void)
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

bool CGUIWindowHome::OnAction(const CAction &action)
{
  static unsigned int min_hold_time = 1000;
  if (action.GetID() == ACTION_NAV_BACK && action.GetHoldTime() < min_hold_time)
  {
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlaying() && (!appPlayer->IsRemotePlaying() || appPlayer->HasAudio()))
    {
      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (gui)
        gui->GetWindowManager().SwitchToFullScreen();

      return true;
    }
  }
  return CGUIWindow::OnAction(action);
}

void CGUIWindowHome::OnInitWindow()
{
  // for shared databases (ie mysql) always force an update on return to home
  // this is a temporary solution until remote announcements can be delivered
  if (StringUtils::EqualsNoCase(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseVideo.type, "mysql") ||
      StringUtils::EqualsNoCase(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseMusic.type, "mysql") )
    m_updateRA = (Audio | Video | Totals);
  AddRecentlyAddedJobs( m_updateRA );

  CGUIWindow::OnInitWindow();
}

void CGUIWindowHome::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                              const std::string& sender,
                              const std::string& message,
                              const CVariant& data)
{
  int ra_flag = 0;

  CLog::Log(LOGDEBUG, LOGANNOUNCE, "GOT ANNOUNCEMENT, type: {}, from {}, message {}",
            AnnouncementFlagToString(flag), sender, message);

  // we are only interested in library changes
  if ((flag & (ANNOUNCEMENT::VideoLibrary | ANNOUNCEMENT::AudioLibrary)) == 0)
    return;

  if (data.isMember("transaction") && data["transaction"].asBoolean())
    return;

  if (message == "OnScanStarted" || message == "OnCleanStarted")
    return;

  bool onUpdate = message == "OnUpdate";
  // always update Totals except on an OnUpdate with no playcount update
  if (!onUpdate || data.isMember("playcount"))
    ra_flag |= Totals;

  // always update the full list except on an OnUpdate
  if (!onUpdate)
  {
    if (flag & ANNOUNCEMENT::VideoLibrary)
      ra_flag |= Video;
    else if (flag & ANNOUNCEMENT::AudioLibrary)
      ra_flag |= Audio;
  }

  CGUIMessage reload(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_THUMBS, ra_flag);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(reload, GetID());
}

void CGUIWindowHome::AddRecentlyAddedJobs(int flag)
{
  bool getAJob = false;

  // this block checks to see if another one is running
  // and keeps track of the flag
  {
    std::unique_lock<CCriticalSection> lockMe(*this);
    if (!m_recentlyAddedRunning)
    {
      getAJob = true;

      flag |= m_cumulativeUpdateFlag; // add the flags from previous calls to AddRecentlyAddedJobs

      m_cumulativeUpdateFlag = 0; // now taken care of in flag.
                                  // reset this since we're going to execute a job

      // we're about to add one so set the indicator
      if (flag)
        m_recentlyAddedRunning = true; // this will happen in the if clause below
    }
    else
      // since we're going to skip a job, mark that one came in and ...
      m_cumulativeUpdateFlag |= flag; // this will be used later
  }

  if (flag && getAJob)
    CServiceBroker::GetJobManager()->AddJob(new CRecentlyAddedJob(flag), this);

  m_updateRA = 0;
}

void CGUIWindowHome::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  int flag = 0;

  {
    std::unique_lock<CCriticalSection> lockMe(*this);

    // the job is finished.
    // did one come in the meantime?
    flag = m_cumulativeUpdateFlag;
    m_recentlyAddedRunning = false; /// we're done.
  }

  if (flag)
    AddRecentlyAddedJobs(0 /* the flag will be set inside AddRecentlyAddedJobs via m_cumulativeUpdateFlag */ );
}


bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_WINDOW_RESET || message.GetParam1() == GUI_MSG_REFRESH_THUMBS)
    {
      int updateRA = (message.GetSenderId() == GetID()) ? message.GetParam2() : (Video | Audio | Totals);

      if (IsActive())
        AddRecentlyAddedJobs(updateRA);
      else
        m_updateRA |= updateRA;
    }
    break;

  default:
    break;
  }

  return CGUIWindow::OnMessage(message);
}
