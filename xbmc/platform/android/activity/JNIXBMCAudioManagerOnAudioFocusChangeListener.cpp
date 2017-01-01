/*
 *      Copyright (C) 2016 Christian Browet
 *      http://xbmc.org
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

  CJNIXBMCAudioManagerOnAudioFocusChangeListener *inst = find_instance(jhobject(thiz));
  if (inst)
    inst->onAudioFocusChange(focusChange);
}

void CJNIXBMCAudioManagerOnAudioFocusChangeListener::onAudioFocusChange(int focusChange)
{
  if(CXBMCApp::get())
    CXBMCApp::get()->onAudioFocusChange(focusChange);
}
