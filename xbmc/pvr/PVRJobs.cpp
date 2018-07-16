/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRJobs.h"

#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "interfaces/AnnouncementManager.h"
#ifdef TARGET_POSIX
#include "platform/linux/XTimeUtils.h"
#endif

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"

namespace PVR
{

CPVRChannelEntryTimeoutJob::CPVRChannelEntryTimeoutJob(int iTimeout)
{
  m_delayTimer.Set(iTimeout);
}

bool CPVRChannelEntryTimeoutJob::DoWork()
{
  while (!ShouldCancel(0, 0))
  {
    if (m_delayTimer.IsTimePast())
    {
      CServiceBroker::GetPVRManager().GUIActions()->GetChannelNavigator().SwitchToCurrentChannel();
      return true;
    }
    Sleep(10);
  }
  return false;
}

CPVRChannelInfoTimeoutJob::CPVRChannelInfoTimeoutJob(int iTimeout)
{
  m_delayTimer.Set(iTimeout);
}

bool CPVRChannelInfoTimeoutJob::DoWork()
{
  while (!ShouldCancel(0, 0))
  {
    if (m_delayTimer.IsTimePast())
    {
      CServiceBroker::GetPVRManager().GUIActions()->GetChannelNavigator().HideInfo();
      return true;
    }
    Sleep(10);
  }
  return false;
}

bool CPVRPlayChannelOnStartupJob::DoWork()
{
  return CServiceBroker::GetPVRManager().GUIActions()->PlayChannelOnStartup();
}

CPVREventlogJob::CPVREventlogJob(bool bNotifyUser, bool bError, const std::string &label, const std::string &msg, const std::string &icon)
{
  AddEvent(bNotifyUser, bError, label, msg, icon);
}

void CPVREventlogJob::AddEvent(bool bNotifyUser, bool bError, const std::string &label, const std::string &msg, const std::string &icon)
{
  m_events.emplace_back(Event(bNotifyUser, bError, label, msg, icon));
}

bool CPVREventlogJob::DoWork()
{
  for (const auto &event : m_events)
  {
    if (event.m_bNotifyUser)
      CGUIDialogKaiToast::QueueNotification(
        event.m_bError ? CGUIDialogKaiToast::Error : CGUIDialogKaiToast::Info, event.m_label.c_str(), event.m_msg, 5000, true);

    // Write event log entry.
    CServiceBroker::GetEventLog().Add(
      EventPtr(new CNotificationEvent(event.m_label, event.m_msg, event.m_icon, event.m_bError ? EventLevel::Error : EventLevel::Information)));
  }
  return true;
}

bool CPVRSearchMissingChannelIconsJob::DoWork(void)
{
  CServiceBroker::GetPVRManager().SearchMissingChannelIcons();
  return true;
}

bool CPVRClientConnectionJob::DoWork(void)
{
  CServiceBroker::GetPVRManager().Clients()->ConnectionStateChange(m_client, m_connectString, m_state, m_message);
  return true;
}

bool CPVRStartupJob::DoWork(void)
{
  CServiceBroker::GetPVRManager().Clients()->Start();
  return true;
}

bool CPVRUpdateAddonsJob::DoWork(void)
{
  CServiceBroker::GetPVRManager().Clients()->UpdateAddons(m_changedAddonId);
  return true;
}

bool CPVREpgsCreateJob::DoWork(void)
{
  return CServiceBroker::GetPVRManager().CreateChannelEpgs();
}

bool CPVRRecordingsUpdateJob::DoWork(void)
{
  CServiceBroker::GetPVRManager().Recordings()->Update();
  return true;
}

bool CPVRTimersUpdateJob::DoWork(void)
{
  return CServiceBroker::GetPVRManager().Timers()->Update();
}

bool CPVRChannelsUpdateJob::DoWork(void)
{
  return CServiceBroker::GetPVRManager().ChannelGroups()->Update(true);
}

bool CPVRChannelGroupsUpdateJob::DoWork(void)
{
  return CServiceBroker::GetPVRManager().ChannelGroups()->Update(false);
}

} // namespace PVR
