#pragma once
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

#include "AEAudioFormat.h"

class CAERemap {
public:
  CAERemap();
  ~CAERemap();

  bool Initialize(CAEChannelInfo input, CAEChannelInfo output, bool finalStage, bool forceNormalize = false, enum AEStdChLayout stdChLayout = AE_CH_LAYOUT_INVALID);
  void Remap(float * const in, float * const out, const unsigned int frames) const;

private:
  typedef struct {
    int       index;
    float     level;
  } AEMixLevel;

  typedef struct {
    bool              in_src;
    bool              in_dst;
    int               outIndex;
    int               srcCount;
    AEMixLevel        srcIndex[AE_CH_MAX];
    int               cpyCount; /* the number of times the channel has been cloned */
  } AEMixInfo;

  AEMixInfo      m_mixInfo[AE_CH_MAX+1];
  CAEChannelInfo m_output;
  int            m_inChannels;
  int            m_outChannels;

  void ResolveMix(const AEChannel from, CAEChannelInfo to);
  void BuildUpmixMatrix(const CAEChannelInfo& input, const CAEChannelInfo& output);
};

