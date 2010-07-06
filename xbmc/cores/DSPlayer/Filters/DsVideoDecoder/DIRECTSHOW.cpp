/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#ifdef HAS_DX




#include "DIRECTSHOW.h"

using namespace DIRECTSHOW;

static void RelBufferS(AVCodecContext *avctx, AVFrame *pic)
{ ((CDecoder*)((CXBMCVideoDecFilter*)avctx->opaque)->GetHardware())->RelBuffer(avctx, pic); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic) 
{  return ((CDecoder*)((CXBMCVideoDecFilter*)avctx->opaque)->GetHardware())->GetBuffer(avctx, pic); }

CDecoder::CDecoder()
 : m_event(true)
{
  m_refs = 0;
  m_event.Set();
  
  
  
  
  /*m_context          = (dxva_context*)calloc(1, sizeof(dxva_context));
  m_context->cfg     = (DXVA2_ConfigPictureDecode*)calloc(1, sizeof(DXVA2_ConfigPictureDecode));
  m_context->surface = (IDirect3DSurface9**)calloc(m_buffer_max, sizeof(IDirect3DSurface9*));*/
  
}

CDecoder::~CDecoder()
{
  
  Close();
  
  
  
}

void CDecoder::Close()
{
  CSingleLock lock(m_section);
  
  m_videoBuffer.clear();
  
  
  
  
  
  lock.Leave();

  
}

#define CHECK(a) \
do { \
  HRESULT res = a; \
  if(FAILED(res)) \
  { \
    CLog::Log(LOGERROR, "DXVA - failed executing "#a" at line %d with error %x", __LINE__, res); \
    return false; \
  } \
} while(0);


bool CDecoder::Open(AVCodecContext *avctx, enum PixelFormat fmt)
{
  

  CSingleLock lock(m_section);
  Close();

  if(avctx->refs > m_refs)
    m_refs = avctx->refs;

  if(m_refs == 0)
  {
    if(avctx->codec_id == CODEC_ID_H264)
      m_refs = 16;
    else
      m_refs = 2;
  }
  CLog::Log(LOGDEBUG, "DXVA - source requires %d references", avctx->refs);

  avctx->get_buffer      = GetBufferS;
  avctx->release_buffer  = RelBufferS;

  return true;
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  CSingleLock lock(m_section);
  int result = Check(avctx);
  if(result)
    return result;

  if(frame)
  {
    directshow_dxva_h264 * render = (directshow_dxva_h264*)frame->data[2];
    if(!render) // old style ffmpeg gave data on plane 0
      render = (directshow_dxva_h264*)frame->data[0];
    if(!render)
      return -1;
    return VC_BUFFER;
  }
  else
    return 0;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, directshow_dxva_h264* picture)
{
  CSingleLock lock(m_section);
  picture = (directshow_dxva_h264*)frame->data[3];
  
  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  CSingleLock lock(m_section);

  
#if 0
  if(m_format.SampleWidth  == 0
  || m_format.SampleHeight == 0)
  {
    if(!Open(avctx, avctx->pix_fmt))
    {
      CLog::Log(LOGERROR, "CDecoder::Check - decoder was not able to reset");
      Close();
      return VC_ERROR;
    }
    return VC_FLUSHED;
  }

  if(avctx->codec_id != CODEC_ID_MPEG2VIDEO
  && avctx->codec_id != CODEC_ID_H264
  && avctx->codec_id != CODEC_ID_VC1
  && avctx->codec_id != CODEC_ID_WMV3)
    return 0;

  DXVA2_DecodeExecuteParams params = {};
  DXVA2_DecodeExtensionData data   = {};
  union {
    DXVA_Status_H264 h264;
    DXVA_Status_VC1  vc1;
  } status = {};

  params.pExtensionData = &data;
  data.Function = DXVA_STATUS_REPORTING_FUNCTION;
  data.pPrivateOutputData    = &status;
  data.PrivateOutputDataSize = avctx->codec_id == CODEC_ID_H264 ? sizeof(DXVA_Status_H264) : sizeof(DXVA_Status_VC1);
  HRESULT hr;
  if(FAILED( hr = m_decoder->Execute(&params)))
  {
    CLog::Log(LOGWARNING, "DXVA - failed to get decoder status - 0x%08X", hr);

    // temporary ugly hack for testing! pretend there was no error.
    if(avctx->codec_id == CODEC_ID_MPEG2VIDEO)
      return 0;

    return VC_ERROR;
  }

  if(avctx->codec_id == CODEC_ID_H264)
  {
    if(status.h264.bStatus)
      CLog::Log(LOGWARNING, "DXVA - decoder problem of status %d with %d", status.h264.bStatus, status.h264.bBufType);
  }
  else
  {
    if(status.vc1.bStatus)
      CLog::Log(LOGWARNING, "DXVA - decoder problem of status %d with %d", status.vc1.bStatus, status.vc1.bBufType);
  }
#endif
  return 0;

}


bool CDecoder::Supports(enum PixelFormat fmt)
{
  if(fmt == PIX_FMT_DIRECTSHOW_H264)
    return true;
  return false;
}

void CDecoder::RelBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CSingleLock lock(m_section);
  directshow_dxva_h264* render = (directshow_dxva_h264*)pic->data[0];

  if(!render)
  {
    CLog::Log(LOGERROR, "CVDPAU::FFDrawSlice - invalid context handle provided");
    return;
  }

  /*render->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;*/
  for(int i=0; i<4; i++)
    pic->data[i]= NULL;
}

int CDecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CSingleLock lock(m_section);
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CXBMCVideoDecFilter* ctx        = (CXBMCVideoDecFilter*)avctx->opaque;
  CDecoder* dec        = (CDecoder*)ctx->GetHardware();
  


  directshow_dxva_h264 * render = NULL;

  for(unsigned int i = 0; i < dec->m_videoBuffer.size(); i++)
  {
      render = dec->m_videoBuffer[i];
      /*render->state = 0;*/
      break;
  }

  if (render == NULL)
  {
    render = (directshow_dxva_h264*)calloc(sizeof(directshow_dxva_h264), 1);
	dec->m_videoBuffer.push_back(render);
  }

  if (render == NULL)
    return -1;

  pic->data[1] =  pic->data[2] = NULL;
  pic->data[0]= (uint8_t*)render;

  pic->linesize[0] = pic->linesize[1] =  pic->linesize[2] = 0;

  pic->age = INT_MAX;
  /*if(pic->reference)
  {
    pic->age = pA->ip_age[0];
    pA->ip_age[0]= pA->ip_age[1]+1;
    pA->ip_age[1]= 1;
    pA->b_age++;
  }
  else
  {
    pic->age = pA->b_age;
    pA->ip_age[0]++;
    pA->ip_age[1]++;
    pA->b_age = 1;
  }*/
  pic->type = FF_BUFFER_TYPE_USER;

/*  render->state |= FF_VDPAU_STATE_USED_FOR_REFERENCE;*/
  pic->reordered_opaque= avctx->reordered_opaque;
  return 0;
#if 0
  CSingleLock lock(m_section);
  

  int           count = 0;
  SVideoBuffer* buf   = NULL;
  for(unsigned i = 0; i < m_buffer_count; i++)
  {
    if(m_buffer[i].used)
      count++;
    else
    {
      if(!buf || buf->age > m_buffer[i].age)
        buf = m_buffer+i;
    }
  }

  if(count >= m_refs+2)
  {
    m_refs++;
#if ALLOW_ADDING_SURFACES
    if(!OpenDecoder())
      return -1;
    return GetBuffer(avctx, pic);
#else
    Close();
    return -1;
#endif
  }

  if(!buf)
  {
   CLog::Log(LOGERROR, "DXVA - unable to find new unused buffer");
    return -1;
  }

  pic->reordered_opaque = avctx->reordered_opaque;
  pic->type = FF_BUFFER_TYPE_USER;
  pic->age  = 256*256*256*64; // as everybody else, i've got no idea about this one
  for(unsigned i = 0; i < 4; i++)
  {
    pic->data[i] = NULL;
    pic->linesize[i] = 0;
  }

  pic->data[0] = (uint8_t*)buf->surface;
  pic->data[3] = (uint8_t*)buf->surface;
  buf->used = true;

  return 0;
#endif
}
#endif