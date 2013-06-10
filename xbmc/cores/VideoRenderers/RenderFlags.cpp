/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "RenderFlags.h"
#include <string>
#include <map>


namespace RenderManager {

  unsigned int GetFlagsColorMatrix(unsigned int color_matrix, unsigned width, unsigned height)
  {
    switch(color_matrix)
    {
      case 7: // SMPTE 240M (1987)
        return CONF_FLAGS_YUVCOEF_240M;
      case 6: // SMPTE 170M
      case 5: // ITU-R BT.470-2
      case 4: // FCC
        return CONF_FLAGS_YUVCOEF_BT601;
      case 1: // ITU-R Rec.709 (1990) -- BT.709
        return CONF_FLAGS_YUVCOEF_BT709;
      case 3: // RESERVED
      case 2: // UNSPECIFIED
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
    }
    return 0;
  }

  unsigned int GetFlagsColorTransfer(unsigned int color_transfer)
  {
    switch(color_transfer)
    {
      case 1: return CONF_FLAGS_TRC_BT709;
      case 4: return CONF_FLAGS_TRC_GAMMA22;
      case 5: return CONF_FLAGS_TRC_GAMMA28;
    }
    return 0;
  }

}
