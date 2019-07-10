/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCSurfaceTextureOnFrameAvailableListener.h"

#include "CompileInfo.h"

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCSurfaceTextureOnFrameAvailableListener";

CJNIXBMCSurfaceTextureOnFrameAvailableListener::CJNIXBMCSurfaceTextureOnFrameAvailableListener()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCSurfaceTextureOnFrameAvailableListener::CJNIXBMCSurfaceTextureOnFrameAvailableListener(const CJNIXBMCSurfaceTextureOnFrameAvailableListener& other)
  : CJNIBase(other)
{
  add_instance(m_object, this);
}

CJNIXBMCSurfaceTextureOnFrameAvailableListener::~CJNIXBMCSurfaceTextureOnFrameAvailableListener()
{
  remove_instance(this);
}

void CJNIXBMCSurfaceTextureOnFrameAvailableListener::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onFrameAvailable", "(Landroid/graphics/SurfaceTexture;)V", (void*)&CJNIXBMCSurfaceTextureOnFrameAvailableListener::_onFrameAvailable},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCSurfaceTextureOnFrameAvailableListener::_onFrameAvailable(JNIEnv* env, jobject thiz, jobject surface)
{
  (void)env;

  CJNIXBMCSurfaceTextureOnFrameAvailableListener *inst = find_instance(thiz);
  if (inst)
    inst->onFrameAvailable(CJNISurfaceTexture(jhobject::fromJNI(surface)));
}
