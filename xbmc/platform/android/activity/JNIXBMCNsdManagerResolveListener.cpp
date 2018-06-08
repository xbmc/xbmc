/*
 *      Copyright (C) 2016 Christian Browet
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

#include "JNIXBMCNsdManagerResolveListener.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Context.h>
#include <androidjni/NsdServiceInfo.h>

#include "CompileInfo.h"

using namespace jni;


static std::string s_className = std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCNsdManagerResolveListener";

CJNIXBMCNsdManagerResolveListener::CJNIXBMCNsdManagerResolveListener()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCNsdManagerResolveListener::CJNIXBMCNsdManagerResolveListener(const CJNIXBMCNsdManagerResolveListener& other)
  : CJNIBase(other)
{
  add_instance(m_object, this);
}

CJNIXBMCNsdManagerResolveListener::~CJNIXBMCNsdManagerResolveListener()
{
  remove_instance(this);
}

void CJNIXBMCNsdManagerResolveListener::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onResolveFailed", "(Landroid/net/nsd/NsdServiceInfo;I)V", (void*)&CJNIXBMCNsdManagerResolveListener::_onResolveFailed},
      {"_onServiceResolved", "(Landroid/net/nsd/NsdServiceInfo;)V", (void*)&CJNIXBMCNsdManagerResolveListener::_onServiceResolved},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCNsdManagerResolveListener::_onResolveFailed(JNIEnv* env, jobject thiz, jobject serviceInfo, jint errorCode)
{
  CJNIXBMCNsdManagerResolveListener *inst = find_instance(thiz);
  if (inst)
    inst->onResolveFailed(CJNINsdServiceInfo(jhobject::fromJNI(serviceInfo)), errorCode);
}

void CJNIXBMCNsdManagerResolveListener::_onServiceResolved(JNIEnv* env, jobject thiz, jobject serviceInfo)
{
  CJNIXBMCNsdManagerResolveListener *inst = find_instance(thiz);
  if (inst)
    inst->onServiceResolved(CJNINsdServiceInfo(jhobject::fromJNI(serviceInfo)));
}
