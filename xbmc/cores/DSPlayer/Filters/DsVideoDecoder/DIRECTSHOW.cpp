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

CDecoder::SVideoBuffer::SVideoBuffer()
{
  surface = NULL;
  Clear();
}

CDecoder::SVideoBuffer::~SVideoBuffer()
{
  Clear();
}

void CDecoder::SVideoBuffer::Init(int index)
{
  surface = (directshow_dxva_h264*)calloc(1, sizeof(directshow_dxva_h264));
  memset(surface,0,sizeof(directshow_dxva_h264));
  for (int i = 0; i < 16; i++)
    surface->picture_params.RefFrameList[i].bPicEntry = 0xff;
  age = 0;
  used = 0;
  surface_index = index;
}
void CDecoder::SVideoBuffer::Clear()
{
  free(surface);  
  age     = 0;
  used    = 0;
}

CDecoder::CDecoder()
{
  m_refs = 0;
  m_buffer_count = 0;
  m_buffer_age = 0;
  
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
  for(unsigned i = 0; i < m_buffer_count; i++)
    m_buffer[i].Clear();
  m_buffer_count = 0;
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

  for (unsigned int i = 0; i < m_buffer_max; i++)
  {
    /**/
    m_buffer[i].Init(i);
    
  }
  m_buffer_count = 16;
  avctx->get_buffer      = GetBufferS;
  avctx->release_buffer  = RelBufferS;

  return true;
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  /*CSingleLock lock(m_section);*/
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
  
  picture = (directshow_dxva_h264*)avctx->coded_frame->data[0];
  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  return 0;

}


bool CDecoder::Supports(enum PixelFormat fmt)
{
  if(fmt == PIX_FMT_DIRECTSHOW_H264)
    return true;
  return false;
}

int CDecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CSingleLock lock(m_section);
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CXBMCVideoDecFilter* ctx = (CXBMCVideoDecFilter*)avctx->opaque;
  CDecoder* dec        = (CDecoder*)ctx->GetHardware();

  int           count = 0;
  SVideoBuffer* buf   = NULL;
  /*TODO verify if there any verification to do before touching the buffer*/
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
    ASSERT(0);
  }
  
  if(!buf)
  {
    CLog::Log(LOGERROR, "%s - unable to find new unused buffer",__FUNCTION__);
    return -1;
  }

 

  
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
  pic->reordered_opaque = avctx->reordered_opaque;
  pic->type = FF_BUFFER_TYPE_USER;
  pic->age = 256*256*256*64; // No matter what we set we need a positive value or it crash
  buf->surface->decoder_surface_index = buf->surface_index;
  for(unsigned i = 0; i < 4; i++)
  {
    pic->data[i] = NULL;
    pic->linesize[i] = 0;
  }
  pic->data[0]= (uint8_t*)buf->surface;
  buf->used = true;

  
  return 0;
}

void CDecoder::RelBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CSingleLock lock(m_section);
  directshow_dxva_h264* surface = (directshow_dxva_h264*)pic->data[0];

  for(unsigned i = 0; i < m_buffer_count; i++)
  {
    if(m_buffer[i].surface == surface)
    {
      m_buffer[i].used = false;
      m_buffer[i].age  = ++m_buffer_age;
      break;
    }
  }
  for(unsigned i = 0; i < 4; i++)
    pic->data[i] = NULL;
}

#endif