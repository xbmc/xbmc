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

#include <vector>

class CJNIInetAddress : public CJNIBase
{
public:
  CJNIInetAddress(const jni::jhobject &object) : CJNIBase(object){}
  ~CJNIInetAddress(){}

  static CJNIInetAddress getLocalHost();
  static CJNIInetAddress getLoopbackAddress();
  static CJNIInetAddress getByName(std::string host);
  
  std::vector<char> getAddress();
  std::string getHostAddress();
  std::string getHostName();
  std::string getCanonicalHostName();
  
  bool        equals(const CJNIInetAddress &other);
  std::string toString()    const;
  int         describeContents() const;
  
protected:
  CJNIInetAddress();  
  static const char *m_classname;
};

typedef std::vector<CJNIInetAddress> CJNIInetAddresss;
