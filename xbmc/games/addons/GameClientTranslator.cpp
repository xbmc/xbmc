/*
 *      Copyright (C) 2016 Team Kodi
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

#include "GameClientTranslator.h"

using namespace GAME;

const char* CGameClientTranslator::ToString(GAME_ERROR error)
{
  switch (error)
  {
  case GAME_ERROR_NO_ERROR:           return "no error";
  case GAME_ERROR_NOT_IMPLEMENTED:    return "not implemented";
  case GAME_ERROR_REJECTED:           return "rejected by the client";
  case GAME_ERROR_INVALID_PARAMETERS: return "invalid parameters for this method";
  case GAME_ERROR_FAILED:             return "the command failed";
  case GAME_ERROR_NOT_LOADED:         return "no game is loaded";
  case GAME_ERROR_RESTRICTED:         return "the required resources are restricted";
  default:
    break;
  }
  return "unknown error";
}

const char* CGameClientTranslator::ToString(GAME_MEMORY memory)
{
  switch (memory)
  {
  case GAME_MEMORY_SAVE_RAM:                return "save ram";
  case GAME_MEMORY_RTC:                     return "rtc";
  case GAME_MEMORY_SYSTEM_RAM:              return "system ram";
  case GAME_MEMORY_VIDEO_RAM:               return "video ram";
  case GAME_MEMORY_SNES_BSX_RAM:            return "snes bsx ram";
  case GAME_MEMORY_SNES_SUFAMI_TURBO_A_RAM: return "snes sufami turbo a ram";
  case GAME_MEMORY_SNES_SUFAMI_TURBO_B_RAM: return "snes sufami turbo b ram";
  case GAME_MEMORY_SNES_GAME_BOY_RAM:       return "snes game boy ram";
  case GAME_MEMORY_SNES_GAME_BOY_RTC:       return "snes game boy rtc";
  default:
    break;
  }
  return "unknown memory";
}

AVPixelFormat CGameClientTranslator::TranslatePixelFormat(GAME_PIXEL_FORMAT format)
{
  switch (format)
  {
  case GAME_PIXEL_FORMAT_YUV420P:  return AV_PIX_FMT_YUV420P;
  case GAME_PIXEL_FORMAT_0RGB8888: return AV_PIX_FMT_0RGB32;
  case GAME_PIXEL_FORMAT_RGB565:   return AV_PIX_FMT_RGB565;
  case GAME_PIXEL_FORMAT_0RGB1555: return AV_PIX_FMT_RGB555;
  default:
    break;
  }
  return AV_PIX_FMT_NONE;
}

AVCodecID CGameClientTranslator::TranslateVideoCodec(GAME_VIDEO_CODEC codec)
{
  switch (codec)
  {
  case GAME_VIDEO_CODEC_H264: return AV_CODEC_ID_H264;
  default:
    break;
  }
  return AV_CODEC_ID_NONE;
}

AEDataFormat CGameClientTranslator::TranslatePCMFormat(GAME_PCM_FORMAT format)
{
  switch (format)
  {
  case GAME_PCM_FORMAT_S16NE: return AE_FMT_S16NE;
  default:
    break;
  }
  return AE_FMT_INVALID;
}

AEChannel CGameClientTranslator::TranslateAudioChannel(GAME_AUDIO_CHANNEL channel)
{
  switch (channel)
  {
  case GAME_CH_FL:   return AE_CH_FL;
  case GAME_CH_FR:   return AE_CH_FR;
  case GAME_CH_FC:   return AE_CH_FC;
  case GAME_CH_LFE:  return AE_CH_LFE;
  case GAME_CH_BL:   return AE_CH_BL;
  case GAME_CH_BR:   return AE_CH_BR;
  case GAME_CH_FLOC: return AE_CH_FLOC;
  case GAME_CH_FROC: return AE_CH_FROC;
  case GAME_CH_BC:   return AE_CH_BC;
  case GAME_CH_SL:   return AE_CH_SL;
  case GAME_CH_SR:   return AE_CH_SR;
  case GAME_CH_TFL:  return AE_CH_TFL;
  case GAME_CH_TFR:  return AE_CH_TFR;
  case GAME_CH_TFC:  return AE_CH_TFC;
  case GAME_CH_TC:   return AE_CH_TC;
  case GAME_CH_TBL:  return AE_CH_TBL;
  case GAME_CH_TBR:  return AE_CH_TBR;
  case GAME_CH_TBC:  return AE_CH_TBC;
  case GAME_CH_BLOC: return AE_CH_BLOC;
  case GAME_CH_BROC: return AE_CH_BROC;
  default:
    break;
  }
  return AE_CH_NULL;
}

AVCodecID CGameClientTranslator::TranslateAudioCodec(GAME_AUDIO_CODEC codec)
{
  switch (codec)
  {
  case GAME_AUDIO_CODEC_OPUS: return AV_CODEC_ID_OPUS;
  default:
    break;
  }
  return AV_CODEC_ID_NONE;
}

GAME_KEY_MOD CGameClientTranslator::GetModifiers(CKey::Modifier modifier)
{
  unsigned int mods = GAME_KEY_MOD_NONE;

  if (modifier & CKey::MODIFIER_CTRL)  mods |= GAME_KEY_MOD_CTRL;
  if (modifier & CKey::MODIFIER_SHIFT) mods |= GAME_KEY_MOD_SHIFT;
  if (modifier & CKey::MODIFIER_ALT)   mods |= GAME_KEY_MOD_ALT;
  if (modifier & CKey::MODIFIER_RALT)  mods |= GAME_KEY_MOD_RALT;
  if (modifier & CKey::MODIFIER_META)  mods |= GAME_KEY_MOD_META;

  return static_cast<GAME_KEY_MOD>(mods);
}

const char* CGameClientTranslator::TranslateRegion(GAME_REGION region)
{
  switch (region)
  {
  case GAME_REGION_NTSC: return "NTSC";
  case GAME_REGION_PAL:  return "PAL";
  default:
    break;
  }
  return "Unknown";
}
