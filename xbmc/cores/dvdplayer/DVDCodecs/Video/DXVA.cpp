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

#ifdef HAS_DX

// setting that here because otherwise SampleFormat is defined to AVSampleFormat
// which we don't use here
#define FF_API_OLD_SAMPLE_FMT 0

#include <windows.h>
#include <d3d9.h>
#include <Initguid.h>
#include <dxva.h>
#include <dxva2api.h>
#include "libavcodec/dxva2.h"
#include "../DVDCodecUtils.h"

#include "DXVA.h"
#include "windowing/WindowingFactory.h"
#include "../../../VideoRenderers/WinRenderer.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "boost/shared_ptr.hpp"
#include "utils/AutoPtrHandle.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "win32/WIN32Util.h"
#include "utils/Log.h"

using namespace DXVA;
using namespace AUTOPTR;
using namespace std;

static void RelBufferS(void *opaque, uint8_t *data)
{ ((CDecoder*)((CDVDVideoCodecFFmpeg*)opaque)->GetHardware())->RelBuffer(data); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic, int flags) 
{  return ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->GetBuffer(avctx, pic, flags); }


DEFINE_GUID(DXVADDI_Intel_ModeH264_A, 0x604F8E64,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_C, 0x604F8E66,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E, 0x604F8E68,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeVC1_E , 0xBCC5DB6D,0xA2B6,0x4AF0,0xAC,0xE4,0xAD,0xB1,0xF7,0x87,0xBC,0x89);

#if _MSC_VER < 1700
DEFINE_GUID(DXVA_ModeMPEG2and1_VLD,   0x86695f12,0x340e,0x4f04,0x9f,0xd3,0x92,0x53,0xdd,0x32,0x74,0x60);
// When exposed by an accelerator, indicates compliance with the August 2010 spec update
DEFINE_GUID(DXVA_ModeVC1_D2010,       0x1b81beA4,0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
#endif

typedef struct {
    const char   *name;
    const GUID   *guid;
    int          codec;
} dxva2_mode_t;

/* XXX Prefered modes must come first */
static const dxva2_mode_t dxva2_modes[] = {
    { "MPEG2 VLD",    &DXVA2_ModeMPEG2_VLD,     AV_CODEC_ID_MPEG2VIDEO },
    { "MPEG1/2 VLD",  &DXVA_ModeMPEG2and1_VLD,  AV_CODEC_ID_MPEG2VIDEO },
    { "MPEG2 MoComp", &DXVA2_ModeMPEG2_MoComp,  0 },
    { "MPEG2 IDCT",   &DXVA2_ModeMPEG2_IDCT,    0 },

#ifndef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO
    /* We must prefer Intel specific ones if the flag doesn't exists */
    { "Intel H.264 VLD, no FGT",                                      &DXVADDI_Intel_ModeH264_E, AV_CODEC_ID_H264 },
    { "Intel H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVADDI_Intel_ModeH264_C, 0 },
    { "Intel H.264 motion compensation (MoComp), no FGT",             &DXVADDI_Intel_ModeH264_A, 0 },
    { "Intel VC-1 VLD",                                               &DXVADDI_Intel_ModeVC1_E,  0 },
#endif

    { "H.264 variable-length decoder (VLD), FGT",               &DXVA2_ModeH264_F, AV_CODEC_ID_H264 },
    { "H.264 VLD, no FGT",                                      &DXVA2_ModeH264_E, AV_CODEC_ID_H264 },
    { "H.264 IDCT, FGT",                                        &DXVA2_ModeH264_D, 0,            },
    { "H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVA2_ModeH264_C, 0,            },
    { "H.264 MoComp, FGT",                                      &DXVA2_ModeH264_B, 0,            },
    { "H.264 motion compensation (MoComp), no FGT",             &DXVA2_ModeH264_A, 0,            },

    { "Windows Media Video 8 MoComp",           &DXVA2_ModeWMV8_B, 0 },
    { "Windows Media Video 8 post processing",  &DXVA2_ModeWMV8_A, 0 },

    { "Windows Media Video 9 IDCT",             &DXVA2_ModeWMV9_C, 0 },
    { "Windows Media Video 9 MoComp",           &DXVA2_ModeWMV9_B, 0 },
    { "Windows Media Video 9 post processing",  &DXVA2_ModeWMV9_A, 0 },

    // prefer VC1_D2010 decoder over the older variant
    { "VC-1 VLD 2010",        &DXVA_ModeVC1_D2010, AV_CODEC_ID_VC1 },
    { "VC-1 VLD 2010",        &DXVA_ModeVC1_D2010, AV_CODEC_ID_WMV3 },
    { "VC-1 VLD",             &DXVA2_ModeVC1_D,    AV_CODEC_ID_VC1 },
    { "VC-1 VLD",             &DXVA2_ModeVC1_D,    AV_CODEC_ID_WMV3 },
    { "VC-1 IDCT",            &DXVA2_ModeVC1_C,    0 },
    { "VC-1 MoComp",          &DXVA2_ModeVC1_B,    0 },
    { "VC-1 post processing", &DXVA2_ModeVC1_A,    0 },

#ifdef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO
    /* Intel specific modes (only useful on older GPUs) */
    { "Intel H.264 VLD, no FGT",                                      &DXVADDI_Intel_ModeH264_E, AV_CODEC_ID_H264 },
    { "Intel H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVADDI_Intel_ModeH264_C, 0 },
    { "Intel H.264 motion compensation (MoComp), no FGT",             &DXVADDI_Intel_ModeH264_A, 0 },
    { "Intel VC-1 VLD",                                               &DXVADDI_Intel_ModeVC1_E,  0 },
#endif

    { NULL, NULL, 0 }
};

// Prefered targets must be first
static const D3DFORMAT render_targets[] = {
    (D3DFORMAT)MAKEFOURCC('N','V','1','2'),
    (D3DFORMAT)MAKEFOURCC('Y','V','1','2'),
    D3DFMT_UNKNOWN
};

// List of PCI Device ID of ATI cards with UVD or UVD+ decoding block.
static DWORD UVDDeviceID [] = {
  0x95C0, // ATI Radeon HD 3400 Series (and others)
  0x95C5, // ATI Radeon HD 3400 Series (and others)
  0x95C4, // ATI Radeon HD 3400 Series (and others)
  0x94C3, // ATI Radeon HD 3410
  0x9589, // ATI Radeon HD 3600 Series (and others)
  0x9598, // ATI Radeon HD 3600 Series (and others)
  0x9591, // ATI Radeon HD 3600 Series (and others)
  0x9501, // ATI Radeon HD 3800 Series (and others)
  0x9505, // ATI Radeon HD 3800 Series (and others)
  0x9507, // ATI Radeon HD 3830
  0x9513, // ATI Radeon HD 3850 X2
  0x950F, // ATI Radeon HD 3850 X2
  0x0000
};

// List of PCI Device ID of nVidia cards with the macroblock width issue. More or less the VP3 block.
// Per NVIDIA Accelerated Linux Graphics Driver, Appendix A Supported NVIDIA GPU Products, cards with note 1.
static DWORD VP3DeviceID [] = {
  0x06E0, // GeForce 9300 GE
  0x06E1, // GeForce 9300 GS
  0x06E2, // GeForce 8400
  0x06E4, // GeForce 8400 GS
  0x06E5, // GeForce 9300M GS
  0x06E6, // GeForce G100
  0x06E8, // GeForce 9200M GS
  0x06E9, // GeForce 9300M GS
  0x06EC, // GeForce G 105M
  0x06EF, // GeForce G 103M
  0x06F1, // GeForce G105M
  0x0844, // GeForce 9100M G
  0x0845, // GeForce 8200M G
  0x0846, // GeForce 9200
  0x0847, // GeForce 9100
  0x0848, // GeForce 8300
  0x0849, // GeForce 8200
  0x084A, // nForce 730a
  0x084B, // GeForce 9200
  0x084C, // nForce 980a/780a SLI
  0x084D, // nForce 750a SLI
  0x0860, // GeForce 9400
  0x0861, // GeForce 9400
  0x0862, // GeForce 9400M G
  0x0863, // GeForce 9400M
  0x0864, // GeForce 9300
  0x0865, // ION
  0x0866, // GeForce 9400M G
  0x0867, // GeForce 9400
  0x0868, // nForce 760i SLI
  0x086A, // GeForce 9400
  0x086C, // GeForce 9300 / nForce 730i
  0x086D, // GeForce 9200
  0x086E, // GeForce 9100M G
  0x086F, // GeForce 8200M G
  0x0870, // GeForce 9400M
  0x0871, // GeForce 9200
  0x0872, // GeForce G102M
  0x0873, // GeForce G102M
  0x0874, // ION
  0x0876, // ION
  0x087A, // GeForce 9400
  0x087D, // ION
  0x087E, // ION LE
  0x087F, // ION LE
  0x0000
};

static std::string GUIDToString(const GUID& guid)
{
  std::string buffer = StringUtils::Format("%08X-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
              , guid.Data1, guid.Data2, guid.Data3
              , guid.Data4[0], guid.Data4[1]
              , guid.Data4[2], guid.Data4[3], guid.Data4[4]
              , guid.Data4[5], guid.Data4[6], guid.Data4[7]);
  return buffer;
}

static const dxva2_mode_t *dxva2_find_mode(const GUID *guid)
{
    for (unsigned i = 0; dxva2_modes[i].name; i++) {
        if (IsEqualGUID(*dxva2_modes[i].guid, *guid))
            return &dxva2_modes[i];
    }
    return NULL;
}

CDXVAContext *CDXVAContext::m_context = NULL;
CCriticalSection CDXVAContext::m_section;
HMODULE CDXVAContext::m_dlHandle = NULL;
DXVA2CreateVideoServicePtr CDXVAContext::m_DXVA2CreateVideoService = NULL;
CDXVAContext::CDXVAContext()
{
  m_context = NULL;
  m_refCount = 0;
  m_service = NULL;
}

void CDXVAContext::Release(CDecoder *decoder)
{
  CSingleLock lock(m_section);
  std::vector<CDecoder*>::iterator it;
  it = find(m_decoders.begin(), m_decoders.end(), decoder);
  if (it != m_decoders.end())
    m_decoders.erase(it);
  m_refCount--;
  if (m_refCount <= 0)
  {
    Close();
    delete this;
    m_context = 0;
  }
}

void CDXVAContext::Close()
{
  CLog::Log(LOGNOTICE, "%s - closing decoder context.", __FUNCTION__);
  DestroyContext();
}

bool CDXVAContext::LoadSymbols()
{
  CSingleLock lock(m_section);
  if (m_dlHandle == NULL)
    m_dlHandle = LoadLibraryEx("dxva2.dll", NULL, 0);
  if (m_dlHandle == NULL)
    return false;
  m_DXVA2CreateVideoService = (DXVA2CreateVideoServicePtr)GetProcAddress(m_dlHandle, "DXVA2CreateVideoService");
  if (m_DXVA2CreateVideoService == NULL)
    return false;
  return true;
}

bool CDXVAContext::EnsureContext(CDXVAContext **ctx, CDecoder *decoder)
{
  CSingleLock lock(m_section);
  if (m_context)
  {
    m_context->m_refCount++;
    *ctx = m_context;
    return true;
  }
  m_context = new CDXVAContext();
  *ctx = m_context;
  {
    if (!m_context->LoadSymbols() || !m_context->CreateContext())
    {
      delete m_context;
      m_context = 0;
      *ctx = NULL;
      return false;
    }
  }
  m_context->m_refCount++;

  if (!m_context->IsValidDecoder(decoder))
    m_context->m_decoders.push_back(decoder);

  *ctx = m_context;
  return true;
}

bool CDXVAContext::CreateContext()
{
  m_DXVA2CreateVideoService(g_Windowing.Get3DDevice(), IID_IDirectXVideoDecoderService, (void**)&m_service);
  QueryCaps();
  return true;
}

void CDXVAContext::DestroyContext()
{
  CoTaskMemFree(m_input_list);
  SAFE_RELEASE(m_service);
}

void CDXVAContext::QueryCaps()
{
  HRESULT res = m_service->GetDecoderDeviceGuids(&m_input_count, &m_input_list);
  if (FAILED(res))
  {
    CLog::Log(LOGNOTICE, "%s - failed getting device guids.", __FUNCTION__);
    return;
  }

  for (unsigned i = 0; i < m_input_count; i++)
  {
    const GUID *g = &m_input_list[i];
    const dxva2_mode_t *mode = dxva2_find_mode(g);
    if (mode)
      CLog::Log(LOGDEBUG, "DXVA - supports '%s'.", mode->name);
    else
      CLog::Log(LOGDEBUG, "DXVA - supports %s.", GUIDToString(*g).c_str());
  }
}

bool CDXVAContext::GetInputAndTarget(int codec, GUID &inGuid, D3DFORMAT &outFormat)
{
  outFormat = D3DFMT_UNKNOWN;
  UINT output_count = 0;
  D3DFORMAT *output_list = NULL;
  for (const dxva2_mode_t* mode = dxva2_modes; mode->name && outFormat == D3DFMT_UNKNOWN; mode++)
  {
    if (mode->codec != codec)
      continue;

    for (unsigned i = 0; i < m_input_count; i++)
    {
      if (!IsEqualGUID(m_input_list[i], *mode->guid))
        continue;

      CLog::Log(LOGDEBUG, "DXVA - trying '%s'.", mode->name);
      HRESULT res = m_service->GetDecoderRenderTargets(m_input_list[i], &output_count, &output_list);
      if (FAILED(res))
      {
        CLog::Log(LOGNOTICE, "%s - failed getting render targets.", __FUNCTION__);
        break;
      }

      for (unsigned j = 0; render_targets[j] != D3DFMT_UNKNOWN; j++)
      {
        for (unsigned k = 0; k < output_count; k++)
        if (output_list[k] == render_targets[j])
        {
          inGuid = m_input_list[i];
          outFormat = output_list[k];
          break;
        }
      }
    }
  }

  CoTaskMemFree(output_list);

  if (outFormat == D3DFMT_UNKNOWN)
    return false;

  return true;
}

bool CDXVAContext::GetConfig(GUID &inGuid, const DXVA2_VideoDesc *format, DXVA2_ConfigPictureDecode &config)
{
  // find what decode configs are available
  UINT cfg_count = 0;
  DXVA2_ConfigPictureDecode *cfg_list = NULL;
  HRESULT res = m_service->GetDecoderConfigurations(inGuid, format, NULL, &cfg_count, &cfg_list);
  if (FAILED(res))
  {
    CLog::Log(LOGNOTICE, "%s - failed getting decoder configuration.", __FUNCTION__);
    return false;
  }

  config = {};
  unsigned bitstream = 2; // ConfigBitstreamRaw = 2 is required for Poulsbo and handles skipping better with nVidia
  for (unsigned i = 0; i< cfg_count; i++)
  {
    CLog::Log(LOGDEBUG,
      "%s - config %d: bitstream type %d%s.",
      __FUNCTION__,
      i,
      cfg_list[i].ConfigBitstreamRaw,
      IsEqualGUID(cfg_list[i].guidConfigBitstreamEncryption, DXVA_NoEncrypt) ? "" : ", encrypted");
    // select first available
    if (config.ConfigBitstreamRaw == 0 && cfg_list[i].ConfigBitstreamRaw != 0)
      config = cfg_list[i];
    // overide with preferred if found
    if (config.ConfigBitstreamRaw != bitstream && cfg_list[i].ConfigBitstreamRaw == bitstream)
      config = cfg_list[i];
  }

  CoTaskMemFree(cfg_list);

  if (!config.ConfigBitstreamRaw)
  {
    CLog::Log(LOGDEBUG, "%s - failed to find a raw input bitstream.", __FUNCTION__);
    return false;
  }

  return true;
}

bool CDXVAContext::CreateSurfaces(int width, int height, D3DFORMAT format, unsigned int count, LPDIRECT3DSURFACE9 *surfaces)
{
  HRESULT res = m_service->CreateSurface((width + 15) & ~15, (height + 15) & ~15, count, format,
                                          D3DPOOL_DEFAULT, 0, DXVA2_VideoDecoderRenderTarget,
                                          surfaces, NULL);
  if (FAILED(res))
  {
    CLog::Log(LOGNOTICE, "%s - failed creating surfaces.", __FUNCTION__);
    return false;
  }

  return true;
}

bool CDXVAContext::CreateDecoder(GUID &inGuid, DXVA2_VideoDesc *format, const DXVA2_ConfigPictureDecode *config, LPDIRECT3DSURFACE9 *surfaces, unsigned int count, IDirectXVideoDecoder **decoder)
{
  CSingleLock lock(m_section);
  std::vector<CDecoder*>::iterator it;
  for (it = m_decoders.begin(); it != m_decoders.end(); ++it)
  {
    (*it)->CloseDXVADecoder();
  }

  HRESULT res = m_service->CreateVideoDecoder(inGuid, format, config, surfaces, count, decoder);
  if (FAILED(res))
  {
    CLog::Log(LOGNOTICE, "%s - failed creating decoder.", __FUNCTION__);
    return false;
  }

  return true;
}

bool CDXVAContext::IsValidDecoder(CDecoder *decoder)
{
  std::vector<CDecoder*>::iterator it;
  it = find(m_decoders.begin(), m_decoders.end(), decoder);
  if (it != m_decoders.end())
    return true;
  return false;
}

CSurfaceContext::CSurfaceContext()
{
}

CSurfaceContext::~CSurfaceContext()
{
  for (vector<IDirect3DSurface9*>::iterator it = m_heldsurfaces.begin(); it != m_heldsurfaces.end(); ++it)
    SAFE_RELEASE(*it);
}

void CSurfaceContext::HoldSurface(IDirect3DSurface9* surface)
{
  surface->AddRef();
  m_heldsurfaces.push_back(surface);
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
  m_device    = NULL;
  m_decoder   = NULL;
  m_buffer_count = 0;
  m_buffer_age   = 0;
  m_refs         = 0;
  m_shared       = 0;
  m_surface_context = NULL;
  m_dxva_context = NULL;
  memset(&m_format, 0, sizeof(m_format));
  m_context          = (dxva_context*)calloc(1, sizeof(dxva_context));
  m_context->cfg     = (DXVA2_ConfigPictureDecode*)calloc(1, sizeof(DXVA2_ConfigPictureDecode));
  m_context->surface = (IDirect3DSurface9**)calloc(DXVA2_MAX_SURFACES, sizeof(IDirect3DSurface9*));
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
  SAFE_RELEASE(m_surface_context);
  for(unsigned i = 0; i < m_buffer_count; i++)
    m_buffer[i].Clear();
  m_buffer_count = 0;
  memset(&m_format, 0, sizeof(m_format));

  if (m_dxva_context)
    m_dxva_context->Release(this);
  m_dxva_context = NULL;
}

static bool CheckH264L41(AVCodecContext *avctx)
{
    unsigned widthmbs  = (avctx->coded_width + 15) / 16;  // width in macroblocks
    unsigned heightmbs = (avctx->coded_height + 15) / 16; // height in macroblocks
    unsigned maxdpbmbs = 32768;                     // Decoded Picture Buffer (DPB) capacity in macroblocks for L4.1

    return (avctx->refs * widthmbs * heightmbs <= maxdpbmbs);
}

static bool IsL41LimitedATI()
{
  D3DADAPTER_IDENTIFIER9 AIdentifier = g_Windowing.GetAIdentifier();

  if(AIdentifier.VendorId == PCIV_ATI)
  {
    for (unsigned idx = 0; UVDDeviceID[idx] != 0; idx++)
    {
      if (UVDDeviceID[idx] == AIdentifier.DeviceId)
        return true;
    }
  }
  return false;
}

static bool HasVP3WidthBug(AVCodecContext *avctx)
{
  // Some nVidia VP3 hardware cannot do certain macroblock widths

  D3DADAPTER_IDENTIFIER9 AIdentifier = g_Windowing.GetAIdentifier();

  if(AIdentifier.VendorId == PCIV_nVidia
  && !CDVDCodecUtils::IsVP3CompatibleWidth(avctx->coded_width))
  {
    // Find the card in a known list of problematic VP3 hardware
    for (unsigned idx = 0; VP3DeviceID[idx] != 0; idx++)
      if (VP3DeviceID[idx] == AIdentifier.DeviceId)
        return true;
  }
  return false;
}

static bool CheckCompatibility(AVCodecContext *avctx)
{
  // The incompatibilities are all for H264
  if(avctx->codec_id != AV_CODEC_ID_H264)
    return true;

  // Macroblock width incompatibility
  if (HasVP3WidthBug(avctx))
  {
    CLog::Log(LOGWARNING,"%s - width %i is not supported with nVidia VP3 hardware. DXVA will not be used.", __FUNCTION__, avctx->coded_width);
    return false;
  }

  // there are many corrupt mpeg2 rips from dvd's which don't
  // follow profile spec properly, they go corrupt on hw, so
  // keep those running in software for the time being.
  if (avctx->codec_id  == AV_CODEC_ID_MPEG2VIDEO
  &&  avctx->height    <= 576
  &&  avctx->width     <= 720)
    return false;

  // Check for hardware limited to H264 L4.1 (ie Bluray).

  // No advanced settings: autodetect.
  // The advanced setting lets the user override the autodetection (in case of false positive or negative)

  bool checkcompat;
  if (!g_advancedSettings.m_DXVACheckCompatibilityPresent)
    checkcompat = IsL41LimitedATI();  // ATI UVD and UVD+ cards can only do L4.1 - corresponds roughly to series 3xxx
  else
    checkcompat = g_advancedSettings.m_DXVACheckCompatibility;

  if (checkcompat && !CheckH264L41(avctx))
  {
      CLog::Log(LOGWARNING, "%s - video exceeds L4.1. DXVA will not be used.", __FUNCTION__);
      return false;
  }

  return true;
}

bool CDecoder::Open(AVCodecContext *avctx, enum PixelFormat fmt, unsigned int surfaces)
{
  if (!CheckCompatibility(avctx))
    return false;

  CSingleLock lock(m_section);
  Close();

  if(m_state == DXVA_LOST)
  {
    CLog::Log(LOGDEBUG, "%s - device is in lost state, we can't start.", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s - open decoder", __FUNCTION__);
  if (!CDXVAContext::EnsureContext(&m_dxva_context, this))
    return false;

  if (!m_dxva_context->GetInputAndTarget(avctx->codec_id, m_input, m_format.Format))
  {
    CLog::Log(LOGDEBUG, "%s - unable to find an input/output format combination.", __FUNCTION__);
    return false;
  }

  m_format.SampleWidth  = avctx->coded_width;
  m_format.SampleHeight = avctx->coded_height;
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

  if (surfaces > m_shared)
    m_shared = surfaces;

  m_refs = std::min<int>(DXVA2_MAX_SURFACES, std::max<int>(20 + DXVA2_QUEUE_SURFACES, avctx->refs) - m_shared);

  CLog::Log(LOGDEBUG, "%s - source requires %d references.", __FUNCTION__, avctx->refs);
  CLog::Log(LOGDEBUG, "%s - will allocate %d surfaces.", __FUNCTION__, m_refs);

  DXVA2_ConfigPictureDecode config = {};
  if (!m_dxva_context->GetConfig(m_input, &m_format, config))
    return false;

  *const_cast<DXVA2_ConfigPictureDecode*>(m_context->cfg) = config;

  m_surface_context = new CSurfaceContext();

  if(!OpenDecoder())
    return false;

  avctx->get_buffer2     = GetBufferS;
  avctx->hwaccel_context = m_context;

  D3DADAPTER_IDENTIFIER9 AIdentifier = g_Windowing.GetAIdentifier();
  if (AIdentifier.VendorId == PCIV_Intel && m_input == DXVADDI_Intel_ModeH264_E)
  {
#ifdef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO
    m_context->workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
#else
    CLog::Log(LOGWARNING, "%s - used Intel ClearVideo decoder, but no support workaround for it in libavcodec.", __FUNCTION__);
#endif
  }
  else if (AIdentifier.VendorId == PCIV_ATI && IsL41LimitedATI())
  {
#ifdef FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG
    m_context->workaround |= FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG;
#else
    CLog::Log(LOGWARNING, "%s - video card with different scaling list zigzag order detected, but no support in libavcodec.", __FUNCTION__);
#endif
  }

  m_state = DXVA_OPEN;
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
    CLog::Log(LOGWARNING, "%s - ignoring invalid surface.", __FUNCTION__);
    return VC_BUFFER;
  }
  else
    return 0;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(picture);
  CSingleLock lock(m_section);
  picture->format = RENDER_FMT_DXVA;
  picture->extended_format = (unsigned int)m_format.Format;
  picture->context = m_surface_context;
  picture->data[3]= frame->data[3];
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
      CLog::Log(LOGERROR, "%s - device didn't reset in reasonable time.", __FUNCTION__);
      return VC_ERROR;
    }
  }

  if(m_format.SampleWidth  == 0
  || m_format.SampleHeight == 0)
  {
    if(!Open(avctx, avctx->pix_fmt, m_shared))
    {
      CLog::Log(LOGERROR, "%s - decoder was not able to reset.", __FUNCTION__);
      Close();
      return VC_ERROR;
    }
    return VC_FLUSHED;
  }

  // Status reports are available only for the DXVA2_ModeH264 and DXVA2_ModeVC1 modes
  if(avctx->codec_id != AV_CODEC_ID_H264
  && avctx->codec_id != AV_CODEC_ID_VC1
  && avctx->codec_id != AV_CODEC_ID_WMV3)
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
  data.PrivateOutputDataSize = avctx->codec_id == AV_CODEC_ID_H264 ? sizeof(DXVA_Status_H264) : sizeof(DXVA_Status_VC1);
  HRESULT hr;
  if(FAILED( hr = m_decoder->Execute(&params)))
  {
    CLog::Log(LOGWARNING, "%s - failed to get decoder status - 0x%08X.", __FUNCTION__, hr);
    return VC_ERROR;
  }

  if(avctx->codec_id == AV_CODEC_ID_H264)
  {
    if(status.h264.bStatus)
      CLog::Log(LOGWARNING, "%s - decoder problem of status %d with %d.", __FUNCTION__, status.h264.bStatus, status.h264.bBufType);
  }
  else
  {
    if(status.vc1.bStatus)
      CLog::Log(LOGWARNING, "%s - decoder problem of status %d with %d.", __FUNCTION__, status.vc1.bStatus, status.vc1.bBufType);
  }
  return 0;
}

bool CDecoder::OpenDecoder()
{
  SAFE_RELEASE(m_decoder);
  m_context->decoder = NULL;

  m_context->surface_count = std::min<unsigned>(DXVA2_MAX_SURFACES, m_refs + m_shared); // refs + processor buffer

  if(m_context->surface_count > m_buffer_count)
  {
    CLog::Log(LOGDEBUG, "%s - allocating %d surfaces.", __FUNCTION__, m_context->surface_count - m_buffer_count);

    if (!m_dxva_context->CreateSurfaces(m_format.SampleWidth, m_format.SampleHeight, m_format.Format,
                                        m_context->surface_count - 1 - m_buffer_count, m_context->surface + m_buffer_count))
      return false;

    IDirect3DDevice9* pDev = (IDirect3DDevice9*) g_Windowing.Get3DDevice();
    for(unsigned i = m_buffer_count; i < m_context->surface_count; i++)
    {
      m_buffer[i].surface = m_context->surface[i];
      m_surface_context->HoldSurface(m_context->surface[i]);

      // fill the surface in black, to avoid the "green screen" in case the first frame fails to decode.
      if (pDev) pDev->ColorFill(m_context->surface[i], NULL, D3DCOLOR_XYUV(0, 128, 128));
    }

    m_buffer_count = m_context->surface_count;
  }

  if (!m_dxva_context->CreateDecoder(m_input, &m_format, m_context->cfg, m_context->surface, m_context->surface_count, &m_decoder))
    return false;

  m_context->decoder = m_decoder;

  return true;
}

bool CDecoder::Supports(enum PixelFormat fmt)
{
  if(fmt == PIX_FMT_DXVA2_VLD)
    return true;
  return false;
}

void CDecoder::RelBuffer(uint8_t *data)
{
  CSingleLock lock(m_section);
  IDirect3DSurface9* surface = (IDirect3DSurface9*)data;

  for(unsigned i = 0; i < m_buffer_count; i++)
  {
    if(m_buffer[i].surface == surface)
    {
      m_buffer[i].used = false;
      break;
    }
  }

  Release();
}

int CDecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags)
{
  CSingleLock lock(m_section);

  if (!m_decoder)
    return -1;

  if(avctx->coded_width  != m_format.SampleWidth
  || avctx->coded_height != m_format.SampleHeight)
  {
    Close();
    if(!Open(avctx, avctx->pix_fmt, m_shared))
    {
      Close();
      return -1;
    }
  }
  unsigned int i;
  int old, old_unused;
  for (i = 0, old = 0, old_unused = -1; i < m_buffer_count; i++) 
  {
    SVideoBuffer* buf = &m_buffer[i];
    if (!buf->used && (old_unused == -1 || buf->age < m_buffer[old_unused].age))
      old_unused = i;
    if (buf->age < m_buffer[old].age)
      old = i;
  }
  if (old_unused == -1) 
  {
    CLog::Log(LOGERROR, "%s - unable to find unused buffer, using oldest.", __FUNCTION__);
    i = old;
  } 
  else 
    i = old_unused;

  LPDIRECT3DSURFACE9 pSurface = m_buffer[i].surface;
  if(!pSurface)
  {
    CLog::Log(LOGERROR, "%s - There is a buffer, but no D3D Surace? WTF?", __FUNCTION__);
    return -1;
  }

  m_buffer[i].age  = ++m_buffer_age;
  m_buffer[i].used = true;

  memset(pic->data, 0, sizeof(pic->data));
  memset(pic->linesize, 0, sizeof(pic->linesize));

  pic->reordered_opaque = avctx->reordered_opaque;
  pic->data[0] = pic->data[3] = (uint8_t*)pSurface;


  AVBufferRef *buffer = av_buffer_create(pic->data[3], 0, RelBufferS, avctx->opaque, 0);
  if (!buffer)
  {
    CLog::Log(LOGERROR, "%s - error creating buffer.", __FUNCTION__);
    return -1;
  }
  pic->buf[0] = buffer;

  Acquire();

  return 0;
}

unsigned CDecoder::GetAllowedReferences()
{
  return m_shared;
}


void CDecoder::CloseDXVADecoder()
{
  CSingleLock lock(m_section);
  SAFE_RELEASE(m_decoder);
}


#endif
