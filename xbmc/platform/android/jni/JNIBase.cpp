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
#include "jutils/jutils-details.hpp"

#include <algorithm>

using namespace jni;
int CJNIBase::m_sdk_version = -1;
int CJNIBase::RESULT_OK = -1;

CJNIBase::CJNIBase(std::string classname)
{
  // Convert "the.class.name" to "the/class/name"
  m_className = classname;
  std::replace(m_className.begin(), m_className.end(), '.', '/');
}

CJNIBase::CJNIBase(const jhobject &object):
    m_object(object)
{
  m_object.setGlobal();
}

CJNIBase::~CJNIBase()
{
  if(!m_object)
    return;
}

void CJNIBase::SetSDKVersion(int version)
{
  m_sdk_version = version;
}

int CJNIBase::GetSDKVersion()
{
  return m_sdk_version;
}

const std::string CJNIBase::GetDotClassName()
{
  std::string dotClassName = m_className;
  std::replace(dotClassName.begin(), dotClassName.end(), '/', '.');
  return dotClassName;
}

const std::string CJNIBase::ExceptionToString()
{
  JNIEnv* jenv = xbmc_jnienv();
  jhthrowable exception = (jhthrowable)jenv->ExceptionOccurred();
  if (!exception)
    return "";

  jenv->ExceptionClear();
  jhclass excClass = find_class(jenv, "java/lang/Throwable");
  jmethodID toStrMethod = get_method_id(jenv, excClass, "toString", "()Ljava/lang/String;");
  jhstring msg = call_method<jhstring>(exception, toStrMethod);
  return (jcast<std::string>(msg));
}
