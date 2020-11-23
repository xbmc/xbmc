/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCURIUtils.h"

#include "CompileInfo.h"
#include "utils/URIUtils.h"

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCURIUtils";

void CJNIXBMCURIUtils::RegisterNatives(JNIEnv *env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_substitutePath", "(Ljava/lang/String;)Ljava/lang/String;", (void*)&CJNIXBMCURIUtils::_substitutePath},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

jstring CJNIXBMCURIUtils::_substitutePath(JNIEnv *env, jobject thiz, jstring path)
{
  std::string strPath = jcast<std::string>(jhstring::fromJNI(path));
  std::string responseData = URIUtils::SubstitutePath(strPath);

  jstring jres = env->NewStringUTF(responseData.c_str());
  return jres;
}
