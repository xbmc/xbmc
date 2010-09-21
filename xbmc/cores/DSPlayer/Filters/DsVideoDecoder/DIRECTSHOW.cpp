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

#ifdef HAS_DX



#include "XBMCVideoDecFilter.h"
#include "DIRECTSHOW.h"
#include "WindowingFactory.h"
#define MAX_RETRY_ON_PENDING 50
#define DO_DXVA_PENDING_LOOP(x)		nTry = 0; \
									while (FAILED(hr = x) && nTry<MAX_RETRY_ON_PENDING) \
									{ \
										if (hr != E_PENDING) break; \
										Sleep(1); \
										nTry++; \
									}

static void RelBufferS(AVCodecContext *avctx, AVFrame *pic)
{ ((CDXVADecoder*)((CXBMCVideoDecFilter*)avctx->opaque)->GetDXVADecoder())->RelBuffer(avctx, pic); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic) 
{  return ((CDXVADecoder*)((CXBMCVideoDecFilter*)avctx->opaque)->GetDXVADecoder())->GetBuffer(avctx, pic); }

static int DXVABeginFrameS(dxva_context *ctx, unsigned index)
{ return ((CDXVADecoder*)ctx->decoder)->DXVABeginFrame(ctx, index); }

static int DXVAEndFrameS(dxva_context *ctx, unsigned index)
{ return ((CDXVADecoder*)ctx->decoder)->DXVAEndFrame(ctx, index); }

static int DXVAExecuteS(dxva_context *ctx)
{ return ((CDXVADecoder*)ctx->decoder)->DXVAExecute(ctx); }

static int DXVAGetBufferS(dxva_context *ctx, unsigned type, void **dxva_data, unsigned *dxva_size)
{ return ((CDXVADecoder*)ctx->decoder)->DXVAGetBuffer(ctx, type, dxva_data, dxva_size); }

static int DXVAReleaseBufferS(dxva_context *ctx, unsigned type)
{ return ((CDXVADecoder*)ctx->decoder)->DXVAReleaseBuffer(ctx, type); }

CDXVADecoder::SVideoBuffer::SVideoBuffer()
{
  surface = NULL;
  Clear();
}

CDXVADecoder::SVideoBuffer::~SVideoBuffer()
{
  Clear();
}

void CDXVADecoder::SVideoBuffer::Init(int index)
{
  //surface = (directshow_dxva_h264*)calloc(1, sizeof(directshow_dxva_h264));
  //memset(surface,0,sizeof(directshow_dxva_h264));
  age = 0;
  used = 0;
  surface_index = index;
  rt_start = 0;
  rt_stop  = 0;
}

void CDXVADecoder::SVideoBuffer::Clear()
{
  SAFE_RELEASE(surface);
  age      = 0;
  used     = 0;
  rt_start = 0;
  rt_stop  = 0;
}
CDXVADecoder* CDXVADecoder::CreateDecoder (CXBMCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, int nPicEntryNumber)
{
	CDXVADecoder*		pDecoder = NULL;
  pDecoder = new CDXVADecoder(pFilter, pAMVideoAccelerator, nPicEntryNumber);
	return pDecoder;
}


CDXVADecoder* CDXVADecoder::CreateDecoder (CXBMCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
{
	CDXVADecoder*		pDecoder = NULL;
  pDecoder	= new CDXVADecoder (pFilter, pDirectXVideoDec, nPicEntryNumber, pDXVA2Config);
	return pDecoder;
}

CDXVADecoder::CDXVADecoder (CXBMCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, int nPicEntryNumber)
{
	m_nEngine				= DECODER_TYPE_DXVA_1;
	m_pAMVideoAccelerator	= pAMVideoAccelerator;
	m_dwBufferIndex			= 0;
	m_nMaxWaiting			= 3;
	Init (pFilter, nPicEntryNumber);
}


CDXVADecoder::CDXVADecoder (CXBMCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
{
	m_nEngine			= DECODER_TYPE_DXVA_2;
	m_pDirectXVideoDec	= pDirectXVideoDec;
	memcpy (&m_DXVA2Config, pDXVA2Config, sizeof(DXVA2_ConfigPictureDecode));

	Init (pFilter, nPicEntryNumber);
};

void CDXVADecoder::Init(CXBMCVideoDecFilter* pFilter, int nPicEntryNumber)
{
  m_references = 0;
  m_refs = 0;
  m_buffer_age = 0;
  m_context          = (dxva_context*)calloc(1, sizeof(dxva_context));
  //m_context->decoder = (dxva_decoder_context*)calloc(1, sizeof(dxva_decoder_context));
  m_context->cfg = (DXVA2_ConfigPictureDecode*)calloc(1, sizeof(DXVA2_ConfigPictureDecode));
  //m_context->exec.pCompressedBuffers = (DXVA2_DecodeBufferDesc*)calloc(6, sizeof(DXVA2_DecodeBufferDesc));
  //m_context->exec.pExtensionData = (DXVA2_DecodeExtensionData*)calloc(6, sizeof(DXVA2_DecodeExtensionData));
  m_context->surface = (IDirect3DSurface9**)calloc(m_buffer_max, sizeof(IDirect3DSurface9*));

	m_pFilter			= pFilter;
	m_nPicEntryNumber	= nPicEntryNumber;
	//m_pPictureStore		= DNew PICTURE_STORE[nPicEntryNumber];
	m_dwNumBuffersInfo	= 0;

	memset (&m_DXVA1Config, 0, sizeof(m_DXVA1Config));
	memset (&m_DXVA1BufferDesc, 0, sizeof(m_DXVA1BufferDesc));
	m_DXVA1Config.guidConfigBitstreamEncryption	= DXVA_NoEncrypt;
	m_DXVA1Config.guidConfigMBcontrolEncryption	= DXVA_NoEncrypt;
	m_DXVA1Config.guidConfigResidDiffEncryption	= DXVA_NoEncrypt;
	m_DXVA1Config.bConfigBitstreamRaw			= 2;

	memset (&m_DXVA1BufferInfo, 0, sizeof(m_DXVA1BufferInfo));
	memset (&m_ExecuteParams, 0, sizeof(m_ExecuteParams));
	Flush();
}

bool CDXVADecoder::ConfigureDXVA1()
{
	HRESULT							hr = S_FALSE;
	DXVA_ConfigPictureDecode		ConfigRequested;

	if (m_pAMVideoAccelerator)
	{
		memset (&ConfigRequested, 0, sizeof(ConfigRequested));
		ConfigRequested.guidConfigBitstreamEncryption	= DXVA_NoEncrypt;
		ConfigRequested.guidConfigMBcontrolEncryption	= DXVA_NoEncrypt;
		ConfigRequested.guidConfigResidDiffEncryption	= DXVA_NoEncrypt;
		ConfigRequested.bConfigBitstreamRaw				= 2;

		writeDXVA_QueryOrReplyFunc (&ConfigRequested.dwFunction, DXVA_QUERYORREPLYFUNCFLAG_DECODER_PROBE_QUERY, DXVA_PICTURE_DECODING_FUNCTION);
		hr = m_pAMVideoAccelerator->Execute (ConfigRequested.dwFunction, &ConfigRequested, sizeof(DXVA_ConfigPictureDecode), &m_DXVA1Config, sizeof(DXVA_ConfigPictureDecode), 0, NULL);

		// Copy to DXVA2 structure (simplify code based on accelerator config)
		m_DXVA2Config.guidConfigBitstreamEncryption	= m_DXVA1Config.guidConfigBitstreamEncryption;
		m_DXVA2Config.guidConfigMBcontrolEncryption	= m_DXVA1Config.guidConfigMBcontrolEncryption;
		m_DXVA2Config.guidConfigResidDiffEncryption	= m_DXVA1Config.guidConfigResidDiffEncryption;
		m_DXVA2Config.ConfigBitstreamRaw			= m_DXVA1Config.bConfigBitstreamRaw;
		m_DXVA2Config.ConfigMBcontrolRasterOrder	= m_DXVA1Config.bConfigMBcontrolRasterOrder;
		m_DXVA2Config.ConfigResidDiffHost			= m_DXVA1Config.bConfigResidDiffHost;
		m_DXVA2Config.ConfigSpatialResid8			= m_DXVA1Config.bConfigSpatialResid8;
		m_DXVA2Config.ConfigResid8Subtraction		= m_DXVA1Config.bConfigResid8Subtraction;
		m_DXVA2Config.ConfigSpatialHost8or9Clipping	= m_DXVA1Config.bConfigSpatialHost8or9Clipping;
		m_DXVA2Config.ConfigSpatialResidInterleaved	= m_DXVA1Config.bConfigSpatialResidInterleaved;
		m_DXVA2Config.ConfigIntraResidUnsigned		= m_DXVA1Config.bConfigIntraResidUnsigned;
		m_DXVA2Config.ConfigResidDiffAccelerator	= m_DXVA1Config.bConfigResidDiffAccelerator;
		m_DXVA2Config.ConfigHostInverseScan			= m_DXVA1Config.bConfigHostInverseScan;
		m_DXVA2Config.ConfigSpecificIDCT			= m_DXVA1Config.bConfigSpecificIDCT;
		m_DXVA2Config.Config4GroupedCoefs			= m_DXVA1Config.bConfig4GroupedCoefs;

		if (SUCCEEDED (hr))
		{
			writeDXVA_QueryOrReplyFunc (&m_DXVA1Config.dwFunction, DXVA_QUERYORREPLYFUNCFLAG_DECODER_LOCK_QUERY, DXVA_PICTURE_DECODING_FUNCTION);
			hr = m_pAMVideoAccelerator->Execute (m_DXVA1Config.dwFunction, &m_DXVA1Config, sizeof(DXVA_ConfigPictureDecode), &ConfigRequested, sizeof(DXVA_ConfigPictureDecode), 0, NULL);

			// TODO : check config!
//			ASSERT (ConfigRequested.bConfigBitstreamRaw == 2);

			AMVAUncompDataInfo		DataInfo;
			DWORD					dwNum = COMP_BUFFER_COUNT;
			DataInfo.dwUncompWidth	= m_pFilter->PictWidthRounded();
			DataInfo.dwUncompHeight	= m_pFilter->PictHeightRounded();
			memcpy (&DataInfo.ddUncompPixelFormat, m_pFilter->GetPixelFormat(), sizeof (DDPIXELFORMAT));
			hr = m_pAMVideoAccelerator->GetCompBufferInfo (m_pFilter->GetDXVADecoderGuid(), &DataInfo, &dwNum, m_ComBufferInfo);
		}
    else
    {
      CLog::Log(LOGERROR,"%s config dxva1 failed",__FUNCTION__);
      return false;
    }
	}
	return true;
}

CDXVADecoder* CDXVADecoder::Acquire()
{
  InterlockedIncrement(&m_references);
  return this;
}

long CDXVADecoder::Release()
{
  long count = InterlockedDecrement(&m_references);
  ASSERT(count >= 0);
  if (count == 0) delete this;
  return count;
}

CDXVADecoder::~CDXVADecoder()
{
  Close();
  free(m_context->surface);
  free(m_context);
}

void CDXVADecoder::Close()
{
  CSingleLock lock(m_section);
  for(unsigned i = 0; i < m_refs; i++)
    m_buffer[i].Clear();
  m_refs = 0;
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

void CDXVADecoder::SetSurfaceArray(UINT nNumRenderTargets, IDirect3DSurface9 **pDecoderRenderTargets)
{
  m_context->surface = pDecoderRenderTargets;
  m_context->surface_count = nNumRenderTargets;
}

bool CDXVADecoder::Open(AVCodecContext *avctx, enum PixelFormat fmt)
{
  CSingleLock lock(m_section);
  Close();

  //Dvdplayer use this but it is better?
  /*if(avctx->refs > m_refs)
    m_refs = avctx->refs;*/
  
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
    m_buffer[i].Init(i);
    
  }

  m_pFilter = (CXBMCVideoDecFilter*)avctx->opaque;
  
  avctx->get_buffer      = GetBufferS;
  avctx->release_buffer  = RelBufferS;
  m_context->cfg = &m_DXVA2Config;
  
  if (m_pFilter->UseDXVA2())
  {
    m_context->decoder = m_pDirectXVideoDec;
    for(int i = 0; i < m_context->surface_count; i++)
    {
      m_buffer[i].surface = m_context->surface[i];
    }
  }
  else
  {
    //Create the dummy surface that will be used to follow the id of the surface
    CreateDummySurface();
    m_pIDXVideoDecoder = new CDXDecWrapper(m_pAMVideoAccelerator, m_refs, (void**)m_context->surface);
    m_context->decoder = m_pIDXVideoDecoder;
  }

  
  avctx->hwaccel_context = m_context;
  return true;
}

void CDXVADecoder::CreateDummySurface()
{
  //m_context->surface_count = m_refs + 1 + 1 + m_nPicEntryNumber; // refs + 1 decode + 1 libavcodec safety + processor buffer
  m_context->surface_count = m_nPicEntryNumber;
HRESULT hr;
  
    CLog::Log(LOGDEBUG, "DXVA - allocating %d surfaces", m_context->surface_count - m_refs);
    for(int i = 0; i < m_context->surface_count; i++)
    {
      
      IDirect3DTexture9* pTexture;
      hr = g_Windowing.Get3DDevice()->CreateTexture(
        640, 480, 1, 
        D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, 
        D3DPOOL_DEFAULT, &pTexture, NULL);

      hr = pTexture->GetSurfaceLevel(0, &m_context->surface[i]);

      m_buffer[i].surface = m_context->surface[i];
    }
     
  

}

bool CDXVADecoder::Flush()
{
  /*CSingleLock lock(m_section);
  for(unsigned i = 0; i < m_buffer_count; i++)
    m_buffer[i].Init(i);*/
  return true;
}
int CDXVADecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
 return 0;
}

/*bool CDXVADecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, directshow_dxva_h264* picture)
{
  CSingleLock lock(m_section);
  
  picture = (directshow_dxva_h264*)avctx->coded_frame->data[0];
  return true;
}*/

int CDXVADecoder::Check(AVCodecContext* avctx)
{
  return 0;

}


bool CDXVADecoder::Supports(enum PixelFormat fmt)
{
  if(fmt == PIX_FMT_DXVA2_VLD)
    return true;
  return false;
}

int CDXVADecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::DebugLog("%s",__FUNCTION__);
  CXBMCVideoDecFilter* ctx = (CXBMCVideoDecFilter*)avctx->opaque;
  CDXVADecoder* dec        = (CDXVADecoder*)ctx->GetDXVADecoder();

  int           count = 0;
  SVideoBuffer* buf   = NULL;
  /*TODO verify if there any verification to do before touching the buffer*/
  for(unsigned i = 0; i < m_context->surface_count ; i++)
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
  buf->rt_start = pic->reordered_opaque = avctx->reordered_opaque;
  buf->rt_stop = buf->rt_start + (ctx->GetAvrTimePerFrame()/10);
  pic->type = FF_BUFFER_TYPE_USER;
  pic->age = INT_MAX; //According to ffmpeg api it should be initialized with this value
  //What is this i forgot :S
  //buf->surface->decoder_surface_index = buf->surface_index;
  for(unsigned i = 0; i < 4; i++)
  {
    pic->data[i] = NULL;
    pic->linesize[i] = 0;
  }

  pic->data[0] = (uint8_t*)buf->surface;
  pic->data[3] = (uint8_t*)buf->surface;
  buf->used = true;

  
  return 0;
}

void CDXVADecoder::RelBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  /*CSingleLock lock(m_section);*/
  /*CLog::DebugLog("%s",__FUNCTION__);*/
  IDirect3DSurface9* surface = (IDirect3DSurface9*)pic->data[3];

  for(unsigned i = 0; i < m_context->surface_count; i++)
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

HRESULT CDXVADecoder::FindFreeDXVA1Buffer(DWORD dwTypeIndex, DWORD& dwBufferIndex)
{
	HRESULT		hr;
	int			nTry;

	dwBufferIndex	= 0; //(dwBufferIndex + 1) % m_ComBufferInfo[DXVA_PICTURE_DECODE_BUFFER].dwNumCompBuffers;

	DO_DXVA_PENDING_LOOP (m_pAMVideoAccelerator->QueryRenderStatus ((DWORD)-1, dwBufferIndex, 0));

	return hr;
}

int CDXVADecoder::DXVABeginFrame(dxva_context *ctx, unsigned index)
{
#if 0
  /*CSingleLock lock(m_section);*/
  CDXVADecoder* dec        = (CDXVADecoder*)ctx->decoder->dxvadecoder;
  IAMVideoAccelerator* acc = dec->GetIAMVideoAccelerator();
  HRESULT				hr   = E_INVALIDARG;

  int					nTry = 0;
  
	for (int i=0; i<20; i++)
	{
    if (ctx->decoder->type == DECODER_TYPE_DXVA_1)
    {
      AMVABeginFrameInfo BeginFrameInfo;
      BeginFrameInfo.dwDestSurfaceIndex	= index;
      BeginFrameInfo.dwSizeInputData		= sizeof(index);
      BeginFrameInfo.pInputData			= &index;
      BeginFrameInfo.dwSizeOutputData		= 0;
      BeginFrameInfo.pOutputData			= NULL;
      DO_DXVA_PENDING_LOOP(acc->BeginFrame(&BeginFrameInfo));
      
      if (SUCCEEDED (hr))
      {
			  //hr = FindFreeDXVA1Buffer ((DWORD)-1, m_dwBufferIndex);
        DWORD dwBufferIndex;
        m_dwBufferIndex = dwBufferIndex = 0;
        DO_DXVA_PENDING_LOOP (acc->QueryRenderStatus ((DWORD)-1, dwBufferIndex, 0));
        dec->m_pFilter->SetCurrentBufferIndex(index);
        ASSERT(dwBufferIndex == 0);
        break;

      }
      else
      {
      ASSERT(0);
      }
    }
    else if (ctx->decoder->type == DECODER_TYPE_DXVA_2)
    {
    
    }

    if (SUCCEEDED (hr)) 
      break;
		Sleep(1);
  }
  if (FAILED(hr))
    return -1;
  else
#endif
  return 0;

}

int CDXVADecoder::DXVAEndFrame(dxva_context *ctx, unsigned index)
{
#if 0
  /*CSingleLock lock(m_section);*/
  CDXVADecoder* dec        = (CDXVADecoder*)ctx->decoder->dxvadecoder;
  IAMVideoAccelerator* acc = dec->GetIAMVideoAccelerator();
  HRESULT				hr   = E_INVALIDARG;
  DWORD		dwDummy	= index;
  AMVAEndFrameInfo EndFrameInfo;
  EndFrameInfo.dwSizeMiscData	= sizeof(dwDummy);
  EndFrameInfo.pMiscData = &dwDummy;


  hr = acc->EndFrame(&EndFrameInfo);
  if (SUCCEEDED (hr))
    return 0;
  else
#endif
    return -1;
  

}
inline DWORD GetDXVA1CompressedType (DWORD dwDXVA2CompressedType)
{
	if (dwDXVA2CompressedType <= DXVA2_BitStreamDateBufferType)
		return dwDXVA2CompressedType + 1;
	else
	{
		switch (dwDXVA2CompressedType)
		{
		case DXVA2_MotionVectorBuffer :
			return DXVA_MOTION_VECTOR_BUFFER;
			break;
		case DXVA2_FilmGrainBuffer :
			return DXVA_FILM_GRAIN_BUFFER;
			break;
		default :
			ASSERT (FALSE);
			return DXVA_COMPBUFFER_TYPE_THAT_IS_NOT_USED;
		}
	}
}
int CDXVADecoder::DXVAGetBuffer(dxva_context *ctx, unsigned type, void **dxva_data, unsigned *dxva_size)
{
#if 0
  //CSingleLock lock(m_section);
  CDXVADecoder* dec        = (CDXVADecoder*)ctx->decoder->dxvadecoder;
  IAMVideoAccelerator* acc = dec->GetIAMVideoAccelerator();
  HRESULT hr;
  LONG		lStride;
  void *data;
  DWORD bufindex = dec->GetCurrentBufferIndex();
  unsigned dxva1type = GetDXVA1CompressedType(type);
  hr = acc->GetBuffer(dxva1type, bufindex, FALSE, (void**)&data, &lStride);
  *dxva_size = dec->GetBufferSize(dxva1type);
  //TODO set dxva_size
  *dxva_data = data;
#endif
  return 0;
}

int CDXVADecoder::DXVAExecute(dxva_context *ctx)
{
#if 0
  /*CSingleLock lock(m_section);*/
  HRESULT hr = E_INVALIDARG;
  CDXVADecoder* dec        = (CDXVADecoder*)ctx->decoder->dxvadecoder;
  IAMVideoAccelerator* acc = dec->GetIAMVideoAccelerator();
  const int count = ctx->exec.NumCompBuffers;

  AMVABUFFERINFO info[MAX_COM_BUFFER];
  DXVA_BufferDescription dsc[MAX_COM_BUFFER];
  for (int i = 0; i < count; i++) 
  {
    //const DXVA2_DecodeBufferDesc *b = &ctx->exec;
    dsc[i].dwTypeIndex      = GetDXVA1CompressedType(ctx->exec.pCompressedBuffers[i].CompressedBufferType);
    dsc[i].dwBufferIndex    = 0;
    dsc[i].dwDataOffset     = ctx->exec.pCompressedBuffers[i].DataOffset;
    dsc[i].dwDataSize       = ctx->exec.pCompressedBuffers[i].DataSize;
    dsc[i].dwFirstMBaddress = ctx->exec.pCompressedBuffers[i].FirstMBaddress;
    dsc[i].dwNumMBsInBuffer = ctx->exec.pCompressedBuffers[i].NumMBsInBuffer;
    dsc[i].dwWidth          = ctx->exec.pCompressedBuffers[i].Width;
    dsc[i].dwHeight         = ctx->exec.pCompressedBuffers[i].Height;
    dsc[i].dwStride         = ctx->exec.pCompressedBuffers[i].Stride;
    dsc[i].dwReservedBits   = ctx->exec.pCompressedBuffers[i].ReservedBits;

    info[i].dwTypeIndex   = GetDXVA1CompressedType(ctx->exec.pCompressedBuffers[i].CompressedBufferType);
    info[i].dwBufferIndex = 0;
    info[i].dwDataOffset  = ctx->exec.pCompressedBuffers[i].DataOffset;
    info[i].dwDataSize    = ctx->exec.pCompressedBuffers[i].DataSize;
  }
  DWORD function = 0x01000000;
  DWORD result;
  hr = acc->Execute(function, dsc, sizeof(*dsc) * count,&result, sizeof(result),count, info);
  if (FAILED(hr))
    ASSERT(0);
  for (int i = 0; i < count; i++) 
  {
    hr = acc->ReleaseBuffer (info[i].dwTypeIndex, info[i].dwBufferIndex);
    if (FAILED(hr))
      ASSERT(0);
  }
  ctx->exec.NumCompBuffers = 0;
#endif
  return 0;
}



int CDXVADecoder::DXVAReleaseBuffer(dxva_context *ctx, unsigned type)
{
#if 0
  //CSingleLock lock(m_section);
  HRESULT hr = E_INVALIDARG;
  CDXVADecoder* dec        = (CDXVADecoder*)ctx->decoder->dxvadecoder;
  IAMVideoAccelerator* acc = dec->GetIAMVideoAccelerator();
  int nTry = 0;
  for (int i=0; i<20; i++)
	{
  DO_DXVA_PENDING_LOOP (acc->ReleaseBuffer(GetDXVA1CompressedType(type), dec->GetCurrentBufferIndex()));
    if (SUCCEEDED (hr))
    {
      return 0;
    }
    else
      return -1;
  }
#endif
return 0;
}

void CDXVADecoder::SetTypeSpecificFlags(IMediaSample* pMS,FF_SLICE_TYPE slice_type, FF_FIELD_TYPE field_type)
{
  if(Com::SmartQIPtr<IMediaSample2> pMS2 = pMS)
	{
		AM_SAMPLE2_PROPERTIES props;
		if(SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props)))
		{
			props.dwTypeSpecificFlags &= ~0x7f;

			if(field_type == PICT_FRAME)
				props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_WEAVE;
			else
			{
				if(field_type == PICT_TOP_FIELD)
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_FIELD1FIRST;
				//if(m_fb.flags & PIC_FLAG_REPEAT_FIRST_FIELD)
				//	props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_REPEAT_FIELD;
			}

			switch (slice_type)
			{
			case I_TYPE :
			case SI_TYPE :
				props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_I_SAMPLE;
				break;
			case P_TYPE :
			case SP_TYPE :
				props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_P_SAMPLE;
				break;
			default :
				props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_B_SAMPLE;
				break;
			}

			pMS2->SetProperties(sizeof(props), (BYTE*)&props);
		}
	}
  REFERENCE_TIME rtstart,rtend;
  HRESULT hr;
  pMS->GetTime(&rtstart,&rtend);
	hr = pMS->SetTime(&rtstart,&rtend);
}


bool CDXVADecoder::DXVADisplayFrame(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
/*  CDXVADecoder* dec        = (CDXVADecoder*)ctx->decoder->dxvadecoder;
  REFERENCE_TIME starttime = (avctx->reordered_opaque * 10);
  REFERENCE_TIME frame_length_avg = dec->m_pFilter->m_rtAvrTimePerFrame;
  Com::SmartPtr<IMediaSample>		pSampleToDeliver;
  if (GetDeliveryBuffer(starttime, frame_length_avg, &pSampleToDeliver)
  {
  
  }*/
return true;
}

bool CDXVADecoder::GetDeliveryBuffer(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, IMediaSample** ppSampleToDeliver)
{
	HRESULT					hr;
  Com::SmartPtr<IMediaSample>	pNewSample;

	// Change aspect ratio for DXVA2
	if (m_nEngine == DECODER_TYPE_DXVA_2)
	{
		m_pFilter->UpdateAspectRatio();
		m_pFilter->ReconnectOutput(m_pFilter->PictWidthRounded(), m_pFilter->PictHeightRounded(), true, m_pFilter->PictWidth(), m_pFilter->PictHeight());
	}
	hr		= m_pFilter->GetOutputPin()->GetDeliveryBuffer(&pNewSample, 0, 0, 0);

	if (SUCCEEDED (hr))
	{
		pNewSample->SetTime(&rtStart, &rtStop);
		pNewSample->SetMediaTime(NULL, NULL);
		*ppSampleToDeliver = pNewSample.Detach();
	}
	return (SUCCEEDED(hr));
}


#endif