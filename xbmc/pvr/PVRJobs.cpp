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

#include "pvr/PVRGUIActions.h"

namespace PVR
{

bool CPVRSetRecordingOnChannelJob::DoWork()
{
  return CPVRGUIActions::GetInstance().SetRecordingOnChannel(m_channel, m_bOnOff);
}

bool CPVRContinueLastChannelJob::DoWork()
{
  return CPVRGUIActions::GetInstance().ContinueLastPlayedChannel();
}

bool CPVREventlogJob::DoWork()
{
  if (m_bNotifyUser)
    CGUIDialogKaiToast::QueueNotification(
      m_bError ? CGUIDialogKaiToast::Error : CGUIDialogKaiToast::Info, m_label.c_str(), m_msg, 5000, true);

  // Write event log entry.
  CEventLog::GetInstance().Add(
      EventPtr(new CNotificationEvent(m_label, m_msg, m_icon, m_bError ? EventLevel::Error : EventLevel::Information)));
  return true;
}

} // namespace PVR
