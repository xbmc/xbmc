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
#include "PackageItemInfo.h"

class CJNIApplicationInfo : public CJNIPackageItemInfo
{
public:
  CJNIApplicationInfo(const jni::jhobject &object);
  ~CJNIApplicationInfo(){};

  std::string sourceDir;
  std::string publicSourceDir;
  std::string dataDir;
  std::string nativeLibraryDir;
  std::string packageName;
  int         uid; 
  int         targetSdkVersion;
  bool        enabled;

private:
  CJNIApplicationInfo();
};

typedef std::vector<CJNIApplicationInfo> CJNIApplicationInfos;
