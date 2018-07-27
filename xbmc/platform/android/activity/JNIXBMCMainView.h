/*
 *  Copyright (C) 2017 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

#include <androidjni/Context.h>
#include <androidjni/Surface.h>
#include <androidjni/SurfaceHolder.h>

#include "threads/Event.h"
#include "utils/Geometry.h"

class CJNIXBMCMainView : virtual public CJNIBase, public CJNISurfaceHolderCallback, public CJNIInterfaceImplem<CJNIXBMCMainView>
{
public:
  CJNIXBMCMainView(CJNISurfaceHolderCallback* callback);
  ~CJNIXBMCMainView();

  static void RegisterNatives(JNIEnv* env);

  // CJNISurfaceHolderCallback interface
  void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height);
  void surfaceCreated(CJNISurfaceHolder holder);
  void surfaceDestroyed(CJNISurfaceHolder holder);

  void attach(const jobject& thiz);
  bool waitForSurface(unsigned int millis);
  bool isActive() { return m_surfaceCreated.Signaled(); }
  CJNISurface getSurface();
  bool isCreated() const;

protected:
  static CJNIXBMCMainView* m_instance;
  CJNISurfaceHolderCallback* m_callback;
  static CEvent m_surfaceCreated;

  static void _attach(JNIEnv* env, jobject thiz);
  static void _surfaceChanged(JNIEnv* env, jobject thiz, jobject holder, jint format, jint width, jint height);
  static void _surfaceCreated(JNIEnv* env, jobject thiz, jobject holder);
  static void _surfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder);
};
