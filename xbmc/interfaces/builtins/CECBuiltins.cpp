/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
