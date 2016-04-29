/*
 *      Copyright (C) 2015-2016 Team KODI
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

#include "Addon_Network.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/Addon/Addon_Network.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

void CAddOnNetwork::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->Network.wake_on_lan     = V2::KodiAPI::AddOn::CAddOnNetwork::wake_on_lan;
  interfaces->Network.get_ip_address  = V2::KodiAPI::AddOn::CAddOnNetwork::get_ip_address;
  interfaces->Network.dns_lookup      = V2::KodiAPI::AddOn::CAddOnNetwork::dns_lookup;
  interfaces->Network.url_encode      = V2::KodiAPI::AddOn::CAddOnNetwork::url_encode;
}

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V3 */
