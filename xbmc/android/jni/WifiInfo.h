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
#include "Enum.h"
#include "NetworkInfo.h"

class CJNISupplicantState : public CJNIEnum
{
public:
  CJNISupplicantState(const jni::jhobject &object) : CJNIEnum(object){};
  ~CJNISupplicantState(){};

private:
  CJNISupplicantState();
};

class CJNIWifiInfo : public CJNIBase
{
public:
  CJNIWifiInfo(const jni::jhobject &object) : CJNIBase(object){};
  ~CJNIWifiInfo(){};

  std::string getSSID()  const;
  std::string getBSSID() const;
  int         getRssi()  const;
  int         getLinkSpeed()  const;
  std::string getMacAddress() const;
  int         getNetworkId()  const;
  int         getIpAddress()  const;
  bool        getHiddenSSID() const;
  std::string toString() const;
  int         describeContents() const;
  CJNISupplicantState getSupplicantState() const;
  static CJNINetworkInfoDetailedState getDetailedStateOf(const CJNISupplicantState &suppState);

private:
  CJNIWifiInfo();
};
