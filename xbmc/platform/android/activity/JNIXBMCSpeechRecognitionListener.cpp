/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCSpeechRecognitionListener.h"

#include "CompileInfo.h"
#include "utils/log.h"

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string s_className =
    std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCSpeechRecognitionListener";

void CJNIXBMCSpeechRecognitionListener::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if (cClass)
  {
    JNINativeMethod methods[] = {
        {"_onReadyForSpeech", "(Landroid/os/Bundle;)V",
         reinterpret_cast<void*>(&CJNIXBMCSpeechRecognitionListener::_onReadyForSpeech)},
        {"_onError", "(I)V", reinterpret_cast<void*>(&CJNIXBMCSpeechRecognitionListener::_onError)},
        {"_onResults", "(Landroid/os/Bundle;)V",
         reinterpret_cast<void*>(&CJNIXBMCSpeechRecognitionListener::_onResults)},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods) / sizeof(methods[0]));
  }
}

CJNIXBMCSpeechRecognitionListener::CJNIXBMCSpeechRecognitionListener() : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCSpeechRecognitionListener::~CJNIXBMCSpeechRecognitionListener()
{
  remove_instance(this);
}

void CJNIXBMCSpeechRecognitionListener::_onReadyForSpeech(JNIEnv* env, jobject thiz, jobject bundle)
{
  CJNIXBMCSpeechRecognitionListener* inst = find_instance(thiz);
  if (inst)
    inst->onReadyForSpeech(CJNIBundle(jhobject::fromJNI(bundle)));
  else
    CLog::LogF(LOGERROR, "Cannot find speech recognizer listener instance");
}

void CJNIXBMCSpeechRecognitionListener::_onError(JNIEnv* env, jobject thiz, jint i)
{
  CLog::LogF(LOGERROR, "Speech recognition error: {}", i);

  CJNIXBMCSpeechRecognitionListener* inst = find_instance(thiz);
  if (inst)
    inst->onError(i);
  else
    CLog::LogF(LOGERROR, "Cannot find speech recognizer listener instance");
}

void CJNIXBMCSpeechRecognitionListener::_onResults(JNIEnv* env, jobject thiz, jobject bundle)
{
  CJNIXBMCSpeechRecognitionListener* inst = find_instance(thiz);
  if (inst)
    inst->onResults(CJNIBundle(jhobject::fromJNI(bundle)));
  else
    CLog::LogF(LOGERROR, "Cannot find speech recognizer listener instance");
}
