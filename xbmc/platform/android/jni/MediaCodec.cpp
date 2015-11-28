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

#include "MediaCodec.h"
#include "MediaCrypto.h"
#include "MediaCodecCryptoInfo.h"
#include "NetworkInfo.h"
#include "Surface.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIMediaCodec::BUFFER_FLAG_CODEC_CONFIG(2);
int CJNIMediaCodec::BUFFER_FLAG_END_OF_STREAM(4);
int CJNIMediaCodec::BUFFER_FLAG_SYNC_FRAME(1);
int CJNIMediaCodec::CONFIGURE_FLAG_ENCODE(1);
int CJNIMediaCodec::CONFIGURE_FLAG_DECODE(0);
int CJNIMediaCodec::CRYPTO_MODE_AES_CTR(1);
int CJNIMediaCodec::CRYPTO_MODE_UNENCRYPTED(0);
int CJNIMediaCodec::INFO_OUTPUT_BUFFERS_CHANGED(-3);
int CJNIMediaCodec::INFO_OUTPUT_FORMAT_CHANGED(-2);
int CJNIMediaCodec::INFO_TRY_AGAIN_LATER(-1);
int CJNIMediaCodec::VIDEO_SCALING_MODE_SCALE_TO_FIT(1);
int CJNIMediaCodec::VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING(2);
const char *CJNIMediaCodec::m_classname = "platform/android/media/MediaCodec";

void CJNIMediaCodec::PopulateStaticFields()
{
  if(GetSDKVersion() >= 16)
  {
    jhclass clazz = find_class("platform/android/media/MediaCodec");
    BUFFER_FLAG_CODEC_CONFIG  = (get_static_field<int>(clazz, "BUFFER_FLAG_CODEC_CONFIG"));
    BUFFER_FLAG_END_OF_STREAM = (get_static_field<int>(clazz, "BUFFER_FLAG_END_OF_STREAM"));
    BUFFER_FLAG_SYNC_FRAME    = (get_static_field<int>(clazz, "BUFFER_FLAG_SYNC_FRAME"));
    CONFIGURE_FLAG_ENCODE     = (get_static_field<int>(clazz, "CONFIGURE_FLAG_ENCODE"));
    // CONFIGURE_FLAG_DECODE is ours to make it easy on params
    CRYPTO_MODE_AES_CTR       = (get_static_field<int>(clazz, "CRYPTO_MODE_AES_CTR"));
    CRYPTO_MODE_UNENCRYPTED   = (get_static_field<int>(clazz, "CRYPTO_MODE_UNENCRYPTED"));
    INFO_OUTPUT_BUFFERS_CHANGED = (get_static_field<int>(clazz, "INFO_OUTPUT_BUFFERS_CHANGED"));
    INFO_OUTPUT_FORMAT_CHANGED= (get_static_field<int>(clazz, "INFO_OUTPUT_FORMAT_CHANGED"));
    INFO_TRY_AGAIN_LATER      = (get_static_field<int>(clazz, "INFO_TRY_AGAIN_LATER"));
    VIDEO_SCALING_MODE_SCALE_TO_FIT = (get_static_field<int>(clazz, "VIDEO_SCALING_MODE_SCALE_TO_FIT"));
    VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING = (get_static_field<int>(clazz, "VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING"));
  }
}

const CJNIMediaCodec CJNIMediaCodec::createDecoderByType(const std::string &type)
{
  // This method doesn't handle errors nicely, it crashes if the codec isn't found.
  // This is fixed in latest AOSP, but not in current 4.1 devices.
  return call_static_method<jhobject>(m_classname,
    "createDecoderByType", "(Ljava/lang/String;)Landroid/media/MediaCodec;",
    jcast<jhstring>(type));
}

const CJNIMediaCodec CJNIMediaCodec::createEncoderByType(const std::string &type)
{
  // This method doesn't handle errors nicely, it crashes if the codec isn't found.
  // This is fixed in latest AOSP, but not in current 4.1 devices.
  return call_static_method<jhobject>(m_classname,
    "createEncoderByType", "(Ljava/lang/String;)Landroid/media/MediaCodec;",
    jcast<jhstring>(type));
}

const CJNIMediaCodec CJNIMediaCodec::createByCodecName(const std::string &name)
{
  // This method doesn't handle errors nicely, it crashes if the codec isn't found.
  // This is fixed in latest AOSP, but not in current 4.1 devices.
  return call_static_method<jhobject>(m_classname,
    "createByCodecName", "(Ljava/lang/String;)Landroid/media/MediaCodec;",
    jcast<jhstring>(name));
}

void CJNIMediaCodec::release()
{
  call_method<void>(m_object,
    "release", "()V");
}

void CJNIMediaCodec::configure(const CJNIMediaFormat &format, const CJNISurface &surface, const CJNIMediaCrypto &crypto, int flags)
{
  call_method<void>(m_object,
    "configure", "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V",
    format.get_raw(), surface.get_raw(), crypto.get_raw(), flags);
}

void CJNIMediaCodec::start()
{
  call_method<void>(m_object,
    "start", "()V");
}

void CJNIMediaCodec::stop()
{
  call_method<void>(m_object,
    "stop", "()V");
}

void CJNIMediaCodec::flush()
{
  call_method<void>(m_object,
    "flush", "()V");
}

void CJNIMediaCodec::queueInputBuffer(int index, int offset, int size, int64_t presentationTimeUs, int flags)
{
  call_method<void>(m_object,
    "queueInputBuffer", "(IIIJI)V",
    index, offset, size, presentationTimeUs, flags);
}

void CJNIMediaCodec::queueSecureInputBuffer(int index, int offset, const CJNIMediaCodecCryptoInfo &info, int64_t presentationTimeUs, int flags)
{
  call_method<void>(m_object,
    "queueSecureInputBuffer", "(IILandroid/media/MediaCodec$CryptoInfo;JI)V",
    index, offset, info.get_raw(), presentationTimeUs, flags);
}

int CJNIMediaCodec::dequeueInputBuffer(int64_t timeoutUs)
{
  return call_method<int>(m_object,
    "dequeueInputBuffer", "(J)I",
    timeoutUs);
}

int CJNIMediaCodec::dequeueOutputBuffer(const CJNIMediaCodecBufferInfo &info, int64_t timeoutUs)
{
  return call_method<int>(m_object,
    "dequeueOutputBuffer", "(Landroid/media/MediaCodec$BufferInfo;J)I",
    info.get_raw(), timeoutUs);
}

void CJNIMediaCodec::releaseOutputBuffer(int index, bool render)
{
  jboolean jboolean_render = (jboolean)render;
  call_method<void>(m_object,
    "releaseOutputBuffer", "(IZ)V",
    index, jboolean_render);
}

const CJNIMediaFormat CJNIMediaCodec::getOutputFormat()
{
  return call_method<jhobject>(m_object,
    "getOutputFormat", "()Landroid/media/MediaFormat;");
}

std::vector<CJNIByteBuffer> CJNIMediaCodec::getInputBuffers()
{
  return jcast<CJNIByteBuffers>(call_method<jhobjectArray>(m_object,
    "getInputBuffers", "()[Ljava/nio/ByteBuffer;"));
}

std::vector<CJNIByteBuffer> CJNIMediaCodec::getOutputBuffers()
{
  return jcast<CJNIByteBuffers>(call_method<jhobjectArray>(m_object,
    "getOutputBuffers", "()[Ljava/nio/ByteBuffer;"));
}

void CJNIMediaCodec::setVideoScalingMode(int mode)
{
  call_method<void>(m_object,
    "setVideoScalingMode", "(I)V",
    mode);
}
