/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Sinks/osx/CoreAudioDevice.h"

// There is only one AudioSystemObject instance system-side.
// Therefore, all CCoreAudioHardware methods are static
class CCoreAudioHardware
{
public:
  static bool           GetAutoHogMode();
  static void           SetAutoHogMode(bool enable);
  static AudioStreamBasicDescription* FormatsList(AudioStreamID stream);
  static AudioStreamID* StreamsList(AudioDeviceID device);
  static void           ResetAudioDevices();
  static void           ResetStream(AudioStreamID streamId);
  static AudioDeviceID  FindAudioDevice(const std::string &deviceName);
  static AudioDeviceID  GetDefaultOutputDevice();
  static void           GetOutputDeviceName(std::string &name);
  static UInt32         GetOutputDevices(CoreAudioDeviceList *pList);
};
