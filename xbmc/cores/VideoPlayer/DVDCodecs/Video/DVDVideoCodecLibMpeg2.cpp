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

#include "DVDVideoCodecLibMpeg2.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "utils/log.h"

enum MPEGProfile
{
  MPEG_422_HL = 0x82,
  MPEG_422_ML = 0x52
};

union TagUnion
{
  double pts;
  struct
  {
    uint32_t u;
    uint32_t l;
  } tag;
};

//Decoder specific flags used internal to decoder
#define DVP_FLAG_LIBMPEG2_MASK      0x0000f00
#define DVP_FLAG_LIBMPEG2_ALLOCATED 0x0000100 //Set to indicate that this has allocated data
#define DVP_FLAG_LIBMPEG2_INUSE     0x0000200 //Set to show that libmpeg2 might need to read data from here again

#undef ALIGN
#define ALIGN(value, alignment) (((value)+(alignment-1))&~(alignment-1))

CDVDVideoCodecLibMpeg2::CDVDVideoCodecLibMpeg2()
{
  m_pHandle = NULL;
  m_pInfo = NULL;
  memset(m_pVideoBuffer, 0, sizeof(DVDVideoPicture) * 3);
  m_pCurrentBuffer = NULL;
  m_irffpattern = 0;
  m_bFilm = false;
  m_bIs422 = false;
  m_hurry = 0;
  m_dts = DVD_NOPTS_VALUE;
  m_dts2 = DVD_NOPTS_VALUE;
}

CDVDVideoCodecLibMpeg2::~CDVDVideoCodecLibMpeg2()
{
  Dispose();
}

//This call could possibly be moved outside of the codec, could allow us to do some type of
//direct rendering. problem is that that the buffer requested isn't allways the next one
//that should go to display.
DVDVideoPicture* CDVDVideoCodecLibMpeg2::GetBuffer(unsigned int width, unsigned int height)
{
  for(int i = 0; i < 3; i++)
  {
    if( !(m_pVideoBuffer[i].iFlags & DVP_FLAG_LIBMPEG2_INUSE) )
    {
      if( m_pVideoBuffer[i].iWidth != width || m_pVideoBuffer[i].iHeight != height ) //Dimensions changed.. realloc needed
      {
        DeleteBuffer(m_pVideoBuffer+i);
      }

      if( !(m_pVideoBuffer[i].iFlags & DVP_FLAG_ALLOCATED) )
      { //Need to allocate

        //First make sure all properties are reset
        memset(&m_pVideoBuffer[i], 0, sizeof(DVDVideoPicture));

        //Allocate for YV12 frame
        unsigned int iPixels = width*height;
        unsigned int iChromaPixels = iPixels/4;

        // If we're dealing with a 4:2:2 format, then we actually need more pixels to
        // store the chroma, as "the two chroma components are sampled at half the
        // sample rate of luma, so horizontal chroma resolution is cut in half."
        //
        // FIXME: Do we need to handle 4:4:4 and 4:1:1 as well?
        //
        if (m_bIs422)
          iChromaPixels = iPixels/2;

        m_pVideoBuffer[i].iLineSize[0] = width;   //Y
        m_pVideoBuffer[i].iLineSize[1] = width/2; //U
        m_pVideoBuffer[i].iLineSize[2] = width/2; //V
        m_pVideoBuffer[i].iLineSize[3] = 0;

        m_pVideoBuffer[i].iWidth = width;
        m_pVideoBuffer[i].iHeight = height;

        m_pVideoBuffer[i].data[0] = (uint8_t*)_aligned_malloc(iPixels, 16);    //Y
        m_pVideoBuffer[i].data[1] = (uint8_t*)_aligned_malloc(iChromaPixels, 16);  //U
        m_pVideoBuffer[i].data[2] = (uint8_t*)_aligned_malloc(iChromaPixels, 16);  //V
        if (!m_pVideoBuffer[i].data[0] || !m_pVideoBuffer[i].data[1] || !m_pVideoBuffer[i].data[2])
        {
          _aligned_free(m_pVideoBuffer[i].data[0]);
          _aligned_free(m_pVideoBuffer[i].data[1]);
          _aligned_free(m_pVideoBuffer[i].data[2]);
          return NULL;
        }

        //Set all data to 0 for less artifacts.. hmm.. what is black in YUV??
        memset( m_pVideoBuffer[i].data[0], 0, iPixels );
        memset( m_pVideoBuffer[i].data[1], 0, iChromaPixels );
        memset( m_pVideoBuffer[i].data[2], 0, iChromaPixels );
      }
      m_pVideoBuffer[i].pts = DVD_NOPTS_VALUE;
      m_pVideoBuffer[i].iFlags = DVP_FLAG_LIBMPEG2_INUSE | DVP_FLAG_ALLOCATED; //Mark as inuse
      return m_pVideoBuffer+i;
    }
  }

  //No free pictures found.
  return NULL;
}

void CDVDVideoCodecLibMpeg2::DeleteBuffer(DVDVideoPicture* pPic)
{
  if(pPic)
  {
    _aligned_free(pPic->data[0]);
    _aligned_free(pPic->data[1]);
    _aligned_free(pPic->data[2]);

    pPic->data[0] = 0;
    pPic->data[1] = 0;
    pPic->data[2] = 0;

    pPic->iLineSize[0] = 0;
    pPic->iLineSize[1] = 0;
    pPic->iLineSize[2] = 0;

    pPic->iFlags &= ~DVP_FLAG_ALLOCATED;
    pPic->iFlags &= ~DVP_FLAG_LIBMPEG2_ALLOCATED;
    if (m_pCurrentBuffer == pPic) m_pCurrentBuffer = NULL;
  }
  else
  {
    //Release all buffers
    for (int i = 0; i < 3; i++) DeleteBuffer(m_pVideoBuffer + i);
  }
}

void CDVDVideoCodecLibMpeg2::ReleaseBuffer(DVDVideoPicture* pPic)
{
  if (pPic)
  {
    pPic->iFlags &= ~DVP_FLAG_LIBMPEG2_INUSE;
  }
  else
  {
    //Release all buffers
    for (int i = 0; i < 3; i++)
    {
      m_pVideoBuffer[i].iFlags &= ~DVP_FLAG_LIBMPEG2_INUSE;
    }
  }
}

bool CDVDVideoCodecLibMpeg2::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!m_dll.Load())
    return false;

  m_dll.mpeg2_accel(MPEG2_ACCEL_X86_MMX);

  m_pHandle = m_dll.mpeg2_init();
  if (!m_pHandle) return false;

  m_pInfo = m_dll.mpeg2_info(m_pHandle);

  return true;
}

void CDVDVideoCodecLibMpeg2::Dispose()
{
  if (m_pHandle)
    m_dll.mpeg2_close(m_pHandle);

  m_pHandle = NULL;
  m_pInfo = NULL;

  DeleteBuffer(NULL);
  m_pCurrentBuffer = NULL;
  m_irffpattern = 0;

  m_dll.Unload();
}

void CDVDVideoCodecLibMpeg2::SetDropState(bool bDrop)
{
  m_hurry = bDrop ? 1 : 0;
}

int CDVDVideoCodecLibMpeg2::Decode(uint8_t* pData, int iSize, double dts, double pts)
{
  int iState = 0;
  if (!m_pHandle) return VC_ERROR;

  if (pData && iSize)
  {
    //buffer more data
    iState = m_dll.mpeg2_parse(m_pHandle);
    if (iState != STATE_BUFFER)
    {
      CLog::Log(LOGDEBUG,"CDVDVideoCodecLibMpeg2::Decode error, we didn't ask for more data");
      return VC_ERROR;
    }

    // libmpeg2 needs more data. Give it and parse the data again
    m_dll.mpeg2_buffer(m_pHandle, pData, pData + iSize);
    TagUnion u;
    u.pts = pts;
    m_dts = dts;

    m_dll.mpeg2_tag_picture(m_pHandle, u.tag.l, u.tag.u);
  }

  iState = m_dll.mpeg2_parse(m_pHandle);

  DVDVideoPicture* pBuffer;
  // loop until we have another STATE_BUFFER or picture
  while (iState != STATE_BUFFER)
  {
    switch (iState)
    {
    case STATE_SEQUENCE:
      {
        // Check for 4:2:2 here.
        if (m_pInfo->sequence->profile_level_id == MPEG_422_HL || m_pInfo->sequence->profile_level_id == MPEG_422_ML)
          m_bIs422 = true;

        //New sequence of frames
        //Release all buffers
        ReleaseBuffer(NULL);

        for (int i = 0; i < 3; i++)
        {
          //Setup all buffers we wish to use.
          pBuffer = GetBuffer(m_pInfo->sequence->width, m_pInfo->sequence->height);
          m_dll.mpeg2_set_buf(m_pHandle, pBuffer->data, pBuffer);
        }
        break;
      }
    case STATE_GOP:
      {
        // check for closed captioning or other userdata
        if (m_pInfo->user_data && m_pInfo->user_data_len > 0)
        {
          return VC_USERDATA;
        }
        break;
      }
    case STATE_PICTURE:
      {
        m_dll.mpeg2_skip(m_pHandle, 0);
        if(m_hurry>0 && m_pInfo->current_picture)
        {
          if((m_pInfo->current_picture->flags&PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_B)
            m_dll.mpeg2_skip(m_pHandle, 1);
        }
        m_dts2 = m_dts;
        m_dts = DVD_NOPTS_VALUE;

        //Not too interesting really
        //we can do everything when we get a full picture instead. simplifies things.

        //if we want to we could setup the 3rd frame buffer here instead
        //but i see no point as of now.

      }
      break;
    case STATE_SLICE:
    case STATE_END:
    case STATE_INVALID_END:
      {
        if( m_pInfo->discard_fbuf ) //LIBMPEG2 is done with this buffer, release it
        {
          if (m_pInfo->discard_fbuf->id)
            ReleaseBuffer((DVDVideoPicture*)m_pInfo->discard_fbuf->id);
          else
            CLog::Log(LOGWARNING, "CDVDVideoCodecLibMpeg2::Decode - libmpeg2 discarded and internal frame");
        }

        if( m_pInfo->display_fbuf && m_pInfo->display_picture && m_pInfo->sequence )
        {
          if(m_pInfo->display_fbuf->id)
          {
            pBuffer = (DVDVideoPicture*)m_pInfo->display_fbuf->id;

            pBuffer->iFlags &= ~(DVP_FLAG_REPEAT_TOP_FIELD | DVP_FLAG_TOP_FIELD_FIRST);

            pBuffer->iFlags |= (m_pInfo->display_picture->nb_fields > 2) ? DVP_FLAG_REPEAT_TOP_FIELD : 0;
            pBuffer->iFlags |= (m_pInfo->display_picture->flags & PIC_FLAG_TOP_FIELD_FIRST) ? DVP_FLAG_TOP_FIELD_FIRST : 0;

            //Detection of repeate frame patterns
            m_irffpattern = m_irffpattern << 1;
            if( pBuffer->iFlags & DVP_FLAG_REPEAT_TOP_FIELD ) m_irffpattern |= 1;

            switch (m_pInfo->display_picture->flags & PIC_MASK_CODING_TYPE)
            {
              case PIC_FLAG_CODING_TYPE_I: pBuffer->iFrameType = FRAME_TYPE_I; break;
              case PIC_FLAG_CODING_TYPE_P: pBuffer->iFrameType = FRAME_TYPE_P; break;
              case PIC_FLAG_CODING_TYPE_B: pBuffer->iFrameType = FRAME_TYPE_B; break;
              case PIC_FLAG_CODING_TYPE_D: pBuffer->iFrameType = FRAME_TYPE_D; break;
              default: pBuffer->iFrameType = FRAME_TYPE_UNDEF; break;
            }

            if(m_pInfo->display_picture->flags & PIC_FLAG_SKIP)
              pBuffer->iFlags |= DVP_FLAG_DROPPED;
            else
              pBuffer->iFlags &= ~DVP_FLAG_DROPPED;

            pBuffer->iDuration = m_pInfo->sequence->frame_period;

            if( ((m_irffpattern & 0xff) == 0xaa || (m_irffpattern & 0xff) == 0x55) )  /* special case for ntsc 3:2 pulldown */
              pBuffer->iRepeatPicture = 0.25;
            else if( pBuffer->iFlags & DVP_FLAG_REPEAT_TOP_FIELD )
              pBuffer->iRepeatPicture = 0.5 * (m_pInfo->display_picture->nb_fields - 2);
            else
              pBuffer->iRepeatPicture = 0.0;

            //Mpeg frametime is calculated using a 27ghz clock.. pts and such normally a 90mhz clock
            pBuffer->iDuration /= (27000000 / DVD_TIME_BASE);


            //Figure out of this frame is interlaced
            //This is taken from how MPC handles it in Mpeg2DecFilter.cpp. Not sure this is entire correct
            //needs to be tested more

            //First try to decide if we have film material
            if( !(m_pInfo->sequence->flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE)
              && m_pInfo->display_picture->flags & PIC_FLAG_PROGRESSIVE_FRAME)
            { //We have a progressive frame in a nonprogressive sequence

                if(!m_bFilm
						  && ( ((m_irffpattern & 0xff) == 0xaa || (m_irffpattern & 0xff) == 0x55) ) )
						    {
                  //We are not in film mode but we did find a repeat first frame
                  //Usually means material is in film and 3:2 pullup has been used
                  //to generate the full ntsc format.
                  //This also means the frames we get are usually progressive

                  CLog::Log(LOGDEBUG,"CDVDVideoCodecLibMpeg2::m_bFilm = true\n");
							    m_bFilm = true;
						    }
						    else if(m_bFilm
						  && !( ((m_irffpattern & 0xff) == 0xaa || (m_irffpattern & 0xff) == 0x55) ) )
						    {
                  //Crap a progressive frame in a nonprogressive sequence that
                  //doesn't have hte repeat flag set. No idea what format the
                  //material is in

							    CLog::Log(LOGDEBUG,"CDVDVideoCodecLibMpeg2::m_bFilm = false\n");
							    m_bFilm = false;
						    }
              }

            // Quoted from MPC Source (Mpeg2DecFilter.cpp)
						//// big trouble here, the progressive_frame bit is not reliable :'(
						//// frames without temporal field diffs can be only detected when ntsc
						//// uses the repeat field flag (signaled with m_fFilm), if it's not set
						//// or we have pal then we might end up blending the fields unnecessarily...

            if( m_pInfo->sequence->flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE )
            { //We've got a progressive sequence
              pBuffer->iFlags &= ~DVP_FLAG_INTERLACED;
            }
            else if( (m_pInfo->sequence->flags & SEQ_MASK_VIDEO_FORMAT) == SEQ_VIDEO_FORMAT_NTSC || (m_pInfo->sequence->flags & SEQ_MASK_VIDEO_FORMAT) == SEQ_VIDEO_FORMAT_UNSPECIFIED )
            {
              //This stuff can be quite messy as film based material can be 24 fps progressive
              if( m_bFilm )
              { //Film material, not interlaced
                pBuffer->iFlags &= ~DVP_FLAG_INTERLACED;
              }
              else if( (m_pInfo->display_picture->flags & PIC_FLAG_PROGRESSIVE_FRAME) )
              { //This can't allways be trusted, but we let user override
                pBuffer->iFlags &= ~DVP_FLAG_INTERLACED;
              }
              else
              { //Very likely to be interlaced
                pBuffer->iFlags |= DVP_FLAG_INTERLACED;
              }
            }
            else
            {
              //Okey not a progressive sequence and we have no idea if this is really progressive
              //have to assume it is not
              pBuffer->iFlags |= DVP_FLAG_INTERLACED;
            }

            pBuffer->iWidth = m_pInfo->sequence->width;
            pBuffer->iHeight = m_pInfo->sequence->height;

            pBuffer->iDisplayWidth = m_pInfo->sequence->display_width;
            pBuffer->iDisplayHeight = m_pInfo->sequence->display_height;

            //Try to figure out aspect ratio based on video frame
            //in the case of dvd video, we should actually override this
            //based on what libdvdnav gives us.
            unsigned int pixel_x = m_pInfo->sequence->pixel_width;
            unsigned int pixel_y = m_pInfo->sequence->pixel_height;

            GuessAspect(m_pInfo->sequence, &pixel_x, &pixel_y);

            // modify our displaywidth to suit
            float fPixelAR = (float)pixel_x / pixel_y;
            pBuffer->iDisplayWidth = (int)((float)pBuffer->iDisplayWidth * fPixelAR);

            // make sure we send the color coeficients with the image
            pBuffer->color_matrix = m_pInfo->sequence->matrix_coefficients;
            pBuffer->color_range = 0; // mpeg2 always have th 16->235/229 color range
            pBuffer->chroma_position = 1; // mpeg2 chroma positioning always left
            pBuffer->color_primaries = m_pInfo->sequence->colour_primaries;
            pBuffer->color_transfer = m_pInfo->sequence->transfer_characteristics;

            TagUnion u;
            u.tag.l = m_pInfo->display_picture->tag;
            u.tag.u = m_pInfo->display_picture->tag2;
            pBuffer->pts = u.pts;
            pBuffer->dts = m_dts2;
            m_dts2 = DVD_NOPTS_VALUE;

            // only return this if it's not first image or an I frame
            if(m_pCurrentBuffer || pBuffer->iFrameType == FRAME_TYPE_I || pBuffer->iFrameType == FRAME_TYPE_UNDEF )
            {
              m_pCurrentBuffer = pBuffer;
              return VC_PICTURE;
            }
          }
          else
            CLog::Log(LOGWARNING, "CDVDVideoCodecLibMpeg2::Decode - libmpeg2 trying to display it's own buffer, skipping...");
        }
        break;
      }
    case STATE_INVALID:
      {
        CLog::Log(LOGDEBUG,"CDVDVideoCodecLibMpeg2::Decode error, invalid state");
        // return VC_ERROR;
      }
    default: break;
    }
    iState = m_dll.mpeg2_parse(m_pHandle);
  }

  if (iState == STATE_BUFFER)
    return VC_BUFFER;

  CLog::Log(LOGDEBUG,"CDVDVideoCodecLibMpeg2::Decode error");

  return VC_ERROR;
}

void CDVDVideoCodecLibMpeg2::Reset()
{
  /* we can't do a full reset here as then libmpeg2 doesn't search for all *
   * start codes, but doesn't have enough state information to continue    *
   * decoding. m_pHandle->sequence.width should be set to -1 on full reset */
  if (m_pHandle)
    m_dll.mpeg2_reset(m_pHandle, 0);

  ReleaseBuffer(NULL);
  m_pCurrentBuffer = NULL;
  m_dts = DVD_NOPTS_VALUE;
  m_dts2 = DVD_NOPTS_VALUE;
}

bool CDVDVideoCodecLibMpeg2::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if(m_pCurrentBuffer && m_pCurrentBuffer->iFlags & DVP_FLAG_ALLOCATED)
  {
    memcpy(pDvdVideoPicture, m_pCurrentBuffer, sizeof(DVDVideoPicture));

    // If we're decoding a 4:2:2 image, we need to skip every other line to get
    // down to 4:2:0. We lose image quality, but hopefully nobody will notice.
    //
    if (m_bIs422)
      pDvdVideoPicture->iLineSize[2] <<= 1;

    pDvdVideoPicture->format = RENDER_FMT_YUV420P;
    return true;
  }
  else
    return false;
}

bool CDVDVideoCodecLibMpeg2::GetUserData(DVDVideoUserData* pDvdVideoUserData)
{
  if (pDvdVideoUserData && m_pInfo && m_pInfo->user_data && m_pInfo->user_data_len > 0)
  {
    pDvdVideoUserData->data = (uint8_t*)m_pInfo->user_data;
    pDvdVideoUserData->size = m_pInfo->user_data_len;
    return true;
  }
  return false;
}

// Redone mpeg2_guess_aspect() code from libmpeg2 (header.c)
// Corrected the pixel ratio compensation as per NTSC/PAL specs.
// This correctly calculates the pixel ratio of sources intended for NTSC or PAL
// screens, and thus we do not need to do any additional compensation within
// the output code.  Note that the mpeg2_guess_aspect() code was correct to within 0.1%
// for PAL and 0.3% for NTSC, but we mayaswell be 100% accurate.
int CDVDVideoCodecLibMpeg2::GuessAspect(const mpeg2_sequence_t * sequence,
                                        unsigned int * pixel_width,
                                        unsigned int * pixel_height)
{
  static struct
  {
    unsigned int width, height;
  }
  video_modes[] = {
                    {720, 576},  /* 625 lines, 13.5 MHz (D1, DV, DVB, DVD) */
                    {704, 576},  /* 625 lines, 13.5 MHz (1/1 D1, DVB, DVD, 4CIF) */
                    {544, 576},  /* 625 lines, 10.125 MHz (DVB, laserdisc) */
                    {528, 576},  /* 625 lines, 10.125 MHz (3/4 D1, DVB, laserdisc) */
                    {480, 576},  /* 625 lines, 9 MHz (2/3 D1, DVB, SVCD) */
                    {352, 576},  /* 625 lines, 6.75 MHz (D2, 1/2 D1, CVD, DVB, DVD) */
                    {352, 288},  /* 625 lines, 6.75 MHz, 1 field (D4, VCD, DVB, DVD, CIF) */
                    {176, 144},  /* 625 lines, 3.375 MHz, half field (QCIF) */
                    {720, 486},  /* 525 lines, 13.5 MHz (D1) */
                    {704, 486},  /* 525 lines, 13.5 MHz */
                    {720, 480},  /* 525 lines, 13.5 MHz (DV, DSS, DVD) */
                    {704, 480},  /* 525 lines, 13.5 MHz (1/1 D1, ATSC, DVD) */
                    {544, 480},  /* 525 lines. 10.125 MHz (DSS, laserdisc) */
                    {528, 480},  /* 525 lines. 10.125 MHz (3/4 D1, laserdisc) */
                    {480, 480},  /* 525 lines, 9 MHz (2/3 D1, SVCD) */
                    {352, 480},  /* 525 lines, 6.75 MHz (D2, 1/2 D1, CVD, DVD) */
                    {352, 240}  /* 525  lines. 6.75 MHz, 1 field (D4, VCD, DSS, DVD) */
                  };
  unsigned int width, height, pix_width, pix_height, i, DAR_16_9;

  *pixel_width = sequence->pixel_width;
  *pixel_height = sequence->pixel_height;
  width = sequence->picture_width;
  height = sequence->picture_height;
  for (i = 0; i < sizeof (video_modes) / sizeof (video_modes[0]); i++)
    if (width == video_modes[i].width && height == video_modes[i].height)
      break;
  if (i == sizeof (video_modes) / sizeof (video_modes[0]) ||
      (sequence->pixel_width == 1 && sequence->pixel_height == 1) ||
      width != sequence->display_width || height != sequence->display_height)
    return 0;

  for (pix_height = 1; height * pix_height < 480; pix_height <<= 1) {}
  height *= pix_height;
  for (pix_width = 1; width * pix_width <= 352; pix_width <<= 1) {}
  width *= pix_width;

  if (! (sequence->flags & SEQ_FLAG_MPEG2))
  {
    static unsigned int mpeg1_check[2][2] = {{11, 54}, {27, 45}};
    DAR_16_9 = (sequence->pixel_height == 27 || sequence->pixel_height == 45);
    if (width < 704 || sequence->pixel_height != mpeg1_check[DAR_16_9][height == 576])
      return 0;
  }
  else
  {
    DAR_16_9 = (3 * sequence->picture_width * sequence->pixel_width >
                4 * sequence->picture_height * sequence->pixel_height);
    switch (width)
    {
    case 528:
    case 544:
      pix_width *= 4;
      pix_height *= 3;
      break;
    case 480:
      pix_width *= 3;
      pix_height *= 2;
      break;
    }
  }
  if (DAR_16_9)
  {
    pix_width *= 4;
    pix_height *= 3;
  }
  if (height == 576)
  { // we are being pedantic here - the old values are 0.1% out
    pix_width *= 128; //59;
    pix_height *= 117; //54;
  }
  else
  { // the old values are 0.3% out
    pix_width *= 4320; //10;
    pix_height *= 4739; //11;
  }
  *pixel_width = pix_width;
  *pixel_height = pix_height;
  //  simplify (pixel_width, pixel_height);
  return (height == 576) ? 1 : 2;
}
