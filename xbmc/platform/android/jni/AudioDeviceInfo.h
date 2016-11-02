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
#include "jutils/jutils-details.hpp"

#include "CharSequence.h"

class CJNIAudioDeviceInfo : public CJNIBase
{
  public:
    static void PopulateStaticFields();

    static int TYPE_AUX_LINE;
    static int TYPE_BLUETOOTH_A2DP;
    static int TYPE_BLUETOOTH_SCO;
    static int TYPE_BUILTIN_EARPIECE;
    static int TYPE_BUILTIN_MIC;
    static int TYPE_BUILTIN_SPEAKER;
    static int TYPE_BUS;
    static int TYPE_DOCK;
    static int TYPE_FM;
    static int TYPE_FM_TUNER;
    static int TYPE_HDMI;
    static int TYPE_HDMI_ARC;
    static int TYPE_IP;
    static int TYPE_LINE_ANALOG;
    static int TYPE_LINE_DIGITAL;
    static int TYPE_TELEPHONY;
    static int TYPE_TV_TUNER;
    static int TYPE_UNKNOWN;
    static int TYPE_USB_ACCESSORY;
    static int TYPE_USB_DEVICE;
    static int TYPE_WIRED_HEADPHONES;
    static int TYPE_WIRED_HEADSET;

    ~CJNIAudioDeviceInfo() {};
    CJNIAudioDeviceInfo(const jni::jhobject &object) : CJNIBase(object) {};

    std::vector<int> getChannelCounts() const;
    std::vector<int> getChannelIndexMasks() const;
    std::vector<int> getChannelMasks() const;
    std::vector<int> getEncodings() const;
    std::vector<int> getSampleRates() const;
    
    CJNICharSequence getProductName() const;
    int getId() const;
    int getType() const;
    bool isSink() const;
    bool isSource() const;

protected:
    CJNIAudioDeviceInfo();
    static void GetStaticValue(jni::jhclass &c, int &field, const char *value);
};

typedef std::vector<CJNIAudioDeviceInfo> CJNIAudioDeviceInfos;

