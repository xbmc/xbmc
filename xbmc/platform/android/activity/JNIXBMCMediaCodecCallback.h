/*
 *  Copyright (C) 2012-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>
#include <androidjni/MediaCodec.h>
#include <androidjni/MediaCodecBufferInfo.h>
#include <androidjni/MediaFormat.h>

namespace jni
{

class CJNIXBMCMediaCodecCallback : public CJNIBase,
                                   public CJNIInterfaceImplem<CJNIXBMCMediaCodecCallback>
{
public:
  CJNIXBMCMediaCodecCallback();
  ~CJNIXBMCMediaCodecCallback() override;

  static void RegisterNatives(JNIEnv* env);

  virtual void onInputBufferAvailable(CJNIMediaCodec codec, int index) = 0;
  virtual void onOutputBufferAvailable(CJNIMediaCodec codec,
                                       int index,
                                       CJNIMediaCodecBufferInfo info) = 0;
  virtual void onOutputFormatChanged(CJNIMediaCodec codec, CJNIMediaFormat format) = 0;
  virtual void onError(CJNIMediaCodec codec) = 0;

protected:
  static void _onInputBufferAvailable(JNIEnv* env, jobject thiz, jobject codec, jint index);
  static void _onOutputBufferAvailable(
      JNIEnv* env, jobject thiz, jobject codec, jint index, jobject info);
  static void _onOutputFormatChanged(JNIEnv* env, jobject thiz, jobject codec, jobject format);
  static void _onError(JNIEnv* env, jobject thiz, jobject codec);
};

} // namespace jni
