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
#include <math.h>

#include "AERemap.h"
#include "AEFactory.h"
#include "AEUtil.h"
#include "utils/log.h"
#include "GUISettings.h"

using namespace std;

CAERemap::CAERemap()
{
}

CAERemap::~CAERemap()
{
}

bool CAERemap::Initialize(const AEChLayout input, const AEChLayout output, bool finalStage)
{
  /* build the downmix matrix */
  memset(m_mixInfo, 0, sizeof(m_mixInfo));
  m_output = output;

  /* figure which channels we have */
  for(int o = 0; output[o] != AE_CH_NULL; ++o) {
    m_mixInfo[output[o]].in_dst = true;
    m_outChannels = o;
  }
  ++m_outChannels;

  /* lookup the channels that exist in the output */
  for(int i = 0; input[i] != AE_CH_NULL; ++i) {
    AEMixInfo  *info = &m_mixInfo[input[i]];
    AEMixLevel *lvl  = &info->srcIndex[info->srcCount++];
    info->in_src = true;
    lvl->index   = i;
    lvl->level   = 1.0f;
    for(int o = 0; output[o] != AE_CH_NULL; ++o)
      if (input[i] == output[o])
      {
        info->outIndex = o;
        break;
      }

    m_inChannels = i;
  }
  ++m_inChannels;

  /* the final stage does not need any down/upmix */
  if (finalStage)
    return true;

  /* downmix from the specified channel to the specified list of channels */
  #define RM(from, ...) \
    static AEChannel downmix_##from[] = {__VA_ARGS__, AE_CH_NULL}; \
    ResolveMix(from, downmix_##from);

  /*
    the order of this is important as we can not mix channels
    into ones that have already been resolved... eg

    TBR -> BR
    TBC -> TBL & TBR

    TBR will get resolved to BR, and then TBC will get
    resolved to TBL, but since TBR has been resolved it will
    never make it to the output. The order has to be reversed
    as the TBC center depends on the TBR downmix... eg

    TBC -> TBL & TBR
    TBR -> BR

    if for any reason out of order mapping becomes required
    looping this list should resolve the channels.
  */
  RM(AE_CH_TBC , AE_CH_TBL, AE_CH_TBR);
  RM(AE_CH_TBR , AE_CH_BR);
  RM(AE_CH_TBL , AE_CH_BL);
  RM(AE_CH_TC  , AE_CH_TFL, AE_CH_TFR);
  RM(AE_CH_TFC , AE_CH_TFL, AE_CH_TFR);
  RM(AE_CH_TFR , AE_CH_FR);
  RM(AE_CH_TFL , AE_CH_FL);
  RM(AE_CH_SR  , AE_CH_BR, AE_CH_FR);
  RM(AE_CH_SL  , AE_CH_BL, AE_CH_FL);
  RM(AE_CH_BC  , AE_CH_BL, AE_CH_BR);
  RM(AE_CH_FROC, AE_CH_FR, AE_CH_FC);
  RM(AE_CH_FLOC, AE_CH_FL, AE_CH_FC);
  RM(AE_CH_BL  , AE_CH_FL);
  RM(AE_CH_BR  , AE_CH_FR);
  RM(AE_CH_LFE , AE_CH_FL  , AE_CH_FR);
  RM(AE_CH_FL  , AE_CH_FC);
  RM(AE_CH_FR  , AE_CH_FC);

  /* since everything eventually mixes down to FC we need special handling for it */
  if (m_mixInfo[AE_CH_FC].in_src)
  {
    /* if there is no output FC channel, try to mix it the best possible way */
    if (!m_mixInfo[AE_CH_FC].in_dst)
    {
      /* if we have TFC & FL & FR */
      if (m_mixInfo[AE_CH_TFC].in_dst && m_mixInfo[AE_CH_FL].in_dst && m_mixInfo[AE_CH_FR].in_dst)
      {
        RM(AE_CH_FC, AE_CH_TFC, AE_CH_FL, AE_CH_FR);
      }
      else
      /* if we have TFC */
      if (m_mixInfo[AE_CH_TFC].in_dst)
      {
        RM(AE_CH_FC, AE_CH_TFC);
      }
      else
      /* if we have FLOC & FROC */
      if (m_mixInfo[AE_CH_FLOC].in_dst && m_mixInfo[AE_CH_FROC].in_dst)
      {
        RM(AE_CH_FC, AE_CH_FLOC, AE_CH_FROC);
      }
      else
      /* if we have TC & FL & FR */
      if (m_mixInfo[AE_CH_TC].in_dst && m_mixInfo[AE_CH_FL].in_dst && m_mixInfo[AE_CH_FR].in_dst)
      {
        RM(AE_CH_FC, AE_CH_TC, AE_CH_FL, AE_CH_FR);
      }
      else
      /* if we have FL & FR */
      if (m_mixInfo[AE_CH_FL].in_dst && m_mixInfo[AE_CH_FR].in_dst)
      {
        RM(AE_CH_FC, AE_CH_FL, AE_CH_FR);
      }
      else
      /* if we have TFL & TFR */
      if (m_mixInfo[AE_CH_TFL].in_dst && m_mixInfo[AE_CH_TFR].in_dst)
      {
        RM(AE_CH_FC, AE_CH_TFL, AE_CH_TFR);
      }
      else
        /* we dont have enough speakers to emulate FC */
        return false;
    }
    else
    {
      /* if there is only one channel in the source and it is the FC and we have FL & FR, upmix to dual mono */
      if (m_inChannels == 1 && m_mixInfo[AE_CH_FL].in_dst && m_mixInfo[AE_CH_FR].in_dst)
      {
        RM(AE_CH_FC, AE_CH_FL, AE_CH_FR);
      }
    }
  }

  #undef RM

  /* normalize the values */
  bool dontnormalize = g_guiSettings.GetBool("audiooutput.dontnormalizelevels");
  CLog::Log(LOGDEBUG, "AERemap: Downmix normalization is %s", (dontnormalize ? "disabled" : "enabled"));
  if (!dontnormalize)
  {
    float max = 0;
    for(int o = 0; output[o] != AE_CH_NULL; ++o)
    {
      AEMixInfo *info = &m_mixInfo[output[o]];
      float sum = 0;
      for(int i = 0; i < info->srcCount; ++i)
        sum += info->srcIndex[i].level;

      if (sum > max)
        max = sum;
    }

    float scale = 1.0f / max;
    for(int o = 0; output[o] != AE_CH_NULL; ++o)
    {
      AEMixInfo *info = &m_mixInfo[output[o]];
      for(int i = 0; i < info->srcCount; ++i)
        info->srcIndex[i].level *= scale;
    }
  }

  /* dump the matrix */
  CLog::Log(LOGINFO, "==[Downmix Matrix]==");
  for(int o = 0; output[o] != AE_CH_NULL; ++o)
  {
    AEMixInfo *info = &m_mixInfo[output[o]];
    if (info->srcCount == 0) continue;
  
    CStdString s = CAEUtil::GetChName(output[o]) + CStdString(" =");
    for(int i = 0; i < info->srcCount; ++i)
    {
      CStdString lvl;
      lvl.Format("(%1.4f)", info->srcIndex[i].level);
      s.append(CStdString(" ") + CAEUtil::GetChName(input[info->srcIndex[i].index]) + lvl);
    }

    CLog::Log(LOGINFO, "%s", s.c_str());
  }
  CLog::Log(LOGINFO, "====================\n");

  return true;
}

void CAERemap::ResolveMix(const AEChannel from, const AEChLayout to)
{
  AEMixInfo *fromInfo = &m_mixInfo[from];
  if (fromInfo->in_dst || !fromInfo->in_src) return;
  unsigned int toCh = CAEUtil::GetChLayoutCount(to);

  for(int i = 0; to[i] != AE_CH_NULL; ++i)
  {
    AEMixInfo *toInfo = &m_mixInfo[to[i]];
    toInfo->in_src = true;
    for(int o = 0; o < fromInfo->srcCount; ++o)
    {
      AEMixLevel *fromLvl = &fromInfo->srcIndex[o];
      AEMixLevel *toLvl   = NULL;

      /* if its already in the output, then we need to combine the levels */
      for(int l = 0; l < toInfo->srcCount; ++l)
         if (toInfo->srcIndex[l].index == fromLvl->index)
         {
           toLvl = &toInfo->srcIndex[l];
           toLvl->level = (fromLvl->level + toLvl->level) / sqrt((float)toCh + 1.0f);
           break;
         }

      if (toLvl)
        continue;

      toLvl = &toInfo->srcIndex[toInfo->srcCount++];
      toLvl->index = fromLvl->index;
      toLvl->level = fromLvl->level / sqrt((float)toCh);
    }
  }

  fromInfo->srcCount = 0;
  fromInfo->in_src   = false;
}

void CAERemap::Remap(float *in, float *out, unsigned int frames)
{
  for(unsigned int f = 0; f < frames; ++f)
  {
    for(int o = 0; m_output[o] != AE_CH_NULL; ++o)
    {
      AEMixInfo *info = &m_mixInfo[m_output[o]];

      /* if there is only 1 source, just copy it so we dont break DPL */
      if (info->srcCount == 1)
        *out = in[info->srcIndex[0].index];
      else
      {
        *out = 0;
        for(int i = 0; i < info->srcCount; ++i)
          *out += in[info->srcIndex[i].index] * info->srcIndex[i].level;
      }

      ++out;
    }
    in += m_inChannels;
  }
}

