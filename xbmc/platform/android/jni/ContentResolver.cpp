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

#include "ContentResolver.h"
#include "Cursor.h"
#include "jutils/jutils-details.hpp"
#include "URI.h"

using namespace jni;

std::string CJNIContentResolver::SCHEME_CONTENT;
std::string CJNIContentResolver::SCHEME_ANDROID_RESOURCE;
std::string CJNIContentResolver::SCHEME_FILE;
std::string CJNIContentResolver::CURSOR_ITEM_BASE_TYPE;
std::string CJNIContentResolver::CURSOR_DIR_BASE_TYPE;

void CJNIContentResolver::PopulateStaticFields()
{
  jhclass clazz = find_class("android/content/ContentResolver");
  SCHEME_CONTENT          = jcast<std::string>(get_static_field<jhstring>(clazz,"SCHEME_CONTENT"));
  SCHEME_ANDROID_RESOURCE = jcast<std::string>(get_static_field<jhstring>(clazz,"SCHEME_ANDROID_RESOURCE"));
  SCHEME_FILE             = jcast<std::string>(get_static_field<jhstring>(clazz,"SCHEME_FILE"));
  CURSOR_ITEM_BASE_TYPE   = jcast<std::string>(get_static_field<jhstring>(clazz,"CURSOR_ITEM_BASE_TYPE"));
  CURSOR_DIR_BASE_TYPE    = jcast<std::string>(get_static_field<jhstring>(clazz,"CURSOR_DIR_BASE_TYPE"));
}

CJNICursor CJNIContentResolver::query(const CJNIURI &uri, const std::vector<std::string> &projection, const std::string &selection, const std::vector<std::string> &selectionArgs, const std::string &sortOrder)
{
  return (CJNICursor)(call_method<jhobject>(m_object,
    "query","(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;",
    uri.get_raw(), jcast<jhobjectArray>(projection), jcast<jhstring>(selection), jcast<jhobjectArray>(selectionArgs), jcast<jhstring>(sortOrder)));
}
