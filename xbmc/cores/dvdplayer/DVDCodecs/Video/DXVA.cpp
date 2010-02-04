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

#include <windows.h>
#include <d3d9.h>
#include <Initguid.h>
#include <dxva.h>
#include <dxva2api.h>
#include "libavcodec/dxva2.h"

#include "DXVA.h"
#include "WindowingFactory.h"
#include "Settings.h"

using namespace DXVA;

typedef HRESULT (__stdcall *DXVA2CreateVideoServicePtr)(IDirect3DDevice9* pDD, REFIID riid, void** ppService);
static DXVA2CreateVideoServicePtr g_DXVA2CreateVideoService;

static bool LoadDXVA()
{
  static CCriticalSection g_section;
  static HMODULE          g_handle;

  CSingleLock lock(g_section);
  if(g_handle == NULL)
    g_handle = LoadLibraryEx("dxva2.dll", NULL, 0);
  if(g_handle == NULL)
    return false;
  g_DXVA2CreateVideoService = (DXVA2CreateVideoServicePtr)GetProcAddress(g_handle, "DXVA2CreateVideoService");
  if(g_DXVA2CreateVideoService == NULL)
    return false;
  return true;
}

static void RelBufferS(AVCodecContext *avctx, AVFrame *pic)
{ ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->RelBuffer(avctx, pic); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic) 
{  return ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->GetBuffer(avctx, pic); }


DEFINE_GUID(DXVADDI_Intel_ModeH264_A, 0x604F8E64,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_C, 0x664F8E66,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E, 0x664F8E68,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeVC1_E , 0xBCC5DB6D,0xA2B6,0x4AF0,0xAC,0xE4,0xAD,0xB1,0xF7,0x87,0xBC,0x89);

typedef struct {
    const char   *name;
    const GUID   *guid;
    int          codec;
} dxva2_mode_t;

/* XXX Prefered modes must come first */
static const dxva2_mode_t dxva2_modes[] = {
    { "MPEG2 VLD",    &DXVA2_ModeMPEG2_VLD,     0 },
    { "MPEG2 MoComp", &DXVA2_ModeMPEG2_MoComp,  0 },
    { "MPEG2 IDCT",   &DXVA2_ModeMPEG2_IDCT,    0 },

    { "H.264 variable-length decoder (VLD), FGT",               &DXVA2_ModeH264_F, CODEC_ID_H264 },
    { "H.264 VLD, no FGT",                                      &DXVA2_ModeH264_E, CODEC_ID_H264 },
    { "H.264 IDCT, FGT",                                        &DXVA2_ModeH264_D, 0,            },
    { "H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVA2_ModeH264_C, 0,            },
    { "H.264 MoComp, FGT",                                      &DXVA2_ModeH264_B, 0,            },
    { "H.264 motion compensation (MoComp), no FGT",             &DXVA2_ModeH264_A, 0,            },

    { "Intel H.264 VLD, no FGT",                                      &DXVADDI_Intel_ModeH264_E, CODEC_ID_H264 },
    { "Intel H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVADDI_Intel_ModeH264_C, 0,            },
    { "Intel H.264 motion compensation (MoComp), no FGT",             &DXVADDI_Intel_ModeH264_A, 0,            },

    { "Windows Media Video 8 MoComp",           &DXVA2_ModeWMV8_B, 0 },
    { "Windows Media Video 8 post processing",  &DXVA2_ModeWMV8_A, 0 },

    { "Windows Media Video 9 IDCT",             &DXVA2_ModeWMV9_C, 0 },
    { "Windows Media Video 9 MoComp",           &DXVA2_ModeWMV9_B, 0 },
    { "Windows Media Video 9 post processing",  &DXVA2_ModeWMV9_A, 0 },

    { "VC-1 VLD",             &DXVA2_ModeVC1_D, CODEC_ID_VC1 },
    { "VC-1 VLD",             &DXVA2_ModeVC1_D, CODEC_ID_WMV3 },
    { "VC-1 IDCT",            &DXVA2_ModeVC1_C, 0 },
    { "VC-1 MoComp",          &DXVA2_ModeVC1_B, 0 },
    { "VC-1 post processing", &DXVA2_ModeVC1_A, 0 },

    { "Intel VC-1 VLD",       &DXVADDI_Intel_ModeVC1_E, CODEC_ID_VC1 },
    { "Intel VC-1 VLD",       &DXVADDI_Intel_ModeVC1_E, CODEC_ID_WMV3 },

    { NULL, NULL, 0 }
};

static const dxva2_mode_t *dxva2_find(const GUID *guid)
{
    for (unsigned i = 0; dxva2_modes[i].name; i++) {
        if (IsEqualGUID(*dxva2_modes[i].guid, *guid))
            return &dxva2_modes[i];
    }
    return NULL;
}

static inline unsigned dxva2_align(unsigned value)
{
  // untill somebody gives me a sample where this is required
  // i'd rather not do this alignment as it will mess with
  // output not being cropped correctly
#if 0
  return (value + 15) & ~15;
#else
  return value;
#endif
}


CDecoder::SVideoBuffer::SVideoBuffer()
{
  surface = NULL;
  Clear();
}

CDecoder::SVideoBuffer::~SVideoBuffer()
{
  Clear();
}

void CDecoder::SVideoBuffer::Clear()
{
  SAFE_RELEASE(surface);
  age     = 0;
  used    = 0;
}

CDecoder::CDecoder()
 : m_event(true)
{
  m_event.Set();
  m_state     = DXVA_OPEN;
  m_service   = NULL;
  m_device    = NULL;
  m_decoder   = NULL;
  m_processor = NULL;
  m_buffer_count = 0;
  memset(&m_format, 0, sizeof(m_format));
  m_context          = (dxva_context*)calloc(1, sizeof(dxva_context));
  m_context->cfg     = (DXVA2_ConfigPictureDecode*)calloc(1, sizeof(DXVA2_ConfigPictureDecode));
  m_context->surface = (IDirect3DSurface9**)calloc(m_buffer_max, sizeof(IDirect3DSurface9*));
  g_Windowing.Register(this);
}

CDecoder::~CDecoder()
{
  g_Windowing.Unregister(this);
  Close();
  free(m_context->surface);
  free(const_cast<DXVA2_ConfigPictureDecode*>(m_context->cfg)); // yes this is foobar
  free(m_context);
}

void CDecoder::Close()
{
  CSingleLock lock(m_section);
  SAFE_RELEASE(m_decoder);
  SAFE_RELEASE(m_service);
  SAFE_RELEASE(m_processor);
  for(unsigned i = 0; i < m_buffer_count; i++)
    m_buffer[i].Clear();
  m_buffer_count = 0;
  memset(&m_format, 0, sizeof(m_format));
  CProcessor* proc = m_processor;
  m_processor = NULL;
  lock.Leave();

  if(proc)
  {
    CSingleExit leave(m_section);
    proc->Release();
  }
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
  if(!LoadDXVA())
    return false;

  CSingleLock lock(m_section);
  Close();

  if(m_state == DXVA_LOST)
  {
    CLog::Log(LOGDEBUG, "DXVA - device is in lost state, we can't start");
    return false;
  }

  CHECK(g_DXVA2CreateVideoService(g_Windowing.Get3DDevice(), IID_IDirectXVideoDecoderService, (void**)&m_service))

  UINT  input_count;
  GUID *input_list;

  CHECK(m_service->GetDecoderDeviceGuids(&input_count, &input_list))

  for(unsigned i = 0; i < input_count; i++)
  {
    const GUID *g            = &input_list[i];
    const dxva2_mode_t *mode = dxva2_find(g);
    if(mode)
      CLog::Log(LOGDEBUG, "DXVA - supports '%s'", mode->name);
    else
      CLog::Log(LOGDEBUG, "DXVA - supports %08X-%04x-%04x-XXXX\n"
                        , g->Data1
                        , g->Data2
                        , g->Data3);
  }

  m_format.Format = D3DFMT_UNKNOWN;
  for(const dxva2_mode_t* mode = dxva2_modes; mode->name; mode++)
  {
    if(mode->codec != avctx->codec_id)
      continue;

    for(unsigned j = 0; j < input_count; j++)
    {
      if(!IsEqualGUID(input_list[j], *mode->guid))
        continue;

      CLog::Log(LOGDEBUG, "DXVA - trying '%s'", mode->name);
      if(OpenTarget(input_list[j]))
      {
        m_input = input_list[j];
        break;
      }
    }
  }
  CoTaskMemFree(input_list);

  if(m_format.Format == D3DFMT_UNKNOWN)
  {
    CLog::Log(LOGDEBUG, "DXVA - unable to find an input/output format combination");
    return false;
  }

  if(!OpenDecoder(avctx))
    return false;

  m_state = DXVA_OPEN;

  { CSingleExit leave(m_section);
    CProcessor* processor = new CProcessor();
    leave.Restore();
    m_processor = processor;
  }

  if(m_state == DXVA_RESET)
  {
    CLog::Log(LOGDEBUG, "DXVA - decoder was reset while trying to create a processor, retrying");
    if(!Open(avctx, fmt))
      return false;
  }

  if(m_state == DXVA_LOST)
  {
    CLog::Log(LOGERROR, "DXVA - device was lost while trying to create a processor");
    return false;
  }

  if(!m_processor->Open(m_format, 4))
    return false;

  avctx->get_buffer      = GetBufferS;
  avctx->release_buffer  = RelBufferS;
  avctx->hwaccel_context = m_context;

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
    for(unsigned i = 0; i < m_buffer_count; i++)
    {
      if(m_buffer[i].surface == (IDirect3DSurface9*)frame->data[3])
        return VC_BUFFER | VC_PICTURE;
    }
    CLog::Log(LOGWARNING, "DXVA - ignoring invalid surface");
    return VC_BUFFER;
  }
  else
    return 0;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  CSingleLock lock(m_section);
  picture->format = DVDVideoPicture::FMT_DXVA;
  picture->proc    = m_processor;
  picture->proc_id = m_processor->Add((IDirect3DSurface9*)frame->data[3]);
  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  CSingleLock lock(m_section);

  if(m_state == DXVA_RESET)
    Close();

  if(m_state == DXVA_LOST)
  {
    Close();
    lock.Leave();
    m_event.WaitMSec(2000);
    lock.Enter();
    if(m_state == DXVA_LOST)
    {
      CLog::Log(LOGERROR, "CDecoder::Check - device didn't reset in reasonable time");
      return VC_ERROR;
    }
  }

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

  if(avctx->codec_id != CODEC_ID_H264
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
  data.Function = 7;
  data.pPrivateOutputData    = &status;
  data.PrivateOutputDataSize = sizeof(status);
  if(FAILED(m_decoder->Execute(&params)))
  {
    CLog::Log(LOGWARNING, "DXVA - failed to get decoder status");
    return VC_ERROR;
  }

  if(avctx->codec_id == CODEC_ID_H264)
  {
    if(status.h264.bStatus)
      CLog::Log(LOGWARNING, "DXVA - decoder problem of status %d with %d", status.h264.bStatus, status.h264.bBufType);
  }
  else if(avctx->codec_id == CODEC_ID_VC1
       || avctx->codec_id == CODEC_ID_WMV3)
  {
    if(status.vc1.bStatus)
      CLog::Log(LOGWARNING, "DXVA - decoder problem of status %d with %d", status.h264.bStatus, status.vc1.bBufType);
  }
  return 0;
}

bool CDecoder::OpenTarget(const GUID &guid)
{
  UINT       output_count = 0;
  D3DFORMAT *output_list  = NULL;
  CHECK(m_service->GetDecoderRenderTargets(guid, &output_count, &output_list))

  for(unsigned k = 0; k < output_count; k++)
  {
    if(output_list[k] == MAKEFOURCC('Y','V','1','2')
    || output_list[k] == MAKEFOURCC('N','V','1','2'))
    {
      m_format.Format = output_list[k];
      CoTaskMemFree(output_list);
      return true;
    }
  }
  CoTaskMemFree(output_list);
  return false;
}

bool CDecoder::OpenDecoder(AVCodecContext *avctx)
{
  m_format.SampleWidth  = dxva2_align(avctx->width);
  m_format.SampleHeight = dxva2_align(avctx->height);
  m_format.SampleFormat.SampleFormat           = DXVA2_SampleProgressiveFrame;
  m_format.SampleFormat.VideoLighting          = DXVA2_VideoLighting_dim;

  if     (avctx->color_range == AVCOL_RANGE_JPEG)
    m_format.SampleFormat.NominalRange = DXVA2_NominalRange_0_255;
  else if(avctx->color_range == AVCOL_RANGE_MPEG)
    m_format.SampleFormat.NominalRange = DXVA2_NominalRange_16_235;
  else
    m_format.SampleFormat.NominalRange = DXVA2_NominalRange_Unknown;

  switch(avctx->chroma_sample_location)
  {
    case AVCHROMA_LOC_LEFT:
      m_format.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Horizontally_Cosited 
                                                   | DXVA2_VideoChromaSubsampling_Vertically_AlignedChromaPlanes;
      break;
    case AVCHROMA_LOC_CENTER:
      m_format.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Vertically_AlignedChromaPlanes;
      break;
    case AVCHROMA_LOC_TOPLEFT:
      m_format.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Horizontally_Cosited 
                                                   | DXVA2_VideoChromaSubsampling_Vertically_Cosited;
      break;
    default:
      m_format.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Unknown;      
  }

  switch(avctx->colorspace)
  {
    case AVCOL_SPC_BT709:
      m_format.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT709;
      break;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
      m_format.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT601;
      break;
    case AVCOL_SPC_SMPTE240M:
      m_format.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_SMPTE240M;
      break;
    case AVCOL_SPC_FCC:
    case AVCOL_SPC_UNSPECIFIED:
    case AVCOL_SPC_RGB:
    default:
      m_format.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_Unknown;
  }

  switch(avctx->color_primaries)
  {
    case AVCOL_PRI_BT709:
      m_format.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_BT709;
      break;
    case AVCOL_PRI_BT470M:
      m_format.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_BT470_2_SysM;
      break;
    case AVCOL_PRI_BT470BG:
      m_format.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_BT470_2_SysBG;
      break;
    case AVCOL_PRI_SMPTE170M:
      m_format.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_SMPTE170M;
      break;
    case AVCOL_PRI_SMPTE240M:
      m_format.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_SMPTE240M;
      break;
    case AVCOL_PRI_FILM:
    case AVCOL_PRI_UNSPECIFIED:
    default:
      m_format.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_Unknown;
  }

  switch(avctx->color_trc)
  {
    case AVCOL_TRC_BT709:
      m_format.SampleFormat.VideoTransferFunction = DXVA2_VideoTransFunc_709;
      break;
    case AVCOL_TRC_GAMMA22:
      m_format.SampleFormat.VideoTransferFunction = DXVA2_VideoTransFunc_22;
      break;
    case AVCOL_TRC_GAMMA28:
      m_format.SampleFormat.VideoTransferFunction = DXVA2_VideoTransFunc_28;
      break;
    default:
      m_format.SampleFormat.VideoTransferFunction = DXVA2_VideoTransFunc_Unknown;
  }

  if (avctx->time_base.den > 0 && avctx->time_base.num > 0)
  {
    m_format.InputSampleFreq.Numerator   = avctx->time_base.num;
    m_format.InputSampleFreq.Denominator = avctx->time_base.den;
  } 
  m_format.OutputFrameFreq = m_format.InputSampleFreq;
  m_format.UABProtectionLevel = FALSE;
  m_format.Reserved = 0;

  if(avctx->codec_id == CODEC_ID_H264)
    m_context->surface_count = 16 + 1 + 1; // 16 ref + 1 decode + 1 libavcodec safety
  else
    m_context->surface_count = 2  + 1 + 1; // 2  ref + 1 decode + 1 libavcodec safety

  CHECK(m_service->CreateSurface( m_format.SampleWidth
                                , m_format.SampleHeight
                                , m_context->surface_count - 1
                                , m_format.Format
                                , D3DPOOL_DEFAULT
                                , 0
                                , DXVA2_VideoDecoderRenderTarget
                                , m_context->surface, NULL ));

  m_buffer_count = m_context->surface_count;
  m_buffer_age   = 0;
  for(unsigned i = 0; i < m_buffer_count; i++)
    m_buffer[i].surface = m_context->surface[i];

  UINT                       cfg_count = 0;
  DXVA2_ConfigPictureDecode *cfg_list  = NULL;
  CHECK(m_service->GetDecoderConfigurations(m_input
                                          , &m_format
                                          , NULL
                                          , &cfg_count
                                          , &cfg_list))

  DXVA2_ConfigPictureDecode config = {};
  for(unsigned i = 0; i< cfg_count; i++)
  {
    CLog::Log(LOGDEBUG, "DXVA - bitstream type %d", cfg_list[i].ConfigBitstreamRaw);
    if(config.ConfigBitstreamRaw == 0 && cfg_list[i].ConfigBitstreamRaw)
      config = cfg_list[i];
    if(config.ConfigBitstreamRaw == 1 && cfg_list[i].ConfigBitstreamRaw == 2)
      config = cfg_list[i];
  }
  CoTaskMemFree(cfg_list);

  if(!config.ConfigBitstreamRaw)
  {
    CLog::Log(LOGDEBUG, "DXVA - failed to find a raw input bitstream");
    return false;
  }

  CHECK(m_service->CreateVideoDecoder(m_input, &m_format, &config
                                    , m_context->surface
                                    , m_context->surface_count
                                    , &m_decoder))

  *const_cast<DXVA2_ConfigPictureDecode*>(m_context->cfg) = config;
  m_context->decoder = m_decoder;

  return true;
}

bool CDecoder::Supports(enum PixelFormat fmt)
{
  if(fmt == PIX_FMT_DXVA2_VLD)
    return true;
  return false;
}

void CDecoder::RelBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CSingleLock lock(m_section);
  IDirect3DSurface9* surface = (IDirect3DSurface9*)pic->data[3];

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

int CDecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CSingleLock lock(m_section);
  if(dxva2_align(avctx->width)  != m_format.SampleWidth
  || dxva2_align(avctx->height) != m_format.SampleHeight)
  {
    Close();
    if(!Open(avctx, avctx->pix_fmt))
    {
      Close();
      return -1;
    }
  }

  SVideoBuffer* buf_old = NULL;
  SVideoBuffer* buf     = NULL;
  for(unsigned i = 0; i < m_buffer_count; i++)
  {
    if(!m_buffer[i].used)
    {
      if(!buf || buf->age > m_buffer[i].age)
        buf = m_buffer+i;
    }

    if(!buf_old || buf_old->age > m_buffer[i].age)
      buf_old = m_buffer+i;
  }

  if(!buf)
  {
    if(buf_old)
      CLog::Log(LOGERROR, "DXVA - unable to find new unused buffer");
    else
    {
      CLog::Log(LOGERROR, "DXVA - unable to find any buffer");    
      return -1;
    }
    buf = buf_old;
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
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//------------------------ PROCESSING SERVICE -------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CProcessor::CProcessor()
{
  m_service = NULL;
  m_process = NULL;
  m_time    = 0;
  m_references = 1;
  g_Windowing.Register(this);
}

CProcessor::~CProcessor()
{
  g_Windowing.Unregister(this);
  ASSERT(m_references == 0);
  Close();
}

void CProcessor::Close()
{
  CSingleLock lock(m_section);
  SAFE_RELEASE(m_process);
  SAFE_RELEASE(m_service);
  for(unsigned i = 0; i < m_sample.size(); i++)
    SAFE_RELEASE(m_sample[i].SrcSurface);
  m_sample.clear();
}


bool CProcessor::Open(const DXVA2_VideoDesc& dsc, unsigned size)
{
  if(!LoadDXVA())
    return false;

  CSingleLock lock(m_section);
  m_desc = dsc;
  m_size = size;

  CHECK(g_DXVA2CreateVideoService(g_Windowing.Get3DDevice(), IID_IDirectXVideoProcessorService, (void**)&m_service));

  GUID*    guid_list;
  unsigned guid_count;
  CHECK(m_service->GetVideoProcessorDeviceGuids(&m_desc, &guid_count, &guid_list));

  if(guid_count == 0)
  {
    CLog::Log(LOGDEBUG, "DXVA - unable to find any processors");
    CoTaskMemFree(guid_list);
    return false;
  }

  m_device = guid_list[0];
  for(unsigned i = 0; i < guid_count; i++)
  {
    GUID* g = &guid_list[i];
    CLog::Log(LOGDEBUG, "DXVA - processor found %08X-%04x-%04x-XXXX\n"
                      , g->Data1
                      , g->Data2
                      , g->Data3);

    if(IsEqualGUID(*g, DXVA2_VideoProcProgressiveDevice))
      m_device = *g;
  }
  CoTaskMemFree(guid_list);

  CLog::Log(LOGDEBUG, "DXVA - processor selected %08X-%04x-%04x-XXXX\n"
                    , m_device.Data1
                    , m_device.Data2
                    , m_device.Data3);


  D3DFORMAT output = D3DFMT_X8R8G8B8;

  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Brightness, &m_brightness));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Contrast  , &m_contrast));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Hue       , &m_hue));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Saturation, &m_saturation));
  CHECK(m_service->CreateVideoProcessor(m_device, &m_desc, output, 0, &m_process));

  m_time = 0;
  return true;
}

REFERENCE_TIME CProcessor::Add(IDirect3DSurface9* source)
{
  CSingleLock lock(m_section);

  m_time += 2;

  DXVA2_VideoSample vs = {};
  vs.Start          = m_time;
  vs.End            = 0; 
  vs.SampleFormat   = m_desc.SampleFormat;
  vs.SrcRect.left   = 0;
  vs.SrcRect.right  = m_desc.SampleWidth;
  vs.SrcRect.top    = 0;
  vs.SrcRect.bottom = m_desc.SampleHeight;
  vs.PlanarAlpha    = DXVA2_Fixed32OpaqueAlpha();
  vs.SampleData     = 0;
  vs.SrcSurface     = source;
  vs.SrcSurface->AddRef();

  if(!m_sample.empty())
    m_sample.front().End = vs.Start;

  m_sample.push_front(vs);
  if(m_sample.size() > m_size)
  {
    SAFE_RELEASE(m_sample.back().SrcSurface);
    m_sample.pop_back();
  }

  return m_time;
}

static DXVA2_Fixed32 ConvertRange(const DXVA2_ValueRange& range, int value, int min, int max, int def)
{
  if(value > def)
    return DXVA2FloatToFixed( DXVA2FixedToFloat(range.DefaultValue)
                            + (DXVA2FixedToFloat(range.MaxValue) - DXVA2FixedToFloat(range.DefaultValue))
                            * (value - def) / (max - def) );
  else if(value < def)
    return DXVA2FloatToFixed( DXVA2FixedToFloat(range.DefaultValue)
                            + (DXVA2FixedToFloat(range.MinValue) - DXVA2FixedToFloat(range.DefaultValue)) 
                            * (value - def) / (max - def) );
  else
    return range.DefaultValue;
}

void CProcessor::CropSource(RECT& src, RECT& dst, const D3DSURFACE_DESC& desc)
{
  if(dst.left < 0)
  {
    src.left -= dst.left 
              * (src.right - src.left) 
              / (dst.right - dst.left);
    dst.left  = 0;
  }
  if(dst.top < 0)
  {
    src.top -= dst.top 
             * (src.bottom - src.top) 
             / (dst.bottom - dst.top);
    dst.top  = 0;
  }
  if(dst.right > (LONG)desc.Width)
  {
    src.right -= (dst.right - desc.Width)
               * (src.right - src.left) 
               / (dst.right - dst.left);
    dst.right  = desc.Width;
  }
  if(dst.bottom > (LONG)desc.Height)
  {
    src.bottom -= (dst.bottom - desc.Height)
                * (src.bottom - src.top) 
                / (dst.bottom - dst.top);
    dst.bottom  = desc.Height;
  }
}

bool CProcessor::Render(const RECT &dst, IDirect3DSurface9* target, REFERENCE_TIME time)
{
  CSingleLock lock(m_section);

  if(m_sample.empty())
    return false;

  SSamples::iterator it = m_sample.begin();
  for(; it != m_sample.end(); it++)
  {
    if(it->Start <= time)
      break;
  }

  if(it == m_sample.end())
  {
    CLog::Log(LOGERROR, "DXVA - failed to find image, all images newer or no images");
    return false;
  }

  DXVA2_VideoSample vs;
  vs = *it;
  vs.DstRect = dst;
  if(vs.End == 0)
    vs.End = vs.Start + 2;

  if(time <  vs.Start
  || time >= vs.End)
    CLog::Log(LOGWARNING, "DXVA - image to render is outside of bounds [%"PRId64":%"PRId64") was %"PRId64,vs.Start, vs.End, time);

  D3DSURFACE_DESC desc;
  CHECK(target->GetDesc(&desc));

  CropSource(vs.SrcRect, vs.DstRect, desc);

  DXVA2_VideoProcessBltParams blt = {};
  blt.TargetFrame = vs.Start;
  blt.TargetRect  = vs.DstRect;


  blt.DestFormat.VideoTransferFunction = DXVA2_VideoTransFunc_sRGB;
  blt.DestFormat.SampleFormat          = DXVA2_SampleProgressiveFrame;
  blt.DestFormat.NominalRange          = DXVA2_NominalRange_0_255;
  blt.Alpha = DXVA2_Fixed32OpaqueAlpha();

  blt.ProcAmpValues.Brightness = ConvertRange( m_brightness, g_settings.m_currentVideoSettings.m_Brightness
                                             , 0, 100, 50);
  blt.ProcAmpValues.Contrast   = ConvertRange( m_contrast, g_settings.m_currentVideoSettings.m_Contrast
                                             , 0, 100, 50);
  blt.ProcAmpValues.Hue        = m_hue.DefaultValue;
  blt.ProcAmpValues.Saturation = m_saturation.DefaultValue;

  blt.BackgroundColor.Y     = 0x1000;
  blt.BackgroundColor.Cb    = 0x8000;
  blt.BackgroundColor.Cr    = 0x8000;
  blt.BackgroundColor.Alpha = 0xffff;

  CHECK(m_process->VideoProcessBlt(target, &blt, &vs, 1, NULL));

  /* erase anything older than what we used */
  for(it++;it != m_sample.end(); it++)
    SAFE_RELEASE(it->SrcSurface);
  m_sample.erase(it, m_sample.end());

  return true;
}

CProcessor* CProcessor::Acquire()
{
  InterlockedIncrement(&m_references);
  return this;
}

long CProcessor::Release()
{
  long count = InterlockedDecrement(&m_references);
  ASSERT(count >= 0);
  if (count == 0) delete this;
  return count;
}
