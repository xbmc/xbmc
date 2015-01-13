/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DVDDemuxSPU.h"
#include "DVDClock.h"
#include "utils/log.h"

#undef ALIGN
#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))

// #define SPU_DEBUG

void DebugLog(const char *format, ...)
{
#ifdef SPU_DEBUG
  static char temp_spubuffer[1024];
  va_list va;

  va_start(va, format);
  _vsnprintf(temp_spubuffer, 1024, format, va);
  va_end(va);

  CLog::Log(LOGDEBUG,temp_spubuffer);
#endif
}

CDVDDemuxSPU::CDVDDemuxSPU()
{
  memset(&m_spuData, 0, sizeof(m_spuData));
  memset(m_clut, 0, sizeof(m_clut));
  m_bHasClut = false;
}

CDVDDemuxSPU::~CDVDDemuxSPU()
{
  if (m_spuData.data) free(m_spuData.data);
}

void CDVDDemuxSPU::Reset()
{
  FlushCurrentPacket();

  // We can't reset this during playback, cause we don't always
  // get a new clut from libdvdnav leading to invalid colors
  // so let's just never reset it. It will only be reset
  // when dvdplayer is destructed and constructed
  // m_bHasClut = false;
  // memset(m_clut, 0, sizeof(m_clut));
}

void CDVDDemuxSPU::FlushCurrentPacket()
{
  if (m_spuData.data) free(m_spuData.data);
  memset(&m_spuData, 0, sizeof(m_spuData));
}

CDVDOverlaySpu* CDVDDemuxSPU::AddData(uint8_t* data, int iSize, double pts)
{
  SPUData* pSPUData = &m_spuData;

  if (pSPUData->iNeededSize > 0 &&
      (pSPUData->iSize != pSPUData->iNeededSize) &&
      ((pSPUData->iSize + iSize) > pSPUData->iNeededSize))
  {
    DebugLog("corrupt spu data: packet does not fit");
    m_spuData.iNeededSize = 0;
    m_spuData.iSize = 0;
    return NULL;
  }

  // check if we are about to start a new packet
  if (pSPUData->iSize == pSPUData->iNeededSize)
  {
    // for now we don't delete the memory assosiated with m_spuData.data
    pSPUData->iSize = 0;

    // check spu data lenght, only needed / possible in the first spu pakcet
    unsigned __int16 length = data[0] << 8 | data[1];
    if (length == 0)
    {
      DebugLog("corrupt spu data: zero packet");
      m_spuData.iNeededSize = 0;
      m_spuData.iSize = 0;
      return NULL;
    }
    if (length > iSize) pSPUData->iNeededSize = length;
    else pSPUData->iNeededSize = iSize;

    // set presentation time stamp
    if (pts > 0) pSPUData->pts = pts;
  }

  // allocate data if not already done ( done in blocks off 16384 bytes )
  // or allocate some more if 16384 bytes is not enough
  if ((pSPUData->iSize + iSize) > pSPUData->iAllocatedSize)
  {
    uint8_t* tmpptr = (uint8_t*)realloc(pSPUData->data, ALIGN(pSPUData->iSize + iSize, 0x4000));
    if (!tmpptr)
    {
      free(pSPUData->data);
      return NULL;
    }
    pSPUData->data = tmpptr;
  }

  if(!pSPUData->data)
    return NULL; // crap realloc failed, this will have leaked some memory due to odd realloc

  // add new data
  memcpy(pSPUData->data + pSPUData->iSize, data, iSize);
  pSPUData->iSize += iSize;

  if (pSPUData->iNeededSize - pSPUData->iSize == 1) // to make it even
  {
    DebugLog("missing 1 byte to complete packet, adding 0xff");

    pSPUData->data[pSPUData->iSize] = 0xff;
    pSPUData->iSize++;
  }

  if (pSPUData->iSize == pSPUData->iNeededSize)
  {
    DebugLog("got complete spu packet\n  length: %i bytes\n  stream: %i\n", pSPUData->iSize);

    return ParsePacket(pSPUData);
  }

  return NULL;
}

#define CMD_END     0xFF
#define FSTA_DSP    0x00
#define STA_DSP     0x01
#define STP_DSP     0x02
#define SET_COLOR   0x03
#define SET_CONTR   0x04
#define SET_DAREA   0x05
#define SET_DSPXA   0x06
#define CHG_COLCON  0x07

CDVDOverlaySpu* CDVDDemuxSPU::ParsePacket(SPUData* pSPUData)
{
  unsigned int alpha[4];
  uint8_t* pUnparsedData = NULL;

  if (pSPUData->iNeededSize != pSPUData->iSize)
  {
    DebugLog("GetPacket, packet is incomplete, missing: %i bytes", (pSPUData->iNeededSize - pSPUData->iSize));
  }

  if (pSPUData->data[pSPUData->iSize - 1] != 0xff)
  {
    DebugLog("GetPacket, missing end of data 0xff");
  }

  CDVDOverlaySpu* pSPUInfo = new CDVDOverlaySpu();
  uint8_t* p = pSPUData->data; // pointer to walk through all data

  // get data length
  unsigned __int16 datalength = p[2] << 8 | p[3]; // datalength + 4 control bytes

  pUnparsedData = pSPUData->data + 4;

  // if it is set to 0 it means it's a menu overlay by defualt
  // this is not what we want too, cause you get strange results on a parse error
  pSPUInfo->iPTSStartTime = -1;

  //skip data packet and goto control sequence
  p += datalength;

  bool bHasNewDCSQ = true;
  while (bHasNewDCSQ)
  {
    DebugLog("  starting new SP_DCSQT");
    // p is beginning of first SP_DCSQT now
    unsigned __int16 delay = p[0] << 8 | p[1];
    unsigned __int16 next_DCSQ = p[2] << 8 | p[3];

    //offset within the Sub-Picture Unit to the next SP_DCSQ. If this is the last SP_DCSQ, it points to itself.
    bHasNewDCSQ = ((pSPUData->data + next_DCSQ) != p);
    // skip 4 bytes
    p += 4;

    while (*p != CMD_END && (unsigned int)(p - pSPUData->data) <= pSPUData->iSize)
    {
      switch (*p)
      {
      case FSTA_DSP:
        p++;
        DebugLog("    GetPacket, FSTA_DSP: Forced Start Display, no arguments");
        pSPUInfo->iPTSStartTime = pSPUData->pts;
        pSPUInfo->iPTSStopTime = 0x9000000000000LL;
        pSPUInfo->bForced = true;
        // delay is always 0, the dvdplayer should decide when to display the packet (menu highlight)
        break;
      case STA_DSP:
        {
          p++;
          pSPUInfo->iPTSStartTime = pSPUData->pts;
          pSPUInfo->iPTSStartTime += (double)delay * 1024 * DVD_TIME_BASE / 90000;
          DebugLog("    GetPacket, STA_DSP: Start Display, delay: %i", ((delay * 1024) / 90000));
        }
        break;
      case STP_DSP:
        {
          p++;
          pSPUInfo->iPTSStopTime = pSPUData->pts;
          pSPUInfo->iPTSStopTime += (double)delay * 1024 * DVD_TIME_BASE / 90000;
          DebugLog("    GetPacket, STP_DSP: Stop Display, delay: %i", ((delay * 1024) / 90000));
        }
        break;
      case SET_COLOR:
        {
          p++;

          if (m_bHasClut)
          {
            pSPUInfo->bHasColor = true;

            unsigned int idx[4];
            // 0, 1, 2, 3
            idx[0] = (p[0] >> 4) & 0x0f;
            idx[1] = (p[0]) & 0x0f;
            idx[2] = (p[1] >> 4) & 0x0f;
            idx[3] = (p[1]) & 0x0f;

            for (int i = 0; i < 4 ; i++) // emphasis 1, emphasis 2, pattern, back ground
            {
              uint8_t* iColor = m_clut[idx[i]];

              pSPUInfo->color[3 - i][0] = iColor[0]; // Y
              pSPUInfo->color[3 - i][1] = iColor[1]; // Cr
              pSPUInfo->color[3 - i][2] = iColor[2]; // Cb
            }
          }

          DebugLog("    GetPacket, SET_COLOR:");
          p += 2;
        }
        break;
      case SET_CONTR:  // alpha
        {
          p++;
          // 3, 2, 1, 0
          alpha[0] = (p[0] >> 4) & 0x0f;
          alpha[1] = (p[0]) & 0x0f;
          alpha[2] = (p[1] >> 4) & 0x0f;
          alpha[3] = (p[1]) & 0x0f;

          // Ignore blank alpha palette.
          if (alpha[0] | alpha[1] | alpha[2] | alpha[3])
          {
            pSPUInfo->bHasAlpha = true;

            // 0, 1, 2, 3
            pSPUInfo->alpha[0] = alpha[3]; //0 // background, should be hidden
            pSPUInfo->alpha[1] = alpha[2]; //1
            pSPUInfo->alpha[2] = alpha[1]; //2 // wm button overlay
            pSPUInfo->alpha[3] = alpha[0]; //3
          }

          DebugLog("    GetPacket, SET_CONTR:");
          p += 2;
        }
        break;
      case SET_DAREA:
        {
          p++;
          pSPUInfo->x = (p[0] << 4) | (p[1] >> 4);
          pSPUInfo->y = (p[3] << 4) | (p[4] >> 4);
          pSPUInfo->width = (((p[1] & 0x0f) << 8) | p[2]) - pSPUInfo->x + 1;
          pSPUInfo->height = (((p[4] & 0x0f) << 8) | p[5]) - pSPUInfo->y + 1;
          DebugLog("    GetPacket, SET_DAREA: x,y:%i,%i width,height:%i,%i",
                   pSPUInfo->x, pSPUInfo->y, pSPUInfo->width, pSPUInfo->height);
          p += 6;
        }
        break;
      case SET_DSPXA:
        {
          p++;
          unsigned __int16 tfaddr = (p[0] << 8 | p[1]); // offset in packet
          unsigned __int16 bfaddr = (p[2] << 8 | p[3]); // offset in packet
          pSPUInfo->pTFData = (tfaddr - 4); //pSPUInfo->pData + (tfaddr - 4); // pSPUData->data = packet startaddr - 4
          pSPUInfo->pBFData = (bfaddr - 4); //pSPUInfo->pData + (bfaddr - 4); // pSPUData->data = packet startaddr - 4
          p += 4;
          DebugLog("    GetPacket, SET_DSPXA: tf: %i bf: %i ", tfaddr, bfaddr);
        }
        break;
      case CHG_COLCON:
        {
          p++;
          unsigned __int16 paramlength = p[0] << 8 | p[1];
          DebugLog("GetPacket, CHG_COLCON, skippin %i bytes", paramlength);
          p += paramlength;
        }
        break;

      default:
        DebugLog("GetPacket, error parsing control sequence");
        delete pSPUInfo;
        return NULL;
        break;
      }
    }
    DebugLog("  end off SP_DCSQT");
    if (*p == CMD_END) p++;
    else
    {
      DebugLog("GetPacket, end off SP_DCSQT, but did not found 0xff (CMD_END)");
    }
  }

  // parse the rle.
  // this should be chnaged so it get's converted to a yuv overlay
  return ParseRLE(pSPUInfo, pUnparsedData);
}

/*****************************************************************************
 * AddNibble: read a nibble from a source packet and add it to our integer.
 *****************************************************************************/
inline unsigned int AddNibble( unsigned int i_code, uint8_t* p_src, unsigned int* pi_index )
{
  if ( *pi_index & 0x1 )
  {
    return ( i_code << 4 | ( p_src[(*pi_index)++ >> 1] & 0xf ) );
  }
  else
  {
    return ( i_code << 4 | p_src[(*pi_index)++ >> 1] >> 4 );
  }
}

/*****************************************************************************
 * ParseRLE: parse the RLE part of the subtitle
 *****************************************************************************
 * This part parses the subtitle graphical data and stores it in a more
 * convenient structure for later decoding. For more information on the
 * subtitles format, see http://sam.zoy.org/doc/dvd/subtitles/index.html
 *****************************************************************************/
CDVDOverlaySpu* CDVDDemuxSPU::ParseRLE(CDVDOverlaySpu* pSPU, uint8_t* pUnparsedData)
{
  uint8_t* p_src = pUnparsedData;

  unsigned int i_code = 0;

  unsigned int i_width = pSPU->width;
  unsigned int i_height = pSPU->height;
  unsigned int i_x, i_y;

  // allocate a buffer for the result
  unsigned __int16* p_dest = (unsigned __int16*)pSPU->result;

  /* The subtitles are interlaced, we need two offsets */
  unsigned int i_id = 0;                   /* Start on the even SPU layer */
  unsigned int pi_table[2];
  
  /* Colormap statistics */
  int i_border = -1;
  int stats[4]; stats[0] = stats[1] = stats[2] = stats[3] = 0;

  pi_table[ 0 ] = pSPU->pTFData << 1;
  pi_table[ 1 ] = pSPU->pBFData << 1;

  for ( i_y = 0 ; i_y < i_height ; i_y++ )
  {
    unsigned int *pi_offset = pi_table + i_id;

    for ( i_x = 0 ; i_x < i_width ; i_x += i_code >> 2 )
    {
      i_code = AddNibble( 0, p_src, pi_offset );

      if ( i_code < 0x04 )
      {
        i_code = AddNibble( i_code, p_src, pi_offset );

        if ( i_code < 0x10 )
        {
          i_code = AddNibble( i_code, p_src, pi_offset );

          if ( i_code < 0x040 )
          {
            i_code = AddNibble( i_code, p_src, pi_offset );

            if ( i_code < 0x0100 )
            {
              /* If the 14 first bits are set to 0, then it's a
               * new line. We emulate it. */
              if ( i_code < 0x0004 )
              {
                i_code |= ( i_width - i_x ) << 2;
              }
              else
              {
                /* We have a boo boo ! */
                CLog::Log(LOGERROR, "ParseRLE: unknown RLE code 0x%.4x", i_code);
                pSPU->Release();
                return NULL;
              }
            }
          }
        }
      }

      if ( ( (i_code >> 2) + i_x + i_y * i_width ) > i_height * i_width )
      {
        CLog::Log(LOGERROR, "ParseRLE: out of bounds, %i at (%i,%i) is out of %ix%i",
                 i_code >> 2, i_x, i_y, i_width, i_height );
        pSPU->Release();
        return NULL;
      }

      // keep trace of all occouring pixels, even keeping the background in mind
      stats[i_code & 0x3] += i_code >> 2;

      // count the number of pixels for every occouring parts, without background
      if (pSPU->alpha[i_code & 0x3] != 0x00)
      {
        // the last non background pixel is probably the border color
        i_border = i_code & 0x3;
        stats[i_border] += i_code >> 2;
      }

      /* Check we aren't overwriting our data range
         This occurs on "The Triplets of BelleVille" region 4 disk (NTSC)"
         where we use around 96k rather than 64k + 20bytes */
      if ((uint8_t *)p_dest >= pSPU->result + sizeof(pSPU->result))
      {
        CLog::Log(LOGERROR, "ParseRLE: Overrunning our data range.  Need %li bytes", (long)((uint8_t *)p_dest - pSPU->result));
        pSPU->Release();
        return NULL;
      }
      *p_dest++ = i_code;
    }

    /* Check that we didn't go too far */
    if ( i_x > i_width )
    {
      CLog::Log(LOGERROR, "ParseRLE: i_x overflowed, %i > %i", i_x, i_width );
      pSPU->Release();
      return NULL;
    }

    /* Byte-align the stream */
    if ( *pi_offset & 0x1 )
    {
      (*pi_offset)++;
    }

    /* Swap fields */
    i_id = ~i_id & 0x1;
  }

  /* We shouldn't get any padding bytes */
  if ( i_y < i_height )
  {
    DebugLog("ParseRLE: padding bytes found in RLE sequence" );
    DebugLog("ParseRLE: send mail to <sam@zoy.org> if you want to help debugging this" );

    /* Skip them just in case */
    while ( i_y < i_height )
    {
      /* Check we aren't overwriting our data range
         This occurs on "The Triplets of BelleVille" region 4 disk (NTSC)"
         where we use around 96k rather than 64k + 20bytes */
      if ((uint8_t *)p_dest >= pSPU->result + sizeof(pSPU->result))
      {
        CLog::Log(LOGERROR, "ParseRLE: Overrunning our data range.  Need %li bytes", (long)((uint8_t *)p_dest - pSPU->result));
        pSPU->Release();
        return NULL;
      }
      *p_dest++ = i_width << 2;
      i_y++;
    }

    pSPU->Release();
    return NULL;
  }

  DebugLog("ParseRLE: valid subtitle, size: %ix%i, position: %i,%i",
           pSPU->width, pSPU->height, pSPU->x, pSPU->y );

  // forced spu's (menu overlays) retrieve their alpha/color information from InputStreamNavigator::GetCurrentButtonInfo
  // also they may contain completely covering data wich is supposed to be hidden normally
  // since whole spu is drawn, if this is done for forced, that may be displayed
  // so we must trust what is given
  if( !pSPU->bForced )
  {
    // Handle color if no palette was found.
    // we only set it if there is a valid i_border color
    if (!pSPU->bHasColor)
    {
      CLog::Log(LOGINFO, "%s - no color palette found, using default", __FUNCTION__);
      FindSubtitleColor(i_border, stats, pSPU);
    }

    // check alpha values, for non forced spu's we use a default value
    if (pSPU->bHasAlpha)
    {
      // check alpha values
      // the array stats represents the nr of pixels for each color channel
      // thus if there are no pixels to display, we assume the alphas are incorrect.
      if (!CanDisplayWithAlphas(pSPU->alpha, stats))
      {
        CLog::Log(LOGINFO, "%s - no  matching color and alpha found, resetting alpha", __FUNCTION__);

        pSPU->alpha[0] = 0x00; // back ground
        pSPU->alpha[1] = 0x0f;
        pSPU->alpha[2] = 0x0f;
        pSPU->alpha[3] = 0x0f;
      }
    }
    else
    {
      CLog::Log(LOGINFO, "%s - ignoring blank alpha palette, using default", __FUNCTION__);

      pSPU->alpha[0] = 0x00; // back ground
      pSPU->alpha[1] = 0x0f;
      pSPU->alpha[2] = 0x0f;
      pSPU->alpha[3] = 0x0f;
    }

  }

  return pSPU;
}

void CDVDDemuxSPU::FindSubtitleColor(int last_color, int stats[4], CDVDOverlaySpu* pSPU)
{
  const int COLOR_INNER = 0;
  const int COLOR_SHADE = 1;
  const int COLOR_BORDER = 2;

  //uint8_t custom_subtitle_color[4][3] = { // blue, yellow and something else (xine)
  //  { 0x80, 0x90, 0x80 }, // inner color
  //  { 0x00, 0x90, 0x00 }, // shade color
  //  { 0x00, 0x90, 0xff }  // border color
  //};

  uint8_t custom_subtitle_color[4][3] = { // inner color white, gray shading and a black border
    { 0xff, 0x80, 0x80 }, // inner color, white
    { 0x80, 0x80, 0x80 }, // shade color, gray
    { 0x00, 0x80, 0x80 }  // border color, black
  };

  //uint8_t custom_subtitle_color[4][3] = { // completely white and a black border
  //  { 0xff, 0x80, 0x80 }, // inner color, white
  //  { 0xff, 0x80, 0x80 }, // shade color, white
  //  { 0x00, 0x80, 0x80 }  // border color, black
  //};


  int nrOfUsedColors = 0;
  for (int i = 0; i < 4; i++)
  {
    if (pSPU->alpha[i] > 0) nrOfUsedColors++;
  }

  if (nrOfUsedColors == 0)
  {
    // nothing todo
    DebugLog("FindSubtitleColor: all 4 alpha channels are 0, nothing todo");
  }
  else if (nrOfUsedColors == 1)
  {
    // only one color is used, probably the inner color
    for (int i = 0; i < 4; i++) // find the position that is used
    {
      if (pSPU->alpha[i] > 0)
      {
        pSPU->color[i][0] = custom_subtitle_color[COLOR_INNER][0]; // Y
        pSPU->color[i][1] = custom_subtitle_color[COLOR_INNER][1]; // Cr ?
        pSPU->color[i][2] = custom_subtitle_color[COLOR_INNER][2]; // Cb ?
        return;
      }
    }

  }
  else
  {
    // old code

    if (last_color >= 0 && last_color < 4)
    {
      int i, i_inner = -1, i_shade = -1;
      // Set the border color, the last color is probably the border color
      pSPU->color[last_color][0] = custom_subtitle_color[COLOR_BORDER][0];
      pSPU->color[last_color][1] = custom_subtitle_color[COLOR_BORDER][1];
      pSPU->color[last_color][2] = custom_subtitle_color[COLOR_BORDER][2];
      stats[last_color] = 0;

    // find the inner colors
    for ( i = 0 ; i < 4 && i_inner == -1 ; i++ )
    {
      if ( stats[i] )
      {
        i_inner = i;
      }
    }

    // try to find the shade color
    for ( ; i < 4 && i_shade == -1 ; i++)
    {
      if ( stats[i] )
      {
        if ( stats[i] > stats[i_inner] )
        {
          i_shade = i_inner;
          i_inner = i;
        }
        else
        {
          i_shade = i;
        }
      }
    }

    /* Set the inner color */
    if ( i_inner != -1 )
    {
      // white color
        pSPU->color[i_inner][0] = custom_subtitle_color[COLOR_INNER][0]; // Y
        pSPU->color[i_inner][1] = custom_subtitle_color[COLOR_INNER][1]; // Cr ?
        pSPU->color[i_inner][2] = custom_subtitle_color[COLOR_INNER][2]; // Cb ?
    }

    /* Set the anti-aliasing color */
    if ( i_shade != -1 )
    {
      // gray
        pSPU->color[i_shade][0] = custom_subtitle_color[COLOR_SHADE][0];
        pSPU->color[i_shade][1] = custom_subtitle_color[COLOR_SHADE][1];
        pSPU->color[i_shade][2] = custom_subtitle_color[COLOR_SHADE][2];
    }

      DebugLog("ParseRLE: using custom palette (border %i, inner %i, shade %i)", last_color, i_inner, i_shade);
    }
  }
}

bool CDVDDemuxSPU::CanDisplayWithAlphas(int a[4], int stats[4])
{
  return(
    a[0] * stats[0] > 0 ||
    a[1] * stats[1] > 0 ||
    a[2] * stats[2] > 0 ||
    a[3] * stats[3] > 0);
}
