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

#include "CoreAudioChannelLayout.h"

#include <AudioToolbox/AudioToolbox.h>

#define MAX_CHANNEL_LABEL 15

const char* g_ChannelLabels[] =
{
  "Unused",           // kAudioChannelLabel_Unused
  "Left",             // kAudioChannelLabel_Left
  "Right",            // kAudioChannelLabel_Right
  "Center",           // kAudioChannelLabel_Center
  "LFE",              // kAudioChannelLabel_LFEScreen
  "Side Left",        // kAudioChannelLabel_LeftSurround
  "Side Right",       // kAudioChannelLabel_RightSurround
  "Left Center",      // kAudioChannelLabel_LeftCenter
  "Right Center",     // kAudioChannelLabel_RightCenter
  "Back Center",      // kAudioChannelLabel_CenterSurround
  "Back Left",        // kAudioChannelLabel_LeftSurroundDirect
  "Back Right",       // kAudioChannelLabel_RightSurroundDirect
  "Top Center",       // kAudioChannelLabel_TopCenterSurround
  "Top Back Left",    // kAudioChannelLabel_VerticalHeightLeft
  "Top Back Center",  // kAudioChannelLabel_VerticalHeightCenter
  "Top Back Right",   // kAudioChannelLabel_VerticalHeightRight
};

CCoreAudioChannelLayout::CCoreAudioChannelLayout() :
  m_pLayout(NULL)
{
}

CCoreAudioChannelLayout::CCoreAudioChannelLayout(AudioChannelLayout& layout) :
m_pLayout(NULL)
{
  CopyLayout(layout);
}

CCoreAudioChannelLayout::~CCoreAudioChannelLayout()
{
  free(m_pLayout);
}

bool CCoreAudioChannelLayout::CopyLayout(AudioChannelLayout& layout)
{
  free(m_pLayout);
  m_pLayout = NULL;

  // This method always produces a layout with a ChannelDescriptions structure

  OSStatus ret = 0;
  UInt32 channels = GetChannelCountForLayout(layout);
  UInt32 size = sizeof(AudioChannelLayout) + (channels - kVariableLengthArray) * sizeof(AudioChannelDescription);

  if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
  {
    // We can copy the whole layout
    m_pLayout = (AudioChannelLayout*)malloc(size);
    memcpy(m_pLayout, &layout, size);
  }
  else if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap)
  {
    // Deconstruct the bitmap to get the layout
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForBitmap, sizeof(layout.mChannelBitmap), &layout.mChannelBitmap, &propSize);
    m_pLayout = (AudioChannelLayout*)malloc(propSize);
    ret = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForBitmap, sizeof(layout.mChannelBitmap), &layout.mChannelBitmap, &propSize, m_pLayout);
    m_pLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
  }
  else
  {
    // Convert the known layout to a custom layout
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForTag,
      sizeof(layout.mChannelLayoutTag), &layout.mChannelLayoutTag, &propSize);
    m_pLayout = (AudioChannelLayout*)malloc(propSize);
    ret = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForTag,
      sizeof(layout.mChannelLayoutTag), &layout.mChannelLayoutTag, &propSize, m_pLayout);
    m_pLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
  }

  return (ret == noErr);
}

UInt32 CCoreAudioChannelLayout::GetChannelCountForLayout(AudioChannelLayout& layout)
{
  UInt32 channels = 0;
  if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap)
  {
    // Channels are in fixed-order('USB Order'), any combination
    UInt32 bitmap = layout.mChannelBitmap;
    for (UInt32 c = 0; c < (sizeof(layout.mChannelBitmap) << 3); c++)
    {
      if (bitmap & 0x1)
        channels++;
      bitmap >>= 1;
    }
  }
  else if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
  {
    // Channels are in any order, any combination
    channels = layout.mNumberChannelDescriptions;
  }
  else
  {
    // Channels are in a predefined order and combination
    channels = AudioChannelLayoutTag_GetNumberOfChannels(layout.mChannelLayoutTag);
  }

  return channels;
}

const char* CCoreAudioChannelLayout::ChannelLabelToString(UInt32 label)
{
  if (label > MAX_CHANNEL_LABEL)
    return "Unknown";
  return g_ChannelLabels[label];
}

const char* CCoreAudioChannelLayout::ChannelLayoutToString(AudioChannelLayout& layout, std::string& str)
{
  AudioChannelLayout* pLayout = NULL;

  if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
  {
    pLayout = &layout;
  }
  else if (layout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap)
  {
    // Deconstruct the bitmap to get the layout
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForBitmap,
      sizeof(layout.mChannelBitmap), &layout.mChannelBitmap, &propSize);
    pLayout = (AudioChannelLayout*)calloc(propSize, 1);
    AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForBitmap,
      sizeof(layout.mChannelBitmap), &layout.mChannelBitmap, &propSize, pLayout);
  }
  else
  {
    // Predefinied layout 'tag'
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForTag,
      sizeof(layout.mChannelLayoutTag), &layout.mChannelLayoutTag, &propSize);
    pLayout = (AudioChannelLayout*)calloc(propSize, 1);
    AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForTag,
      sizeof(layout.mChannelLayoutTag), &layout.mChannelLayoutTag, &propSize, pLayout);
  }

  for (UInt32 c = 0; c < pLayout->mNumberChannelDescriptions; c++)
  {
    str += "[";
    str += ChannelLabelToString(pLayout->mChannelDescriptions[c].mChannelLabel);
    str += "] ";
  }

  if (layout.mChannelLayoutTag != kAudioChannelLayoutTag_UseChannelDescriptions)
    free(pLayout);

  return str.c_str();
}

bool CCoreAudioChannelLayout::AllChannelUnknown()
{
  AudioChannelLayout* pLayout = NULL;

  if (!m_pLayout)
    return false;

  if (m_pLayout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
  {
    pLayout = m_pLayout;
  }
  else if (m_pLayout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap)
  {
    // Deconstruct the bitmap to get the layout
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForBitmap,
      sizeof(m_pLayout->mChannelBitmap), &m_pLayout->mChannelBitmap, &propSize);
    pLayout = (AudioChannelLayout*)calloc(propSize, 1);
    AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForBitmap,
      sizeof(m_pLayout->mChannelBitmap), &m_pLayout->mChannelBitmap, &propSize, pLayout);
  }
  else
  {
    // Predefinied layout 'tag'
    UInt32 propSize = 0;
    AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForTag,
      sizeof(m_pLayout->mChannelLayoutTag), &m_pLayout->mChannelLayoutTag, &propSize);
    pLayout = (AudioChannelLayout*)calloc(propSize, 1);
    AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForTag,
      sizeof(m_pLayout->mChannelLayoutTag), &m_pLayout->mChannelLayoutTag, &propSize, pLayout);
  }

  for (UInt32 c = 0; c < pLayout->mNumberChannelDescriptions; c++)
  {
    if (pLayout->mChannelDescriptions[c].mChannelLabel != kAudioChannelLabel_Unknown)
    {
      return false;
    }
  }

  if (m_pLayout->mChannelLayoutTag != kAudioChannelLayoutTag_UseChannelDescriptions)
    free(pLayout);

  return true;
}

