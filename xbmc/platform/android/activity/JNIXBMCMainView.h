/*
 *  Copyright (C) 2017 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Event.h"
#include "utils/Geometry.h"

#include <androidjni/Context.h>
#include <androidjni/JNIBase.h>
#include <androidjni/Surface.h>
#include <androidjni/SurfaceHolder.h>

namespace jni
{

class CJNIXBMCMainView : virtual public CJNIBase, public CJNISurfaceHolderCallback, public CJNIInterfaceImplem<CJNIXBMCMainView>
{
public:
  CJNIXBMCMainView(CJNISurfaceHolderCallback* callback);
  ~CJNIXBMCMainView() override = default;

  static void RegisterNatives(JNIEnv* env);

  // CJNISurfaceHolderCallback interface
  void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) override;
  void surfaceCreated(CJNISurfaceHolder holder) override;
  void surfaceDestroyed(CJNISurfaceHolder holder) override;

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

} // namespace jni
