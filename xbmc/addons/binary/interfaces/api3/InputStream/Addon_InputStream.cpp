/*
 *      Copyright (C) 2012-2016 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_InputStream.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/InputStream/Addon_InputStream.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace InputStream
{
extern "C"
{

void CAddOnInputStream::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->InputStream.get_codec_by_name      = (void (*)(void*, const char*, V3::KodiAPI::kodi_codec*))V2::KodiAPI::InputStream::CAddOnInputStream::get_codec_by_name;
  interfaces->InputStream.free_demux_packet      = V2::KodiAPI::InputStream::CAddOnInputStream::free_demux_packet;
  interfaces->InputStream.allocate_demux_packet  = V2::KodiAPI::InputStream::CAddOnInputStream::allocate_demux_packet;
}

} /* extern "C" */
} /* namespace InputStream */

} /* namespace KodiAPI */
} /* namespace V3 */
