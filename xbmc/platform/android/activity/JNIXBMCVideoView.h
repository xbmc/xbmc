/*
 *  Copyright (C) 2016 Christian Browet
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
#include <androidjni/Rect.h>
#include <androidjni/Surface.h>
#include <androidjni/SurfaceHolder.h>

namespace jni
{

class CJNIXBMCVideoView : virtual public CJNIBase, public CJNISurfaceHolderCallback, public CJNIInterfaceImplem<CJNIXBMCVideoView>
{
public:
  CJNIXBMCVideoView(const jni::jhobject &object);
  ~CJNIXBMCVideoView() override = default;

  static void RegisterNatives(JNIEnv* env);

  static CJNIXBMCVideoView* createVideoView(CJNISurfaceHolderCallback* callback);

  // CJNISurfaceHolderCallback interface
  void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) override;
  void surfaceCreated(CJNISurfaceHolder holder) override;
  void surfaceDestroyed(CJNISurfaceHolder holder) override;

  static void _surfaceChanged(JNIEnv* env, jobject thiz, jobject holder, jint format, jint width, jint height);
  static void _surfaceCreated(JNIEnv* env, jobject thiz, jobject holder);
  static void _surfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder);

  bool waitForSurface(unsigned int millis);
  bool isActive() { return m_surfaceCreated.Signaled(); }
  CJNISurface getSurface();
  const CRect& getSurfaceRect();
  void setSurfaceRect(const CRect& rect);
  void add();
  void release();
  int ID() const;
  bool isCreated() const;

protected:
  CJNISurfaceHolderCallback* m_callback = nullptr;
  CEvent m_surfaceCreated;
  CRect m_surfaceRect;

private:
  CJNIXBMCVideoView() = default;
};

} // namespace jni
