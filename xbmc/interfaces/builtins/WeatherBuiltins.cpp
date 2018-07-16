/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdlib.h>

#include "WeatherBuiltins.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"

/*! \brief Switch to a given weather location.
 *  \param params The parameters.
 *  \details params[0] = 1, 2 or 3.
 */
static int SetLocation(const std::vector<std::string>& params)
{
  int loc = atoi(params[0].c_str());
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, 0, 0, loc);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, WINDOW_WEATHER);

  return 0;
}

/*! \brief Switch weather location or refresh current.
 *  \param params (ignored)
 *
 *  The Direction template parameter can be -1 for previous location,
 *  1 for next location or 0 to refresh current location.
 */
  template<int Direction>
static int SwitchLocation(const std::vector<std::string>& params)
{
  CGUIMessage msg(GUI_MSG_MOVE_OFFSET, 0, 0, Direction);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, WINDOW_WEATHER);

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_16 Weather built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`Weather.Refresh`</b>
///     ,
///     Force weather data refresh.
///   }
///   \table_row2_l{
///     <b>`Weather.LocationNext`</b>
///     ,
///     Switch to next weather location
///   }
///   \table_row2_l{
///     <b>`Weather.LocationPrevious`</b>
///     ,
///     Switch to previous weather location
///   }
///   \table_row2_l{
///     <b>`Weather.LocationSet`</b>
///     ,
///     Switch to given weather location (parameter can be 1-3).
///     @param[in] parameter             1-3
///   }
/// \table_end
///

CBuiltins::CommandMap CWeatherBuiltins::GetOperations() const
{
  return {
           {"weather.refresh",          {"Force weather data refresh", 0, SwitchLocation<0>}},
           {"weather.locationnext",     {"Switch to next weather location", 0, SwitchLocation<1>}},
           {"weather.locationprevious", {"Switch to previous weather location", 0, SwitchLocation<-1>}},
           {"weather.locationset",      {"Switch to given weather location (parameter can be 1-3)", 1, SetLocation}}
         };
}
