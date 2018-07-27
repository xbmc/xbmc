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

class CJNIXBMCNsdManagerResolveListener : public CJNINsdManagerResolveListener, public CJNIInterfaceImplem<CJNIXBMCNsdManagerResolveListener>
{
public:
public:
  CJNIXBMCNsdManagerResolveListener();
  CJNIXBMCNsdManagerResolveListener(const CJNIXBMCNsdManagerResolveListener& other);
  explicit CJNIXBMCNsdManagerResolveListener(const jni::jhobject &object) : CJNIBase(object) {}
  virtual ~CJNIXBMCNsdManagerResolveListener();

  static void RegisterNatives(JNIEnv* env);

  // CJNINsdManagerResolveListener interface
  void onResolveFailed(const CJNINsdServiceInfo& serviceInfo, int errorCode) = 0;
  void onServiceResolved(const CJNINsdServiceInfo& serviceInfo) = 0;

protected:
  static void _onResolveFailed(JNIEnv* env, jobject thiz, jobject serviceInfo, jint errorCode);
  static void _onServiceResolved(JNIEnv* env, jobject thiz, jobject serviceInfo);

};

}

