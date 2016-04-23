/*
 *      Copyright (C) 2014-2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_Peripheral.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/Peripheral/Addon_Peripheral.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace Peripheral
{
extern "C"
{

void CAddOnPeripheral::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->Peripheral.trigger_scan        = V2::KodiAPI::Peripheral::CAddOnPeripheral::trigger_scan;
  interfaces->Peripheral.refresh_button_maps = V2::KodiAPI::Peripheral::CAddOnPeripheral::refresh_button_maps;
}

} /* extern "C" */
} /* namespace Peripheral */

} /* namespace KodiAPI */
} /* namespace V3 */
