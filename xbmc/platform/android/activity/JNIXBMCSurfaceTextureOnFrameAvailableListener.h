/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>
#include <androidjni/SurfaceTexture.h>

namespace jni
{

class CJNIXBMCSurfaceTextureOnFrameAvailableListener : public CJNISurfaceTextureOnFrameAvailableListener, public CJNIInterfaceImplem<CJNIXBMCSurfaceTextureOnFrameAvailableListener>
{
public:
  CJNIXBMCSurfaceTextureOnFrameAvailableListener();
  CJNIXBMCSurfaceTextureOnFrameAvailableListener(const CJNIXBMCSurfaceTextureOnFrameAvailableListener& other);
  CJNIXBMCSurfaceTextureOnFrameAvailableListener(const jni::jhobject &object) : CJNIBase(object) {}
  ~CJNIXBMCSurfaceTextureOnFrameAvailableListener() override;

  static void RegisterNatives(JNIEnv* env);

  void onFrameAvailable(CJNISurfaceTexture) override {}

protected:
  static void _onFrameAvailable(JNIEnv* env, jobject thiz, jobject surface);
};

} // namespace jni
