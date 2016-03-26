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

#include "JNIBase.h"
#include "ByteBuffer.h"

class CJNIMediaFormat : public CJNIBase
{
public:
  CJNIMediaFormat(const jni::jhobject &object) : CJNIBase(object) {};
  //~CJNIMediaFormat() {};

  bool        containsKey(const std::string &name);
  int         getInteger( const std::string &name);
  int64_t     getLong(    const std::string &name);
  float       getFloat(   const std::string &name);
  std::string getString(  const std::string &name);
  const       CJNIByteBuffer getByteBuffer(const std::string &name);
  void        setInteger( const std::string &name, int     value);
  void        setLong(    const std::string &name, int64_t value);
  void        setFloat(   const std::string &name, float   value);
  void        setString(  const std::string &name, const std::string &value);
  void        setByteBuffer(const std::string &name, CJNIByteBuffer &bytes);
  std::string toString()  const;

  static void  PopulateStaticFields();
  static const CJNIMediaFormat createAudioFormat(const std::string &mime, int sampleRate, int channelCount);
  static const CJNIMediaFormat createVideoFormat(const std::string &mime, int width, int height);

  static std::string KEY_MIME;
  static std::string KEY_SAMPLE_RATE;
  static std::string KEY_CHANNEL_COUNT;
  static std::string KEY_WIDTH;
  static std::string KEY_HEIGHT;
  static std::string KEY_MAX_INPUT_SIZE;
  static std::string KEY_BIT_RATE;
  static std::string KEY_COLOR_FORMAT;
  static std::string KEY_FRAME_RATE;
  static std::string KEY_I_FRAME_INTERVAL;
  static std::string KEY_DURATION;
  static std::string KEY_IS_ADTS;
  static std::string KEY_CHANNEL_MASK;
  static std::string KEY_AAC_PROFILE;
  static std::string KEY_FLAC_COMPRESSION_LEVEL;
  static std::string KEY_ROTATION;

private:
  CJNIMediaFormat();

  static const char *m_classname;
};
