/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CoreAudioHelpers.h"

#include <sstream>

// Helper Functions
std::string GetError(OSStatus error)
{
  char buffer[128] = {};

  *(UInt32 *)(buffer + 1) = CFSwapInt32HostToBig(error);
  if (isprint(buffer[1]) && isprint(buffer[2]) &&
      isprint(buffer[3]) && isprint(buffer[4]))
  {
    buffer[0] = buffer[5] = '\'';
    buffer[6] = '\0';
  }
  else
  {
    // no, format it as an integer
    snprintf(buffer, sizeof(buffer), "%d", (int)error);
  }

  return std::string(buffer);
}

const char* StreamDescriptionToString(AudioStreamBasicDescription desc, std::string &str)
{
  char fourCC[5] = {
    (char)((desc.mFormatID >> 24) & 0xFF),
    (char)((desc.mFormatID >> 16) & 0xFF),
    (char)((desc.mFormatID >>  8) & 0xFF),
    (char) (desc.mFormatID        & 0xFF),
    0
  };

  std::stringstream sstr;
  switch (desc.mFormatID)
  {
    case kAudioFormatLinearPCM:
      sstr  << "["
            << fourCC
            << "] "
            << ((desc.mFormatFlags & kAudioFormatFlagIsNonMixable) ? "" : "Mixable " )
            << ((desc.mFormatFlags & kAudioFormatFlagIsNonInterleaved) ? "Non-" : "" )
            << "Interleaved "
            << desc.mChannelsPerFrame
            << " Channel "
            << desc.mBitsPerChannel
            << "-bit "
            << ((desc.mFormatFlags & kAudioFormatFlagIsFloat) ? "Floating Point " : "Signed Integer ")
            << ((desc.mFormatFlags & kAudioFormatFlagIsBigEndian) ? "BE" : "LE")
            << " ("
            << (UInt32)desc.mSampleRate
            << "Hz)";
      str = sstr.str();
      break;
    case kAudioFormatAC3:
      sstr  << "["
            << fourCC
            << "] "
            << ((desc.mFormatFlags & kAudioFormatFlagIsBigEndian) ? "BE" : "LE")
            << " AC-3/DTS ("
            << (UInt32)desc.mSampleRate
            << "Hz)";
      str = sstr.str();
      break;
    case kAudioFormat60958AC3:
      sstr  << "["
            << fourCC
            << "] AC-3/DTS for S/PDIF "
            << ((desc.mFormatFlags & kAudioFormatFlagIsBigEndian) ? "BE" : "LE")
            << " ("
            << (UInt32)desc.mSampleRate
            << "Hz)";
      str = sstr.str();
      break;
    default:
      sstr  << "["
            << fourCC
            << "]";
      break;
  }
  return str.c_str();
}
