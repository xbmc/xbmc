/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCNsdManagerResolveListener.h"

#include "CompileInfo.h"

#include <androidjni/Context.h>
#include <androidjni/NsdServiceInfo.h>
#include <androidjni/jutils-details.hpp>

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
