/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "CoreAudioMixMap.h"

#include "CoreAudioUnit.h"
#include "CoreAudioAEHAL.h"
#include "utils/log.h"


#include <AudioToolbox/AudioToolbox.h>

UInt32 CCoreAudioMixMap::m_deviceChannels = 0;

CCoreAudioMixMap::CCoreAudioMixMap() :
  m_isValid(false)
{
  m_pMap = (Float32*)calloc(sizeof(AudioChannelLayout), 1);
}

CCoreAudioMixMap::CCoreAudioMixMap(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout) :
  m_isValid(false)
{
  Rebuild(inLayout, outLayout);
}

CCoreAudioMixMap::~CCoreAudioMixMap()
{
  free(m_pMap);
  m_pMap = NULL;
}

void CCoreAudioMixMap::Rebuild(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout)
{
  // map[in][out] = mix-level of input_channel[in] into output_channel[out]

  free(m_pMap);
  m_pMap = NULL;

  m_inChannels  = CCoreAudioChannelLayout::GetChannelCountForLayout(inLayout);
  m_outChannels = CCoreAudioChannelLayout::GetChannelCountForLayout(outLayout);

  // Try to find a 'well-known' matrix
  const AudioChannelLayout* layouts[] = {&inLayout, &outLayout};
  UInt32 propSize = 0;
  OSStatus ret = AudioFormatGetPropertyInfo(kAudioFormatProperty_MatrixMixMap,
    sizeof(layouts), layouts, &propSize);
  m_pMap = (Float32*)calloc(1,propSize);

  // Try and get a predefined mixmap
  ret = AudioFormatGetProperty(kAudioFormatProperty_MatrixMixMap,
    sizeof(layouts), layouts, &propSize, m_pMap);
  if (!ret)
  {
    // Nothing else to do...a map already exists
    m_isValid = true;
    return;
  }

  // No predefined mixmap was available. Going to have to build it manually
  CLog::Log(LOGDEBUG, "CCoreAudioMixMap::CreateMap: Unable to locate pre-defined mixing matrix");

  m_isValid = false;
}

CCoreAudioMixMap *CCoreAudioMixMap::CreateMixMap(CAUOutputDevice  *audioUnit, AEAudioFormat &format, AudioChannelLayoutTag layoutTag)
{
  if (!audioUnit)
    return NULL;

  AudioStreamBasicDescription fmt;
  AudioStreamBasicDescription inputFormat;

  // get the stream input format
  audioUnit->GetFormatDesc(format, &inputFormat, &fmt);

  unsigned int channels = format.m_channelLayout.Count();
  CAEChannelInfo channelLayout = format.m_channelLayout;
  bool hasLFE = false;
  // Convert XBMC input channel layout format to CoreAudio layout format
  AudioChannelLayout* pInLayout = (AudioChannelLayout*)malloc(sizeof(AudioChannelLayout) + sizeof(AudioChannelDescription) * channels);
  pInLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
  pInLayout->mChannelBitmap = 0;
  pInLayout->mNumberChannelDescriptions = channels;
  for (unsigned int chan=0; chan < channels; chan++)
  {
    AudioChannelDescription* pDesc = &pInLayout->mChannelDescriptions[chan];
    // Convert from XBMC channel tag to CoreAudio channel tag
    pDesc->mChannelLabel = g_LabelMap[(unsigned int)channelLayout[chan]];
    pDesc->mChannelFlags = kAudioChannelFlags_AllOff;
    pDesc->mCoordinates[0] = 0.0f;
    pDesc->mCoordinates[1] = 0.0f;
    pDesc->mCoordinates[2] = 0.0f;
    if (pDesc->mChannelLabel == kAudioChannelLabel_LFEScreen)
      hasLFE = true;
  }
  // HACK: Fix broken channel layouts coming from some aac sources
  // that include rear channel but no side channels.
  // 5.1 streams should include front and side channels.
  // Rear channels are added by 6.1 and 7.1, so any 5.1
  // source that claims to have rear channels is wrong.
  if (inputFormat.mChannelsPerFrame == 6 && hasLFE)
  {
    // Check for 5.1 configuration (as best we can without getting too silly)
    for (unsigned int chan=0; chan < inputFormat.mChannelsPerFrame; chan++)
    {
      AudioChannelDescription* pDesc = &pInLayout->mChannelDescriptions[chan];
      if (pDesc->mChannelLabel == kAudioChannelLabel_LeftSurround || pDesc->mChannelLabel == kAudioChannelLabel_LeftSurround)
        break; // Required condition cannot be true

      if (pDesc->mChannelLabel == kAudioChannelLabel_LeftSurroundDirect)
      {
        // Change [Back Left] to [Side Left]
        pDesc->mChannelLabel = kAudioChannelLabel_LeftSurround;
        CLog::Log(LOGINFO, "CCoreAudioGraph::CreateMixMap: "
          "Detected faulty input channel map...fixing(Back Left-->Side Left)");
      }
      if (pDesc->mChannelLabel == kAudioChannelLabel_RightSurroundDirect)
      {
        // Change [Back Left] to [Side Left]
        pDesc->mChannelLabel = kAudioChannelLabel_RightSurround;
        CLog::Log(LOGINFO, "CCoreAudioGraph::CreateMixMap: "
          "Detected faulty input channel map...fixing(Back Right-->Side Right)");
      }
    }
  }

  CCoreAudioChannelLayout sourceLayout(*pInLayout);
  free(pInLayout);
  pInLayout = NULL;

  std::string strInLayout;
  CLog::Log(LOGDEBUG, "CCoreAudioGraph::CreateMixMap: Source Stream Layout: %s",
    CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)sourceLayout, strInLayout));

  // Get User-Configured (XBMC) Speaker Configuration
  AudioChannelLayout guiLayout;
  guiLayout.mChannelLayoutTag = layoutTag;
  CCoreAudioChannelLayout userLayout(guiLayout);
  std::string strUserLayout;
  CLog::Log(LOGDEBUG, "CCoreAudioGraph::CreateMixMap: User-Configured Speaker Layout: %s",
    CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)userLayout, strUserLayout));

  // Get OS-Configured (Audio MIDI Setup) Speaker Configuration (Channel Layout)
  CCoreAudioChannelLayout deviceLayout;
  if (!audioUnit->GetPreferredChannelLayout(deviceLayout))
    return NULL;
    
  m_deviceChannels = CCoreAudioChannelLayout::GetChannelCountForLayout(*deviceLayout);

  // When all channels on the output device are unknown take the gui layout
  //if(deviceLayout.AllChannelUnknown())
  //  deviceLayout.CopyLayout(guiLayout);

  std::string strOutLayout;
  CLog::Log(LOGDEBUG, "CCoreAudioGraph::CreateMixMap: Output Device Layout: %s",
    CCoreAudioChannelLayout::ChannelLayoutToString(*(AudioChannelLayout*)deviceLayout, strOutLayout));

  // TODO:
  // Reconcile the OS and GUI layout configurations. Clamp to the minimum number of speakers
  // For each OS-defined output, see if it exists in the GUI configuration
  // If it does, add it to the 'union' layout (bitmap?)
  // User may have configured 5.1 in GUI, but only 2.0 in OS
  // Resulting layout would be {FL, FR}
  // User may have configured 2.0 in GUI, and 5.1 in OS
  // Resulting layout would be {FL, FR}

  // Correct any configuration incompatibilities
  //if (CCoreAudioChannelLayout::GetChannelCountForLayout(guiLayout) < CCoreAudioChannelLayout::GetChannelCountForLayout(deviceLayout))
  //  deviceLayout.CopyLayout(guiLayout);

  // TODO: Skip matrix mixer if input/output are compatible

  AudioChannelLayout* layoutCandidates[] = {(AudioChannelLayout*)deviceLayout, (AudioChannelLayout*)userLayout, NULL};

  // Try to construct a mapping matrix for the mixer.
  // Work through the layout candidates and see if any will work
  CCoreAudioMixMap *mixMap = new CCoreAudioMixMap();
  for (AudioChannelLayout** pLayout = layoutCandidates; *pLayout != NULL; pLayout++)
  {
    mixMap->Rebuild(*sourceLayout, **pLayout);
    if (mixMap->IsValid())
      break;
  }
  return mixMap;
}

bool CCoreAudioMixMap::SetMixingMatrix(CAUMatrixMixer *mixerUnit,
  CCoreAudioMixMap *mixMap, AudioStreamBasicDescription *inputFormat,
  AudioStreamBasicDescription *fmt, int channelOffset)
{
  if (!mixerUnit || !inputFormat || !fmt)
    return false;

  // Configure the mixing matrix
  Float32* val = (Float32*)*mixMap;
  for (UInt32 i = 0; i < inputFormat->mChannelsPerFrame; ++i)
  {
    val = (Float32*)*mixMap + i*m_deviceChannels;
    for (UInt32 j = 0; j < fmt->mChannelsPerFrame; ++j)
    {
      AudioUnitSetParameter(mixerUnit->GetUnit(),
        kMatrixMixerParam_Volume, kAudioUnitScope_Global, ( (i + channelOffset) << 16 ) | j, *val++, 0);
    }
  }

  CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: "
    "Mixer Output Format: %d channels, %0.1f kHz, %d bits, %d bytes per frame",
    (int)fmt->mChannelsPerFrame, fmt->mSampleRate / 1000.0f, (int)fmt->mBitsPerChannel, (int)fmt->mBytesPerFrame);

  if (!mixerUnit->InitMatrixMixerVolumes())
    return false;

  return true;
}

