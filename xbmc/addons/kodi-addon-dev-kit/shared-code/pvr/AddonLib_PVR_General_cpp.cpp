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
#include "kodi/api2/pvr/General.hpp"

#include <string>
#include <stdarg.h>

namespace V2
{
namespace KodiAPI
{

namespace PVR
{

  namespace General
  {
    void AddMenuHook(PVR_MENUHOOK* hook)
    {
      g_interProcess.AddMenuHook(hook);
    }

    void Recording(const std::string& strRecordingName, const std::string& strFileName, bool bOn)
    {
      g_interProcess.Recording(strRecordingName, strFileName, bOn);
    }
  };

}; /* namespace PVR */

}; /* namespace KodiAPI */
}; /* namespace V2 */
