#pragma once
/*
 *      Copyright (C) 2016 Team XBMC
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

#include <vector>

#include "JNIBase.h"
#include "View.h"

class CJNIInputManagerInputDeviceListener : public CJNIBase
{
public:
  CJNIInputManagerInputDeviceListener(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIInputManagerInputDeviceListener() {};

  static void _onInputDeviceAdded(JNIEnv *env, jobject context, jint deviceId);
  static void _onInputDeviceChanged(JNIEnv *env, jobject context, jint deviceId);
  static void _onInputDeviceRemoved(JNIEnv *env, jobject context, jint deviceId);

protected:
  CJNIInputManagerInputDeviceListener();

  virtual void onInputDeviceAdded(int deviceId) = 0;
  virtual void onInputDeviceChanged(int deviceId) = 0;
  virtual void onInputDeviceRemoved(int deviceId) = 0;

private:
  static CJNIInputManagerInputDeviceListener *m_listenerInstance;
};

class CJNIInputManager : public CJNIBase
{
public:
  CJNIInputManager(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIInputManager() {};

  const CJNIViewInputDevice getInputDevice(int id) const;
  std::vector<int> getInputDeviceIds() const;

private:
  CJNIInputManager();
};
