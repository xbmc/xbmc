/*
 *  Copyright (C) 2012-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCMediaCodecCallback.h"

#include "CompileInfo.h"

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string className =
    std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCMediaCodecCallback";

CJNIXBMCMediaCodecCallback::CJNIXBMCMediaCodecCallback() : CJNIBase(className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(ToDotClassName(className)));
  m_object.setGlobal();
  add_instance(m_object, this);
}

CJNIXBMCMediaCodecCallback::~CJNIXBMCMediaCodecCallback()
{
  Unregister();
}

void CJNIXBMCMediaCodecCallback::Unregister()
{
  remove_instance(this);
}

void CJNIXBMCMediaCodecCallback::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(className.c_str());
  if (cClass)
  {
    JNINativeMethod methods[] = {
        {"_onInputBufferAvailable", "(Landroid/media/MediaCodec;I)V",
         reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onInputBufferAvailable)},
        {"_onOutputBufferAvailable",
         "(Landroid/media/MediaCodec;ILandroid/media/MediaCodec$BufferInfo;)V",
         reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onOutputBufferAvailable)},
        {"_onOutputFormatChanged", "(Landroid/media/MediaCodec;Landroid/media/MediaFormat;)V",
         reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onOutputFormatChanged)},
        {"_onError", "(Landroid/media/MediaCodec;Landroid/media/MediaCodec$CodecException;)V",
         reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onError)},
        {"_onStartError", "()V",
         reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onStartError)}};

    env->RegisterNatives(cClass, methods, sizeof(methods) / sizeof(methods[0]));
  }
}

void CJNIXBMCMediaCodecCallback::_onInputBufferAvailable(JNIEnv* env,
                                                         jobject thiz,
                                                         jobject codec,
                                                         jint index)
{
  CJNIXBMCMediaCodecCallback* inst = find_instance(thiz);
  if (inst)
    inst->onInputBufferAvailable(CJNIMediaCodec(jhobject::fromJNI(codec)), index);
}

void CJNIXBMCMediaCodecCallback::_onOutputBufferAvailable(
    JNIEnv* env, jobject thiz, jobject codec, jint index, jobject info)
{
  CJNIXBMCMediaCodecCallback* inst = find_instance(thiz);
  if (inst)
    inst->onOutputBufferAvailable(CJNIMediaCodec(jhobject::fromJNI(codec)), index,
                                  CJNIMediaCodecBufferInfo(jhobject::fromJNI(info)));
}

void CJNIXBMCMediaCodecCallback::_onOutputFormatChanged(JNIEnv* env,
                                                        jobject thiz,
                                                        jobject codec,
                                                        jobject format)
{
  CJNIXBMCMediaCodecCallback* inst = find_instance(thiz);
  if (inst)
    inst->onOutputFormatChanged(CJNIMediaCodec(jhobject::fromJNI(codec)),
                                CJNIMediaFormat(jhobject::fromJNI(format)));
}

void CJNIXBMCMediaCodecCallback::_onError(JNIEnv* env, jobject thiz, jobject codec, jobject e)
{
  CJNIXBMCMediaCodecCallback* inst = find_instance(thiz);
  if (inst)
    inst->onError(CJNIMediaCodec(jhobject::fromJNI(codec)),
                  CJNIMediaCodecCodecException(jhobject::fromJNI(e)));
}

void CJNIXBMCMediaCodecCallback::resetGeneration()
{
  call_method<void>(m_object, "resetGeneration", "()V");
}

void CJNIXBMCMediaCodecCallback::flushAndRestart(const CJNIMediaCodec& codec)
{
  call_method<void>(m_object, "flushAndRestart", "(Landroid/media/MediaCodec;)V", codec.get_raw());
}

void CJNIXBMCMediaCodecCallback::_onStartError(JNIEnv* env, jobject thiz)
{
  CJNIXBMCMediaCodecCallback* inst = find_instance(thiz);
  if (inst)
    inst->onStartError();
}
