/*
 *      Copyright (C) 2012-2015 Team Kodi
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

#include "PVRJobs.h"

#include "dialogs/GUIDialogKaiToast.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "interfaces/AnnouncementManager.h"

#include "PlayListPlayer.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "ServiceBroker.h"
#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif

namespace PVR
{

bool CPVRSetRecordingOnChannelJob::DoWork()
{
  return CServiceBroker::GetPVRManager().GUIActions()->SetRecordingOnChannel(m_channel, m_bOnOff);
}

CPVRChannelEntryTimeoutJob::CPVRChannelEntryTimeoutJob(int timeout)
{
  m_delayTimer.Set(timeout);
}

bool CPVRChannelEntryTimeoutJob::DoWork()
{
  while (!ShouldCancel(0, 0))
  {
    if (m_delayTimer.IsTimePast())
    {
      CServiceBroker::GetPVRManager().ChannelPreviewSelect();
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
    CEventLog::GetInstance().Add(
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
