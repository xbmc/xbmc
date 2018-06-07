/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AudioTranslator.h"

using namespace KODI;
using namespace RETRO;

AEDataFormat CAudioTranslator::TranslatePCMFormat(PCMFormat format)
{
  switch (format)
  {
  case PCMFormat::FMT_S16NE: return AE_FMT_S16NE;
  default:
    break;
  }
  return AE_FMT_INVALID;
}

AEChannel CAudioTranslator::TranslateAudioChannel(AudioChannel channel)
{
  switch (channel)
  {
  case AudioChannel::CH_FL:   return AE_CH_FL;
  case AudioChannel::CH_FR:   return AE_CH_FR;
  case AudioChannel::CH_FC:   return AE_CH_FC;
  case AudioChannel::CH_LFE:  return AE_CH_LFE;
  case AudioChannel::CH_BL:   return AE_CH_BL;
  case AudioChannel::CH_BR:   return AE_CH_BR;
  case AudioChannel::CH_FLOC: return AE_CH_FLOC;
  case AudioChannel::CH_FROC: return AE_CH_FROC;
  case AudioChannel::CH_BC:   return AE_CH_BC;
  case AudioChannel::CH_SL:   return AE_CH_SL;
  case AudioChannel::CH_SR:   return AE_CH_SR;
  case AudioChannel::CH_TFL:  return AE_CH_TFL;
  case AudioChannel::CH_TFR:  return AE_CH_TFR;
  case AudioChannel::CH_TFC:  return AE_CH_TFC;
  case AudioChannel::CH_TC:   return AE_CH_TC;
  case AudioChannel::CH_TBL:  return AE_CH_TBL;
  case AudioChannel::CH_TBR:  return AE_CH_TBR;
  case AudioChannel::CH_TBC:  return AE_CH_TBC;
  case AudioChannel::CH_BLOC: return AE_CH_BLOC;
  case AudioChannel::CH_BROC: return AE_CH_BROC;
  default:
    break;
  }
  return AE_CH_NULL;
}
