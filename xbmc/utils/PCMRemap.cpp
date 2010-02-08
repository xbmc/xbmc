/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <cstdlib>
#include <string.h>
#include <stdio.h>

#include "PCMRemap.h"
#include "utils/log.h"

CPCMRemap::CPCMRemap() :
  m_inSet   (false),
  m_outSet  (false),
  m_inChannels  (0),
  m_outChannels (0),
  m_inSampleSize(0),
  m_inMap   (NULL ),
  m_outMap  (NULL ),
  m_chLookup(NULL )
{
}

CPCMRemap::~CPCMRemap()
{
  Dispose();
}

void CPCMRemap::Dispose()
{
  delete[] m_chLookup;
  m_chLookup = NULL;
}

/*
  builds a lookup table to convert from the input mapping to the output
  mapping, this decreases the amount of work per sample to remap it.
*/
void CPCMRemap::BuildMap()
{
  unsigned int in_ch, out_ch;

  if (!m_inSet || !m_outSet) return;
  Dispose();

  delete[] m_chLookup;
  m_chLookup = new int8_t[m_inChannels];
  for(in_ch = 0; in_ch < m_inChannels; ++in_ch)
  {
    for(out_ch = 0; out_ch < m_outChannels; ++out_ch)
    {
      if (m_outMap[out_ch] == m_inMap[in_ch])
      {
        m_chLookup[in_ch] = out_ch;
        break;
      }
    }
  }
}

void CPCMRemap::DumpMap(CStdString info, unsigned int channels, int8_t *channelMap)
{
  if (channelMap == NULL)
  {
    CLog::Log(LOGINFO, "CPCMRemap: %s channel map: NULL", info.c_str());
    return;
  }

  CStdString mapping = "", convert;
  for(unsigned int i = 0; i < channels; ++i)
  {
    convert.Format("%d", channelMap[i]);
    mapping += ((i == 0) ? "" : ",") + convert;
  }
  CLog::Log(LOGINFO, "CPCMRemap: %s channel map: %s\n", info.c_str(), mapping.c_str());
}

void CPCMRemap::Reset()
{
  m_inMap  = NULL;
  m_outMap = NULL;
  m_inSet  = false;
  m_outSet = false;
}

/* sets the input format */
void CPCMRemap::SetInputFormat(unsigned int channels, int8_t *channelMap, unsigned int sampleSize)
{
  m_inChannels   = channels;
  m_inMap        = channelMap;
  m_inSampleSize = sampleSize;
  m_inSet        = channelMap != NULL;

  DumpMap("Input", channels, channelMap);
  BuildMap();
}

/* sets the output format required */
void CPCMRemap::SetOutputFormat(unsigned int channels, int8_t *channelMap)
{
  m_outChannels   = channels;
  m_outMap        = channelMap;
  m_outSet        = channelMap != NULL;

  DumpMap("Output", channels, channelMap);
  BuildMap();
}

/* remap the supplied data into out, which must be pre-allocated */
void CPCMRemap::Remap(void *data, void *out, unsigned int samples)
{
  unsigned int i, ch;
  uint8_t      *insample, *outsample;
  uint8_t      *src, *dst;

  insample  = (uint8_t*)data;
  outsample = (uint8_t*)out;

  /*
    the output may have channels the input does not have, so zero the data
    to stop random data being sent to them.
  */
  memset(out, 0, samples * (m_inSampleSize * m_outChannels));
  for(i = 0; i < samples; ++i)
  {
    for(ch = 0; ch < m_inChannels; ++ch)
    {
      src = insample  + (ch             * m_inSampleSize);
      dst = outsample + (m_chLookup[ch] * m_inSampleSize);
      memcpy(dst, src, m_inSampleSize);
    }
    insample  += m_inSampleSize * m_inChannels;
    outsample += m_inSampleSize * m_outChannels;
  }
}

bool CPCMRemap::CanRemap()
{
  return (m_inSet && m_outSet);
}

