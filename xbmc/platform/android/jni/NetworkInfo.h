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

class CJNINetworkInfoState : public CJNIEnum
{
public:
  CJNINetworkInfoState(const jni::jhobject &object) : CJNIEnum(object) {};
  ~CJNINetworkInfoState() {};

private:
  CJNINetworkInfoState();
};

class CJNINetworkInfoDetailedState : public CJNIEnum
{
public:
  CJNINetworkInfoDetailedState(const jni::jhobject &object) : CJNIEnum(object) {};
  ~CJNINetworkInfoDetailedState() {};

private:
  CJNINetworkInfoDetailedState();
};

class CJNINetworkInfo : public CJNIBase
{
public:
  CJNINetworkInfo();
  CJNINetworkInfo(const jni::jhobject &object) : CJNIBase(object){};
  ~CJNINetworkInfo(){};

  int         getType()     const;
  int         getSubtype()  const;
  std::string getTypeName() const;
  std::string getSubtypeName() const;
  bool        isConnectedOrConnecting() const;
  bool        isConnected() const;
  bool        isAvailable() const;
  bool        isFailover()  const;
  bool        isRoaming()   const;

  CJNINetworkInfoState getState() const;
  CJNINetworkInfoDetailedState getDetailedState() const;

  std::string getReason()   const;
  std::string getExtraInfo() const;
  std::string toString()    const;
  int         describeContents() const;
};
