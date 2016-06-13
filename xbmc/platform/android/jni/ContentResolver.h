#pragma once
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

#include <string>
#include <vector>

#include "JNIBase.h"

class CJNICursor;
class CJNIURI;

class CJNIContentResolver : public CJNIBase
{
public:
  CJNIContentResolver(const jni::jhobject &object) : CJNIBase(object) {};

  CJNICursor query(const CJNIURI &uri, const std::vector<std::string> &projection, const std::string &selection, const std::vector<std::string> &selectionArgs, const std::string &sortOrder);

  static void PopulateStaticFields();
  static std::string SCHEME_CONTENT;
  static std::string SCHEME_ANDROID_RESOURCE;
  static std::string SCHEME_FILE;
  static std::string CURSOR_ITEM_BASE_TYPE;
  static std::string CURSOR_DIR_BASE_TYPE;

private:
  CJNIContentResolver();
};
