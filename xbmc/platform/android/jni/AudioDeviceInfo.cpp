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

#include "AudioDeviceInfo.h"
#include "JNIBase.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIAudioDeviceInfo::TYPE_AUX_LINE = -1;
int CJNIAudioDeviceInfo::TYPE_BLUETOOTH_A2DP = -1;
int CJNIAudioDeviceInfo::TYPE_BLUETOOTH_SCO = -1;
int CJNIAudioDeviceInfo::TYPE_BUILTIN_EARPIECE = -1;
int CJNIAudioDeviceInfo::TYPE_BUILTIN_MIC = -1;
int CJNIAudioDeviceInfo::TYPE_BUILTIN_SPEAKER = -1;
int CJNIAudioDeviceInfo::TYPE_BUS = -1;
int CJNIAudioDeviceInfo::TYPE_DOCK = -1;
int CJNIAudioDeviceInfo::TYPE_FM = -1;
int CJNIAudioDeviceInfo::TYPE_FM_TUNER = -1;
int CJNIAudioDeviceInfo::TYPE_HDMI = -1;
int CJNIAudioDeviceInfo::TYPE_HDMI_ARC = -1;
int CJNIAudioDeviceInfo::TYPE_IP = -1;
int CJNIAudioDeviceInfo::TYPE_LINE_ANALOG = -1;
int CJNIAudioDeviceInfo::TYPE_LINE_DIGITAL = -1;
int CJNIAudioDeviceInfo::TYPE_TELEPHONY = -1;
int CJNIAudioDeviceInfo::TYPE_TV_TUNER = -1;
int CJNIAudioDeviceInfo::TYPE_UNKNOWN = -1;
int CJNIAudioDeviceInfo::TYPE_USB_ACCESSORY = -1;
int CJNIAudioDeviceInfo::TYPE_USB_DEVICE = -1;
int CJNIAudioDeviceInfo::TYPE_WIRED_HEADPHONES = -1;
int CJNIAudioDeviceInfo::TYPE_WIRED_HEADSET = -1;

void CJNIAudioDeviceInfo::GetStaticValue(jhclass& c, int& field, const char* value)
{
  jfieldID fid = get_static_field_id<jclass>(c, value, "I");
  if (fid != NULL)
    field = get_static_field<int>(c, fid);
  else
    xbmc_jnienv()->ExceptionClear();
}

void CJNIAudioDeviceInfo::PopulateStaticFields()
{
  int sdk = CJNIBase::GetSDKVersion();
  if (sdk >= 23)
  {
    jhclass c = find_class("android/media/AudioDeviceInfo");
    CJNIAudioDeviceInfo::TYPE_AUX_LINE = get_static_field<int>(c, "TYPE_AUX_LINE");
    CJNIAudioDeviceInfo::TYPE_BLUETOOTH_A2DP = get_static_field<int>(c, "TYPE_BLUETOOTH_A2DP");
    CJNIAudioDeviceInfo::TYPE_BLUETOOTH_SCO = get_static_field<int>(c, "TYPE_BLUETOOTH_SCO");
    CJNIAudioDeviceInfo::TYPE_BUILTIN_EARPIECE = get_static_field<int>(c, "TYPE_BUILTIN_EARPIECE");
    CJNIAudioDeviceInfo::TYPE_BUILTIN_MIC = get_static_field<int>(c, "TYPE_BUILTIN_MIC");
    CJNIAudioDeviceInfo::TYPE_BUILTIN_SPEAKER = get_static_field<int>(c, "TYPE_BUILTIN_SPEAKER");
    CJNIAudioDeviceInfo::TYPE_DOCK = get_static_field<int>(c, "TYPE_DOCK");
    CJNIAudioDeviceInfo::TYPE_FM = get_static_field<int>(c, "TYPE_FM");
    CJNIAudioDeviceInfo::TYPE_FM_TUNER = get_static_field<int>(c, "TYPE_FM_TUNER");
    CJNIAudioDeviceInfo::TYPE_HDMI = get_static_field<int>(c, "TYPE_HDMI");
    CJNIAudioDeviceInfo::TYPE_HDMI_ARC = get_static_field<int>(c, "TYPE_HDMI_ARC");
    CJNIAudioDeviceInfo::TYPE_IP = get_static_field<int>(c, "TYPE_IP");
    CJNIAudioDeviceInfo::TYPE_LINE_ANALOG = get_static_field<int>(c, "TYPE_LINE_ANALOG");
    CJNIAudioDeviceInfo::TYPE_LINE_DIGITAL = get_static_field<int>(c, "TYPE_LINE_DIGITAL");
    CJNIAudioDeviceInfo::TYPE_TELEPHONY = get_static_field<int>(c, "TYPE_TELEPHONY");
    CJNIAudioDeviceInfo::TYPE_TV_TUNER = get_static_field<int>(c, "TYPE_TV_TUNER");
    CJNIAudioDeviceInfo::TYPE_UNKNOWN = get_static_field<int>(c, "TYPE_UNKNOWN");
    CJNIAudioDeviceInfo::TYPE_USB_ACCESSORY = get_static_field<int>(c, "TYPE_USB_ACCESSORY");
    CJNIAudioDeviceInfo::TYPE_USB_DEVICE = get_static_field<int>(c, "TYPE_USB_DEVICE");
    CJNIAudioDeviceInfo::TYPE_WIRED_HEADPHONES = get_static_field<int>(c, "TYPE_WIRED_HEADPHONES");
    CJNIAudioDeviceInfo::TYPE_WIRED_HEADSET = get_static_field<int>(c, "TYPE_WIRED_HEADSET");

    if (sdk >= 24)
    {
      GetStaticValue(c, CJNIAudioDeviceInfo::TYPE_BUS, "TYPE_BUS");
    }
  }
}

std::vector<int> CJNIAudioDeviceInfo::getChannelCounts() const
{
  return jcast<std::vector<int>>(
    call_method<jhintArray>(m_object, "getChannelCounts", "()[I"));
}
    
std::vector<int> CJNIAudioDeviceInfo::getChannelIndexMasks() const
{
  return jcast<std::vector<int>>(
    call_method<jhintArray>(m_object, "getChannelIndexMasks", "()[I"));
}

std::vector<int> CJNIAudioDeviceInfo::getChannelMasks() const
{
  return jcast<std::vector<int>>(
    call_method<jhintArray>(m_object, "getChannelMasks", "()[I"));
}

std::vector<int> CJNIAudioDeviceInfo::getEncodings() const
{
  return jcast<std::vector<int>>(
    call_method<jhintArray>(m_object, "getEncodings", "()[I"));
}

std::vector<int> CJNIAudioDeviceInfo::getSampleRates() const
{
  return jcast<std::vector<int>>(
    call_method<jhintArray>(m_object, "getSampleRates", "()[I"));
}

CJNICharSequence CJNIAudioDeviceInfo::getProductName() const
{
  return call_method<jhobject>(m_object,
    "getProductName", "()Ljava/lang/CharSequence;");
}

int CJNIAudioDeviceInfo::getId() const
{
  return call_method<jint>(m_object,
    "getId", "()I");
  
}

int CJNIAudioDeviceInfo::getType() const
{
  return call_method<jint>(m_object,
    "getType", "()I");
}

bool CJNIAudioDeviceInfo::isSink() const
{
  return call_method<jboolean>(m_object,
    "isSink", "()Z");
}

bool CJNIAudioDeviceInfo::isSource() const
{
  return call_method<jboolean>(m_object,
    "isSource", "()Z");
}
