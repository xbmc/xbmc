/* TODO:
 * - add the autocrop idea from vlc
 */

#include "stdafx.h" 
#include "DVDDemuxSPU.h"
#include "..\..\util.h"
#include "..\..\utils\log.h"
#include "DVDPlayerDLL.h"
#include "DVDClock.h"

// #define SPU_DEBUG

void DebugLog(const char *format, ...)
{
#ifdef SPU_DEBUG
  static char temp_spubuffer[1024];
	va_list va;

	va_start(va, format);
	_vsnprintf(temp_spubuffer, 1024, format, va);
	va_end(va);
	
	CLog::DebugLog(temp_spubuffer);
#endif
}

CDVDDemuxSPU::CDVDDemuxSPU()
{
  for (int i = 0; i < DVD_MAX_SPUSTREAMS; i++)
  {
    m_spuSreams[i].data = NULL;
    m_spuSreams[i].iSize = 0;
    m_spuSreams[i].iAllocatedSize = 0;
    m_spuSreams[i].iNeededSize = 0;
  }
  
  memset(m_clut, 0, sizeof(m_clut));
  m_bHasClut = false;
}

CDVDDemuxSPU::~CDVDDemuxSPU()
{
  for (int i = 0; i < DVD_MAX_SPUSTREAMS; i++)
  {
    if (m_spuSreams[i].data) delete[] m_spuSreams[i].data;
  }
}

SPUInfo* CDVDDemuxSPU::AddData(BYTE* data, int iSize, int iStream, __int64 pts)
{
  // mpeg spu streams start from 0x20 and end with 0x3f
  int iIndex = iStream - 0x20;
  if (iIndex < 0 || iIndex > DVD_MAX_SPUSTREAMS || iSize < 2) return NULL;;
  
  SPUData* pSPUData = &m_spuSreams[iIndex];
  
  if (pSPUData->iNeededSize > 0 &&
     (pSPUData->iSize != pSPUData->iNeededSize) &&
     ((pSPUData->iSize + iSize) > pSPUData->iNeededSize))
  {
    DebugLog("corrupt spu data: packet does not fit");
    return NULL;
  }
  
  // check if we are about to start a new packet
  if (pSPUData->iSize == pSPUData->iNeededSize)
  {
    // for now we don't delete the memory assosiated with m_spuSreams[i].data
    pSPUData->iSize = 0;
    
    // check spu data lenght, only needed / possible in the first spu pakcet
    unsigned __int16 lenght = data[0] << 8 | data[1];
    
    if (lenght == 0)
    {
      DebugLog("corrupt spu data: zero packet");
      return NULL;;
    }
    if (lenght > iSize) pSPUData->iNeededSize = lenght;
    else pSPUData->iNeededSize = iSize;
    
    // set presentation time stamp
    if (pts > 0) pSPUData->pts = pts;
  }

  // allocate data if not already done ( done in blocks off 16384 bytes )
  // or allocate some more if 16384 bytes is not enough
  if (pSPUData->data == NULL)
  {
    pSPUData->data = new BYTE[0x4000];
    pSPUData->iAllocatedSize = 0x4000;
  }
  else if ((pSPUData->iSize + iSize) > pSPUData->iAllocatedSize)
  {
    // allocate 16384 bytes more
    pSPUData->iAllocatedSize += 0x4000;
    
    BYTE* newdata = new BYTE[pSPUData->iAllocatedSize];
    // copy over data
    fast_memcpy(newdata, pSPUData->data, pSPUData->iSize);
    delete[] pSPUData->data;
    pSPUData->data = newdata;
  }
  
  // add new data
  fast_memcpy(pSPUData->data + pSPUData->iSize, data, iSize);
  pSPUData->iSize += iSize;
  
  if (pSPUData->iNeededSize - pSPUData->iSize == 1) // to make it even
  {
    DebugLog("missing 1 byte to complete packet, adding 0xff");
    
    pSPUData->data[pSPUData->iSize] = 0xff;
    pSPUData->iSize++;
  }
  if (pSPUData->iSize == pSPUData->iNeededSize)
  {
    DebugLog("got complete spu packet\n  lenght: %i bytes\n  stream: %i\n", pSPUData->iSize, iStream);

    SPUInfo* pSPUInfo = ParsePacket(pSPUData);
    if (!pSPUInfo) return NULL;
    
    // fill missing vars in SPUInfo
    pSPUInfo->iStream = iStream;

    return pSPUInfo;
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

SPUInfo* CDVDDemuxSPU::ParsePacket(SPUData* pSPUData)
{
  unsigned int alpha[4];
  
  if (pSPUData->iNeededSize != pSPUData->iSize)
  {
    DebugLog("GetPacket, packet is incomplete, missing: %i bytes", (pSPUData->iNeededSize - pSPUData->iSize));
  }

  if (pSPUData->data[pSPUData->iSize - 1] != 0xff)
  {
    DebugLog("GetPacket, missing end of data 0xff");
  }
  
  SPUInfo* pSPUInfo = new SPUInfo;
  fast_memset(pSPUInfo, 0, sizeof(SPUInfo));
   
  BYTE* p = pSPUData->data; // pointer to walk through all data
  
  // get data length
  unsigned __int16 datalenght = p[2] << 8 | p[3]; // datalength + 4 control bytes
  
  pSPUInfo->iSPUSize = datalenght - 4;
  pSPUInfo->pData = pSPUData->data + 4;
  
  // if it is set to 0 it means it's a menu overlay by defualt
  // this is not what we want too, cause you get strange results on a parse error
  pSPUInfo->iPTSStartTime = -1;
  
  //skip data packet and goto control sequence  
  p += datalenght;
  
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
    
    while(*p != CMD_END && (unsigned int)(p - pSPUData->data) <= pSPUData->iSize)
    {
      switch(*p)
      {
        case FSTA_DSP:
          p++;
          DebugLog("    GetPacket, FSTA_DSP: Forced Start Display, no arguments");
          pSPUInfo->iPTSStartTime = 0;
          pSPUInfo->iPTSStopTime = 0x9000000000000LL;
          pSPUInfo->bForced = true;
          // delay is always 0, the dvdplayer should decide when to display the packet (menu highlight)
        break;
        case STA_DSP:
        {
          p++;
          pSPUInfo->iPTSStartTime = pSPUData->pts;
          pSPUInfo->iPTSStartTime += (((__int64)(delay * 1024 * DVD_TIME_BASE)) / 90000);
          DebugLog("    GetPacket, STA_DSP: Start Display, delay: %i", ((delay * 1024) / 90000));
        }
        break;
        case STP_DSP:
        {
          p++;
          pSPUInfo->iPTSStopTime = pSPUData->pts;
          pSPUInfo->iPTSStopTime += (((__int64)delay * 1024 * DVD_TIME_BASE) / 90000);
          DebugLog("    GetPacket, STP_DSP: Stop Display, delay: %i", ((delay * 1024) / 90000));
        }
        break;
        case SET_COLOR: // yuv, after a bit of testing it seems this info here is incomplete and can only be used for subtitles
                        // we need to get the colors from libdvdnav to support menu overlay's
        {
          p++;

          if(m_bHasClut)
          {
            unsigned int idx[4];
            // 0, 1, 2, 3
            idx[0] = (p[0] >> 4) & 0x0f;
            idx[1] = (p[0]) & 0x0f;
            idx[2] = (p[1] >> 4) & 0x0f;
            idx[3] = (p[1]) & 0x0f;

            for (int i = 0; i < 4 ; i++) // emphasis 1, emphasis 2, pattern, back ground
            {
              BYTE* iColor = m_clut[idx[i]]; // do we really to add a 1 ?
/*
              pSPUInfo->color[3 - i][0] = (iColor >> 16) & 0xff;
              pSPUInfo->color[3 - i][1] = (iColor >> 0) & 0xff;
              pSPUInfo->color[3 - i][2] = (iColor >> 8) & 0xff;
*/
              pSPUInfo->color[3 - i][0] = iColor[0]; // Y
              pSPUInfo->color[3 - i][1] = iColor[1]; // Cr
              pSPUInfo->color[3 - i][2] = iColor[2]; // Cb
            }
          }
          pSPUInfo->bHasColor = true; 
          DebugLog("    GetPacket, SET_COLOR:");
          p += 2;
        }
        break;
        case SET_CONTR: // alpha
        {
          p++;
          // 3, 2, 1, 0
          alpha[0] = (p[0] >> 4) & 0x0f;
          alpha[1] = (p[0]) & 0x0f;
          alpha[2] = (p[1] >> 4) & 0x0f;
          alpha[3] = (p[1]) & 0x0f;

          /* Ignore blank alpha palette. */
          if(alpha[0] | alpha[1] | alpha[2] | alpha[3])
          {
            // 0, 1, 2, 3
            pSPUInfo->alpha[0] = alpha[3];//0 // background, should be hidden
            pSPUInfo->alpha[1] = alpha[2];//1
            pSPUInfo->alpha[2] = alpha[1];//2 // wm button overlay
            pSPUInfo->alpha[3] = alpha[0];//3
          }
          else
          {
            DebugLog("    GetPacket, SET_CONTR: ignoring blank alpha palette, using default" );
            pSPUInfo->alpha[0] = 0x00; // back ground
            pSPUInfo->alpha[1] = 0x0f;
            pSPUInfo->alpha[2] = 0x0f;
            pSPUInfo->alpha[3] = 0x0f;
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
          pSPUInfo->width  = (((p[1] & 0x0f) << 8) | p[2]) - pSPUInfo->x + 1; 
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
          unsigned __int16 paramlenght = p[0] << 8 | p[1];
          DebugLog("GetPacket, CHG_COLCON, skippin %i bytes", paramlenght);
          p += paramlenght;
        }
        break;

        default:
        DebugLog("GetPacket, error parsing control sequence");
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
  return ParseRLE(pSPUInfo);
}

/*****************************************************************************
 * AddNibble: read a nibble from a source packet and add it to our integer.
 *****************************************************************************/
inline unsigned int AddNibble( unsigned int i_code, BYTE* p_src, unsigned int* pi_index )
{
  if( *pi_index & 0x1 )
  {
    return( i_code << 4 | ( p_src[(*pi_index)++ >> 1] & 0xf ) );
  }
  else
  {
    return( i_code << 4 | p_src[(*pi_index)++ >> 1] >> 4 );
  }
}

/*****************************************************************************
 * ParseRLE: parse the RLE part of the subtitle
 *****************************************************************************
 * This part parses the subtitle graphical data and stores it in a more
 * convenient structure for later decoding. For more information on the
 * subtitles format, see http://sam.zoy.org/doc/dvd/subtitles/index.html
 *****************************************************************************/
SPUInfo* CDVDDemuxSPU::ParseRLE(SPUInfo* pSPU)
{
    BYTE* p_src = pSPU->pData;

    unsigned int i_code = 0;

    unsigned int i_width = pSPU->width;
    unsigned int i_height = pSPU->height;
    unsigned int i_x, i_y;

    unsigned __int16* p_dest = (unsigned __int16*)pSPU->result;

    /* The subtitles are interlaced, we need two offsets */
    unsigned int  i_id = 0;                   /* Start on the even SPU layer */
    unsigned int  pi_table[ 2 ];
    unsigned int *pi_offset;

    /* Colormap statistics */
    int i_border = 0;//-1;
    int stats[4]; stats[0] = stats[1] = stats[2] = stats[3] = 0;

    pi_table[ 0 ] = pSPU->pTFData << 1;
    pi_table[ 1 ] = pSPU->pBFData << 1;

    for( i_y = 0 ; i_y < i_height ; i_y++ )
    {
        pi_offset = pi_table + i_id;

        for( i_x = 0 ; i_x < i_width ; i_x += i_code >> 2 )
        {
            i_code = AddNibble( 0, p_src, pi_offset );

            if( i_code < 0x04 )
            {
                i_code = AddNibble( i_code, p_src, pi_offset );

                if( i_code < 0x10 )
                {
                    i_code = AddNibble( i_code, p_src, pi_offset );

                    if( i_code < 0x040 )
                    {
                        i_code = AddNibble( i_code, p_src, pi_offset );

                        if( i_code < 0x0100 )
                        {
                            /* If the 14 first bits are set to 0, then it's a
                             * new line. We emulate it. */
                            if( i_code < 0x0004 )
                            {
                                i_code |= ( i_width - i_x ) << 2;
                            }
                            else
                            {
                                /* We have a boo boo ! */
                                DebugLog("ParseRLE: unknown RLE code 0x%.4x", i_code);
                                return false;
                            }
                        }
                    }
                }
            }

            if( ( (i_code >> 2) + i_x + i_y * i_width ) > i_height * i_width )
            {
                DebugLog("ParseRLE: out of bounds, %i at (%i,%i) is out of %ix%i",
                         i_code >> 2, i_x, i_y, i_width, i_height );
                return false;
            }

            /* Try to find the border color */
            if(pSPU->alpha[i_code & 0x3] != 0x00)
            {
                i_border = i_code & 0x3;
                stats[i_border] += i_code >> 2;
            }

            *p_dest++ = i_code;
        }

        /* Check that we didn't go too far */
        if( i_x > i_width )
        {
            DebugLog("ParseRLE: i_x overflowed, %i > %i", i_x, i_width );
            return false;
        }

        /* Byte-align the stream */
        if( *pi_offset & 0x1 )
        {
            (*pi_offset)++;
        }

        /* Swap fields */
        i_id = ~i_id & 0x1;
    }

    /* We shouldn't get any padding bytes */
    if( i_y < i_height )
    {
        DebugLog("ParseRLE: padding bytes found in RLE sequence" );
        DebugLog("ParseRLE: send mail to <sam@zoy.org> if you want to help debugging this" );

        /* Skip them just in case */
        while( i_y < i_height )
        {
            *p_dest++ = i_width << 2;
            i_y++;
        }

        return false;
    }

    DebugLog("ParseRLE: valid subtitle, size: %ix%i, position: %i,%i",
             pSPU->width, pSPU->height, pSPU->x, pSPU->y );

    // Handle color if no palette was found.  for xbmc we do this always for subtitles
    if((!pSPU->bHasColor || pSPU->iPTSStartTime > 0.0) && i_border >= 0 && i_border < 4)
    {
        int i, i_inner = -1, i_shade = -1;

        /* Set the border color */
        pSPU->color[i_border][0] = 0x00;
        pSPU->color[i_border][1] = 0x80;
        pSPU->color[i_border][2] = 0x80;
        stats[i_border] = 0;

        /* Find the inner colors */
        for( i = 0 ; i < 4 && i_inner == -1 ; i++ )
        {
          if( stats[i] )
          {
            i_inner = i;
          }
        }

        for( ; i < 4 && i_shade == -1 ; i++)
        {
            if( stats[i] )
            {
                if( stats[i] > stats[i_inner] )
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
        if( i_inner != -1 )
        {
          // white color
          pSPU->color[i_inner][0] = 0xff; // Y
          pSPU->color[i_inner][1] = 0x80; // Cr ?
          pSPU->color[i_inner][2] = 0x80; // Cb ?
        }

        /* Set the anti-aliasing color */
        if( i_shade != -1 )
        {
          // gray
          pSPU->color[i_shade][0] = 0x80;
          pSPU->color[i_shade][1] = 0x80;
          pSPU->color[i_shade][2] = 0x80;
        }

        DebugLog("ParseRLE: using custom palette (border %i, inner %i, shade %i)",
                 i_border, i_inner, i_shade );
    }

    return pSPU;
}
