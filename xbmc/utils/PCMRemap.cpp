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

#define __STDC_LIMIT_MACROS

#include <cstdlib>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "MathUtils.h"
#include "PCMRemap.h"
#include "utils/log.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#ifdef _WIN32
#include "../win32/PlatformDefs.h"
#endif

static enum PCMChannels PCMLayoutMap[PCM_MAX_LAYOUT][PCM_MAX_CH + 1] =
{
  /* 2.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_INVALID},
  /* 2.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_LOW_FREQUENCY, PCM_INVALID},
  /* 3.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_INVALID},
  /* 3.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_INVALID},
  /* 4.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_INVALID},
  /* 4.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_LOW_FREQUENCY, PCM_INVALID},
  /* 5.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_INVALID},
  /* 5.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_LOW_FREQUENCY, PCM_INVALID},
  /* 7.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_SIDE_LEFT, PCM_SIDE_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_INVALID},
  /* 7.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_SIDE_LEFT, PCM_SIDE_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_LOW_FREQUENCY, PCM_INVALID}
};

/*
  map missing output into channel @ volume level
  the order of this table is important, mix tables can not depend on channels that have not been defined yet
  eg, FC can only be mixed into FL, FR as they are the only channels that have been defined
*/
#define PCM_MAX_MIX 3
static struct PCMMapInfo PCMDownmixTable[PCM_MAX_CH][PCM_MAX_MIX] =
{
  /* PCM_FRONT_LEFT */
  {
    {PCM_INVALID}
  },
  /* PCM_FRONT_RIGHT */
  {
    {PCM_INVALID}
  },
  /* PCM_FRONT_CENTER */
  {
    {PCM_FRONT_LEFT_OF_CENTER , 1.0},
    {PCM_FRONT_RIGHT_OF_CENTER, 1.0},
    {PCM_INVALID}
  },
  /* PCM_LOW_FREQUENCY */
  {
    /*
      A/52B 7.8 paragraph 2 recomends +10db
      but due to horrible clipping when normalize
      is disabled we set this to 1.0
    */
    {PCM_FRONT_LEFT           , 1.0},//3.5}, 
    {PCM_FRONT_RIGHT          , 1.0},//3.5},
    {PCM_INVALID}
  },
  /* PCM_BACK_LEFT */
  {
    {PCM_FRONT_LEFT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_BACK_RIGHT */
  {
    {PCM_FRONT_RIGHT          , 1.0},
    {PCM_INVALID}
  },
  /* PCM_FRONT_LEFT_OF_CENTER */
  {
    {PCM_FRONT_LEFT           , 1.0},
    {PCM_FRONT_CENTER         , 1.0, true},
    {PCM_INVALID}
  },
  /* PCM_FRONT_RIGHT_OF_CENTER */
  {
    {PCM_FRONT_RIGHT          , 1.0},
    {PCM_FRONT_CENTER         , 1.0, true},
    {PCM_INVALID}
  },
  /* PCM_BACK_CENTER */
  {
    {PCM_BACK_LEFT            , 1.0},
    {PCM_BACK_RIGHT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_SIDE_LEFT */
  {
    {PCM_FRONT_LEFT           , 1.0},
    {PCM_BACK_LEFT            , 1.0},
    {PCM_INVALID}
  },
  /* PCM_SIDE_RIGHT */
  {
    {PCM_FRONT_RIGHT          , 1.0},
    {PCM_BACK_RIGHT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_FRONT_LEFT */
  {
    {PCM_FRONT_LEFT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_FRONT_RIGHT */
  {
    {PCM_FRONT_RIGHT          , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_FRONT_CENTER */
  {
    {PCM_TOP_FRONT_LEFT       , 1.0},
    {PCM_TOP_FRONT_RIGHT      , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_CENTER */
  {
    {PCM_TOP_FRONT_LEFT       , 1.0},
    {PCM_TOP_FRONT_RIGHT      , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_BACK_LEFT */
  {
    {PCM_BACK_LEFT            , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_BACK_RIGHT */
  {
    {PCM_BACK_RIGHT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_BACK_CENTER */
  {
    {PCM_TOP_BACK_LEFT        , 1.0},
    {PCM_TOP_BACK_RIGHT       , 1.0},
    {PCM_INVALID}
  }
};

CPCMRemap::CPCMRemap() :
  m_inSet       (false),
  m_outSet      (false),
  m_inChannels  (0),
  m_outChannels (0),
  m_inSampleSize(0),
  m_ignoreLayout(false),
  m_buf(NULL),
  m_bufsize(0),
  m_attenuation (1.0),
  m_attenuationInc(0.0),
  m_attenuationMin(1.0),
  m_sampleRate  (48000.0), //safe default
  m_holdCounter (0),
  m_limiterEnabled(false)
{
  Dispose();
}

CPCMRemap::~CPCMRemap()
{
  Dispose();
}

void CPCMRemap::Dispose()
{
  free(m_buf);
  m_buf = NULL;
  m_bufsize = 0;
}

/* resolves the channels recursively and returns the new index of tablePtr */
struct PCMMapInfo* CPCMRemap::ResolveChannel(enum PCMChannels channel, float level, bool ifExists, std::vector<enum PCMChannels> path, struct PCMMapInfo *tablePtr)
{
  if (channel == PCM_INVALID) return tablePtr;

  /* if its a 1 to 1 mapping, return */
  if (m_useable[channel])
  {
    tablePtr->channel = channel;
    tablePtr->level   = level;

    ++tablePtr;
    tablePtr->channel = PCM_INVALID;
    return tablePtr;
  } else
    if (ifExists)
      level /= 2;

  struct PCMMapInfo *info;
  std::vector<enum PCMChannels>::iterator itt;

  for(info = PCMDownmixTable[channel]; info->channel != PCM_INVALID; ++info)
  {
    /* make sure we are not about to recurse into ourself */
    bool found = false;
    for(itt = path.begin(); itt != path.end(); ++itt)
      if (*itt == info->channel)
      {
        found = true;
        break;
      }

    if (found)
      continue;

    path.push_back(channel);
    float  l = (info->level * (level / 100)) * 100;
    tablePtr = ResolveChannel(info->channel, l, info->ifExists, path, tablePtr);
    path.pop_back();
  }

  return tablePtr;
}

/*
  Builds a lookup table without extra adjustments, useful if we simply
  want to find out which channels are active.
  For final adjustments, BuildMap() is used.
*/
void CPCMRemap::ResolveChannels()
{
  unsigned int in_ch, out_ch;
  bool hasSide = false;
  bool hasBack = false;
  
  memset(m_useable, 0, sizeof(m_useable));

  if (!m_outSet)
  {
    /* Output format is not known yet, assume the full configured map.
     * Note that m_ignoreLayout-using callers normally ignore the result of
     * this function when !m_outSet, when it is called only for an advice for
     * the caller of SetInputFormat about the best possible output map, and
     * they can still set their output format arbitrarily in their call to
     * SetOutputFormat. */
    for (enum PCMChannels *chan = PCMLayoutMap[m_channelLayout]; *chan != PCM_INVALID; ++chan)
         m_useable[*chan] = true;
  }
  else if (m_ignoreLayout)
  {
    for(out_ch = 0; out_ch < m_outChannels; ++out_ch)
      m_useable[m_outMap[out_ch]] = true;
  }
  else
  {
    /* figure out what channels we have and can use */
    for(enum PCMChannels *chan = PCMLayoutMap[m_channelLayout]; *chan != PCM_INVALID; ++chan)
    {
      for(out_ch = 0; out_ch < m_outChannels; ++out_ch)
        if (m_outMap[out_ch] == *chan)
        {
          m_useable[*chan] = true;
          break;
        }
    }
  }

  /* force mono audio to front left and front right */
  if (!m_ignoreLayout && m_inChannels == 1 && m_inMap[0] == PCM_FRONT_CENTER
      && m_useable[PCM_FRONT_LEFT] && m_useable[PCM_FRONT_RIGHT])
  {
    CLog::Log(LOGDEBUG, "CPCMRemap: Mapping mono audio to front left and front right");
    m_useable[PCM_FRONT_CENTER] = false;
    m_useable[PCM_FRONT_LEFT_OF_CENTER] = false;
    m_useable[PCM_FRONT_RIGHT_OF_CENTER] = false;
  }

  /* see if our input has side/back channels */
  for(in_ch = 0; in_ch < m_inChannels; ++in_ch)
    switch(m_inMap[in_ch])
    {
      case PCM_SIDE_LEFT:
      case PCM_SIDE_RIGHT:
        hasSide = true;
        break;

      case PCM_BACK_LEFT:
      case PCM_BACK_RIGHT:
        hasBack = true;
        break;

      default:;
    }

  /* if our input has side, and not back channels, and our output doesnt have side channels */
  if (hasSide && !hasBack && (!m_useable[PCM_SIDE_LEFT] || !m_useable[PCM_SIDE_RIGHT]))
  {
    CLog::Log(LOGDEBUG, "CPCMRemap: Forcing side channel map to back channels");
    for(in_ch = 0; in_ch < m_inChannels; ++in_ch)
           if (m_inMap[in_ch] == PCM_SIDE_LEFT ) m_inMap[in_ch] = PCM_BACK_LEFT;
      else if (m_inMap[in_ch] == PCM_SIDE_RIGHT) m_inMap[in_ch] = PCM_BACK_RIGHT;   
  }

  /* resolve all the channels */
  struct PCMMapInfo table[PCM_MAX_CH + 1], *info, *dst;
  std::vector<enum PCMChannels> path;

  for (int i = 0; i < PCM_MAX_CH + 1; i++)
  {
    for (int j = 0; j < PCM_MAX_CH + 1; j++)
      m_lookupMap[i][j].channel = PCM_INVALID;
  }

  memset(m_counts, 0, sizeof(m_counts));
  for(in_ch = 0; in_ch < m_inChannels; ++in_ch) {

    for (int i = 0; i < PCM_MAX_CH + 1; i++)
      table[i].channel = PCM_INVALID;

    ResolveChannel(m_inMap[in_ch], 1.0f, false, path, table);
    for(info = table; info->channel != PCM_INVALID; ++info)
    {
      /* find the end of the table */
      for(dst = m_lookupMap[info->channel]; dst->channel != PCM_INVALID; ++dst);

      /* append it to the table and set its input offset */
      dst->channel   = m_inMap[in_ch];
      dst->in_offset = in_ch * 2;
      dst->level     = info->level;
      m_counts[dst->channel]++;
    }
  }
}

/*
  builds a lookup table to convert from the input mapping to the output
  mapping, this decreases the amount of work per sample to remap it.
*/
void CPCMRemap::BuildMap()
{
  struct PCMMapInfo *dst;
  unsigned int out_ch;

  if (!m_inSet || !m_outSet) return;

  m_inStride  = m_inSampleSize * m_inChannels ;
  m_outStride = m_inSampleSize * m_outChannels;

  /* see if we need to normalize the levels */
  bool dontnormalize = g_guiSettings.GetBool("audiooutput.dontnormalizelevels");
  CLog::Log(LOGDEBUG, "CPCMRemap: Downmix normalization is %s", (dontnormalize ? "disabled" : "enabled"));

  ResolveChannels();

  /* convert the levels into RMS values */
  float loudest    = 0.0;
  bool  hasLoudest = false;

  for(out_ch = 0; out_ch < m_outChannels; ++out_ch)
  {
    float scale = 0;
    int count = 0;
    for(dst = m_lookupMap[m_outMap[out_ch]]; dst->channel != PCM_INVALID; ++dst)
    {
      dst->copy  = false;
      dst->level = dst->level / sqrt((float)m_counts[dst->channel]);
      scale     += dst->level;
      ++count;
    }

    /* if there is only 1 channel to mix, and the level is 1.0, then just copy the channel */
    dst = m_lookupMap[m_outMap[out_ch]];
    if (count == 1 && dst->level > 0.99 && dst->level < 1.01)
      dst->copy = true;
    
    /* normalize the levels if it is turned on */
    if (!dontnormalize)
      for(dst = m_lookupMap[m_outMap[out_ch]]; dst->channel != PCM_INVALID; ++dst)
      {
        dst->level /= scale;
        /* find the loudest output level we have that is not 1-1 */
        if (dst->level < 1.0 && loudest < dst->level)
        {
          loudest    = dst->level;
          hasLoudest = true;
        }
      }
  }
  
  /* adjust the channels that are too loud */
  for(out_ch = 0; out_ch < m_outChannels; ++out_ch)
  {
    CStdString s = "", f;
    for(dst = m_lookupMap[m_outMap[out_ch]]; dst->channel != PCM_INVALID; ++dst)
    {
      if (hasLoudest && dst->copy)
      {
        dst->level = loudest;
        dst->copy  = false;
      }

      f.Format("%s(%f%s) ",  PCMChannelStr(dst->channel).c_str(), dst->level, dst->copy ? "*" : "");
      s += f;
    }
    CLog::Log(LOGDEBUG, "CPCMRemap: %s = %s\n", PCMChannelStr(m_outMap[out_ch]).c_str(), s.c_str());
  }
}

void CPCMRemap::DumpMap(CStdString info, unsigned int channels, enum PCMChannels *channelMap)
{
  if (channelMap == NULL)
  {
    CLog::Log(LOGINFO, "CPCMRemap: %s channel map: NULL", info.c_str());
    return;
  }

  CStdString mapping;
  for(unsigned int i = 0; i < channels; ++i)
    mapping += ((i == 0) ? "" : ",") + PCMChannelStr(channelMap[i]);

  CLog::Log(LOGINFO, "CPCMRemap: %s channel map: %s\n", info.c_str(), mapping.c_str());
}

void CPCMRemap::Reset()
{
  m_inSet  = false;
  m_outSet = false;
  Dispose();
}

/* sets the input format, and returns the requested channel layout */
enum PCMChannels *CPCMRemap::SetInputFormat(unsigned int channels, enum PCMChannels *channelMap, unsigned int sampleSize, unsigned int sampleRate)
{
  m_inChannels   = channels;
  m_inSampleSize = sampleSize;
  m_sampleRate   = (float)sampleRate;
  m_inSet        = channelMap != NULL;
  if (channelMap)
    memcpy(m_inMap, channelMap, sizeof(enum PCMChannels) * channels);

  /* fix me later */
  assert(sampleSize == 2);

  /* get the audio layout, and count the channels in it */
  m_channelLayout  = (enum PCMLayout)g_guiSettings.GetInt("audiooutput.channellayout");
  if (m_channelLayout >= PCM_MAX_LAYOUT) m_channelLayout = PCM_LAYOUT_2_0;

  //spdif only has 2 pcm channels, so don't try to use more
  if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_IEC958)
  {
    CLog::Log(LOGINFO, "CPCMRemap: Configured speaker layout: %s (iec958)\n", PCMLayoutStr(m_channelLayout).c_str());
    m_channelLayout = PCM_LAYOUT_2_0;
  }
  else
    CLog::Log(LOGINFO, "CPCMRemap: Configured speaker layout: %s\n", PCMLayoutStr(m_channelLayout).c_str());

  
  DumpMap("I", channels, channelMap);
  BuildMap();

  /* now remove the empty channels from PCMLayoutMap;
   * we don't perform upmixing so we want the minimum amount of those */
  if (channelMap) {
    if (!m_outSet)
      ResolveChannels(); /* Do basic channel resolving to find out the empty channels;
                          * If m_outSet == true, this was done already by BuildMap() above */
    int i = 0;
    for (enum PCMChannels *chan = PCMLayoutMap[m_channelLayout]; *chan != PCM_INVALID; ++chan)
      if (m_lookupMap[*chan][0].channel != PCM_INVALID) {
        /* something is mapped here, so add the channel */
        m_layoutMap[i++] = *chan;
      }
    m_layoutMap[i] = PCM_INVALID;
  } else
    memcpy(m_layoutMap, PCMLayoutMap[m_channelLayout], sizeof(PCMLayoutMap[m_channelLayout]));

  m_attenuation = 1.0;
  m_attenuationInc = 1.0;
  m_holdCounter = 0;

  return m_layoutMap;
}

/* sets the output format supported by the audio renderer */
void CPCMRemap::SetOutputFormat(unsigned int channels, enum PCMChannels *channelMap, bool ignoreLayout/* = false */)
{
  m_outChannels   = channels;
  m_outSet        = channelMap != NULL;
  m_ignoreLayout  = ignoreLayout;
  if (channelMap)
    memcpy(m_outMap, channelMap, sizeof(enum PCMChannels) * channels);

  DumpMap("O", channels, channelMap);
  BuildMap();

  m_attenuation = 1.0;
  m_attenuationInc = 1.0;
  m_holdCounter = 0;
}

void CPCMRemap::Remap(void *data, void *out, unsigned int samples, long drc)
{
  float gain = 1.0f;
  if (drc > 0)
    gain = pow(10.0f, (float)drc / 2000.0f);

  Remap(data, out, samples, gain);
}

/* remap the supplied data into out, which must be pre-allocated */
void CPCMRemap::Remap(void *data, void *out, unsigned int samples, float gain /*= 1.0f*/)
{
  CheckBufferSize(samples * m_outChannels * sizeof(float));

  //set output buffer to 0
  memset(out, 0, samples * m_outChannels * m_inSampleSize);

  //set intermediate buffer to 0
  memset(m_buf, 0, m_bufsize);

  ProcessInput(data, out, samples, gain);
  AddGain(m_buf, samples * m_outChannels, gain);
  ProcessLimiter(samples, gain);
  ProcessOutput(out, samples, gain);
}

void CPCMRemap::CheckBufferSize(int size)
{
  if (m_bufsize < size)
  {
    m_bufsize = size;
    m_buf = (float*)realloc(m_buf, m_bufsize);
  }
}

void CPCMRemap::ProcessInput(void* data, void* out, unsigned int samples, float gain)
{
  for (unsigned int ch = 0; ch < m_outChannels; ch++)
  {
    struct PCMMapInfo *info = m_lookupMap[m_outMap[ch]];
    if (info->channel == PCM_INVALID)
      continue;

    if (info->copy && gain == 1.0f) //do direct copy
    {
      uint8_t* src = (uint8_t*)data + info->in_offset;
      uint8_t* dst = (uint8_t*)out  + ch * m_inSampleSize;
      uint8_t* dstend = dst + samples * m_outStride;
      while (dst != dstend)
      {
        *(int16_t*)dst = *(int16_t*)src;
        src += m_inStride;
        dst += m_outStride;
      }
    }
    else //needs some volume change or mixing, put into intermediate buffer
    {
      for(; info->channel != PCM_INVALID; info++)
      {
        uint8_t* src = (uint8_t*)data + info->in_offset;
        float*   dst = m_buf + ch;
        float*   dstend = dst + samples * m_outChannels;
        while (dst != dstend)
        {
          *dst += (float)(*(int16_t*)src) * info->level;
          src += m_inStride;
          dst += m_outChannels;
        }
      }
    }
  }
}

void CPCMRemap::AddGain(float* buf, unsigned int samples, float gain)
{
  if (gain != 1.0f) //needs a gain change
  {
    float* ptr = m_buf;
    float* end = m_buf + samples;
    while (ptr != end)
      *(ptr++) *= gain;
  }
}

void CPCMRemap::ProcessLimiter(unsigned int samples, float gain)
{
  //check total gain for each output channel
  float highestgain = 1.0f;
  for (unsigned int ch = 0; ch < m_outChannels; ch++)
  {
    struct PCMMapInfo *info = m_lookupMap[m_outMap[ch]];
    if (info->channel == PCM_INVALID)
      continue;

    float chgain = 0.0f;
    for(; info->channel != PCM_INVALID; info++)
      chgain += info->level * gain;

    if (chgain > highestgain)
      highestgain = chgain;
  }

  m_attenuationMin = 1.0f;

  //if one of the channels can clip, enable a limiter
  if (highestgain > 1.0001f) 
  {
    m_attenuationMin = m_attenuation;

    if (!m_limiterEnabled)
    {
      CLog::Log(LOGDEBUG, "CPCMRemap:: max gain: %f, enabling limiter", highestgain);
      m_limiterEnabled = true;
    }

    for (unsigned int i = 0; i < samples; i++)
    {
      //for each collection of samples, get the highest absolute value
      float maxAbs = 0.0f;
      for (unsigned int outch = 0; outch < m_outChannels; outch++)
      {
        float absval = fabs(m_buf[i * m_outChannels + outch]) / 32768.0f;
        if (maxAbs < absval)
          maxAbs = absval;
      }

      //if attenuatedAbs is higher than 1.0f, audio is clipping
      float attenuatedAbs = maxAbs * m_attenuation;
      if (attenuatedAbs > 1.0f)
      {
        //set m_attenuation so that m_attenuation * sample is the maximum output value
        m_attenuation = 1.0f / maxAbs;
        if (m_attenuation < m_attenuationMin)
          m_attenuationMin = m_attenuation;
        //value to add to m_attenuation to make it 1.0f
        m_attenuationInc = 1.0f - m_attenuation;
        //amount of samples to hold m_attenuation
        m_holdCounter = MathUtils::round_int(m_sampleRate * g_advancedSettings.m_limiterHold);
      }
      else if (m_attenuation < 1.0f && attenuatedAbs > 0.95f)
      {
        //if we're attenuating and we get within 5% of clipping, hold m_attenuation
        m_attenuationInc = 1.0f - m_attenuation;
        m_holdCounter = MathUtils::round_int(m_sampleRate * g_advancedSettings.m_limiterHold);
      }

      //apply attenuation
      for (unsigned int outch = 0; outch < m_outChannels; outch++)
        m_buf[i * m_outChannels + outch] *= m_attenuation;

      if (m_holdCounter)
      {
        //hold m_attenuation
        m_holdCounter--;
      }
      else if (m_attenuationInc > 0.0f)
      {
        //move m_attenuation to 1.0 in g_advancedSettings.m_limiterRelease seconds
        m_attenuation += m_attenuationInc / m_sampleRate / g_advancedSettings.m_limiterRelease;
        if (m_attenuation > 1.0f)
        {
          m_attenuation = 1.0f;
          m_attenuationInc = 0.0f;
        }
      }
    }
  }
  else
  {
    if (m_limiterEnabled)
    {
      CLog::Log(LOGDEBUG, "CPCMRemap:: max gain: %f, disabling limiter", highestgain);
      m_limiterEnabled = false;
    }

    //reset the limiter
    m_attenuation = 1.0f;
    m_attenuationInc = 0.0f;
    m_holdCounter = 0;
  }
}

void CPCMRemap::ProcessOutput(void* out, unsigned int samples, float gain)
{
  //copy from intermediate buffer to output
  for (unsigned int ch = 0; ch < m_outChannels; ch++)
  {
    struct PCMMapInfo *info = m_lookupMap[m_outMap[ch]];
    if (info->channel == PCM_INVALID)
      continue;

    if (!info->copy || gain != 1.0f)
    {
      float* src = m_buf + ch;
      uint8_t* dst = (uint8_t*)out + ch * m_inSampleSize;
      uint8_t* dstend = dst + samples * m_outStride;

      while(dst != dstend)
      {
        *(int16_t*)dst = MathUtils::round_int(std::min(std::max(*src, (float)INT16_MIN), (float)INT16_MAX));
        src += m_outChannels;
        dst += m_outStride;
      }
    }
  }
}

bool CPCMRemap::CanRemap()
{
  return (m_inSet && m_outSet);
}

int CPCMRemap::InBytesToFrames(int bytes)
{
  return bytes / m_inSampleSize / m_inChannels;
}

int CPCMRemap::FramesToOutBytes(int frames)
{
  return frames * m_inSampleSize * m_outChannels;
}

int CPCMRemap::FramesToInBytes(int frames)
{
  return frames * m_inSampleSize * m_inChannels;
}

CStdString CPCMRemap::PCMChannelStr(enum PCMChannels ename)
{
  const char* PCMChannelName[] =
  {
    "FL",
    "FR",
    "CE",
    "LFE",
    "BL",
    "BR",
    "FLOC",
    "FROC",
    "BC",
    "SL",
    "SR",
    "TFL",
    "TFR",
    "TFC",
    "TC",
    "TBL",
    "TBR",
    "TBC"
  };

  int namepos = (int)ename;
  CStdString namestr;

  if (namepos < 0 || namepos >= (int)(sizeof(PCMChannelName) / sizeof(const char*)))
    namestr.Format("UNKNOWN CHANNEL:%i", namepos);
  else
    namestr = PCMChannelName[namepos];

  return namestr;
}

CStdString CPCMRemap::PCMLayoutStr(enum PCMLayout ename)
{
  const char* PCMLayoutName[] =
  {
    "2.0",
    "2.1",
    "3.0",
    "3.1",
    "4.0",
    "4.1",
    "5.0",
    "5.1",
    "7.0",
    "7.1"
  };

  int namepos = (int)ename;
  CStdString namestr;

  if (namepos < 0 || namepos >= (int)(sizeof(PCMLayoutName) / sizeof(const char*)))
    namestr.Format("UNKNOWN LAYOUT:%i", namepos);
  else
    namestr = PCMLayoutName[namepos];

  return namestr;
}

