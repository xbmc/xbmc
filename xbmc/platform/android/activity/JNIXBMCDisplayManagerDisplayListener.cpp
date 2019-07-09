/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCDisplayManagerDisplayListener.h"

#include "CompileInfo.h"
#include "XBMCApp.h"

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCDisplayManagerDisplayListener";

CJNIXBMCDisplayManagerDisplayListener::CJNIXBMCDisplayManagerDisplayListener()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();
}

void CJNIXBMCDisplayManagerDisplayListener::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onDisplayAdded", "(I)V", reinterpret_cast<void*>(&CJNIXBMCDisplayManagerDisplayListener::_onDisplayAdded)},
      {"_onDisplayChanged", "(I)V", reinterpret_cast<void*>(&CJNIXBMCDisplayManagerDisplayListener::_onDisplayChanged)},
      {"_onDisplayRemoved", "(I)V", reinterpret_cast<void*>(&CJNIXBMCDisplayManagerDisplayListener::_onDisplayRemoved)},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCDisplayManagerDisplayListener::_onDisplayAdded(JNIEnv *env, jobject context, jint displayId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  CXBMCApp::get()->onDisplayAdded(displayId);
}

void CJNIXBMCDisplayManagerDisplayListener::_onDisplayChanged(JNIEnv *env, jobject context, jint displayId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  CXBMCApp::get()->onDisplayChanged(displayId);
}

void CJNIXBMCDisplayManagerDisplayListener::_onDisplayRemoved(JNIEnv *env, jobject context, jint displayId)
{
  static_cast<void>(env);
  static_cast<void>(context);

  CXBMCApp::get()->onDisplayRemoved(displayId);
}
