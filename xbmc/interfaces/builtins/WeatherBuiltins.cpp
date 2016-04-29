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

#include "WeatherBuiltins.h"

#include "guilib/GUIWindowManager.h"

/*! \brief Switch to a given weather location.
 *  \param params The parameters.
 *  \details params[0] = 1, 2 or 3.
 */
static int SetLocation(const std::vector<std::string>& params)
{
  int loc = atoi(params[0].c_str());
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, 0, 0, loc);
  g_windowManager.SendMessage(msg, WINDOW_WEATHER);

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
  g_windowManager.SendMessage(msg, WINDOW_WEATHER);

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
