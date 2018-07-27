/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCAudioManagerOnAudioFocusChangeListener.h"
#include <androidjni/jutils-details.hpp>

#include <androidjni/Context.h>
#include "CompileInfo.h"
#include "XBMCApp.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCAudioManagerOnAudioFocusChangeListener";

CJNIXBMCAudioManagerOnAudioFocusChangeListener::CJNIXBMCAudioManagerOnAudioFocusChangeListener()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCAudioManagerOnAudioFocusChangeListener::CJNIXBMCAudioManagerOnAudioFocusChangeListener(const CJNIXBMCAudioManagerOnAudioFocusChangeListener& other)
  : CJNIBase(other)
{
  add_instance(m_object, this);
}

CJNIXBMCAudioManagerOnAudioFocusChangeListener::~CJNIXBMCAudioManagerOnAudioFocusChangeListener()
{
  remove_instance(this);
}

void CJNIXBMCAudioManagerOnAudioFocusChangeListener::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onAudioFocusChange", "(I)V", (void*)&CJNIXBMCAudioManagerOnAudioFocusChangeListener::_onAudioFocusChange},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCAudioManagerOnAudioFocusChangeListener::_onAudioFocusChange(JNIEnv *env, jobject thiz, jint focusChange)
{
  (void)env;

  CJNIXBMCAudioManagerOnAudioFocusChangeListener *inst = find_instance(thiz);
  if (inst)
    inst->onAudioFocusChange(focusChange);
}

void CJNIXBMCAudioManagerOnAudioFocusChangeListener::onAudioFocusChange(int focusChange)
{
  if(CXBMCApp::get())
    CXBMCApp::get()->onAudioFocusChange(focusChange);
}
