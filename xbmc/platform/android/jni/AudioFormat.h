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

namespace jni
{

class CJNIAudioFormat : public CJNIBase
{
public:
  CJNIAudioFormat(const jni::jhobject &object) : CJNIBase(object) {}

  static void PopulateStaticFields();

  static int ENCODING_PCM_16BIT;
  static int ENCODING_PCM_FLOAT;
  static int ENCODING_AC3;
  static int ENCODING_E_AC3;
  static int ENCODING_DTS;
  static int ENCODING_DTS_HD;
  static int ENCODING_DOLBY_TRUEHD;
  static int ENCODING_IEC61937;

  static int CHANNEL_OUT_MONO;
  static int CHANNEL_OUT_STEREO;
  static int CHANNEL_OUT_5POINT1;
  static int CHANNEL_OUT_7POINT1_SURROUND;

  static int CHANNEL_OUT_FRONT_LEFT;
  static int CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
  static int CHANNEL_OUT_FRONT_CENTER;
  static int CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
  static int CHANNEL_OUT_FRONT_RIGHT;
  static int CHANNEL_OUT_LOW_FREQUENCY;
  static int CHANNEL_OUT_SIDE_LEFT;
  static int CHANNEL_OUT_SIDE_RIGHT;
  static int CHANNEL_OUT_BACK_LEFT;
  static int CHANNEL_OUT_BACK_CENTER;
  static int CHANNEL_OUT_BACK_RIGHT;

  static int CHANNEL_INVALID;

  int getChannelCount() const;
  int getChannelIndexMask() const;
  int getChannelMask() const;
  int getEncoding() const;
  int getSampleRate() const;

protected:
    static void GetStaticValue(jhclass &c, int &field, char *value);
    static const char *m_classname;
};

class CJNIAudioFormatBuilder : public CJNIBase
{
public:
  CJNIAudioFormatBuilder();
  CJNIAudioFormatBuilder(const jni::jhobject &object) : CJNIBase(object) {}

  CJNIAudioFormat build();

  CJNIAudioFormatBuilder setChannelIndexMask(int channelIndexMask);
  CJNIAudioFormatBuilder setChannelMask(int channelMask);
  CJNIAudioFormatBuilder setEncoding(int encoding);
  CJNIAudioFormatBuilder setSampleRate(int sampleRate);

protected:
  static const char *m_classname;
};

}
