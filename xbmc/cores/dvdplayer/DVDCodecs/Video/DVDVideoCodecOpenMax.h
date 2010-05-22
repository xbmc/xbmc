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
#include "utils/Event.h"

#include <queue>
#include <semaphore.h>      
#include <OMX_Core.h>

typedef struct omx_bitstream_ctx {
    uint8_t  length_size;
    uint8_t  first_idr;
    uint8_t *sps_pps_data;
    uint32_t size;
} omx_bitstream_ctx;

typedef struct omx_demux_packet {
  OMX_U8 *buff;
  int size;
  double dts;
  double pts;
} omx_demux_packet;

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
  // bitstream to bytestream (Annex B) conversion routines.
  bool bitstream_convert_init(void *in_extradata, int in_extrasize);
  bool bitstream_convert(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size);
  void bitstream_alloc_and_copy( uint8_t **poutbuf, int *poutbuf_size,
    const uint8_t *sps_pps, uint32_t sps_pps_size, const uint8_t *in, uint32_t in_size);

  // OpenMax Decoder Callback routines.
  static OMX_ERRORTYPE DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
  static OMX_ERRORTYPE DecoderEmptyBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
  static OMX_ERRORTYPE DecoderFillBufferDone(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

  // OpenMax helper routines
  OMX_ERRORTYPE PrimeFillBuffers(void);
  OMX_ERRORTYPE AllocOMXInputBuffers(void);
  OMX_ERRORTYPE FreeOMXInputBuffers(bool wait);
  OMX_ERRORTYPE AllocOMXOutputBuffers(void);
  OMX_ERRORTYPE FreeOMXOutputBuffers(bool wait);
  OMX_ERRORTYPE SetStateForComponent(OMX_STATETYPE state);
  OMX_ERRORTYPE StartDecoder(void);
  OMX_ERRORTYPE StopDecoder(void);

  DllLibOpenMax     *m_dll;
  OMX_HANDLETYPE    m_omx_decoder;   // openmax decoder component reference
  DVDVideoPicture   m_videobuffer;
  const char        *m_pFormatName;
  bool              m_drop_pictures;
  int               m_decoded_width;
  int               m_decoded_height;

  std::queue<double> m_dts_queue;
  std::queue<omx_demux_packet> m_demux_queue;

  // OpenMax input buffers (demuxer packets)
  pthread_mutex_t   m_omx_avaliable_mutex;
  std::queue<OMX_BUFFERHEADERTYPE*> m_omx_input_avaliable;
  std::vector<OMX_BUFFERHEADERTYPE*> m_omx_input_buffers;
  bool              m_omx_input_eos;
  int               m_omx_input_port;
  //sem_t             *m_omx_flush_input;
  CEvent            m_input_consumed_event;

  // OpenMax output buffers (video frames)
  pthread_mutex_t   m_omx_ready_mutex;
  std::queue<OMX_BUFFERHEADERTYPE*> m_omx_output_ready;
  std::vector<OMX_BUFFERHEADERTYPE*> m_omx_output_buffers;
  bool              m_omx_output_eos;
  int               m_omx_output_port;
  //sem_t             *m_omx_flush_output;

  // OpenMax state tracking
  volatile int      m_omx_state;
  sem_t             *m_omx_state_change;
  volatile bool     m_videoplayback_done;
  
  // bitstream to bytestream convertion (Annex B)
  uint32_t          m_sps_pps_size;
  omx_bitstream_ctx m_sps_pps_context;
  bool              m_convert_bitstream;
};

#endif
