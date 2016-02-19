/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess.h"
#include "kodi/api2/pvr/Trigger.hpp"

#include <string>
#include <stdarg.h>

namespace V2
{
namespace KodiAPI
{

namespace PVR
{

  namespace Trigger
  {
    void TimerUpdate(void)
    {
      g_interProcess.TriggerTimerUpdate();
    }

    void RecordingUpdate(void)
    {
      g_interProcess.TriggerRecordingUpdate();
    }

    void ChannelUpdate(void)
    {
      g_interProcess.TriggerChannelUpdate();
    }

    void EpgUpdate(unsigned int iChannelUid)
    {
      g_interProcess.TriggerEpgUpdate(iChannelUid);
    }

    void ChannelGroupsUpdate(void)
    {
      g_interProcess.TriggerChannelGroupsUpdate();
    }
  };

}; /* namespace PVR */

}; /* namespace KodiAPI */
}; /* namespace V2 */
