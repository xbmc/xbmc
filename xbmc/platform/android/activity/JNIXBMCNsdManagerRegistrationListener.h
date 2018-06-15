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

