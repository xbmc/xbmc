#pragma once
/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
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

#if defined(HAVE_LIBOPENMAX)

#include "DVDVideoCodec.h"

#include <queue>
#include <semaphore.h>      
#include <OMX_Core.h>

typedef struct omx_bitstream_ctx {
    uint8_t  length_size;
    uint8_t  first_idr;
    uint8_t *sps_pps_data;
    uint32_t size;
} omx_bitstream_ctx;

class DllLibOpenMax;
class CDVDVideoCodecOpenMax : public CDVDVideoCodec
{
public:
  CDVDVideoCodecOpenMax();
  virtual ~CDVDVideoCodecOpenMax();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose(void);
  virtual int  Decode(BYTE *pData, int iSize, double dts, double pts);
  virtual void Reset(void);
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }
  
protected:
  bool bitstream_convert_init(uint8_t *in_extradata, int in_extrasize);
  bool bitstream_convert(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size);
  void bitstream_alloc_and_copy( uint8_t **poutbuf, int *poutbuf_size,
    const uint8_t *sps_pps, uint32_t sps_pps_size, const uint8_t *in, uint32_t in_size);

  static OMX_ERRORTYPE DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
  static OMX_ERRORTYPE DecoderEmptyBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
  static OMX_ERRORTYPE DecoderFillBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

  OMX_ERRORTYPE PrimeFillBuffers(void);
  OMX_ERRORTYPE AllocOMXInputBuffers(void);
  OMX_ERRORTYPE FreeOMXInputBuffers(void);
  OMX_ERRORTYPE AllocOMXOutputBuffers(void);
  OMX_ERRORTYPE FreeOMXOutputBuffers(void);
  OMX_ERRORTYPE SetStateForAllComponents(OMX_STATETYPE state);
  OMX_ERRORTYPE StartDecoder(void);
  OMX_ERRORTYPE StopDecoder(void);

  DllLibOpenMax     *m_dll;
  OMX_HANDLETYPE    m_omx_decoder;   // openmax decoder component reference
  const char        *m_pFormatName;
  bool              m_drop_pictures;
  int               m_decoded_width;
  int               m_decoded_height;
  DVDVideoPicture   m_videobuffer;

  std::queue<double> m_dts_queue;

  // OpenMAX input buffers (demuxer packets)
  pthread_mutex_t   m_free_input_mutex;
  std::queue<OMX_BUFFERHEADERTYPE*> m_free_input_buffers;
  std::vector<OMX_BUFFERHEADERTYPE*> m_input_buffers;
  int               m_input_buffer_count;
  int               m_input_buffer_size;
  int               m_input_port;
  bool              m_input_eos;

  // OpenMAX output buffers (video frames)
  pthread_mutex_t   m_ready_output_mutex;
  std::queue<OMX_BUFFERHEADERTYPE*> m_ready_output_buffers;
  std::vector<OMX_BUFFERHEADERTYPE*> m_output_buffers;
  int               m_output_buffer_count;
  int               m_output_buffer_size;
  int               m_output_port;
  bool              m_output_eos;

  volatile int      m_omx_state;
  sem_t             *m_omx_state_change;
  volatile bool     m_videoplayback_done;
  
  // bitstream to bytestream convertion (Annex B)
  uint32_t          m_sps_pps_size;
  omx_bitstream_ctx m_sps_pps_context;
  bool              m_convert_bitstream;
};

#endif
