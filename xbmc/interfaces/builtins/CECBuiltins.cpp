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

#include "CECBuiltins.h"

#include "messaging/ApplicationMessenger.h"

using namespace KODI::MESSAGING;

/*! \brief Wake up device through CEC.
 *  \param params (ignored)
 */
static int ActivateSource(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_CECACTIVATESOURCE);

  return 0;
}

/*! \brief Put device in standby through CEC.
 *  \param params (ignored)
 */
static int Standby(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_CECSTANDBY);

  return 0;
}

/*! \brief Toggle device state through CEC.
 *  \param params (ignored)
 */
static int ToggleState(const std::vector<std::string>& params)
{
  bool result;
  CApplicationMessenger::GetInstance().SendMsg(TMSG_CECTOGGLESTATE, 0, 0, static_cast<void*>(&result));

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_4 CEC built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`CECActivateSource`</b>
///     ,
///     Wake up playing device via a CEC peripheral
///   }
///   \table_row2_l{
///     <b>`CECStandby`</b>
///     ,
///     Put playing device on standby via a CEC peripheral
///   }
///   \table_row2_l{
///     <b>`CECToggleState`</b>
///     ,
///     Toggle state of playing device via a CEC peripheral
///   }
///  \table_end
///

CBuiltins::CommandMap CCECBuiltins::GetOperations() const
{
  return {
           {"cectogglestate",    {"Toggle state of playing device via a CEC peripheral", 0, ToggleState}},
           {"cecactivatesource", {"Wake up playing device via a CEC peripheral", 0, ActivateSource}},
           {"cecstandby",        {"Put playing device on standby via a CEC peripheral", 0, Standby}}
         };
}
