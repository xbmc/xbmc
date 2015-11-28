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
#include "Activity.h"
#include "ClassLoader.h"

#include "jutils/jutils-details.hpp"

#include "platform/android/activity/JNIMainActivity.h"
#include <algorithm>

using namespace jni;

int CJNIAudioManager::STREAM_MUSIC(3);

int CJNIAudioManager::AUDIOFOCUS_GAIN(0x00000001);
int CJNIAudioManager::AUDIOFOCUS_LOSS(0xffffffff);
int CJNIAudioManager::AUDIOFOCUS_REQUEST_GRANTED(0x00000001);
int CJNIAudioManager::AUDIOFOCUS_REQUEST_FAILED(0x00000000);

void CJNIAudioManager::PopulateStaticFields()
{
  jhclass clazz = find_class("platform/android/media/AudioManager");
  STREAM_MUSIC  = (get_static_field<int>(clazz, "STREAM_MUSIC"));
  AUDIOFOCUS_GAIN  = (get_static_field<int>(clazz, "AUDIOFOCUS_GAIN"));
  AUDIOFOCUS_LOSS  = (get_static_field<int>(clazz, "AUDIOFOCUS_LOSS"));
  AUDIOFOCUS_REQUEST_GRANTED  = (get_static_field<int>(clazz, "AUDIOFOCUS_REQUEST_GRANTED"));
  AUDIOFOCUS_REQUEST_FAILED  = (get_static_field<int>(clazz, "AUDIOFOCUS_REQUEST_FAILED"));
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

int CJNIAudioManager::requestAudioFocus(const CJNIAudioManagerAudioFocusChangeListener &listener, int streamType, int durationHint)
{
  return call_method<int>(m_object,
                          "requestAudioFocus",
                          "(Landroid/media/AudioManager$OnAudioFocusChangeListener;II)I", listener.get_raw(), streamType, durationHint);
}

int CJNIAudioManager::abandonAudioFocus(const CJNIAudioManagerAudioFocusChangeListener &listener)
{
  return call_method<int>(m_object,
                          "abandonAudioFocus",
                          "(Landroid/media/AudioManager$OnAudioFocusChangeListener;)I", listener.get_raw());
}

bool CJNIAudioManager::isBluetoothA2dpOn()
{
  return call_method<jboolean>(m_object,
                               "isBluetoothA2dpOn",
                               "()Z");
}

bool CJNIAudioManager::isWiredHeadsetOn()
{
  return call_method<jboolean>(m_object,
                               "isWiredHeadsetOn",
                               "()Z");
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
CJNIAudioManagerAudioFocusChangeListener* CJNIAudioManagerAudioFocusChangeListener::m_listenerInstance(NULL);

CJNIAudioManagerAudioFocusChangeListener::CJNIAudioManagerAudioFocusChangeListener()
: CJNIBase(CJNIContext::getPackageName() + ".XBMCOnAudioFocusChangeListener")
{
  CJNIMainActivity *appInstance = CJNIMainActivity::GetAppInstance();
  if (!appInstance)
    return;

  // Convert "the/class/name" to "the.class.name" as loadClass() expects it.
  std::string dotClassName = GetClassName();
  std::replace(dotClassName.begin(), dotClassName.end(), '/', '.');
  m_object = new_object(appInstance->getClassLoader().loadClass(dotClassName));
  m_object.setGlobal();

  m_listenerInstance = this;
}

void CJNIAudioManagerAudioFocusChangeListener::_onAudioFocusChange(JNIEnv *env, jobject context, jint focusChange)
{
  (void)env;
  (void)context;
  if (m_listenerInstance)
  {
    m_listenerInstance->onAudioFocusChange(focusChange);
  }
}
