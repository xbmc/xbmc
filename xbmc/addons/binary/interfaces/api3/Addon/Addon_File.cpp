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

#include "FileItem.h"
#include "Util.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/Addon/Addon_File.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"
#include "utils/Crc32.h"
#include "filesystem/File.h"

#include "Addon_File.h"

namespace V3
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

void CAddOnFile::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->File.open_file               = V2::KodiAPI::AddOn::CAddOnFile::open_file;
  interfaces->File.open_file_for_write     = V2::KodiAPI::AddOn::CAddOnFile::open_file_for_write;
  interfaces->File.read_file               = V2::KodiAPI::AddOn::CAddOnFile::read_file;
  interfaces->File.read_file_string        = V2::KodiAPI::AddOn::CAddOnFile::read_file_string;
  interfaces->File.write_file              = V2::KodiAPI::AddOn::CAddOnFile::write_file;
  interfaces->File.flush_file              = V2::KodiAPI::AddOn::CAddOnFile::flush_file;
  interfaces->File.seek_file               = V2::KodiAPI::AddOn::CAddOnFile::seek_file;
  interfaces->File.truncate_file           = V2::KodiAPI::AddOn::CAddOnFile::truncate_file;
  interfaces->File.get_file_position       = V2::KodiAPI::AddOn::CAddOnFile::get_file_position;
  interfaces->File.get_file_length         = V2::KodiAPI::AddOn::CAddOnFile::get_file_length;
  interfaces->File.get_file_download_speed = V2::KodiAPI::AddOn::CAddOnFile::get_file_download_speed;
  interfaces->File.close_file              = V2::KodiAPI::AddOn::CAddOnFile::close_file;
  interfaces->File.get_file_chunk_size     = V2::KodiAPI::AddOn::CAddOnFile::get_file_chunk_size;
  interfaces->File.file_exists             = V2::KodiAPI::AddOn::CAddOnFile::file_exists;
  interfaces->File.stat_file               = V2::KodiAPI::AddOn::CAddOnFile::stat_file;
  interfaces->File.delete_file             = V2::KodiAPI::AddOn::CAddOnFile::delete_file;
  interfaces->File.get_file_md5            = V2::KodiAPI::AddOn::CAddOnFile::get_file_md5;
  interfaces->File.get_cache_thumb_name    = V2::KodiAPI::AddOn::CAddOnFile::get_cache_thumb_name;
  interfaces->File.make_legal_filename     = V2::KodiAPI::AddOn::CAddOnFile::make_legal_filename;
  interfaces->File.make_legal_path         = V2::KodiAPI::AddOn::CAddOnFile::make_legal_path;
  interfaces->File.curl_create             = V2::KodiAPI::AddOn::CAddOnFile::curl_create;
  interfaces->File.curl_add_option         = V2::KodiAPI::AddOn::CAddOnFile::curl_add_option;
  interfaces->File.curl_open               = V2::KodiAPI::AddOn::CAddOnFile::curl_open;
}

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V3 */
