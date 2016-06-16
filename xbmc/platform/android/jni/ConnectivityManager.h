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

class CJNINetworkInfo;

class CJNIConnectivityManager : public CJNIBase
{
public:
  CJNIConnectivityManager(const jni::jhobject &object) : CJNIBase(object) {};

  bool isNetworkTypeValid(int);
  void setNetworkPreference(int);
  int  getNetworkPreference();
  CJNINetworkInfo getActiveNetworkInfo();
  CJNINetworkInfo getNetworkInfo(int);
  std::vector<CJNINetworkInfo> getAllNetworkInfo();
  int  startUsingNetworkFeature(int, std::string);
  int  stopUsingNetworkFeature(int, std::string);
  bool requestRouteToHost(int, int);
  bool getBackgroundDataSetting();

  static void PopulateStaticFields();
  static int TYPE_MOBILE;
  static int TYPE_WIFI;
  static int TYPE_MOBILE_MMS;
  static int TYPE_MOBILE_SUPL;
  static int TYPE_MOBILE_DUN;
  static int TYPE_MOBILE_HIPRI;
  static int TYPE_WIMAX;
  static int TYPE_BLUETOOTH;
  static int TYPE_DUMMY;
  static int TYPE_ETHERNET;
  static int DEFAULT_NETWORK_PREFERENCE;


private:
  CJNIConnectivityManager();
};
