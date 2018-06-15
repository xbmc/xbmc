/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

