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

class CJNIXBMCNsdManagerDiscoveryListener : public CJNINsdManagerDiscoveryListener, public CJNIInterfaceImplem<CJNIXBMCNsdManagerDiscoveryListener>
{
public:
  CJNIXBMCNsdManagerDiscoveryListener();
  CJNIXBMCNsdManagerDiscoveryListener(const CJNIXBMCNsdManagerDiscoveryListener& other); 
  explicit CJNIXBMCNsdManagerDiscoveryListener(const jni::jhobject &object) : CJNIBase(object) {}
  virtual ~CJNIXBMCNsdManagerDiscoveryListener();
  
  static void RegisterNatives(JNIEnv* env);
   
  // CJNINsdManagerDiscoveryListener interface
public:
  void onDiscoveryStarted(const std::string& serviceType) = 0;
  void onDiscoveryStopped(const std::string& serviceType) = 0;
  void onServiceFound(const CJNINsdServiceInfo& serviceInfo) = 0;
  void onServiceLost(const CJNINsdServiceInfo& serviceInfo) = 0;
  void onStartDiscoveryFailed(const std::string& serviceType, int errorCode) = 0;
  void onStopDiscoveryFailed(const std::string& serviceType, int errorCode) = 0;

protected:
  static void _onDiscoveryStarted(JNIEnv* env, jobject thiz, jstring serviceType);
  static void _onDiscoveryStopped(JNIEnv* env, jobject thiz, jstring serviceType);
  static void _onServiceFound(JNIEnv* env, jobject thiz, jobject serviceInfo);
  static void _onServiceLost(JNIEnv* env, jobject thiz, jobject serviceInfo);
  static void _onStartDiscoveryFailed(JNIEnv* env, jobject thiz, jstring serviceType, jint errorCode);
  static void _onStopDiscoveryFailed(JNIEnv* env, jobject thiz, jstring serviceType, jint errorCode);
};

}

