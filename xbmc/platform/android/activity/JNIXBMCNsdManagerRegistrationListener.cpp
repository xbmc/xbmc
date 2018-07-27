/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCNsdManagerRegistrationListener.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Context.h>
#include <androidjni/NsdServiceInfo.h>

#include "CompileInfo.h"
#include "utils/log.h"

using namespace jni;


static std::string s_className = std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCNsdManagerRegistrationListener";

CJNIXBMCNsdManagerRegistrationListener::CJNIXBMCNsdManagerRegistrationListener()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCNsdManagerRegistrationListener::CJNIXBMCNsdManagerRegistrationListener(const CJNIXBMCNsdManagerRegistrationListener& other)
  : CJNIBase(other)
{
  add_instance(m_object, this);
}

CJNIXBMCNsdManagerRegistrationListener::~CJNIXBMCNsdManagerRegistrationListener()
{
  remove_instance(this);
}

void CJNIXBMCNsdManagerRegistrationListener::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onRegistrationFailed", "(Landroid/net/nsd/NsdServiceInfo;I)V", (void*)&CJNIXBMCNsdManagerRegistrationListener::_onRegistrationFailed},
      {"_onServiceRegistered", "(Landroid/net/nsd/NsdServiceInfo;)V", (void*)&CJNIXBMCNsdManagerRegistrationListener::_onServiceRegistered},
      {"_onServiceUnregistered", "(Landroid/net/nsd/NsdServiceInfo;)V", (void*)&CJNIXBMCNsdManagerRegistrationListener::_onServiceUnregistered},
      {"_onUnregistrationFailed", "(Landroid/net/nsd/NsdServiceInfo;I)V", (void*)&CJNIXBMCNsdManagerRegistrationListener::_onUnregistrationFailed},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCNsdManagerRegistrationListener::_onRegistrationFailed(JNIEnv* env, jobject thiz, jobject serviceInfo, jint errorCode)
{
  CJNINsdServiceInfo si = CJNINsdServiceInfo(jhobject::fromJNI(serviceInfo));
  CLog::Log(LOGERROR, "ZeroconfAndroid: %s.%s registration failed: %d", si.getServiceName().c_str(), si.getServiceType().c_str(), errorCode);
}

void CJNIXBMCNsdManagerRegistrationListener::_onServiceRegistered(JNIEnv* env, jobject thiz, jobject serviceInfo)
{
  CJNINsdServiceInfo si = CJNINsdServiceInfo(jhobject::fromJNI(serviceInfo));
  CLog::Log(LOGINFO, "ZeroconfAndroid: %s.%s now registered and active", si.getServiceName().c_str(), si.getServiceType().c_str());
}

void CJNIXBMCNsdManagerRegistrationListener::_onServiceUnregistered(JNIEnv* env, jobject thiz, jobject serviceInfo)
{
  CJNINsdServiceInfo si = CJNINsdServiceInfo(jhobject::fromJNI(serviceInfo));
  CLog::Log(LOGINFO, "ZeroconfAndroid: %s.%s registration removed", si.getServiceName().c_str(), si.getServiceType().c_str());
}

void CJNIXBMCNsdManagerRegistrationListener::_onUnregistrationFailed(JNIEnv* env, jobject thiz, jobject serviceInfo, jint errorCode)
{
  CJNINsdServiceInfo si = CJNINsdServiceInfo(jhobject::fromJNI(serviceInfo));
  CLog::Log(LOGERROR, "ZeroconfAndroid: %s.%s unregistration failed: %d", si.getServiceName().c_str(), si.getServiceType().c_str(), errorCode);
}

