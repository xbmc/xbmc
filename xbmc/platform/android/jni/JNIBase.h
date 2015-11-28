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

#include "jutils/jutils.hpp"
class CJNIBase
{
  friend class CJNIContext; //for SetSDKVersion()

  typedef void (CJNIBase::*safe_bool_type)();
  void non_null_object() {}

public:
  operator safe_bool_type() const { return !m_object ?  0 : &CJNIBase::non_null_object; }
  const jni::jhobject& get_raw() const { return m_object; }
  static int GetSDKVersion();

protected:
  CJNIBase(jni::jhobject const& object);
  CJNIBase(std::string classname);
  ~CJNIBase();

  const std::string & GetClassName() {return m_className;};

  jni::jhobject m_object;

private:
  static void SetSDKVersion(int);
  std::string m_className;
  static int m_sdk_version;
};

