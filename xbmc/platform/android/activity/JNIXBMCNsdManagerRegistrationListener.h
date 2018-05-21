#pragma once
/*
 *      Copyright (C) 2016 Christian Browet
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <androidjni/JNIBase.h>

#include <androidjni/NsdManager.h>

namespace jni
{

class CJNIXBMCNsdManagerRegistrationListener : public CJNINsdManagerRegistrationListener, public CJNIInterfaceImplem<CJNIXBMCNsdManagerRegistrationListener>
{
public:
  CJNIXBMCNsdManagerRegistrationListener();
  CJNIXBMCNsdManagerRegistrationListener(const CJNIXBMCNsdManagerRegistrationListener& other); 
  explicit CJNIXBMCNsdManagerRegistrationListener(const jni::jhobject &object) : CJNIBase(object) {}
  virtual ~CJNIXBMCNsdManagerRegistrationListener();

  static void RegisterNatives(JNIEnv* env);
  
  // CJNINsdManagerRegistrationListener interface
public:
  void onRegistrationFailed(const CJNINsdServiceInfo& serviceInfo, int errorCode) override {}
  void onServiceRegistered(const CJNINsdServiceInfo& serviceInfo) override {}
  void onServiceUnregistered(const CJNINsdServiceInfo& serviceInfo) override {}
  void onUnregistrationFailed(const CJNINsdServiceInfo& serviceInfo, int errorCode) override {}
  
protected:
  static void _onRegistrationFailed(JNIEnv* env, jobject thiz, jobject serviceInfo, jint errorCode);
  static void _onServiceRegistered(JNIEnv* env, jobject thiz, jobject serviceInfo);
  static void _onServiceUnregistered(JNIEnv* env, jobject thiz, jobject serviceInfo);
  static void _onUnregistrationFailed(JNIEnv* env, jobject thiz, jobject serviceInfo, jint errorCode);
};

}

