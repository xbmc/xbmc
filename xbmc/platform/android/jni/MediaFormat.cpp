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

#include "JNIBase.h"
#include "MediaFormat.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

std::string CJNIMediaFormat::KEY_MIME;
std::string CJNIMediaFormat::KEY_SAMPLE_RATE;
std::string CJNIMediaFormat::KEY_CHANNEL_COUNT;
std::string CJNIMediaFormat::KEY_WIDTH;
std::string CJNIMediaFormat::KEY_HEIGHT;
std::string CJNIMediaFormat::KEY_MAX_INPUT_SIZE;
std::string CJNIMediaFormat::KEY_BIT_RATE;
std::string CJNIMediaFormat::KEY_COLOR_FORMAT;
std::string CJNIMediaFormat::KEY_FRAME_RATE;
std::string CJNIMediaFormat::KEY_I_FRAME_INTERVAL;
std::string CJNIMediaFormat::KEY_DURATION;
std::string CJNIMediaFormat::KEY_IS_ADTS;
std::string CJNIMediaFormat::KEY_CHANNEL_MASK;
std::string CJNIMediaFormat::KEY_AAC_PROFILE;
std::string CJNIMediaFormat::KEY_FLAC_COMPRESSION_LEVEL;
const char *CJNIMediaFormat::m_classname = "platform/android/media/MediaFormat";

void CJNIMediaFormat::PopulateStaticFields()
{
  if(GetSDKVersion() >= 16)
  {
    jhclass clazz = find_class("platform/android/media/MediaFormat");
    KEY_MIME            = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_MIME"));
    KEY_SAMPLE_RATE     = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_SAMPLE_RATE"));
    KEY_CHANNEL_COUNT   = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_CHANNEL_COUNT"));
    KEY_WIDTH           = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_WIDTH"));
    KEY_HEIGHT          = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_HEIGHT"));
    KEY_MAX_INPUT_SIZE  = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_MAX_INPUT_SIZE"));
    KEY_BIT_RATE        = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_BIT_RATE"));
    KEY_COLOR_FORMAT    = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_COLOR_FORMAT"));
    KEY_FRAME_RATE      = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_FRAME_RATE"));
    KEY_I_FRAME_INTERVAL= jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_I_FRAME_INTERVAL"));
    KEY_DURATION        = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_DURATION"));
    KEY_IS_ADTS         = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_IS_ADTS"));
    KEY_CHANNEL_MASK    = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_CHANNEL_MASK"));
    KEY_AAC_PROFILE     = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_AAC_PROFILE"));
    KEY_FLAC_COMPRESSION_LEVEL = jcast<std::string>(get_static_field<jhstring>(clazz, "KEY_FLAC_COMPRESSION_LEVEL"));
  }
}

const CJNIMediaFormat CJNIMediaFormat::createAudioFormat(const std::string &mime, int sampleRate, int channelCount)
{
  return call_static_method<jhobject>(m_classname,
    "createAudioFormat", "(Ljava/lang/String;II)Landroid/media/MediaFormat;",
    jcast<jhstring>(mime), sampleRate, channelCount);
}

const CJNIMediaFormat CJNIMediaFormat::createVideoFormat(const std::string &mime, int width, int height)
{
  return call_static_method<jhobject>(m_classname,
    "createVideoFormat", "(Ljava/lang/String;II)Landroid/media/MediaFormat;",
    jcast<jhstring>(mime), width, height);
}

bool CJNIMediaFormat::containsKey(const std::string &name)
{
  return call_method<jboolean>(m_object,
    "containsKey", "(Ljava/lang/String;)Z",
    jcast<jhstring>(name));
}

int CJNIMediaFormat::getInteger(const std::string &name)
{
  return call_method<jint>(m_object,
    "getInteger",
    "(Ljava/lang/String;)I",
    jcast<jhstring>(name));
}

int64_t CJNIMediaFormat::getLong(const std::string &name)
{
  return call_method<jlong>(m_object,
    "getLong", "(Ljava/lang/String;)J",
    jcast<jhstring>(name));
}

float CJNIMediaFormat::getFloat(const std::string &name)
{
  return call_method<jfloat>(m_object,
    "getFloat", "(Ljava/lang/String;)F",
    jcast<jhstring>(name));
}

std::string CJNIMediaFormat::getString(const std::string &name)
{
  jhstring jhstring_rtn = call_method<jhstring>(m_object,
    "getString", "(Ljava/lang/String;)Ljava/lang/String;",
    jcast<jhstring>(name));
  return jcast<std::string>(jhstring_rtn);
}

const CJNIByteBuffer CJNIMediaFormat::getByteBuffer(const std::string &name)
{
  return call_method<jhobject>(m_object,
    "getByteBuffer", "(Ljava/lang/String;)Ljava/nio/ByteBuffer;",
    jcast<jhstring>(name));
}

void  CJNIMediaFormat::setInteger(const std::string &name, int value)
{
  call_method<void>(m_object,
    "setInteger", "(Ljava/lang/String;I)V",
    jcast<jhstring>(name), value);
}

void  CJNIMediaFormat::setLong(const std::string &name, int64_t value)
{
  call_method<void>(m_object,
    "setLong", "(Ljava/lang/String;J)V",
    jcast<jhstring>(name), value);
}

void  CJNIMediaFormat::setFloat(const std::string &name, float value)
{
  call_method<void>(m_object,
    "setFloat", "(Ljava/lang/String;F)V",
    jcast<jhstring>(name), value);
}

void  CJNIMediaFormat::setString(const std::string &name,const std::string &value)
{
  call_method<void>(m_object,
    "setString", "(Ljava/lang/String;Ljava/lang/String;)V",
    jcast<jhstring>(name), jcast<jhstring>(value));
}

void  CJNIMediaFormat::setByteBuffer(const std::string &name, CJNIByteBuffer &bytes)
{
  call_method<void>(m_object,
    "setByteBuffer", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V",
    jcast<jhstring>(name), bytes.get_raw());
}

std::string CJNIMediaFormat::toString() const
{
  jhstring jhstring_rtn = call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;");
  return jcast<std::string>(jhstring_rtn);
}
