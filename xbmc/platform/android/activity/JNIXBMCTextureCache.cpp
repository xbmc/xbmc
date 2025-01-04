/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCTextureCache.h"

#include "CompileInfo.h"
#include "TextureCache.h"

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCTextureCache";

void CJNIXBMCTextureCache::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if (cClass)
  {
    JNINativeMethod methods[] = {
        {"_unwrapImageURL", "(Ljava/lang/String;)Ljava/lang/String;",
         (void*)&CJNIXBMCTextureCache::_unwrapImageURL},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods) / sizeof(methods[0]));
  }
}

jstring CJNIXBMCTextureCache::_unwrapImageURL(JNIEnv* env, jobject thiz, jstring image)
{
  std::string strImage = jcast<std::string>(jhstring::fromJNI(image));
  std::string responseData = CTextureUtils::UnwrapImageURL(strImage);

  jstring jres = env->NewStringUTF(responseData.c_str());
  return jres;
}
