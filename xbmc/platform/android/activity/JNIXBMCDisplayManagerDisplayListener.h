/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

namespace jni
{

class CJNIXBMCDisplayManagerDisplayListener : public CJNIBase
{
public:
  CJNIXBMCDisplayManagerDisplayListener();
  CJNIXBMCDisplayManagerDisplayListener(const jni::jhobject &object) : CJNIBase(object) {}

  static void RegisterNatives(JNIEnv* env);

protected:
  static void _onDisplayAdded(JNIEnv* env, jobject thiz, int displayId);
  static void _onDisplayChanged(JNIEnv* env, jobject thiz, int displayId);
  static void _onDisplayRemoved(JNIEnv* env, jobject thiz, int displayId);
};

} // namespace jni
