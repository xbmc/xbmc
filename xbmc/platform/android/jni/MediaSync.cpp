/*
 *      Copyright (C) 2016 Chris Browet
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

#include "MediaSync.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIMediaSync::MEDIASYNC_ERROR_AUDIOTRACK_FAIL         = 0x00000001;
int CJNIMediaSync::MEDIASYNC_ERROR_SURFACE_FAIL            = 0x00000002;

void CJNIMediaSync::PopulateStaticFields()
{
  if (CJNIBase::GetSDKVersion() >= 23)
  {
    jhclass c = find_class("android/media/MediaSync");
    CJNIMediaSync::MEDIASYNC_ERROR_AUDIOTRACK_FAIL = get_static_field<int>(c, "MEDIASYNC_ERROR_AUDIOTRACK_FAIL");
    CJNIMediaSync::MEDIASYNC_ERROR_SURFACE_FAIL = get_static_field<int>(c, "MEDIASYNC_ERROR_SURFACE_FAIL");
  }
}

CJNIMediaSync::CJNIMediaSync()
  : CJNIBase("android/media/MediaSync")
{
  m_object = new_object(GetClassName());

  JNIEnv* jenv = xbmc_jnienv();
  jthrowable exception = jenv->ExceptionOccurred();
  if (exception)
  {
    jenv->ExceptionClear();
    jhclass excClass = find_class(jenv, "java/lang/Throwable");
    jmethodID toStrMethod = get_method_id(jenv, excClass, "toString", "()Ljava/lang/String;");
    jhstring msg = call_method<jhstring>(exception, toStrMethod);
    throw std::invalid_argument(jcast<std::string>(msg));
  }
  m_object.setGlobal();
}

void CJNIMediaSync::flush()
{
  call_method<void>(m_object, "flush", "()V");
}

void CJNIMediaSync::release()
{
  call_method<void>(m_object, "release", "()V");
}

CJNIMediaTimestamp CJNIMediaSync::getTimestamp()
{
  return call_method<jhobject>(m_object,
    "getTimestamp", "()Landroid/media/MediaTimestamp;");
}

void CJNIMediaSync::setAudioTrack(const CJNIAudioTrack& audioTrack)
{
  call_method<void>(m_object,
    "setAudioTrack", "(Landroid/media/AudioTrack;)V",
    audioTrack.get_raw());
}

void CJNIMediaSync::queueAudio(uint8_t* audioData, int sizeInBytes, int bufferId, int64_t presentationTimeUs)
{
  CJNIByteBuffer bytebuffer = CJNIByteBuffer::allocateDirect(sizeInBytes);
  void *dst_ptr = xbmc_jnienv()->GetDirectBufferAddress(bytebuffer.get_raw());
  memcpy(dst_ptr, audioData, sizeInBytes);

  queueAudio(bytebuffer, bufferId, presentationTimeUs);
}

void CJNIMediaSync::queueAudio(const CJNIByteBuffer& audioData, int bufferId, int64_t presentationTimeUs)
{
  call_method<void>(m_object,
    "queueAudio", "(Ljava/nio/ByteBuffer;IJ)V",
                    audioData.get_raw(), bufferId, presentationTimeUs);
}

CJNIPlaybackParams CJNIMediaSync::getPlaybackParams()
{
  return call_method<jhobject>(m_object,
    "getPlaybackParams", "()Landroid/media/PlaybackParams;");
}

void CJNIMediaSync::setPlaybackParams(const CJNIPlaybackParams& params)
{
  call_method<void>(m_object,
    "setPlaybackParams", "(Landroid/media/PlaybackParams;)V",
    params.get_raw());
}

