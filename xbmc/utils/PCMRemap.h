#ifndef __PCM_REMAP__H__
#define __PCM_REMAP__H__

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

#include <stdint.h>
#include <vector>
#include "StdString.h"

#define PCM_MAX_CH 18
enum PCMChannels
{
  PCM_INVALID = -1,
  PCM_FRONT_LEFT,
  PCM_FRONT_RIGHT,
  PCM_FRONT_CENTER,
  PCM_LOW_FREQUENCY,
  PCM_BACK_LEFT,
  PCM_BACK_RIGHT,
  PCM_FRONT_LEFT_OF_CENTER,
  PCM_FRONT_RIGHT_OF_CENTER,
  PCM_BACK_CENTER,
  PCM_SIDE_LEFT,
  PCM_SIDE_RIGHT,
  PCM_TOP_FRONT_LEFT,
  PCM_TOP_FRONT_RIGHT,
  PCM_TOP_FRONT_CENTER,
  PCM_TOP_CENTER,
  PCM_TOP_BACK_LEFT,
  PCM_TOP_BACK_RIGHT,
  PCM_TOP_BACK_CENTER
};

#define PCM_MAX_LAYOUT 10
enum PCMLayout
{
  PCM_LAYOUT_2_0 = 0,
  PCM_LAYOUT_2_1,
  PCM_LAYOUT_3_0,
  PCM_LAYOUT_3_1,
  PCM_LAYOUT_4_0,
  PCM_LAYOUT_4_1,
  PCM_LAYOUT_5_0,
  PCM_LAYOUT_5_1,
  PCM_LAYOUT_7_0,
  PCM_LAYOUT_7_1
};

struct PCMMapInfo
{
  enum  PCMChannels channel;
  float level;
  bool  ifExists;
  int   in_offset;
  bool  copy;
};

//!  Channels remapper class
/*!
   The usual set-up process:
   - user calls SetInputFormat with the input channels information
   - SetInputFormat responds with a channelmap corresponding to the speaker
     layout that the user has configured, with empty (according to information
     calculated from the input channelmap) channels removed
   - user uses this information to create the desired output channelmap,
     and calls SetOutputFormat to set it (if the channelmap contains channels
     that do not exist in the configured speaker layout, they will contain
     only silence unless ignoreLayout is true)
 */

class CPCMRemap
{
protected:
  bool               m_inSet, m_outSet;
  enum PCMLayout     m_channelLayout;
  unsigned int       m_inChannels, m_outChannels;
  unsigned int       m_inSampleSize;
  enum PCMChannels   m_inMap [PCM_MAX_CH];
  enum PCMChannels   m_outMap[PCM_MAX_CH];
  enum PCMChannels   m_layoutMap[PCM_MAX_CH + 1];

  bool               m_ignoreLayout;
  bool               m_useable  [PCM_MAX_CH];
  int                m_inStride, m_outStride;
  struct PCMMapInfo  m_lookupMap[PCM_MAX_CH + 1][PCM_MAX_CH + 1];
  int                m_counts[PCM_MAX_CH];

  float*             m_buf;
  int                m_bufsize;
  float              m_attenuation;
  float              m_attenuationInc;
  float              m_attenuationMin; //lowest attenuation value during a call of Remap(), used for the codec info
  float              m_sampleRate;
  unsigned int       m_holdCounter;
  bool               m_limiterEnabled;

  struct PCMMapInfo* ResolveChannel(enum PCMChannels channel, float level, bool ifExists, std::vector<enum PCMChannels> path, struct PCMMapInfo *tablePtr);
  void               ResolveChannels(); //!< Partial BuildMap(), just enough to see which output channels are active
  void               BuildMap();
  void               DumpMap(CStdString info, int unsigned channels, enum PCMChannels *channelMap);
  void               Dispose();
  CStdString         PCMChannelStr(enum PCMChannels ename);
  CStdString         PCMLayoutStr(enum PCMLayout ename);

  void               CheckBufferSize(int size);
  void               ProcessInput(void* data, void* out, unsigned int samples, float gain);
  void               AddGain(float* buf, unsigned int samples, float gain);
  void               ProcessLimiter(unsigned int samples, float gain);
  void               ProcessOutput(void* out, unsigned int samples, float gain);

public:

  CPCMRemap();
  ~CPCMRemap();

  void Reset();
  enum PCMChannels *SetInputFormat (unsigned int channels, enum PCMChannels *channelMap, unsigned int sampleSize, unsigned int sampleRate);
  void SetOutputFormat(unsigned int channels, enum PCMChannels *channelMap, bool ignoreLayout = false);
  void Remap(void *data, void *out, unsigned int samples, long drc);
  void Remap(void *data, void *out, unsigned int samples, float gain = 1.0f);
  bool CanRemap();
  int  InBytesToFrames (int bytes );
  int  FramesToOutBytes(int frames);
  int  FramesToInBytes (int frames);
  float GetCurrentAttenuation() { return m_attenuationMin; }
};

#endif
