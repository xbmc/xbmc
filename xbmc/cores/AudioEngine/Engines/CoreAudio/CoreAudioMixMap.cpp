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
#include <sstream>

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
  if (ret)
  {
    // If we for some reason don't find a predefined matrix let's build a diagonal matrix,
    // basically guessing here, but we need to have a mixmap that matches the output and input
    CLog::Log(LOGDEBUG, "CCoreAudioMixMap::CreateMap: No pre-defined mapping from %d to %d channels, building diagonal matrix.", m_inChannels, m_outChannels);
    for (UInt32 chan = 0; chan < std::min(m_inChannels, m_outChannels); ++chan)
    {
      Float32 *vol = m_pMap + (chan * m_outChannels + chan);
      CLog::Log(LOGDEBUG, "CCoreAudioMixMap::Rebuild %d = %f", chan, *vol);
      *vol = 1.;
    }
  }

  m_isValid = true;
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
      if (pDesc->mChannelLabel == kAudioChannelLabel_LeftSurround || pDesc->mChannelLabel == kAudioChannelLabel_RightSurround)
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
  CCoreAudioMixMap *mixMap = new CCoreAudioMixMap();
  mixMap->Rebuild(*sourceLayout, *(AudioChannelLayout*)deviceLayout);
  return mixMap;
}

bool CCoreAudioMixMap::SetMixingMatrix(CAUMatrixMixer *mixerUnit,
  CCoreAudioMixMap *mixMap, AudioStreamBasicDescription *inputFormat,
  AudioStreamBasicDescription *fmt, int channelOffset)
{
  if (!mixerUnit || !inputFormat || !fmt)
    return false;

  // Fetch the mixing unit size
  UInt32 dims[2];
  UInt32 size = sizeof(dims);
  AudioUnitGetProperty(mixerUnit->GetUnit(),
    kAudioUnitProperty_MatrixDimensions, kAudioUnitScope_Global, 0, dims, &size);

  if(inputFormat->mChannelsPerFrame + channelOffset > dims[0])
  {
    CLog::Log(LOGERROR, "CCoreAudioMixMap::SetMixingMatrix - input format doesn't fit mixer size %u+%u > %u"
                      , inputFormat->mChannelsPerFrame, channelOffset, dims[0]);
    return false;
  }

  if(fmt->mChannelsPerFrame > dims[1])
  {
    CLog::Log(LOGERROR, "CCoreAudioMixMap::SetMixingMatrix - ouput format doesn't fit mixer size %u > %u"
              , fmt->mChannelsPerFrame, dims[1]);
    return false;
  }

  if(fmt->mChannelsPerFrame < dims[1])
  {
    CLog::Log(LOGWARNING, "CCoreAudioMixMap::SetMixingMatrix - ouput format doesn't specify all outputs %u < %u"
              , fmt->mChannelsPerFrame, dims[1]);
  }

  // Configure the mixing matrix
  // The return from kAudioFormatProperty_MatrixMixMap (See Rebuild above) 
  // is a Float32* which is laid out like this:
  //
  // mapping 2 chan -> 2 chan
  // 1 0 0 1
  //
  // or better represented in a tow dimensional array:
  //
  // 1 0
  // 0 1
  //
  // mapping 6 chan -> 6 chan:
  // 1 0 0 0 0 0
  // 0 1 0 0 0 0
  // 0 0 1 0 0 0
  // ....

  Float32* val = (Float32*)*mixMap;
  for (UInt32 i = 0; i < inputFormat->mChannelsPerFrame; ++i)
  {
    UInt32 j = 0;
    std::stringstream layoutStr;
    for (; j < fmt->mChannelsPerFrame; ++j)
    {
      Float32 *vol = val + (i * mixMap->m_outChannels + j);
      layoutStr << *vol << ", ";
      AudioUnitSetParameter(mixerUnit->GetUnit(),
        kMatrixMixerParam_Volume, kAudioUnitScope_Global, ( (i + channelOffset) << 16 ) | j, *vol, 0);
    }
    // zero out additional outputs from this input
    for (; j < dims[1]; ++j)
    {
      AudioUnitSetParameter(mixerUnit->GetUnit(),
        kMatrixMixerParam_Volume, kAudioUnitScope_Global, ( (i + channelOffset) << 16 ) | j, 0.0f, 0);
      layoutStr << "0, ";
    }

    CLog::Log(LOGDEBUG, "CCoreAudioMixMap::SetMixingMatrix channel %d = [%s]", i, layoutStr.str().c_str());
  }

  CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: "
    "Mixer Output Format: %d channels, %0.1f kHz, %d bits, %d bytes per frame",
    (int)fmt->mChannelsPerFrame, fmt->mSampleRate / 1000.0f, (int)fmt->mBitsPerChannel, (int)fmt->mBytesPerFrame);

  if (!mixerUnit->InitMatrixMixerVolumes())
    return false;

  return true;
}

