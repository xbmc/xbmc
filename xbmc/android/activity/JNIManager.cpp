/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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
#include "JNIManager.h"
#include "XBMCApp.h"
#include "BroadcastReceiver.h"
#include <android/log.h>


CAndroidJNIManager::CAndroidJNIManager()
{
  m_broadcastReceiver = NULL;
  m_oActivity = NULL;
}


CAndroidJNIManager::~CAndroidJNIManager()
{
  delete m_broadcastReceiver;
}

bool CAndroidJNIManager::RegisterClass(JNIEnv* env, CAndroidJNIBase *native)
{
    __android_log_print(ANDROID_LOG_VERBOSE, "XBMC", "Registering class");
  jclass javaClass = NULL;
  if (!native)
    return false;

  // Don't register more than once.
  if (native->m_class)
    return false;

  try
  {
    javaClass = env->FindClass(native->GetClassName().c_str());
    if (javaClass && native->m_jniMethods.size())
    {
      if(env->RegisterNatives(javaClass, (JNINativeMethod*)&native->m_jniMethods[0], native->m_jniMethods.size()) != 0)
        return false;
    }
  }
  catch(...)
  {
  }
  if (javaClass)
  {
    native->m_class = reinterpret_cast<jclass>(env->NewGlobalRef(javaClass));
    __android_log_print(ANDROID_LOG_VERBOSE, "XBMC", "Registered class %s",native->GetClassName().c_str());
    return true;
  }
  return false;
}

bool CAndroidJNIManager::Load(JavaVM* vm, int jniVersion)
  {
    if (!vm)
      return false;

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), jniVersion) != JNI_OK) {
        return false;
    }

    m_broadcastReceiver = new CBroadcastReceiver();
    RegisterClass(env, m_broadcastReceiver);

    return true;
  }

  // This is a special function called when libxbmc is loaded. It sets up our
  // internal functions so that we can use them in native code much more simply.
  // It loads a array of methods, params, and function-pointers.
extern "C"  jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
  int jniVersion = JNI_VERSION_1_6;
  return CAndroidJNIManager::GetInstance().Load(vm, jniVersion) == true ? jniVersion : -1;
}
