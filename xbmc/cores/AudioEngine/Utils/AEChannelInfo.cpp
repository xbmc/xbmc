/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#include "AEChannelInfo.h"
#include <string.h>

CAEChannelInfo::CAEChannelInfo()
{
  Reset();
}

CAEChannelInfo::CAEChannelInfo(const enum AEChannel* rhs)
{
  *this = rhs;
}

CAEChannelInfo::CAEChannelInfo(const AEStdChLayout rhs)
{
  *this = rhs;
}

CAEChannelInfo::~CAEChannelInfo()
{
}

void CAEChannelInfo::ResolveChannels(const CAEChannelInfo& rhs)
{
  /* mono gets upmixed to dual mono */
  if (m_channelCount == 1 && m_channels[0] == AE_CH_FC)
  {
    Reset();
    *this += AE_CH_FL;
    *this += AE_CH_FR;
    return;
  }

  bool srcHasSL = false;
  bool srcHasSR = false;
  bool srcHasRL = false;
  bool srcHasRR = false;

  bool dstHasSL = false;
  bool dstHasSR = false;
  bool dstHasRL = false;
  bool dstHasRR = false;

  for (unsigned int c = 0; c < rhs.m_channelCount; ++c)
    switch(rhs.m_channels[c])
    {
      case AE_CH_SL: dstHasSL = true; break;
      case AE_CH_SR: dstHasSR = true; break;
      case AE_CH_BL: dstHasRL = true; break;
      case AE_CH_BR: dstHasRR = true; break;
      default:
        break;
    }

  CAEChannelInfo newInfo;
  for (unsigned int i = 0; i < m_channelCount; ++i)
  {
    switch (m_channels[i])
    {
      case AE_CH_SL: srcHasSL = true; break;
      case AE_CH_SR: srcHasSR = true; break;
      case AE_CH_BL: srcHasRL = true; break;
      case AE_CH_BR: srcHasRR = true; break;
      default:
        break;
    }

    bool found = false;
    for (unsigned int c = 0; c < rhs.m_channelCount; ++c)
      if (m_channels[i] == rhs.m_channels[c])
      {
        found = true;
        break;
      }

    if (found)
      newInfo += m_channels[i];
  }

  /* we need to ensure we end up with rear or side channels for downmix to work */
  if (srcHasSL && !dstHasSL && dstHasRL)
    newInfo += AE_CH_BL;
  if (srcHasSR && !dstHasSR && dstHasRR)
    newInfo += AE_CH_BR;
  if (srcHasRL && !dstHasRL && dstHasSL)
    newInfo += AE_CH_SL;
  if (srcHasRR && !dstHasRR && dstHasSR)
    newInfo += AE_CH_SR;

  *this = newInfo;
}

void CAEChannelInfo::Reset()
{
  m_channelCount = 0;
  for (unsigned int i = 0; i < AE_CH_MAX; ++i)
    m_channels[i] = AE_CH_NULL;
}

CAEChannelInfo& CAEChannelInfo::operator=(const CAEChannelInfo& rhs)
{
  if (this == &rhs)
    return *this;

  /* clone the information */
  m_channelCount = rhs.m_channelCount;
  memcpy(m_channels, rhs.m_channels, sizeof(enum AEChannel) * rhs.m_channelCount);

  return *this;
}

CAEChannelInfo& CAEChannelInfo::operator=(const enum AEChannel* rhs)
{
  Reset();
  if (rhs == NULL)
    return *this;

  while (m_channelCount < AE_CH_MAX && rhs[m_channelCount] != AE_CH_NULL)
  {
    m_channels[m_channelCount] = rhs[m_channelCount];
    ++m_channelCount;
  }

  /* the last entry should be NULL, if not we were passed a non null terminated list */
  ASSERT(rhs[m_channelCount] == AE_CH_NULL);

  return *this;
}

CAEChannelInfo& CAEChannelInfo::operator=(const enum AEStdChLayout rhs)
{
  ASSERT(rhs > AE_CH_LAYOUT_INVALID && rhs < AE_CH_LAYOUT_MAX);

  static enum AEChannel layouts[AE_CH_LAYOUT_MAX][9] = {
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL , AE_CH_BR , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL , AE_CH_BR , AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_SL , AE_CH_SR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_SL , AE_CH_SR, AE_CH_LFE, AE_CH_NULL}
  };

  *this = layouts[rhs];
  return *this;
}

bool CAEChannelInfo::operator==(const CAEChannelInfo& rhs)
{
  /* if the channel count doesnt match, no need to check further */
  if (m_channelCount != rhs.m_channelCount)
    return false;

  /* make sure the channel order is the same */
  for (unsigned int i = 0; i < m_channelCount; ++i)
    if (m_channels[i] != rhs.m_channels[i])
      return false;

  return true;
}

bool CAEChannelInfo::operator!=(const CAEChannelInfo& rhs)
{
  return !(*this == rhs);
}

CAEChannelInfo& CAEChannelInfo::operator+=(const enum AEChannel& rhs)
{
  ASSERT(m_channelCount < AE_CH_MAX);
  ASSERT(rhs > AE_CH_NULL && rhs < AE_CH_MAX);

  m_channels[m_channelCount++] = rhs;
  return *this;
}

CAEChannelInfo& CAEChannelInfo::operator-=(const enum AEChannel& rhs)
{
  ASSERT(rhs > AE_CH_NULL && rhs < AE_CH_MAX);

  unsigned int i = 0;
  while(i < m_channelCount && m_channels[i] != rhs)
    i++;
  if (i >= m_channelCount)
    return *this; // Channel not found

  for(; i < m_channelCount-1; i++)
    m_channels[i] = m_channels[i+1];

  m_channels[i] = AE_CH_NULL;
  m_channelCount--;
  return *this;
}

const enum AEChannel CAEChannelInfo::operator[](unsigned int i) const
{
  ASSERT(i < m_channelCount);
  return m_channels[i];
}

CAEChannelInfo::operator std::string()
{
  if (m_channelCount == 0)
    return "NULL";

  std::string s;
  for (unsigned int i = 0; i < m_channelCount - 1; ++i)
  {
    s.append(GetChName(m_channels[i]));
    s.append(",");
  }
  s.append(GetChName(m_channels[m_channelCount-1]));

  return s;
}

const char* CAEChannelInfo::GetChName(const enum AEChannel ch)
{
  ASSERT(ch >= 0 || ch < AE_CH_MAX);

  static const char* channels[AE_CH_MAX] =
  {
    "RAW" ,
    "FL"  , "FR" , "FC" , "LFE", "BL"  , "BR"  , "FLOC",
    "FROC", "BC" , "SL" , "SR" , "TFL" , "TFR" , "TFC" ,
    "TC"  , "TBL", "TBR", "TBC", "BLOC", "BROC",

    /* p16v devices */
    "UNKNOWN1",
    "UNKNOWN2",
    "UNKNOWN3",
    "UNKNOWN4",
    "UNKNOWN5",
    "UNKNOWN6",
    "UNKNOWN7",
    "UNKNOWN8"
  };

  return channels[ch];
}

bool CAEChannelInfo::HasChannel(const enum AEChannel ch) const
{
  for (unsigned int i = 0; i < m_channelCount; ++i)
    if (m_channels[i] == ch)
      return true;
  return false;
}

bool CAEChannelInfo::ContainsChannels(CAEChannelInfo& rhs) const
{
  for (unsigned int i = 0; i < rhs.m_channelCount; ++i)
  {
    if (!HasChannel(rhs.m_channels[i]))
      return false;
  }
  return true;
}
