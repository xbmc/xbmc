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

#include "JNIBase.h"
#include "List.h"

class CJNIIntent;
class CJNIDrawable;
class CJNIApplicationInfo;
class CJNICharSequence;
class CJNIResources;

class CJNIPackageManager : public CJNIBase
{
public:
  CJNIPackageManager(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIPackageManager() {};

  bool              hasSystemFeature(const std::string &feature);
  CJNIIntent        getLaunchIntentForPackage(const std::string &package);
  CJNIIntent        getLeanbackLaunchIntentForPackage(const std::string &package);
  CJNIDrawable      getApplicationIcon(const std::string &package);
  CJNIList<CJNIApplicationInfo> getInstalledApplications(int flags);
  CJNICharSequence  getApplicationLabel(const CJNIApplicationInfo &info);
  CJNIResources     getResourcesForApplication(const std::string &package);
  CJNIResources     getResourcesForApplication(const CJNIApplicationInfo &info);

  static void       PopulateStaticFields();
  static int        GET_ACTIVITIES;

private:
  CJNIPackageManager();
};
