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

#include <algorithm>

#include "InputManager.h"
#include "ClassLoader.h"
#include "Context.h"
#include "platform/android/activity/JNIMainActivity.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

/************************************************************************/
/************************************************************************/
CJNIInputManagerInputDeviceListener* CJNIInputManagerInputDeviceListener::m_listenerInstance = nullptr;

CJNIInputManagerInputDeviceListener::CJNIInputManagerInputDeviceListener()
  : CJNIBase(CJNIContext::getPackageName() + ".XBMCInputDeviceListener")
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

void CJNIInputManagerInputDeviceListener::_onInputDeviceAdded(JNIEnv *env, jobject context, jint deviceId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  if (m_listenerInstance != nullptr)
    m_listenerInstance->onInputDeviceAdded(deviceId);
}

void CJNIInputManagerInputDeviceListener::_onInputDeviceChanged(JNIEnv *env, jobject context, jint deviceId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  if (m_listenerInstance != nullptr)
    m_listenerInstance->onInputDeviceChanged(deviceId);
}

void CJNIInputManagerInputDeviceListener::_onInputDeviceRemoved(JNIEnv *env, jobject context, jint deviceId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  if (m_listenerInstance != nullptr)
    m_listenerInstance->onInputDeviceRemoved(deviceId);
}

/************************************************************************/
/************************************************************************/
const CJNIViewInputDevice CJNIInputManager::getInputDevice(int id) const
{
  return call_method<jhobject>(m_object,
    "getInputDevice", "(I)Landroid/view/InputDevice;",
    id);
}

std::vector<int> CJNIInputManager::getInputDeviceIds() const
{
  return jcast<std::vector<int>>(call_method<jhintArray>(m_object,
    "getInputDeviceIds", "()[I"));
}
