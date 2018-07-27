/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderFlags.h"
#include <string>
#include <map>

unsigned int GetFlagsColorMatrix(unsigned int color_matrix, unsigned width, unsigned height)
{
  switch(color_matrix)
  {
    case 10: // BT2020_CL
    case  9: // BT2020_NCL
      return CONF_FLAGS_YUVCOEF_BT2020;
    case  7: // SMPTE 240M (1987)
      return CONF_FLAGS_YUVCOEF_240M;
    case  6: // SMPTE 170M
    case  5: // ITU-R BT.470-2
    case  4: // FCC
      return CONF_FLAGS_YUVCOEF_BT601;
    case  1: // ITU-R Rec.709 (1990) -- BT.709
      return CONF_FLAGS_YUVCOEF_BT709;
    case  3: // RESERVED
    case  2: // UNSPECIFIED
    default:
      if(width > 1024 || height >= 600)
        return CONF_FLAGS_YUVCOEF_BT709;
      else
        return CONF_FLAGS_YUVCOEF_BT601;
      break;
  }
}

unsigned int GetFlagsChromaPosition(unsigned int chroma_position)
{
  switch(chroma_position)
  {
    case 1: return CONF_FLAGS_CHROMA_LEFT;
    case 2: return CONF_FLAGS_CHROMA_CENTER;
    case 3: return CONF_FLAGS_CHROMA_TOPLEFT;
  }
  return 0;
}

unsigned int GetFlagsColorPrimaries(unsigned int color_primaries)
{
  switch(color_primaries)
  {
    case 1: return CONF_FLAGS_COLPRI_BT709;
    case 4: return CONF_FLAGS_COLPRI_BT470M;
    case 5: return CONF_FLAGS_COLPRI_BT470BG;
    case 6: return CONF_FLAGS_COLPRI_170M;
    case 7: return CONF_FLAGS_COLPRI_240M;
    case 9: return CONF_FLAGS_COLPRI_BT2020;
  }
  return 0;
}

unsigned int GetFlagsStereoMode(const std::string& mode)
{
  static std::map<std::string, unsigned int> convert;
  if(convert.empty())
  {
    convert["mono"]                   = 0u;
    convert["left_right"]             = CONF_FLAGS_STEREO_MODE_SBS | CONF_FLAGS_STEREO_CADANCE_LEFT_RIGHT;
    convert["bottom_top"]             = CONF_FLAGS_STEREO_MODE_TAB | CONF_FLAGS_STEREO_CADANCE_RIGHT_LEFT;
    convert["top_bottom"]             = CONF_FLAGS_STEREO_MODE_TAB | CONF_FLAGS_STEREO_CADANCE_LEFT_RIGHT;
    convert["checkerboard_rl"]        = 0u;
    convert["checkerboard_lr"]        = 0u;
    convert["row_interleaved_rl"]     = 0u;
    convert["row_interleaved_lr"]     = 0u;
    convert["col_interleaved_rl"]     = 0u;
    convert["col_interleaved_lr"]     = 0u;
    convert["anaglyph_cyan_red"]      = 0u;
    convert["right_left"]             = CONF_FLAGS_STEREO_MODE_SBS | CONF_FLAGS_STEREO_CADANCE_RIGHT_LEFT;
    convert["anaglyph_green_magenta"] = 0u;
    convert["anaglyph_yellow_blue"]   = 0u;
    convert["block_lr"]               = 0u;
    convert["block_rl"]               = 0u;
  }
  return convert[mode];
}

