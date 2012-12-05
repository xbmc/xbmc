#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"

#if defined(TARGET_DARWIN_OSX)
#include <list>
#include <vector>
#include <string>

#include <CoreAudio/CoreAudio.h>

typedef std::vector<SInt32> CoreAudioChannelList;
typedef std::list<AudioChannelLayoutTag> AudioChannelLayoutList;

const AudioChannelLayoutTag g_LayoutMap[] =
{
  kAudioChannelLayoutTag_Stereo,            // PCM_LAYOUT_2_0 = 0,
  kAudioChannelLayoutTag_Stereo,            // PCM_LAYOUT_2_0 = 0,
  kAudioChannelLayoutTag_DVD_4,             // PCM_LAYOUT_2_1,
  kAudioChannelLayoutTag_MPEG_3_0_A,        // PCM_LAYOUT_3_0,
  kAudioChannelLayoutTag_DVD_10,            // PCM_LAYOUT_3_1,
  kAudioChannelLayoutTag_DVD_3,             // PCM_LAYOUT_4_0,
  kAudioChannelLayoutTag_DVD_6,             // PCM_LAYOUT_4_1,
  kAudioChannelLayoutTag_MPEG_5_0_A,        // PCM_LAYOUT_5_0,
  kAudioChannelLayoutTag_MPEG_5_1_A,        // PCM_LAYOUT_5_1,
  kAudioChannelLayoutTag_AudioUnit_7_0,     // PCM_LAYOUT_7_0, ** This layout may be incorrect...no content to testß˚ **
  kAudioChannelLayoutTag_MPEG_7_1_A,        // PCM_LAYOUT_7_1
};

const AudioChannelLabel g_LabelMap[] =
{
  kAudioChannelLabel_Unused,                // PCM_FRONT_LEFT,
  kAudioChannelLabel_Left,                  // PCM_FRONT_LEFT,
  kAudioChannelLabel_Right,                 // PCM_FRONT_RIGHT,
  kAudioChannelLabel_Center,                // PCM_FRONT_CENTER,
  kAudioChannelLabel_LFEScreen,             // PCM_LOW_FREQUENCY,
  kAudioChannelLabel_LeftSurroundDirect,    // PCM_BACK_LEFT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_RightSurroundDirect,   // PCM_BACK_RIGHT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_LeftCenter,            // PCM_FRONT_LEFT_OF_CENTER,
  kAudioChannelLabel_RightCenter,           // PCM_FRONT_RIGHT_OF_CENTER,
  kAudioChannelLabel_CenterSurround,        // PCM_BACK_CENTER,
  kAudioChannelLabel_LeftSurround,          // PCM_SIDE_LEFT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_RightSurround,         // PCM_SIDE_RIGHT, *** This is incorrect, but has been changed to match dvdplayer
  kAudioChannelLabel_VerticalHeightLeft,    // PCM_TOP_FRONT_LEFT,
  kAudioChannelLabel_VerticalHeightRight,   // PCM_TOP_FRONT_RIGHT,
  kAudioChannelLabel_VerticalHeightCenter,  // PCM_TOP_FRONT_CENTER,
  kAudioChannelLabel_TopCenterSurround,     // PCM_TOP_CENTER,
  kAudioChannelLabel_TopBackLeft,           // PCM_TOP_BACK_LEFT,
  kAudioChannelLabel_TopBackRight,          // PCM_TOP_BACK_RIGHT,
  kAudioChannelLabel_TopBackCenter          // PCM_TOP_BACK_CENTER
};

class CCoreAudioChannelLayout
{
public:
  CCoreAudioChannelLayout();
  CCoreAudioChannelLayout(AudioChannelLayout &layout);
  virtual ~CCoreAudioChannelLayout();

  operator AudioChannelLayout*() {return m_pLayout;}

  bool                CopyLayout(AudioChannelLayout &layout);
  static UInt32       GetChannelCountForLayout(AudioChannelLayout &layout);
  static const char*  ChannelLabelToString(UInt32 label);
  static const char*  ChannelLayoutToString(AudioChannelLayout &layout, std::string &str);
  bool                AllChannelUnknown();
protected:
  AudioChannelLayout* m_pLayout;
};

#endif