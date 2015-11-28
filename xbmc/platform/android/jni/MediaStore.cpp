/*
 *      Copyright (C) 2013 Team XBMC
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

#include "MediaStore.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

std::string CJNIMediaStoreMediaColumns::DATA;
std::string CJNIMediaStoreMediaColumns::SIZE;
std::string CJNIMediaStoreMediaColumns::DISPLAY_NAME;
std::string CJNIMediaStoreMediaColumns::TITLE;
std::string CJNIMediaStoreMediaColumns::DATE_ADDED;
std::string CJNIMediaStoreMediaColumns::DATE_MODIFIED;
std::string CJNIMediaStoreMediaColumns::MIME_TYPE;

void CJNIMediaStoreMediaColumns::PopulateStaticFields()
{
  jhclass clazz = find_class("platform/android/provider/MediaStore$MediaColumns");
  DATA          = (jcast<std::string>(get_static_field<jhstring>(clazz, "DATA")));
  SIZE          = (jcast<std::string>(get_static_field<jhstring>(clazz, "SIZE")));
  DISPLAY_NAME  = (jcast<std::string>(get_static_field<jhstring>(clazz, "DISPLAY_NAME")));
  TITLE         = (jcast<std::string>(get_static_field<jhstring>(clazz, "TITLE")));
  DATE_ADDED    = (jcast<std::string>(get_static_field<jhstring>(clazz, "DATE_ADDED")));
  DATE_MODIFIED = (jcast<std::string>(get_static_field<jhstring>(clazz, "DATE_MODIFIED")));
  MIME_TYPE     = (jcast<std::string>(get_static_field<jhstring>(clazz, "MIME_TYPE")));
}
