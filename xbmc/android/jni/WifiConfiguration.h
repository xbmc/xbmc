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
#include "BitSet.h"

class CJNIWifiConfiguration : public CJNIBase
{
public:
  CJNIWifiConfiguration(const jni::jhobject &object);
  ~CJNIWifiConfiguration() {};

  int         networkId;
  int         status;
  std::string SSID;
  std::string BSSID;
  std::string preSharedKey;
//  std::string[] wepKeys;
  int         wepTxKeyIndex;
  int         priority;
  bool        hiddenSSID;
  CJNIBitSet  allowedKeyManagement;
  CJNIBitSet  allowedProtocols;
  CJNIBitSet  allowedAuthAlgorithms;
  CJNIBitSet  allowedPairwiseCiphers;
  CJNIBitSet  allowedGroupCiphers;

  std::string toString();
  int         describeContents();

private:
  CJNIWifiConfiguration();
};
