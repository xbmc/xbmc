#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AEAudioFormat.h"

class CAERemap {
public:
  CAERemap();
  ~CAERemap();

  bool Initialize(const AEChLayout input, const AEChLayout output, bool finalStage, bool forceNormalize = false);
  void Remap(float *in, float *out, unsigned int frames);

private:
  typedef struct {
    int   index;
    float level;
  } AEMixLevel;

  typedef struct {
    bool              in_src;
    bool              in_dst;
    int               outIndex;
    int               srcCount;
    AEMixLevel        srcIndex[AE_CH_MAX];
  } AEMixInfo;

  AEMixInfo  m_mixInfo[AE_CH_MAX+1];
  AEChLayout m_output;
  int        m_inChannels;
  int        m_outChannels;

  void ResolveMix(const AEChannel from, const AEChLayout to);
};

