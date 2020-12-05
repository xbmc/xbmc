/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

namespace jni
{

  class CJNIXBMCURIUtils : public CJNIBase
  {
  public:
	CJNIXBMCURIUtils(const jni::jhobject &object) : CJNIBase(object) {}

    static void RegisterNatives(JNIEnv* env);


  protected:
    ~CJNIXBMCURIUtils() override = default;

    static jstring _substitutePath(JNIEnv* env, jobject thiz, jstring path);

  };

}
