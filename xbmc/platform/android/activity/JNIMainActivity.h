/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/Activity.h>
#include <androidjni/InputManager.h>
#include <androidjni/Rect.h>

namespace jni
{

class CJNIMainActivity : public CJNIActivity, public CJNIInputManagerInputDeviceListener
{
public:
  explicit CJNIMainActivity(const ANativeActivity *nativeActivity);
  ~CJNIMainActivity() override;

  static CJNIMainActivity* GetAppInstance() { return m_appInstance; }

  static void RegisterNatives(JNIEnv* env);

  static void _onNewIntent(JNIEnv *env, jobject context, jobject intent);
  static void _onActivityResult(JNIEnv *env, jobject context, jint requestCode, jint resultCode, jobject resultData);
  static void _onVolumeChanged(JNIEnv *env, jobject context, jint volume);
  static void _doFrame(JNIEnv *env, jobject context, jlong frameTimeNanos);
  static void _onInputDeviceAdded(JNIEnv *env, jobject context, jint deviceId);
  static void _onInputDeviceChanged(JNIEnv *env, jobject context, jint deviceId);
  static void _onInputDeviceRemoved(JNIEnv *env, jobject context, jint deviceId);
  static void _onVisibleBehindCanceled(JNIEnv *env, jobject context);

  static void _callNative(JNIEnv *env, jobject context, jlong funcAddr, jlong variantAddr);
  static void runNativeOnUiThread(void (*callback)(void*), void* variant);
  static void registerMediaButtonEventReceiver();
  static void unregisterMediaButtonEventReceiver();

  CJNIRect getDisplayRect();

private:
  static CJNIMainActivity *m_appInstance;

protected:
  virtual void onNewIntent(CJNIIntent intent)=0;
  virtual void onActivityResult(int requestCode, int resultCode, CJNIIntent resultData)=0;
  virtual void onVolumeChanged(int volume)=0;
  virtual void doFrame(int64_t frameTimeNanos)=0;
  void onVisibleBehindCanceled() override = 0;

  virtual void onDisplayAdded(int displayId)=0;
  virtual void onDisplayChanged(int displayId)=0;
  virtual void onDisplayRemoved(int displayId)=0;
};

} // namespace jni
