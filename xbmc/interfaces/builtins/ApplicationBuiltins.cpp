/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationBuiltins.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationVolumeHandling.h"
#include "filesystem/ZipManager.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <stdlib.h>

/*! \brief Extract an archive.
 *  \param params The parameters
 *  \details params[0] = The archive URL.
 *           params[1] = Destination path (optional).
 *                       If not given, extracts to folder with archive.
 */
static int Extract(const std::vector<std::string>& params)
{
    // Detects if file is zip or rar then extracts
    std::string strDestDirect;
    if (params.size() < 2)
      strDestDirect = URIUtils::GetDirectory(params[0]);
    else
      strDestDirect = params[1];

    URIUtils::AddSlashAtEnd(strDestDirect);

    if (URIUtils::IsZIP(params[0]))
      g_ZipManager.ExtractArchive(params[0],strDestDirect);
    else
      CLog::Log(LOGERROR, "Extract, No archive given");

  return 0;
}

/*! \brief Mute volume.
 *  \param params (ignored)
 */
static int Mute(const std::vector<std::string>& params)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
  appVolume->ToggleMute();

  return 0;
}

/*! \brief Notify all listeners on announcement bus.
 *  \param params The parameters.
 *  \details params[0] = sender.
 *           params[1] = data.
 *           params[2] = JSON with extra parameters (optional).
 */
static int NotifyAll(const std::vector<std::string>& params)
{
  CVariant data;
  if (params.size() > 2)
  {
    if (!CJSONVariantParser::Parse(params[2], data))
    {
      CLog::Log(LOGERROR, "NotifyAll failed to parse data: {}", params[2]);
      return -3;
    }
  }

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Other, params[0].c_str(), params[1].c_str(), data);

  return 0;
}

/*! \brief Set volume.
 *  \param params the parameters.
 *  \details params[0] = Volume level.
 *           params[1] = "showVolumeBar" to show volume bar (optional).
 */
static int SetVolume(const std::vector<std::string>& params)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
  float oldVolume = appVolume->GetVolumePercent();
  float volume = static_cast<float>(strtod(params[0].c_str(), nullptr));

  appVolume->SetVolume(volume);
  if (oldVolume != volume)
  {
    if (params.size() > 1 && StringUtils::EqualsNoCase(params[1], "showVolumeBar"))
    {
      CServiceBroker::GetAppMessenger()->PostMsg(
          TMSG_VOLUME_SHOW, oldVolume < volume ? ACTION_VOLUME_UP : ACTION_VOLUME_DOWN);
    }
  }

  return 0;
}

/*! \brief Toggle debug info.
 *  \param params (ignored)
 */
static int ToggleDebug(const std::vector<std::string>& params)
{
  bool debug = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO);
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO, !debug);
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->SetDebugMode(!debug);

  return 0;
}

/*! \brief Toggle DPMS state.
 *  \param params (ignored)
 */
static int ToggleDPMS(const std::vector<std::string>& params)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ToggleDPMS(true);

  return 0;
}

/*! \brief Send a WOL packet to a given host.
 *  \param params The parameters.
 *  \details params[0] = The MAC of the host to wake.
 */
static int WakeOnLAN(const std::vector<std::string>& params)
{
  CServiceBroker::GetNetwork().WakeOnLan(params[0].c_str());

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_3 Application built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`Extract(url [\, dest])`</b>
///     ,
///     Extracts a specified archive to an optionally specified 'absolute' path.
///     @param[in] url                   The archive URL.
///     @param[in] dest                  Destination path (optional).
///             @note If not given\, extracts to folder with archive.
///   }
///   \table_row2_l{
///     <b>`Mute`</b>
///     ,
///     Mutes (or unmutes) the volume.
///   }
///   \table_row2_l{
///     <b>`NotifyAll(sender\, data [\, json])`</b>
///     ,
///     Notify all connected clients
///     @param[in] sender                 Sender.
///     @param[in] data                   Data.
///     @param[in] json                   JSON with extra parameters (optional).
///   }
///   \table_row2_l{
///     <b>`SetVolume(percent[\,showvolumebar])`</b>
///     ,
///     Sets the volume to the percentage specified. Optionally\, show the Volume
///     Dialog in Kodi when setting the volume.
///     @param[in] percent               Volume level.
///     @param[in] showvolumebar         Add "showVolumeBar" to show volume bar (optional).
///   }
///   \table_row2_l{
///     <b>`ToggleDebug`</b>
///     ,
///     Toggles debug mode on/off
///   }
///   \table_row2_l{
///     <b>`ToggleDPMS`</b>
///     ,
///     Toggle DPMS mode manually
///   }
///   \table_row2_l{
///     <b>`WakeOnLan(mac)`</b>
///     ,
///     Sends the wake-up packet to the broadcast address for the specified MAC
///     address (Format: FF:FF:FF:FF:FF:FF or FF-FF-FF-FF-FF-FF).
///     @param[in] mac                   The MAC of the host to wake.
///   }
///  \table_end
///

CBuiltins::CommandMap CApplicationBuiltins::GetOperations() const
{
  return {
           {"extract", {"Extracts the specified archive", 1, Extract}},
           {"mute", {"Mute the player", 0, Mute}},
           {"notifyall", {"Notify all connected clients", 2, NotifyAll}},
           {"setvolume", {"Set the current volume", 1, SetVolume}},
           {"toggledebug", {"Enables/disables debug mode", 0, ToggleDebug}},
           {"toggledpms", {"Toggle DPMS mode manually", 0, ToggleDPMS}},
           {"wakeonlan", {"Sends the wake-up packet to the broadcast address for the specified MAC address", 1, WakeOnLAN}}
         };
}
