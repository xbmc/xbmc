/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsPowerManagement.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "network/Network.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <memory>
#include <string>
#include <vector>

using namespace PVR;
using namespace KODI::MESSAGING;

CPVRGUIActionsPowerManagement::CPVRGUIActionsPowerManagement()
  : m_settings({CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME,
                CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME})
{
}

bool CPVRGUIActionsPowerManagement::CanSystemPowerdown(bool bAskUser /*= true*/) const
{
  bool bReturn(true);
  if (CServiceBroker::GetPVRManager().IsStarted())
  {
    std::shared_ptr<CPVRTimerInfoTag> cause;
    if (!AllLocalBackendsIdle(cause))
    {
      if (bAskUser)
      {
        std::string text;

        if (cause)
        {
          if (cause->IsRecording())
          {
            text = StringUtils::Format(
                g_localizeStrings.Get(19691), // "PVR is currently recording...."
                cause->Title(), cause->ChannelName());
          }
          else
          {
            // Next event is due to a local recording or reminder.
            const CDateTime now(CDateTime::GetUTCDateTime());
            const CDateTime start(cause->StartAsUTC());
            const CDateTimeSpan prestart(0, 0, cause->MarginStart(), 0);

            CDateTimeSpan diff(start - now);
            diff -= prestart;
            int mins = diff.GetSecondsTotal() / 60;

            std::string dueStr;
            if (mins > 1)
            {
              // "%d minutes"
              dueStr = StringUtils::Format(g_localizeStrings.Get(19694), mins);
            }
            else
            {
              // "about a minute"
              dueStr = g_localizeStrings.Get(19695);
            }

            text = StringUtils::Format(
                cause->IsReminder()
                    ? g_localizeStrings.Get(19690) // "PVR has scheduled a reminder...."
                    : g_localizeStrings.Get(19692), // "PVR will start recording...."
                cause->Title(), cause->ChannelName(), dueStr);
          }
        }
        else
        {
          // Next event is due to automatic daily wakeup of PVR.
          const CDateTime now(CDateTime::GetUTCDateTime());

          CDateTime dailywakeuptime;
          dailywakeuptime.SetFromDBTime(
              m_settings.GetStringValue(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME));
          dailywakeuptime = dailywakeuptime.GetAsUTCDateTime();

          const CDateTimeSpan diff(dailywakeuptime - now);
          int mins = diff.GetSecondsTotal() / 60;

          std::string dueStr;
          if (mins > 1)
          {
            // "%d minutes"
            dueStr = StringUtils::Format(g_localizeStrings.Get(19694), mins);
          }
          else
          {
            // "about a minute"
            dueStr = g_localizeStrings.Get(19695);
          }

          text = StringUtils::Format(g_localizeStrings.Get(19693), // "Daily wakeup is due in...."
                                     dueStr);
        }

        // Inform user about PVR being busy. Ask if user wants to powerdown anyway.
        bReturn = HELPERS::ShowYesNoDialogText(CVariant{19685}, // "Confirm shutdown"
                                               CVariant{text}, CVariant{222}, // "Shutdown anyway",
                                               CVariant{19696}, // "Cancel"
                                               10000) // timeout value before closing
                  == HELPERS::DialogResponse::CHOICE_YES;
      }
      else
        bReturn = false; // do not powerdown (busy, but no user interaction requested).
    }
  }
  return bReturn;
}

bool CPVRGUIActionsPowerManagement::AllLocalBackendsIdle(
    std::shared_ptr<CPVRTimerInfoTag>& causingEvent) const
{
  // active recording on local backend?
  const std::vector<std::shared_ptr<CPVRTimerInfoTag>> activeRecordings =
      CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();
  for (const auto& timer : activeRecordings)
  {
    if (EventOccursOnLocalBackend(timer))
    {
      causingEvent = timer;
      return false;
    }
  }

  // soon recording on local backend?
  if (IsNextEventWithinBackendIdleTime())
  {
    const std::shared_ptr<CPVRTimerInfoTag> timer =
        CServiceBroker::GetPVRManager().Timers()->GetNextActiveTimer(false);
    if (!timer)
    {
      // Next event is due to automatic daily wakeup of PVR!
      causingEvent.reset();
      return false;
    }

    if (EventOccursOnLocalBackend(timer))
    {
      causingEvent = timer;
      return false;
    }
  }
  return true;
}

bool CPVRGUIActionsPowerManagement::EventOccursOnLocalBackend(
    const std::shared_ptr<CPVRTimerInfoTag>& event) const
{
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(CFileItem(event));
  if (client)
  {
    const std::string hostname = client->GetBackendHostname();
    if (!hostname.empty() && CServiceBroker::GetNetwork().IsLocalHost(hostname))
      return true;
  }
  return false;
}

bool CPVRGUIActionsPowerManagement::IsNextEventWithinBackendIdleTime() const
{
  // timers going off soon?
  const CDateTime now(CDateTime::GetUTCDateTime());
  const CDateTimeSpan idle(
      0, 0, m_settings.GetIntValue(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);
  const CDateTime next(CServiceBroker::GetPVRManager().Timers()->GetNextEventTime());
  const CDateTimeSpan delta(next - now);

  return (delta <= idle);
}
