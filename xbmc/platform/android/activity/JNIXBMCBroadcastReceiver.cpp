/*
 *  Copyright (C) 2012-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCBroadcastReceiver.h"

#include "CompileInfo.h"

#include <androidjni/BroadcastReceiver.h>

using namespace jni;

namespace
{

static std::string className = std::string(CCompileInfo::GetClass()) + "/XBMCBroadcastReceiver";

} // namespace

void CJNIXBMCBroadcastReceiver::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(className.c_str());
  if (cClass)
  {
    JNINativeMethod methods[] = {
        {"_onReceive", "(Landroid/content/Intent;)V",
         reinterpret_cast<void*>(&CJNIBroadcastReceiver::_onReceive)},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods) / sizeof(methods[0]));
  }
}
