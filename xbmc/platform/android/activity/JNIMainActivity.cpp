/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIMainActivity.h"

#include "CompileInfo.h"

#include <androidjni/Activity.h>
#include <androidjni/Intent.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

CJNIMainActivity* CJNIMainActivity::m_appInstance(NULL);

CJNIMainActivity::CJNIMainActivity(const ANativeActivity *nativeActivity)
  : CJNIActivity(nativeActivity)
{
  m_appInstance = this;
}

CJNIMainActivity::~CJNIMainActivity()
{
  m_appInstance = NULL;
}

void CJNIMainActivity::RegisterNatives(JNIEnv* env)
{
  std::string pkgRoot = CCompileInfo::GetClass();

  const std::string mainClass = pkgRoot + "/Main";
  const std::string settingsObserver = pkgRoot + "/XBMCSettingsContentObserver";
  const std::string inputDeviceListener = pkgRoot + "/XBMCInputDeviceListener";

  jclass cMain = env->FindClass(mainClass.c_str());
  if (cMain)
  {
    JNINativeMethod methods[] = {
        {"_onNewIntent", "(Landroid/content/Intent;)V",
         reinterpret_cast<void*>(&CJNIMainActivity::_onNewIntent)},
        {"_onActivityResult", "(IILandroid/content/Intent;)V",
         reinterpret_cast<void*>(&CJNIMainActivity::_onActivityResult)},
        {"_doFrame", "(J)V", reinterpret_cast<void*>(&CJNIMainActivity::_doFrame)},
        {"_callNative", "(JJ)V", reinterpret_cast<void*>(&CJNIMainActivity::_callNative)},
        {"_onVisibleBehindCanceled", "()V",
         reinterpret_cast<void*>(&CJNIMainActivity::_onVisibleBehindCanceled)},
    };
    env->RegisterNatives(cMain, methods, sizeof(methods) / sizeof(methods[0]));
  }

  jclass cSettingsObserver = env->FindClass(settingsObserver.c_str());
  if (cSettingsObserver)
  {
    JNINativeMethod methods[] = {
        {"_onVolumeChanged", "(I)V", reinterpret_cast<void*>(&CJNIMainActivity::_onVolumeChanged)},
    };
    env->RegisterNatives(cSettingsObserver, methods, sizeof(methods) / sizeof(methods[0]));
  }

  jclass cInputDeviceListener = env->FindClass(inputDeviceListener.c_str());
  if (cInputDeviceListener)
  {
    JNINativeMethod methods[] = {
        {"_onInputDeviceAdded", "(I)V",
         reinterpret_cast<void*>(&CJNIMainActivity::_onInputDeviceAdded)},
        {"_onInputDeviceChanged", "(I)V",
         reinterpret_cast<void*>(&CJNIMainActivity::_onInputDeviceChanged)},
        {"_onInputDeviceRemoved", "(I)V",
         reinterpret_cast<void*>(&CJNIMainActivity::_onInputDeviceRemoved)}};
    env->RegisterNatives(cInputDeviceListener, methods, sizeof(methods) / sizeof(methods[0]));
  }
}

void CJNIMainActivity::_onNewIntent(JNIEnv *env, jobject context, jobject intent)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onNewIntent(CJNIIntent(jhobject::fromJNI(intent)));
}

void CJNIMainActivity::_onActivityResult(JNIEnv *env, jobject context, jint requestCode, jint resultCode, jobject resultData)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onActivityResult(requestCode, resultCode, CJNIIntent(jhobject::fromJNI(resultData)));
}

void CJNIMainActivity::_callNative(JNIEnv *env, jobject context, jlong funcAddr, jlong variantAddr)
{
  (void)env;
  (void)context;
  ((void (*)(CVariant *))funcAddr)((CVariant *)variantAddr);
}

void CJNIMainActivity::_onVisibleBehindCanceled(JNIEnv* env, jobject context)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onVisibleBehindCanceled();
}

void CJNIMainActivity::runNativeOnUiThread(void (*callback)(void*), void* variant)
{
  call_method<void>(m_context,
                    "runNativeOnUiThread", "(JJ)V", (jlong)callback, (jlong)variant);
}

void CJNIMainActivity::_onVolumeChanged(JNIEnv *env, jobject context, jint volume)
{
  (void)env;
  (void)context;
  if(m_appInstance)
    m_appInstance->onVolumeChanged(volume);
}

void CJNIMainActivity::_onInputDeviceAdded(JNIEnv *env, jobject context, jint deviceId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  if (m_appInstance != nullptr)
    m_appInstance->onInputDeviceAdded(deviceId);
}

void CJNIMainActivity::_onInputDeviceChanged(JNIEnv *env, jobject context, jint deviceId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  if (m_appInstance != nullptr)
    m_appInstance->onInputDeviceChanged(deviceId);
}

void CJNIMainActivity::_onInputDeviceRemoved(JNIEnv *env, jobject context, jint deviceId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  if (m_appInstance != nullptr)
    m_appInstance->onInputDeviceRemoved(deviceId);
}

void CJNIMainActivity::_doFrame(JNIEnv *env, jobject context, jlong frameTimeNanos)
{
  (void)env;
  (void)context;
  if(m_appInstance)
    m_appInstance->doFrame(frameTimeNanos);
}

CJNIRect CJNIMainActivity::getDisplayRect()
{
  return call_method<jhobject>(m_context,
                               "getDisplayRect", "()Landroid/graphics/Rect;");
}

void CJNIMainActivity::registerMediaButtonEventReceiver()
{
  call_method<void>(m_context,
                    "registerMediaButtonEventReceiver", "()V");
}

void CJNIMainActivity::unregisterMediaButtonEventReceiver()
{
  call_method<void>(m_context,
                    "unregisterMediaButtonEventReceiver", "()V");
}
