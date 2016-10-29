/*
 *      Copyright (C) 2016 Chris Browet
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

#include "PlaybackParams.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIPlaybackParams::CJNIPlaybackParams()
  : CJNIBase("android/media/PlaybackParams")
{
  m_object = new_object(GetClassName());

  JNIEnv* jenv = xbmc_jnienv();
  jthrowable exception = jenv->ExceptionOccurred();
  if (exception)
  {
    jenv->ExceptionClear();
    jhclass excClass = find_class(jenv, "java/lang/Throwable");
    jmethodID toStrMethod = get_method_id(jenv, excClass, "toString", "()Ljava/lang/String;");
    jhstring msg = call_method<jhstring>(exception, toStrMethod);
    throw std::invalid_argument(jcast<std::string>(msg));
  }
  m_object.setGlobal();
}

CJNIPlaybackParams::CJNIPlaybackParams(const jhobject& object)
  : CJNIBase(object)
{
}

CJNIPlaybackParams CJNIPlaybackParams::setSpeed(float speed)
{
  return call_method<jhobject>(m_object,
    "setSpeed", "(F)Landroid/media/PlaybackParams;", speed);
}

float CJNIPlaybackParams::getSpeed()
{
  return call_method<jfloat>(m_object,
    "getSpeed", "()F");
}

