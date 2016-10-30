#pragma once
/*
 *      Copyright (C) 2016 Christian Browet
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

class CJNIInetAddress;

class CJNINetworkInterface : public CJNIBase
{
public:
  CJNINetworkInterface(const jni::jhobject &object) : CJNIBase(object){}
  ~CJNINetworkInterface(){}

  static CJNINetworkInterface getByName(const std::string& name);
  static CJNINetworkInterface getByIndex(int index);
  static CJNINetworkInterface getByInetAddress(const CJNIInetAddress& addr);
  
  std::string getName();
  std::string getDisplayName();
  std::vector<char> getHardwareAddress();
  int getIndex();
  int getMTU();
  
  bool isLoopback();
  bool isPointToPoint();
  bool isUp();
  bool isVirtual();
  bool supportsMulticast();
  
  bool        equals(const CJNINetworkInterface &other);
  std::string toString()    const;
  
protected:
  CJNINetworkInterface();
  static const char *m_classname;
};

typedef std::vector<CJNINetworkInterface> CJNINetworkInterfaces;
