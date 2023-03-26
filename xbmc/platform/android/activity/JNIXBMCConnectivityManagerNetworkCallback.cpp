/*
 *  Copyright (C) 2012-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCConnectivityManagerNetworkCallback.h"

#include "CompileInfo.h"

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

namespace
{

static std::string className =
    std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCConnectivityManagerNetworkCallback";

} // namespace

CJNIXBMCConnectivityManagerNetworkCallback::CJNIXBMCConnectivityManagerNetworkCallback()
  : CJNIBase(className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCConnectivityManagerNetworkCallback::~CJNIXBMCConnectivityManagerNetworkCallback()
{
  remove_instance(this);
}

void CJNIXBMCConnectivityManagerNetworkCallback::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(className.c_str());
  if (cClass)
  {
    JNINativeMethod methods[] = {
        {"_onAvailable", "(Landroid/net/Network;)V",
         reinterpret_cast<void*>(&CJNIXBMCConnectivityManagerNetworkCallback::_onAvailable)},
        {"_onLost", "(Landroid/net/Network;)V",
         reinterpret_cast<void*>(&CJNIXBMCConnectivityManagerNetworkCallback::_onLost)},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods) / sizeof(methods[0]));
  }
}

void CJNIXBMCConnectivityManagerNetworkCallback::_onAvailable(JNIEnv* env,
                                                              jobject thiz,
                                                              jobject network)
{
  CJNIXBMCConnectivityManagerNetworkCallback* inst = find_instance(thiz);
  if (inst)
    inst->onAvailable(CJNINetwork(jhobject::fromJNI(network)));
}

void CJNIXBMCConnectivityManagerNetworkCallback::_onLost(JNIEnv* env, jobject thiz, jobject network)
{
  CJNIXBMCConnectivityManagerNetworkCallback* inst = find_instance(thiz);
  if (inst)
    inst->onLost(CJNINetwork(jhobject::fromJNI(network)));
}
