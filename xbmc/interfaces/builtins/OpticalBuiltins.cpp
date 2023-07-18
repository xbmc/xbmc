/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OpticalBuiltins.h"

#include "ServiceBroker.h"

#ifdef HAS_OPTICAL_DRIVE
#include "storage/MediaManager.h"
#endif

#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif

/*! \brief Eject the tray of an optical drive.
 *  \param params (ignored)
 */
static int Eject(const std::vector<std::string>& params)
{
#ifdef HAS_OPTICAL_DRIVE
  CServiceBroker::GetMediaManager().ToggleTray();
#endif

  return 0;
}

/*! \brief Rip currently inserted CD.
 *  \param params (ignored)
 */
static int RipCD(const std::vector<std::string>& params)
{
#ifdef HAS_CDDA_RIPPER
  KODI::CDRIP::CCDDARipper::GetInstance().RipCD();
#endif

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_9 Optical container built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`EjectTray`</b>
///     ,
///     Either opens or closes the DVD tray\, depending on its current state.
///   }
///   \table_row2_l{
///     <b>`RipCD`</b>
///     ,
///     Will rip the inserted CD from the DVD-ROM drive.
///   }
/// \table_end
///

CBuiltins::CommandMap COpticalBuiltins::GetOperations() const
{
  return {
           {"ejecttray", {"Close or open the DVD tray", 0, Eject}},
           {"ripcd",     {"Rip the currently inserted audio CD", 0, RipCD}}
         };
}
