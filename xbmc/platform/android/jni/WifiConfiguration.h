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
#include "BitSet.h"

class CJNIWifiConfiguration : public CJNIBase
{
public:
  CJNIWifiConfiguration();
  CJNIWifiConfiguration(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIWifiConfiguration() {};

  int         getnetworkId() const;
  int         getstatus() const;
  std::string getSSID() const;
  std::string getBSSID() const;
  std::string getpreSharedKey() const;
  std::vector<std::string> getwepKeys() const;
  int         getwepTxKeyIndex() const;
  int         getpriority() const;
  bool        gethiddenSSID() const;
  CJNIBitSet  getallowedKeyManagement() const;
  CJNIBitSet  getallowedProtocols() const;
  CJNIBitSet  getallowedAuthAlgorithms() const;
  CJNIBitSet  getallowedPairwiseCiphers() const;
  CJNIBitSet  getallowedGroupCiphers() const;

  void setnetworkId(int);
  void setstatus(int);
  void setSSID(const std::string &);
  void setBSSID(const std::string &);
  void setpreSharedKey(const std::string &);
  void setwepKeys(const std::vector<std::string>&);
  void setwepTxKeyIndex(int);
  void setpriority(int);
  void sethiddenSSID(bool);
  void setallowedKeyManagement(const CJNIBitSet&);
  void setallowedProtocols(const CJNIBitSet&);
  void setallowedAuthAlgorithms(const CJNIBitSet&);
  void setallowedPairwiseCiphers(const CJNIBitSet&);
  void setallowedGroupCiphers(const CJNIBitSet&);


  std::string toString();
  int         describeContents();

private:
};
