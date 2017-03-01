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

#include <d3d9.h>
#include <dxva.h>
#include <d3d11.h>
#include <Initguid.h>
#include <windows.h>
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "../DVDCodecUtils.h"
#include "DXVA.h"
#include "settings/AdvancedSettings.h"
#include "utils/Log.h"
#include "utils/StringUtils.h"
#include "windowing/WindowingFactory.h"

using namespace DXVA;

static void RelBufferS(void *opaque, uint8_t *data)
{ ((CDecoder*)opaque)->RelBuffer(data); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic, int flags) 
{  return ((CDecoder*)((ICallbackHWAccel*)avctx->opaque)->GetHWAccel())->GetBuffer(avctx, pic, flags); }

DEFINE_GUID(DXVADDI_Intel_ModeH264_A, 0x604F8E64,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_C, 0x604F8E66,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E, 0x604F8E68,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeVC1_E , 0xBCC5DB6D,0xA2B6,0x4AF0,0xAC,0xE4,0xAD,0xB1,0xF7,0x87,0xBC,0x89);

// redefine DXVA_NoEncrypt with other macro, solves unresolved external symbol linker error
#ifdef DXVA_NoEncrypt 
#undef DXVA_NoEncrypt
#endif
DEFINE_GUID(DXVA_NoEncrypt, 0x1b81beD0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

typedef struct {
    const char   *name;
    const GUID   *guid;
    int          codec;
} dxva2_mode_t;

/* XXX Prefered modes must come first */
static const dxva2_mode_t dxva2_modes[] = {
    { "MPEG2 VLD",    &D3D11_DECODER_PROFILE_MPEG2_VLD,     AV_CODEC_ID_MPEG2VIDEO },
    { "MPEG1/2 VLD",  &D3D11_DECODER_PROFILE_MPEG2and1_VLD, AV_CODEC_ID_MPEG2VIDEO },
    { "MPEG2 MoComp", &D3D11_DECODER_PROFILE_MPEG2_MOCOMP,  0 },
    { "MPEG2 IDCT",   &D3D11_DECODER_PROFILE_MPEG2_IDCT,    0 },

#ifndef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO
    /* We must prefer Intel specific ones if the flag doesn't exists */
    { "Intel H.264 VLD, no FGT",                                      &DXVADDI_Intel_ModeH264_E, AV_CODEC_ID_H264 },
    { "Intel H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVADDI_Intel_ModeH264_C, 0 },
    { "Intel H.264 motion compensation (MoComp), no FGT",             &DXVADDI_Intel_ModeH264_A, 0 },
    { "Intel VC-1 VLD",                                               &DXVADDI_Intel_ModeVC1_E,  0 },
#endif

    { "H.264 variable-length decoder (VLD), FGT",               &D3D11_DECODER_PROFILE_H264_VLD_FGT,      AV_CODEC_ID_H264 },
    { "H.264 VLD, no FGT",                                      &D3D11_DECODER_PROFILE_H264_VLD_NOFGT,    AV_CODEC_ID_H264 },
    { "H.264 IDCT, FGT",                                        &D3D11_DECODER_PROFILE_H264_IDCT_FGT,     0, },
    { "H.264 inverse discrete cosine transform (IDCT), no FGT", &D3D11_DECODER_PROFILE_H264_IDCT_NOFGT,   0, },
    { "H.264 MoComp, FGT",                                      &D3D11_DECODER_PROFILE_H264_MOCOMP_FGT,   0, },
    { "H.264 motion compensation (MoComp), no FGT",             &D3D11_DECODER_PROFILE_H264_MOCOMP_NOFGT, 0, },

    { "Windows Media Video 8 MoComp",           &D3D11_DECODER_PROFILE_WMV8_MOCOMP,   0 },
    { "Windows Media Video 8 post processing",  &D3D11_DECODER_PROFILE_WMV8_POSTPROC, 0 },

    { "Windows Media Video 9 IDCT",             &D3D11_DECODER_PROFILE_WMV9_IDCT,     0 },
    { "Windows Media Video 9 MoComp",           &D3D11_DECODER_PROFILE_WMV9_MOCOMP,   0 },
    { "Windows Media Video 9 post processing",  &D3D11_DECODER_PROFILE_WMV9_POSTPROC, 0 },

    { "VC-1 VLD",             &D3D11_DECODER_PROFILE_VC1_VLD,      AV_CODEC_ID_VC1 },
    { "VC-1 VLD",             &D3D11_DECODER_PROFILE_VC1_VLD,      AV_CODEC_ID_WMV3 },
    { "VC-1 VLD 2010",        &D3D11_DECODER_PROFILE_VC1_D2010,    AV_CODEC_ID_VC1 },
    { "VC-1 VLD 2010",        &D3D11_DECODER_PROFILE_VC1_D2010,    AV_CODEC_ID_WMV3 },
    { "VC-1 IDCT",            &D3D11_DECODER_PROFILE_VC1_IDCT,     0 },
    { "VC-1 MoComp",          &D3D11_DECODER_PROFILE_VC1_MOCOMP,   0 },
    { "VC-1 post processing", &D3D11_DECODER_PROFILE_VC1_POSTPROC, 0 },

    /* HEVC / H.265 */
    { "HEVC / H.265 variable-length decoder, main",   &D3D11_DECODER_PROFILE_HEVC_VLD_MAIN,   AV_CODEC_ID_HEVC },
    { "HEVC / H.265 variable-length decoder, main10", &D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10, AV_CODEC_ID_HEVC },

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
static const DXGI_FORMAT render_targets_dxgi[] = {
  DXGI_FORMAT_NV12,
  DXGI_FORMAT_P010,
  DXGI_FORMAT_P016,
  DXGI_FORMAT_UNKNOWN
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
    return nullptr;
}

//-----------------------------------------------------------------------------
// DXVA Context
//-----------------------------------------------------------------------------

CDXVAContext *CDXVAContext::m_context = nullptr;
CCriticalSection CDXVAContext::m_section;
CDXVAContext::CDXVAContext()
{
  m_context = nullptr;
  m_refCount = 0;
  m_service = nullptr;
  m_vcontext = nullptr;
  m_atiWorkaround = false;
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
    m_context = nullptr;
  }
}

void CDXVAContext::Close()
{
  CLog::Log(LOGNOTICE, "DXVA::Close - closing decoder context");
  DestroyContext();
}

bool CDXVAContext::EnsureContext(CDXVAContext **ctx, CDecoder *decoder)
{
  CSingleLock lock(m_section);
  if (m_context)
  {
    m_context->m_refCount++;
    *ctx = m_context;
    if (!m_context->IsValidDecoder(decoder))
      m_context->m_decoders.push_back(decoder);
    return true;
  }
  m_context = new CDXVAContext();
  *ctx = m_context;
  {
    if (!m_context->CreateContext())
    {
      delete m_context;
      m_context = nullptr;
      *ctx = nullptr;
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
  if ( FAILED(g_Windowing.Get3D11Device()->QueryInterface(__uuidof(ID3D11VideoDevice), reinterpret_cast<void**>(&m_service)))
    || FAILED(g_Windowing.GetImmediateContext()->QueryInterface(__uuidof(ID3D11VideoContext), reinterpret_cast<void**>(&m_vcontext))))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - failed to get Video Device and Context.");
    return false;
  }

  QueryCaps();

  // Some older Ati devices can only open a single decoder at a given time
  std::string renderer = g_Windowing.GetRenderRenderer();
  if (renderer.find("Radeon HD 2") != std::string::npos ||
      renderer.find("Radeon HD 3") != std::string::npos ||
      renderer.find("Radeon HD 4") != std::string::npos ||
      renderer.find("Radeon HD 5") != std::string::npos)
  {
    m_atiWorkaround = true;
  }

  return true;
}

void CDXVAContext::DestroyContext()
{
  delete[] m_input_list;
  SAFE_RELEASE(m_service);
  SAFE_RELEASE(m_vcontext);
}

void CDXVAContext::QueryCaps()
{
  m_input_count = m_service->GetVideoDecoderProfileCount();
  
  m_input_list = new GUID[m_input_count];
  for (unsigned i = 0; i < m_input_count; i++)
  {
    if (FAILED(m_service->GetVideoDecoderProfile(i, &m_input_list[i])))
    {
      CLog::Log(LOGNOTICE, "%s - failed getting device guids", __FUNCTION__);
      return;
    }
    const dxva2_mode_t *mode = dxva2_find_mode(&m_input_list[i]);
    if (mode)
      CLog::Log(LOGDEBUG, "DXVA - supports '%s'", mode->name);
    else
      CLog::Log(LOGDEBUG, "DXVA - supports %s", GUIDToString(m_input_list[i]).c_str());
  }
}

bool CDXVAContext::GetInputAndTarget(int codec, bool bHighBitdepth, GUID &inGuid, DXGI_FORMAT &outFormat)
{
  outFormat = DXGI_FORMAT_UNKNOWN;

  // iterate through our predefined dxva modes and find the first matching for desired codec
  // once we found a mode, get a target we support in render_targets_dxgi DXGI_FORMAT_UNKNOWN
  for (const dxva2_mode_t* mode = dxva2_modes; mode->name && outFormat == DXGI_FORMAT_UNKNOWN; mode++)
  {
    if (mode->codec != codec)
      continue;

    for (unsigned i = 0; i < m_input_count && outFormat == DXGI_FORMAT_UNKNOWN; i++)
    {
      bool supported = IsEqualGUID(m_input_list[i], *mode->guid);
      if (codec == AV_CODEC_ID_HEVC)
      {
        if (bHighBitdepth && !IsEqualGUID(m_input_list[i], D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10))
          supported = false;
        else if (!bHighBitdepth && IsEqualGUID(m_input_list[i], D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10))
          supported = false;
      }
      if (!supported)
        continue;

      CLog::Log(LOGDEBUG, "DXVA - trying '%s'", mode->name);
      for (unsigned j = 0; render_targets_dxgi[j] != DXGI_FORMAT_UNKNOWN && outFormat == DXGI_FORMAT_UNKNOWN; j++)
      {
        BOOL supported;
        if (bHighBitdepth && render_targets_dxgi[j] != DXGI_FORMAT_P010 && render_targets_dxgi[j] != DXGI_FORMAT_P016)
          continue;
        if (!bHighBitdepth && (render_targets_dxgi[j] == DXGI_FORMAT_P010 || render_targets_dxgi[j] == DXGI_FORMAT_P016))
          continue;

        HRESULT res = m_service->CheckVideoDecoderFormat(&m_input_list[i], render_targets_dxgi[j], &supported);
        if (FAILED(res))
        {
          CLog::Log(LOGNOTICE, "%s - failed check supported decoder format.", __FUNCTION__);
          break;
        }
        if (supported)
        {
          inGuid = m_input_list[i];
          outFormat = render_targets_dxgi[j];
          break;
        }
      }
    }
  }

  if (outFormat == DXGI_FORMAT_UNKNOWN)
    return false;

  return true;
}

bool CDXVAContext::GetConfig(const D3D11_VIDEO_DECODER_DESC *format, D3D11_VIDEO_DECODER_CONFIG &config)
{
  // find what decode configs are available
  UINT cfg_count = 0;
  HRESULT res = m_service->GetVideoDecoderConfigCount(format, &cfg_count);

  if (FAILED(res))
  {
    CLog::Log(LOGNOTICE, "%s - failed getting decoder configuration count", __FUNCTION__);
    return false;
  }

  config = {};
  unsigned bitstream = 2; // ConfigBitstreamRaw = 2 is required for Poulsbo and handles skipping better with nVidia
  for (unsigned i = 0; i< cfg_count; i++)
  {
    D3D11_VIDEO_DECODER_CONFIG pConfig = {0};
    if (FAILED(m_service->GetVideoDecoderConfig(format, i, &pConfig)))
    {
      CLog::Log(LOGNOTICE, "%s - failed getting decoder configuration", __FUNCTION__);
      return false;
    }

    CLog::Log(LOGDEBUG,
      "DXVA - config %d: bitstream type %d%s",
      i,
      pConfig.ConfigBitstreamRaw,
      IsEqualGUID(pConfig.guidConfigBitstreamEncryption, DXVA_NoEncrypt) ? "" : ", encrypted");

    // select first available
    if (config.ConfigBitstreamRaw == 0 && pConfig.ConfigBitstreamRaw != 0)
      config = pConfig;
    // override with preferred if found
    if (config.ConfigBitstreamRaw != bitstream && pConfig.ConfigBitstreamRaw == bitstream)
      config = pConfig;
  }

  if (!config.ConfigBitstreamRaw)
  {
    CLog::Log(LOGDEBUG, "DXVA - failed to find a raw input bitstream");
    return false;
  }

  return true;
}

bool CDXVAContext::CreateSurfaces(D3D11_VIDEO_DECODER_DESC format, unsigned int count, unsigned int alignment, ID3D11VideoDecoderOutputView **surfaces)
{
  HRESULT hr = S_OK;
  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  CD3D11_TEXTURE2D_DESC texDesc(format.OutputFormat, 
                                FFALIGN(format.SampleWidth, alignment), 
                                FFALIGN(format.SampleHeight, alignment), 
                                count, 1, D3D11_BIND_DECODER);

  ID3D11Texture2D *texture = nullptr;
  if (FAILED(pDevice->CreateTexture2D(&texDesc, NULL, &texture)))
  {
    CLog::Log(LOGERROR, "%s - failed creating decoder texture array", __FUNCTION__);
    return false;
  }

  D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC vdovDesc = {0};
  vdovDesc.DecodeProfile = format.Guid;
  vdovDesc.Texture2D.ArraySlice = 0;
  vdovDesc.ViewDimension = D3D11_VDOV_DIMENSION_TEXTURE2D;

  size_t i;
  for (i = 0; i < count; ++i)
  {
    vdovDesc.Texture2D.ArraySlice = D3D11CalcSubresource(0, i, texDesc.MipLevels);
    hr = m_service->CreateVideoDecoderOutputView(texture, &vdovDesc, &surfaces[i]);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "%s - failed creating surfaces", __FUNCTION__);
      break;
    }
  }
  SAFE_RELEASE(texture);

  if (FAILED(hr))
  {
    for (size_t j = 0; j < i; ++j)
      SAFE_RELEASE(surfaces[j]);
  }

  return SUCCEEDED(hr);
}

bool CDXVAContext::CreateDecoder(D3D11_VIDEO_DECODER_DESC *format, const D3D11_VIDEO_DECODER_CONFIG *config, ID3D11VideoDecoder **decoder, ID3D11VideoContext **context)
{
  CSingleLock lock(m_section);

  int retry = 0;
  while (retry < 2)
  {
    if (!m_atiWorkaround || retry > 0)
    {
      ID3D11VideoDecoder* pDecoder = nullptr;
      HRESULT res = m_service->CreateVideoDecoder(format, config, &pDecoder);
      if (!FAILED(res))
      {
        *decoder = pDecoder;
        *context = m_vcontext;
        m_vcontext->AddRef();
        return true;
      }
    }

    if (retry == 0)
    {
      CLog::Log(LOGNOTICE, "%s - hw may not support multiple decoders, releasing existing ones", __FUNCTION__);
      std::vector<CDecoder*>::iterator it;
      for (it = m_decoders.begin(); it != m_decoders.end(); ++it)
      {
        (*it)->CloseDXVADecoder();
      }
    }
    retry++;
  }

  CLog::Log(LOGERROR, "%s - failed creating decoder", __FUNCTION__);
  return false;
}

bool CDXVAContext::IsValidDecoder(CDecoder *decoder)
{
  std::vector<CDecoder*>::iterator it;
  it = find(m_decoders.begin(), m_decoders.end(), decoder);
  if (it != m_decoders.end())
    return true;
  return false;
}

//-----------------------------------------------------------------------------
// DXVA Video Surface states
//-----------------------------------------------------------------------------
#define SURFACE_USED_FOR_REFERENCE 0x01
#define SURFACE_USED_FOR_RENDER 0x02

CSurfaceContext::CSurfaceContext()
{
}

CSurfaceContext::~CSurfaceContext()
{
  CLog::Log(LOGDEBUG, "%s - destructing surface context", __FUNCTION__);
  Reset();
}

void CSurfaceContext::AddSurface(ID3D11View* view)
{
  CSingleLock lock(m_section);
  m_state[view] = 0;
  m_freeViews.push_back(view);
}

void CSurfaceContext::ClearReference(ID3D11View* surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) == m_state.end())
  {
    CLog::Log(LOGWARNING, "%s - surface invalid", __FUNCTION__);
    return;
  }
  m_state[surf] &= ~SURFACE_USED_FOR_REFERENCE;
  if (m_state[surf] == 0)
  {
    m_freeViews.push_back(surf);
  }
}

bool CSurfaceContext::MarkRender(ID3D11View* surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) == m_state.end())
  {
    CLog::Log(LOGWARNING, "%s - surface invalid", __FUNCTION__);
    return false;
  }
  std::list<ID3D11View*>::iterator it;
  it = std::find(m_freeViews.begin(), m_freeViews.end(), surf);
  if (it != m_freeViews.end())
  {
    m_freeViews.erase(it);
  }
  m_state[surf] |= SURFACE_USED_FOR_RENDER;
  return true;
}

void CSurfaceContext::ClearRender(ID3D11View* surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) == m_state.end())
  {
    CLog::Log(LOGWARNING, "%s - surface invalid", __FUNCTION__);
    return;
  }
  m_state[surf] &= ~SURFACE_USED_FOR_RENDER;
  if (m_state[surf] == 0)
  {
    m_freeViews.push_back(surf);
  }
}

bool CSurfaceContext::IsValid(ID3D11View* surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) != m_state.end())
    return true;
  else
    return false;
}

ID3D11View* CSurfaceContext::GetFree(ID3D11View* surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) != m_state.end())
  {
    std::list<ID3D11View*>::iterator it;
    it = std::find(m_freeViews.begin(), m_freeViews.end(), surf);
    if (it == m_freeViews.end())
    {
      CLog::Log(LOGWARNING, "%s - surface not free", __FUNCTION__);
    }
    else
    {
      m_freeViews.erase(it);
      m_state[surf] = SURFACE_USED_FOR_REFERENCE;
      return surf;
    }
  }
  if (!m_freeViews.empty())
  {
    ID3D11View* freeSurf = m_freeViews.front();
    m_freeViews.pop_front();
    m_state[freeSurf] = SURFACE_USED_FOR_REFERENCE;
    return freeSurf;
  }
  return nullptr;
}

ID3D11View* CSurfaceContext::GetAtIndex(unsigned int idx)
{
  if (idx >= m_state.size())
    return nullptr;
  std::map<ID3D11View*, int>::iterator it = m_state.begin();
  for (unsigned int i = 0; i < idx; i++)
    ++it;
  return it->first;
}

void CSurfaceContext::Reset()
{
  CSingleLock lock(m_section);
  for (std::map<ID3D11View*, int>::iterator it = m_state.begin(); it != m_state.end(); ++it)
  {
    it->first->Release();
  }
  m_freeViews.clear();
  m_state.clear();
}

int CSurfaceContext::Size()
{
  CSingleLock lock(m_section);
  return m_state.size();
}

bool CSurfaceContext::HasFree()
{
  CSingleLock lock(m_section);
  return !m_freeViews.empty();
}

bool CSurfaceContext::HasRefs()
{
  CSingleLock lock(m_section);
  for (std::map<ID3D11View*, int>::iterator it = m_state.begin(); it != m_state.end(); ++it)
  {
    if (it->second & SURFACE_USED_FOR_REFERENCE)
      return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// DXVA RenderPictures
//-----------------------------------------------------------------------------

CRenderPicture::CRenderPicture(CSurfaceContext *context)
{
  surface_context = context->Acquire();
}

CRenderPicture::~CRenderPicture()
{
  surface_context->ClearRender(view);
  surface_context->Release();
}

//-----------------------------------------------------------------------------
// DXVA Decoder
//-----------------------------------------------------------------------------

CDecoder::CDecoder(CProcessInfo& processInfo)
 : m_event(true),
   m_processInfo(processInfo)
{
  m_event.Set();
  m_state     = DXVA_OPEN;
  m_device    = nullptr;
  m_decoder   = nullptr;
  m_vcontext  = nullptr;
  m_refs         = 0;
  m_shared       = 0;
  m_surface_context = nullptr;
  m_presentPicture = nullptr;
  m_dxva_context = nullptr;
  memset(&m_format, 0, sizeof(m_format));
  m_context          = (AVD3D11VAContext*)calloc(1, sizeof(AVD3D11VAContext));
  m_context->cfg     = reinterpret_cast<D3D11_VIDEO_DECODER_CONFIG*>(calloc(1, sizeof(D3D11_VIDEO_DECODER_CONFIG)));
  m_context->surface = reinterpret_cast<ID3D11VideoDecoderOutputView**>(calloc(32, sizeof(ID3D11VideoDecoderOutputView*)));
  m_surface_alignment = 16;
  g_Windowing.Register(this);
}

CDecoder::~CDecoder()
{
  CLog::Log(LOGDEBUG, "%s - destructing decoder, %p", __FUNCTION__, this);
  g_Windowing.Unregister(this);
  Close();
  free(m_context->surface);
  free(m_context->cfg);
  free(m_context);
}

long CDecoder::Release()
{
  // if ffmpeg holds any references, flush buffers
  if (m_surface_context && m_surface_context->HasRefs())
  {
    avcodec_flush_buffers(m_avctx);
  }
  return IHardwareDecoder::Release();
}

void CDecoder::Close()
{
  CSingleLock lock(m_section);
  SAFE_RELEASE(m_decoder);
  SAFE_RELEASE(m_vcontext);
  SAFE_RELEASE(m_surface_context);
  SAFE_RELEASE(m_presentPicture);
  memset(&m_format, 0, sizeof(m_format));

  if (m_dxva_context)
  {
    CLog::Log(LOGNOTICE, "%s - closing decoder", __FUNCTION__);
    m_dxva_context->Release(this);
  }
  m_dxva_context = nullptr;
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
  DXGI_ADAPTER_DESC AIdentifier = g_Windowing.GetAIdentifier();

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

  DXGI_ADAPTER_DESC AIdentifier = g_Windowing.GetAIdentifier();

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

static bool HasATIMP2Bug(AVCodecContext *avctx)
{
  DXGI_ADAPTER_DESC AIdentifier = g_Windowing.GetAIdentifier();
  if (AIdentifier.VendorId != PCIV_ATI)
    return false;

  // AMD/ATI card doesn't like some SD MPEG2 content
  // here are params of these videos
  return avctx->height <= 576
      && avctx->colorspace == AVCOL_SPC_BT470BG
      && avctx->color_primaries == AVCOL_PRI_BT470BG 
      && avctx->color_trc == AVCOL_TRC_GAMMA28;
}

static bool CheckCompatibility(AVCodecContext *avctx)
{
  if (avctx->codec_id == AV_CODEC_ID_MPEG2VIDEO && HasATIMP2Bug(avctx))
    return false;

  // The incompatibilities are all for H264
  if(avctx->codec_id != AV_CODEC_ID_H264)
    return true;

  // Macroblock width incompatibility
  if (HasVP3WidthBug(avctx))
  {
    CLog::Log(LOGWARNING,"DXVA - width %i is not supported with nVidia VP3 hardware. DXVA will not be used", avctx->coded_width);
    return false;
  }

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
      CLog::Log(LOGWARNING, "DXVA - compatibility check: video exceeds L4.1. DXVA will not be used.");
      return false;
  }

  return true;
}

bool CDecoder::Open(AVCodecContext *avctx, AVCodecContext* mainctx, enum AVPixelFormat fmt, unsigned int surfaces)
{
  if (!CheckCompatibility(avctx))
    return false;

  CSingleLock lock(m_section);
  Close();

  if(m_state == DXVA_LOST)
  {
    CLog::Log(LOGDEBUG, "DXVA - device is in lost state, we can't start");
    return false;
  }

  CLog::Log(LOGDEBUG, "DXVA - open decoder");
  if (!CDXVAContext::EnsureContext(&m_dxva_context, this))
    return false;

  bool bHighBitdepth = (avctx->codec_id == AV_CODEC_ID_HEVC && (avctx->sw_pix_fmt == AV_PIX_FMT_YUV420P10 || avctx->profile == FF_PROFILE_HEVC_MAIN_10));
  if (!m_dxva_context->GetInputAndTarget(avctx->codec_id, bHighBitdepth, m_format.Guid, m_format.OutputFormat))
  {
    CLog::Log(LOGDEBUG, "DXVA - unable to find an input/output format combination");
    return false;
  }

  CLog::Log(LOGDEBUG, "DXVA - Selected input/output format: %d", m_format.OutputFormat);
  CLog::Log(LOGDEBUG, "DXVA - source requires %d references", avctx->refs);
  if (m_format.Guid == DXVADDI_Intel_ModeH264_E && avctx->refs > 11)
  {
    const dxva2_mode_t *mode = dxva2_find_mode(&m_format.Guid);
    CLog::Log(LOGWARNING, "DXVA - too many references %d for selected decoder '%s'.", avctx->refs, mode->name);
    return false;
  }

  m_format.SampleWidth = avctx->coded_width;
  m_format.SampleHeight = avctx->coded_height;

  if (surfaces > m_shared)
    m_shared = surfaces;

  if(avctx->refs > m_refs)
    m_refs = avctx->refs+2;
  if (avctx->codec_id == AV_CODEC_ID_HEVC)
    m_refs = 16;

  if(m_refs == 0)
  {
    if( avctx->codec_id == AV_CODEC_ID_H264
     || avctx->codec_id == AV_CODEC_ID_HEVC)
      m_refs = 16;
    else
      m_refs = 2;
  }
  /* decoding MPEG-2 requires additional alignment on some Intel GPUs,
     but it causes issues for H.264 on certain AMD GPUs..... */
  if (avctx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
    m_surface_alignment = 32;
  /* the HEVC DXVA2 spec asks for 128 pixel aligned surfaces to ensure
     all coding features have enough room to work with */
  else if (avctx->codec_id == AV_CODEC_ID_HEVC)
    m_surface_alignment = 128;
  else
    m_surface_alignment = 16;

  if (!m_dxva_context->GetConfig(&m_format, *m_context->cfg))
    return false;

  m_surface_context = new CSurfaceContext();

  if(!OpenDecoder())
    return false;

  avctx->get_buffer2 = GetBufferS;
  avctx->hwaccel_context = m_context;
  avctx->slice_flags = SLICE_FLAG_ALLOW_FIELD | SLICE_FLAG_CODED_ORDER;

  mainctx->get_buffer2 = GetBufferS;
  mainctx->hwaccel_context = m_context;
  mainctx->slice_flags = SLICE_FLAG_ALLOW_FIELD | SLICE_FLAG_CODED_ORDER;

  m_avctx = mainctx;
  DXGI_ADAPTER_DESC AIdentifier = g_Windowing.GetAIdentifier();
  if (AIdentifier.VendorId == PCIV_Intel && m_format.Guid == DXVADDI_Intel_ModeH264_E)
  {
#ifdef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO
    m_context->workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
#else
    CLog::Log(LOGWARNING, "DXVA - used Intel ClearVideo decoder, but no support workaround for it in libavcodec");
#endif
  }
  else if (AIdentifier.VendorId == PCIV_ATI && IsL41LimitedATI())
  {
#ifdef FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG
    m_context->workaround |= FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG;
#else
    CLog::Log(LOGWARNING, "DXVA - video card with different scaling list zigzag order detected, but no support in libavcodec");
#endif
  }

  std::list<EINTERLACEMETHOD> deintMethods;
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_DXVA_AUTO);
  m_processInfo.UpdateDeinterlacingMethods(deintMethods);
  m_processInfo.SetDeinterlacingMethodDefault(EINTERLACEMETHOD::VS_INTERLACEMETHOD_DXVA_AUTO);

  m_state = DXVA_OPEN;
  return true;
}

CDVDVideoCodec::VCReturn CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  CSingleLock lock(m_section);
  CDVDVideoCodec::VCReturn result = Check(avctx);
  if(result != CDVDVideoCodec::VC_NONE)
    return result;

  if(frame)
  {
    if (m_surface_context->IsValid(reinterpret_cast<ID3D11View*>(frame->data[3])))
    {
      SAFE_RELEASE(m_presentPicture);
      m_presentPicture = new CRenderPicture(m_surface_context);
      m_presentPicture->view = reinterpret_cast<ID3D11View*>(frame->data[3]);
      m_surface_context->MarkRender(m_presentPicture->view);
      return CDVDVideoCodec::VC_PICTURE;
    }
    CLog::Log(LOGWARNING, "DXVA - ignoring invalid surface");
    return CDVDVideoCodec::VC_BUFFER;
  }
  else
    return CDVDVideoCodec::VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, DVDVideoPicture* picture)
{
  ((ICallbackHWAccel*)avctx->opaque)->GetPictureCommon(picture);
  CSingleLock lock(m_section);

  picture->dxva = m_presentPicture;
  picture->format = RENDER_FMT_DXVA;
  picture->extended_format = (unsigned int)m_format.OutputFormat;

  int queued, discard, free;
  m_processInfo.GetRenderBuffers(queued, discard, free);
  if (free > 1)
  {
    g_Windowing.RequestDecodingTime();
  }
  else
  {
    g_Windowing.ReleaseDecodingTime();
  }

  return true;
}

CDVDVideoCodec::VCReturn CDecoder::Check(AVCodecContext* avctx)
{
  CSingleLock lock(m_section);

  // we may not have a hw decoder on systems (AMD HD2xxx, HD3xxx) which are only capable
  // of opening a single decoder and VideoPlayer opened a new stream without having flushed
  // current one.
  if (!m_decoder)
    return CDVDVideoCodec::VC_BUFFER;

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
      return CDVDVideoCodec::VC_ERROR;
    }
  }

  if(m_format.SampleWidth  == 0
  || m_format.SampleHeight == 0)
  {
    if(!Open(avctx, avctx, avctx->pix_fmt, m_shared))
    {
      CLog::Log(LOGERROR, "CDecoder::Check - decoder was not able to reset");
      Close();
      return CDVDVideoCodec::VC_ERROR;
    }
    return CDVDVideoCodec::VC_FLUSHED;
  }
  else
  {
    if(avctx->refs > m_refs)
    {
      CLog::Log(LOGWARNING, "CDecoder::Check - number of required reference frames increased, recreating decoder");
      Close();
      return CDVDVideoCodec::VC_FLUSHED;
    }
  }

  // Status reports are available only for the DXVA2_ModeH264 and DXVA2_ModeVC1 modes
  if(avctx->codec_id != AV_CODEC_ID_H264
  && avctx->codec_id != AV_CODEC_ID_VC1
  && avctx->codec_id != AV_CODEC_ID_WMV3)
    return CDVDVideoCodec::VC_NONE;
  
  D3D11_VIDEO_DECODER_EXTENSION data = {0};
  union {
    DXVA_Status_H264 h264;
    DXVA_Status_VC1  vc1;
  } status = {};

  /* I'm not sure, but MSDN says nothing about extensions functions in D3D11, try to using with same way as in DX9 */
  data.Function = DXVA_STATUS_REPORTING_FUNCTION;
  data.pPrivateOutputData    = &status;
  data.PrivateOutputDataSize = avctx->codec_id == AV_CODEC_ID_H264 ? sizeof(DXVA_Status_H264) : sizeof(DXVA_Status_VC1);
  HRESULT hr;
  if (FAILED(hr = m_dxva_context->GetVideoContext()->DecoderExtension(m_decoder, &data)))
  {
    CLog::Log(LOGWARNING, "DXVA - failed to get decoder status - 0x%08X", hr);
    return CDVDVideoCodec::VC_ERROR;
  }

  if(avctx->codec_id == AV_CODEC_ID_H264)
  {
    if(status.h264.bStatus)
      CLog::Log(LOGWARNING, "DXVA - decoder problem of status %d with %d", status.h264.bStatus, status.h264.bBufType);
  }
  else
  {
    if(status.vc1.bStatus)
      CLog::Log(LOGWARNING, "DXVA - decoder problem of status %d with %d", status.vc1.bStatus, status.vc1.bBufType);
  }
  return CDVDVideoCodec::VC_NONE;
}

bool CDecoder::OpenDecoder()
{
  SAFE_RELEASE(m_decoder);
  SAFE_RELEASE(m_vcontext);
  m_context->decoder = nullptr;
  m_context->video_context = nullptr;

  m_context->surface_count = m_refs + 1 + 1 + m_shared; // refs + 1 decode + 1 libavcodec safety + processor buffer

  CLog::Log(LOGDEBUG, "DXVA - allocating %d surfaces with format %d", m_context->surface_count, m_format.OutputFormat);

  if (!m_dxva_context->CreateSurfaces(m_format, m_context->surface_count, m_surface_alignment, m_context->surface))
    return false;

  for(unsigned i = 0; i < m_context->surface_count; i++)
  {
    m_surface_context->AddSurface(m_context->surface[i]);
  }

  if (!m_dxva_context->CreateDecoder(&m_format, m_context->cfg, &m_decoder, &m_vcontext))
    return false;

  m_context->decoder = m_decoder;
  m_context->video_context = m_vcontext;

  return true;
}

bool CDecoder::Supports(enum AVPixelFormat fmt)
{
  if(fmt == AV_PIX_FMT_D3D11VA_VLD)
    return true;
  return false;
}

void CDecoder::RelBuffer(uint8_t *data)
{
  {
    CSingleLock lock(m_section);
    ID3D11VideoDecoderOutputView* view = (ID3D11VideoDecoderOutputView*)(uintptr_t)data;

    if (!m_surface_context->IsValid(view))
    {
      CLog::Log(LOGWARNING, "%s - return of invalid surface", __FUNCTION__);
    }
    m_surface_context->ClearReference(view);
  }

  IHardwareDecoder::Release();
}

int CDecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags)
{
  CSingleLock lock(m_section);

  if (!m_decoder)
    return -1;

  ID3D11View* view = reinterpret_cast<ID3D11View*>(pic->data[3]);
  view = m_surface_context->GetFree(view != nullptr ? view : nullptr);
  if (view == nullptr)
  {
    CLog::Log(LOGERROR, "%s - no surface available", __FUNCTION__);
    m_state = DXVA_LOST;
    return -1;
  }

  pic->reordered_opaque = avctx->reordered_opaque;

  for(unsigned i = 0; i < 4; i++)
  {
    pic->data[i] = nullptr;
    pic->linesize[i] = 0;
  }

  pic->data[0] = (uint8_t*)view;
  pic->data[3] = (uint8_t*)view;
  AVBufferRef *buffer = av_buffer_create(pic->data[3], 0, RelBufferS, this, 0);
  if (!buffer)
  {
    CLog::Log(LOGERROR, "DXVA - error creating buffer");
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
