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

#include "OpticalBuiltins.h"

#include "system.h"

#ifdef HAS_DVD_DRIVE
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
#ifdef HAS_DVD_DRIVE
  g_mediaManager.ToggleTray();
#endif

  return 0;
}

/*! \brief Rip currently inserted CD.
 *  \param params (ignored)
 */
static int RipCD(const std::vector<std::string>& params)
{
#ifdef HAS_CDDA_RIPPER
  CCDDARipper::GetInstance().RipCD();
#endif

  return 0;
}

CBuiltins::CommandMap COpticalBuiltins::GetOperations() const
{
  return {
           {"ejecttray", {"Close or open the DVD tray", 0, Eject}},
           {"ripcd",     {"Rip the currently inserted audio CD", 0, RipCD}}
         };
}
