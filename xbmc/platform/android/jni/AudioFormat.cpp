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

#include "AudioFormat.h"
#include "JNIBase.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIAudioFormat::ENCODING_PCM_16BIT = 0x00000002;
int CJNIAudioFormat::ENCODING_PCM_FLOAT = 0x00000004;
int CJNIAudioFormat::ENCODING_AC3       = 0x00000005;
int CJNIAudioFormat::ENCODING_E_AC3     = 0x00000006;
int CJNIAudioFormat::ENCODING_DTS       = 0x00000007;
int CJNIAudioFormat::ENCODING_DTS_HD    = 0x00000008;

// As of version 22 and Android 5 Nvidia defines this solely for their Shield
int CJNIAudioFormat::ENCODING_DOLBY_TRUEHD    = 0x00000009;

int CJNIAudioFormat::CHANNEL_OUT_STEREO  = 0x0000000c;
int CJNIAudioFormat::CHANNEL_OUT_5POINT1 = 0x000000fc;

int CJNIAudioFormat::CHANNEL_OUT_FRONT_LEFT            = 0x00000004;
int CJNIAudioFormat::CHANNEL_OUT_FRONT_LEFT_OF_CENTER  = 0x00000100;
int CJNIAudioFormat::CHANNEL_OUT_FRONT_CENTER          = 0x00000010;
int CJNIAudioFormat::CHANNEL_OUT_FRONT_RIGHT_OF_CENTER = 0x00000200;
int CJNIAudioFormat::CHANNEL_OUT_FRONT_RIGHT           = 0x00000008;
int CJNIAudioFormat::CHANNEL_OUT_LOW_FREQUENCY         = 0x00000020;
int CJNIAudioFormat::CHANNEL_OUT_SIDE_LEFT             = 0x00000800;
int CJNIAudioFormat::CHANNEL_OUT_SIDE_RIGHT            = 0x00001000;
int CJNIAudioFormat::CHANNEL_OUT_BACK_LEFT             = 0x00000040;
int CJNIAudioFormat::CHANNEL_OUT_BACK_CENTER           = 0x00000400;
int CJNIAudioFormat::CHANNEL_OUT_BACK_RIGHT            = 0x00000080;

int CJNIAudioFormat::CHANNEL_INVALID                   = 0x00000000;

void CJNIAudioFormat::PopulateStaticFields()
{
  int sdk = CJNIBase::GetSDKVersion();
  if (sdk >= 3)
  {
    jhclass c = find_class("android/media/AudioFormat");
    CJNIAudioFormat::ENCODING_PCM_16BIT = get_static_field<int>(c, "ENCODING_PCM_16BIT");
    if (sdk >= 5)
    {
      CJNIAudioFormat::CHANNEL_OUT_STEREO = get_static_field<int>(c, "CHANNEL_OUT_STEREO");
      CJNIAudioFormat::CHANNEL_OUT_5POINT1 = get_static_field<int>(c, "CHANNEL_OUT_5POINT1");
      CJNIAudioFormat::CHANNEL_OUT_FRONT_LEFT = get_static_field<int>(c, "CHANNEL_OUT_FRONT_LEFT");
      CJNIAudioFormat::CHANNEL_OUT_FRONT_LEFT_OF_CENTER = get_static_field<int>(c, "CHANNEL_OUT_FRONT_LEFT_OF_CENTER");
      CJNIAudioFormat::CHANNEL_OUT_FRONT_CENTER = get_static_field<int>(c, "CHANNEL_OUT_FRONT_CENTER");
      CJNIAudioFormat::CHANNEL_OUT_FRONT_RIGHT_OF_CENTER = get_static_field<int>(c, "CHANNEL_OUT_FRONT_RIGHT_OF_CENTER");
      CJNIAudioFormat::CHANNEL_OUT_FRONT_RIGHT = get_static_field<int>(c, "CHANNEL_OUT_FRONT_RIGHT");
      CJNIAudioFormat::CHANNEL_OUT_LOW_FREQUENCY = get_static_field<int>(c, "CHANNEL_OUT_LOW_FREQUENCY");
      CJNIAudioFormat::CHANNEL_OUT_BACK_LEFT = get_static_field<int>(c, "CHANNEL_OUT_BACK_LEFT");
      CJNIAudioFormat::CHANNEL_OUT_BACK_CENTER = get_static_field<int>(c, "CHANNEL_OUT_BACK_CENTER");
      CJNIAudioFormat::CHANNEL_OUT_BACK_RIGHT = get_static_field<int>(c, "CHANNEL_OUT_BACK_RIGHT");
      CJNIAudioFormat::CHANNEL_INVALID = get_static_field<int>(c, "CHANNEL_INVALID");

      if (sdk >= 21)
      {
        CJNIAudioFormat::CHANNEL_OUT_SIDE_LEFT = get_static_field<int>(c, "CHANNEL_OUT_SIDE_LEFT");
        CJNIAudioFormat::CHANNEL_OUT_SIDE_RIGHT = get_static_field<int>(c, "CHANNEL_OUT_SIDE_RIGHT");

        CJNIAudioFormat::ENCODING_PCM_FLOAT = get_static_field<int>(c, "ENCODING_PCM_FLOAT");
        CJNIAudioFormat::ENCODING_AC3 = get_static_field<int>(c, "ENCODING_AC3");
        CJNIAudioFormat::ENCODING_E_AC3 = get_static_field<int>(c, "ENCODING_E_AC3");

        if (sdk >= 23)
        {
          CJNIAudioFormat::ENCODING_DTS = get_static_field<int>(c, "ENCODING_DTS");
          CJNIAudioFormat::ENCODING_DTS_HD = get_static_field<int>(c, "ENCODING_DTS_HD");
          // Nvidia Shield v6 firmware uses another ID, which will also be the future ID
          // though other v23 version would not return a value if we'd use the get_static_field
          // method to query this value. The hardcoded value can be removed after probably is out
          CJNIAudioFormat::ENCODING_DOLBY_TRUEHD = 0x0000000d;
        }
      }
    }
  }
}

