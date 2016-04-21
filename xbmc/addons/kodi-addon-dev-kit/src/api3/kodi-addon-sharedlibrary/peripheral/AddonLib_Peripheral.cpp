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
#include KITINCLUDE(ADDON_API_LEVEL, peripheral/Peripheral.hpp)

#include <string>
#include <stdarg.h>

API_NAMESPACE

namespace KodiAPI
{
namespace Peripheral
{

  void TriggerScan(void)
  {
    g_interProcess.m_Callbacks->Peripheral.trigger_scan(g_interProcess.m_Handle);
  }

  void RefreshButtonMaps(const std::string& deviceName, const std::string& controllerId)
  {
    g_interProcess.m_Callbacks->Peripheral.refresh_button_maps(g_interProcess.m_Handle, deviceName.c_str(), controllerId.c_str());
  }

} /* namespace Peripheral */
} /* namespace KodiAPI */

END_NAMESPACE()
