#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

#include "JNIBase.h"
#include "ByteBuffer.h"
#include "MediaCodecBufferInfo.h"
#include "MediaFormat.h"

class CJNISurface;
class CJNIMediaCodec;
class CJNIMediaCrypto;
class CJNIMediaCodecCryptoInfo;

class CJNIMediaCodec : public CJNIBase
{
public:
  CJNIMediaCodec(const jni::jhobject &object) : CJNIBase(object) {};
  //~CJNIMediaCodec() {};

  void  release();
  void  configure(const CJNIMediaFormat &format, const CJNISurface &surface, const CJNIMediaCrypto &crypto, int flags);
  void  start();
  void  stop();
  void  flush();
  void  queueInputBuffer(int index, int offset, int size, int64_t presentationTimeUs, int flags);
  void  queueSecureInputBuffer(int index, int offset, const CJNIMediaCodecCryptoInfo &info, int64_t presentationTimeUs, int flags);
  int   dequeueInputBuffer(int64_t timeoutUs);
  int   dequeueOutputBuffer(const CJNIMediaCodecBufferInfo &info, int64_t timeoutUs);
  void  releaseOutputBuffer(int index, bool render);
  const CJNIMediaFormat getOutputFormat();
  const CJNIByteBuffer getInputBuffer(int index);
  const CJNIByteBuffer getOutputBuffer(int index);
  void  setVideoScalingMode(int mode);

  static void  PopulateStaticFields();
  static const CJNIMediaCodec createDecoderByType(const std::string &type);
  static const CJNIMediaCodec createEncoderByType(const std::string &type);
  static const CJNIMediaCodec createByCodecName(  const std::string &name);

  static int BUFFER_FLAG_CODEC_CONFIG;
  static int BUFFER_FLAG_END_OF_STREAM;
  static int BUFFER_FLAG_SYNC_FRAME;
  static int CONFIGURE_FLAG_ENCODE;
  static int CONFIGURE_FLAG_DECODE;
  static int CRYPTO_MODE_AES_CTR;
  static int CRYPTO_MODE_UNENCRYPTED;
  static int INFO_OUTPUT_BUFFERS_CHANGED;
  static int INFO_OUTPUT_FORMAT_CHANGED;
  static int INFO_TRY_AGAIN_LATER;
  static int VIDEO_SCALING_MODE_SCALE_TO_FIT;
  static int VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING;

private:
  CJNIMediaCodec();

  static const char *m_classname;
};
