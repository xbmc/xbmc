/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCNsdManagerDiscoveryListener.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Context.h>
#include <androidjni/NsdServiceInfo.h>

#include "CompileInfo.h"
#include "utils/log.h"

using namespace jni;


static std::string s_className = std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCNsdManagerDiscoveryListener";

CJNIXBMCNsdManagerDiscoveryListener::CJNIXBMCNsdManagerDiscoveryListener()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCNsdManagerDiscoveryListener::CJNIXBMCNsdManagerDiscoveryListener(const CJNIXBMCNsdManagerDiscoveryListener& other)
  : CJNIBase(other)
{
  add_instance(m_object, this);
}

CJNIXBMCNsdManagerDiscoveryListener::~CJNIXBMCNsdManagerDiscoveryListener()
{
  remove_instance(this);
}

void CJNIXBMCNsdManagerDiscoveryListener::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onDiscoveryStarted", "(Ljava/lang/String;)V", (void*)&CJNIXBMCNsdManagerDiscoveryListener::_onDiscoveryStarted},
      {"_onDiscoveryStopped", "(Ljava/lang/String;)V", (void*)&CJNIXBMCNsdManagerDiscoveryListener::_onDiscoveryStopped},
      {"_onServiceFound", "(Landroid/net/nsd/NsdServiceInfo;)V", (void*)&CJNIXBMCNsdManagerDiscoveryListener::_onServiceFound},
      {"_onServiceLost", "(Landroid/net/nsd/NsdServiceInfo;)V", (void*)&CJNIXBMCNsdManagerDiscoveryListener::_onServiceLost},
      {"_onStartDiscoveryFailed", "(Ljava/lang/String;I)V", (void*)&CJNIXBMCNsdManagerDiscoveryListener::_onStartDiscoveryFailed},
      {"_onStopDiscoveryFailed", "(Ljava/lang/String;I)V", (void*)&CJNIXBMCNsdManagerDiscoveryListener::_onStopDiscoveryFailed},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCNsdManagerDiscoveryListener::_onDiscoveryStarted(JNIEnv* env, jobject thiz, jstring serviceType)
{
  (void)env;
  (void)thiz;

  std::string st = jcast<std::string>(jhstring(serviceType));
  CLog::Log(LOGDEBUG, "CJNIXBMCNsdManagerDiscoveryListener::onDiscoveryStarted type: %s", st.c_str());
}

void CJNIXBMCNsdManagerDiscoveryListener::_onDiscoveryStopped(JNIEnv* env, jobject thiz, jstring serviceType)
{
  (void)env;
  (void)thiz;

  std::string st = jcast<std::string>(jhstring(serviceType));
  CLog::Log(LOGDEBUG, "CJNIXBMCNsdManagerDiscoveryListener::onDiscoveryStopped type: %s", st.c_str());
}

void CJNIXBMCNsdManagerDiscoveryListener::_onServiceFound(JNIEnv* env, jobject thiz, jobject serviceInfo)
{
  CJNIXBMCNsdManagerDiscoveryListener *inst = find_instance(thiz);
  if (inst)
    inst->onServiceFound(CJNINsdServiceInfo(jhobject::fromJNI(serviceInfo)));
}

void CJNIXBMCNsdManagerDiscoveryListener::_onServiceLost(JNIEnv* env, jobject thiz, jobject serviceInfo)
{
  CJNIXBMCNsdManagerDiscoveryListener *inst = find_instance(thiz);
  if (inst)
    inst->onServiceLost(CJNINsdServiceInfo(jhobject::fromJNI(serviceInfo)));
}

void CJNIXBMCNsdManagerDiscoveryListener::_onStartDiscoveryFailed(JNIEnv* env, jobject thiz, jstring serviceType, jint errorCode)
{
  (void)env;

  CJNIXBMCNsdManagerDiscoveryListener *inst = find_instance(thiz);
  if (inst)
    inst->onStartDiscoveryFailed(jcast<std::string>(jhstring(serviceType)), errorCode);
}

void CJNIXBMCNsdManagerDiscoveryListener::_onStopDiscoveryFailed(JNIEnv* env, jobject thiz, jstring serviceType, jint errorCode)
{
  (void)env;

  CJNIXBMCNsdManagerDiscoveryListener *inst = find_instance(thiz);
  if (inst)
    inst->onStopDiscoveryFailed(jcast<std::string>(jhstring(serviceType)), errorCode);
}
