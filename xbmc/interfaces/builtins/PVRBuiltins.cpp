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

#include "PVRBuiltins.h"

#include "Application.h"
#include "pvr/PVRManager.h"

/*! \brief Start the PVR manager.
 *  \param params (ignored)
 */
static int Start(const std::vector<std::string>& params)
{
  g_application.StartPVRManager();

  return 0;
}

/*! \brief Stop the PVR manager.
 *   \param params (ignored)
 */
static int Stop(const std::vector<std::string>& params)
{
  g_application.StopPVRManager();

  return 0;
}

/*! \brief Search for missing channel icons
 *   \param params (ignored)
 */
static int SearchMissingIcons(const std::vector<std::string>& params)
{
  PVR::CPVRManager::GetInstance().TriggerSearchMissingChannelIcons();

  return 0;
}

CBuiltins::CommandMap CPVRBuiltins::GetOperations() const
{
  return {
           {"startpvrmanager",                {"(Re)Starts the PVR manager",       0, Start}}, // deprecated alias
           {"pvr.startmanager",               {"(Re)Starts the PVR manager",       0, Start}},
           {"stoppvrmanager",                 {"Stops the PVR manager",            0, Stop}},
           {"pvr.stopmanager",                {"Stops the PVR manager",            0, Stop}},
           {"pvr.searchmissingchannelicons",  {"Search for missing channel icons", 0, SearchMissingIcons}}
         };
}
