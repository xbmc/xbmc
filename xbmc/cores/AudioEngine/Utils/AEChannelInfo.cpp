/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AEChannelInfo.h"
#include <string.h>

CAEChannelInfo::CAEChannelInfo()
{
  Reset();
}

CAEChannelInfo::CAEChannelInfo(const AEChLayout rhs)
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

void CAEChannelInfo::Reset()
{
  m_channelCount = 0;
  for(unsigned int i = 0; i < AE_CH_MAX; ++i)
    m_channels[i] = AE_CH_NULL;
}

CAEChannelInfo& CAEChannelInfo::operator=(const CAEChannelInfo& rhs)
{
  if (this == &rhs)
    return *this;

  /* clone the information */
  m_channelCount = rhs.m_channelCount;
  memcpy(m_channels, rhs.m_channels, sizeof(enum AEChannel) * rhs.m_channelCount);
  m_channels[m_channelCount] = AE_CH_NULL;

  return *this;
}

CAEChannelInfo& CAEChannelInfo::operator=(const enum AEChannel* rhs)
{
  Reset();
  if(rhs == NULL)
    return *this;

  /* count the channels */
  m_channelCount = 0;
  while(rhs[m_channelCount] != AE_CH_NULL)
    ++m_channelCount;

  /* copy the info */
  memcpy(m_channels, rhs, sizeof(enum AEChannel) * m_channelCount);
  m_channels[m_channelCount] = AE_CH_NULL;

  return *this;
}

CAEChannelInfo& CAEChannelInfo::operator=(const enum AEStdChLayout rhs)
{
  ASSERT(rhs >= 0 && rhs < AE_CH_LAYOUT_MAX);

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
  for(unsigned int i = 0; i < m_channelCount; ++i)
    if (m_channels[i] != rhs.m_channels[i])
      return false;

  return true;
}

const enum AEChannel CAEChannelInfo::operator[](unsigned int i)
{
  ASSERT(i < m_channelCount);
  return m_channels[i];
}

CAEChannelInfo::operator CStdString()
{
  if (m_channelCount == 0)
    return "NULL";

  CStdString s;
  for(unsigned int i = 0; i < m_channelCount - 1; ++i)
  {
    s.append(GetChName(m_channels[i]));
    s.append(",");
  }
  s.append(GetChName(m_channels[m_channelCount-1]));

  return s;
}

CAEChannelInfo::operator unsigned int()
{
  return m_channelCount;
}

const char* CAEChannelInfo::GetChName(const enum AEChannel ch)
{
  if (ch < 0 || ch >= AE_CH_MAX)
    return "UNKNOWN";

  static const char* channels[AE_CH_MAX] =
  {
    "RAW" ,
    "FL"  , "FR" , "FC" , "LFE", "BL" , "BR" , "FLOC",
    "FROC", "BC" , "SL" , "SR" , "TFL", "TFR", "TFC" ,
    "TC"  , "TBL", "TBR", "TBC"
  };

  return channels[ch];
}

