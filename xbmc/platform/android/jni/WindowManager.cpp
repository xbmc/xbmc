/*
 *      Copyright (C) 2014 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "WindowManager.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

float CJNIWindowManagerLayoutParams::getpreferredRefreshRate() const
{
  if (GetSDKVersion() >= 21)
    return get_field<jfloat>(m_object, "preferredRefreshRate");
  else
    return -1.0;
}

void CJNIWindowManagerLayoutParams::setpreferredRefreshRate(float rate)
{
  if (GetSDKVersion() >= 21)
    set_field(m_object, "preferredRefreshRate", rate);
}

int CJNIWindowManagerLayoutParams::getpreferredDisplayModeId() const
{
  jhclass clazz = get_class(m_object);
  jfieldID fid = get_field_id<jclass>(clazz, "preferredDisplayModeId", "I");

  if (fid != NULL)
    return get_field<jint>(m_object, fid);
  else
  {
    xbmc_jnienv()->ExceptionClear();
    return -1;
  }
}

void CJNIWindowManagerLayoutParams::setpreferredDisplayModeId(int modeid)
{
  jhclass clazz = get_class(m_object);
  jfieldID fid = get_field_id<jclass>(clazz, "preferredDisplayModeId", "I");

  if (fid != NULL)
    return set_field(m_object, fid, modeid);
  else
    xbmc_jnienv()->ExceptionClear();
}
