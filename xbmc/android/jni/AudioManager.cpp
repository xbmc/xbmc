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

#include "AudioManager.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIAudioManager::STREAM_MUSIC(3);

void CJNIAudioManager::PopulateStaticFields()
{
  jhclass clazz = find_class("android/media/AudioManager");
  STREAM_MUSIC  = (get_static_field<int>(clazz, "STREAM_MUSIC"));
}

int CJNIAudioManager::getStreamMaxVolume()
{
  return call_method<jint>(m_object,
    "getStreamMaxVolume", "(I)I",
    STREAM_MUSIC);
}

int CJNIAudioManager::getStreamVolume()
{
  return call_method<int>(m_object,
    "getStreamVolume", "(I)I",
    STREAM_MUSIC);
}

void CJNIAudioManager::setStreamVolume(int index /* 0 */, int flags /* NONE */)
{
  call_method<void>(m_object,
    "setStreamVolume", "(III)V",
    STREAM_MUSIC, index, flags);
}
