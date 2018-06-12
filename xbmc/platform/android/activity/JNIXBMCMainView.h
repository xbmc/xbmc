/*
 *      Copyright (C) 2017 Christian Browet
 *      http://kodi.tv
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
