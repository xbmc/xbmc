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

#include "AudioTrack.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIAudioTrack::MODE_STREAM       = 0x00000001;
int CJNIAudioTrack::PLAYSTATE_PLAYING = 0x00000003;

void CJNIAudioTrack::PopulateStaticFields()
{
  if (CJNIBase::GetSDKVersion() >= 3)
  {
    jhclass c = find_class("android/media/AudioTrack");
    CJNIAudioTrack::PLAYSTATE_PLAYING = get_static_field<int>(c, "PLAYSTATE_PLAYING");
    if (CJNIBase::GetSDKVersion() >= 5)
      CJNIAudioTrack::MODE_STREAM = get_static_field<int>(c, "MODE_STREAM");
  }
}

CJNIAudioTrack::CJNIAudioTrack(int streamType, int sampleRateInHz, int channelConfig, int audioFormat, int bufferSizeInBytes, int mode) throw(std::invalid_argument)
  : CJNIBase("android/media/AudioTrack")
{
  m_object = new_object(GetClassName(), "<init>", "(IIIIII)V",
                        streamType, sampleRateInHz, channelConfig,
                        audioFormat, bufferSizeInBytes, mode);

  /* AudioTrack constructor may throw IllegalArgumentException, pass it to
   * caller instead of getting us killed */
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

  m_buffer = jharray(xbmc_jnienv()->NewByteArray(bufferSizeInBytes));

  m_object.setGlobal();
  m_buffer.setGlobal();
}

void CJNIAudioTrack::play()
{
  call_method<void>(m_object, "play", "()V");
}

void CJNIAudioTrack::stop()
{
  call_method<void>(m_object, "stop", "()V");

  JNIEnv* jenv = xbmc_jnienv();

  // might toss an exception on release so catch it.
  jthrowable exception = jenv->ExceptionOccurred();
  if (exception)
  {
    jenv->ExceptionDescribe();
    jenv->ExceptionClear();
  }
}

void CJNIAudioTrack::flush()
{
  call_method<void>(m_object, "flush", "()V");
}

void CJNIAudioTrack::release()
{
  call_method<void>(m_object, "release", "()V");

  JNIEnv* jenv  = xbmc_jnienv();
}

int CJNIAudioTrack::write(char* audioData, int offsetInBytes, int sizeInBytes)
{
  int     written = 0;
  JNIEnv* jenv    = xbmc_jnienv();
  char*   pArray;
  
  // Write a buffer of audio data to Java AudioTrack.
  // Warning, no other JNI function can be called after
  // GetPrimitiveArrayCritical until ReleasePrimitiveArrayCritical.
  if ((pArray = (char*)jenv->GetPrimitiveArrayCritical(m_buffer, NULL)))
  {
    memcpy(pArray + offsetInBytes, audioData, sizeInBytes);
    jenv->ReleasePrimitiveArrayCritical(m_buffer, pArray, 0);
    written = call_method<int>(m_object, "write", "([BII)I", m_buffer, offsetInBytes, sizeInBytes);
  }

  return written;
}

int CJNIAudioTrack::getPlayState()
{
  return call_method<int>(m_object, "getPlayState", "()I");
}

int CJNIAudioTrack::getPlaybackHeadPosition()
{
  return call_method<int>(m_object, "getPlaybackHeadPosition", "()I");
}

int CJNIAudioTrack::getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat)
{
  return call_static_method<int>( "android/media/AudioTrack", "getMinBufferSize", "(III)I",
                                  sampleRateInHz, channelConfig, audioFormat);
}

int CJNIAudioTrack::getNativeOutputSampleRate(int streamType)
{
  return call_static_method<int>( "android/media/AudioTrack", "getNativeOutputSampleRate", "(I)I",
                                  streamType);
}

