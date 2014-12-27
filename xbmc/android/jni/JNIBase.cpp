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

using namespace jni;
int CJNIBase::m_sdk_version = -1;

CJNIBase::CJNIBase(std::string classname):
    m_className(classname)
{
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
