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

#include "Addon_Directory.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/Addon/Addon_Directory.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

void CAddOnDirectory::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->Directory.can_open_directory    = V2::KodiAPI::AddOn::CAddOnDirectory::can_open_directory;
  interfaces->Directory.create_directory      = V2::KodiAPI::AddOn::CAddOnDirectory::create_directory;
  interfaces->Directory.directory_exists      = V2::KodiAPI::AddOn::CAddOnDirectory::directory_exists;
  interfaces->Directory.remove_directory      = V2::KodiAPI::AddOn::CAddOnDirectory::remove_directory;
  interfaces->VFS.get_directory               = (bool (*)(void*, const char*, const char*, VFSDirEntry**, unsigned int*))V2::KodiAPI::AddOn::CAddOnDirectory::get_directory;
  interfaces->VFS.free_directory              = (void (*)(void*, VFSDirEntry*, unsigned int))V2::KodiAPI::AddOn::CAddOnDirectory::free_directory;
}

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V3 */
