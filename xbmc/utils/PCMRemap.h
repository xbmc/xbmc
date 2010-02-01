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
#include "guilib/StdString.h"

#define PCM_FRONT_LEFT            0
#define PCM_FRONT_RIGHT           1
#define PCM_FRONT_CENTER          2
#define PCM_LOW_FREQUENCY         3
#define PCM_BACK_LEFT             4
#define PCM_BACK_RIGHT            5
#define PCM_FRONT_LEFT_OF_CENTER  6
#define PCM_FRONT_RIGHT_OF_CENTER 7
#define PCM_BACK_CENTER           8
#define PCM_SIDE_LEFT             9
#define PCM_SIDE_RIGHT            10
#define PCM_TOP_CENTER            11
#define PCM_TOP_FRONT_LEFT        12
#define PCM_TOP_FRONT_CENTER      13
#define PCM_TOP_FRONT_RIGHT       14
#define PCM_TOP_BACK_LEFT         15
#define PCM_TOP_BACK_CENTER       16
#define PCM_TOP_BACK_RIGHT        17

class CPCMRemap
{
protected:
  bool         m_inSet     , m_outSet;
  unsigned int m_inChannels, m_outChannels;
  unsigned int m_inSampleSize;
  int8_t      *m_inMap     , *m_outMap;
  int8_t      *m_chLookup;

  void BuildMap();
  void DumpMap(CStdString info, int unsigned channels, int8_t *channelMap);
  void Dispose();
public:

  CPCMRemap();
  ~CPCMRemap();

  void Reset();
  void SetInputFormat (unsigned int channels, int8_t *channelMap, unsigned int sampleSize);
  void SetOutputFormat(unsigned int channels, int8_t *channelMap);
  void Remap(void *data, void *out, unsigned int samples);
  bool CanRemap();
};

#endif
