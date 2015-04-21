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
#include <dxva2api.h>

#include "DXVAHD.h"
#include "windowing/WindowingFactory.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "utils/AutoPtrHandle.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "RenderFlags.h"
#include "win32/WIN32Util.h"
#include "utils/Log.h"

using namespace DXVA;
using namespace AUTOPTR;
using namespace std;

#define CHECK(a) \
do { \
  HRESULT res = a; \
  if(FAILED(res)) \
  { \
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing "#a" at line %d with error %x", __LINE__, res); \
    return false; \
  } \
} while(0);

#define LOGIFERROR(a) \
do { \
  HRESULT res = a; \
  if(FAILED(res)) \
  { \
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing "#a" at line %d with error %x", __LINE__, res); \
  } \
} while(0);

static std::string GUIDToString(const GUID& guid)
{
  std::string buffer = StringUtils::Format("%08X-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
                                          , guid.Data1, guid.Data2, guid.Data3
                                          , guid.Data4[0], guid.Data4[1]
                                          , guid.Data4[2], guid.Data4[3], guid.Data4[4]
                                          , guid.Data4[5], guid.Data4[6], guid.Data4[7]);
  return buffer;
}

DXVAHDCreateVideoServicePtr CProcessorHD::m_DXVAHDCreateVideoService = NULL;

CProcessorHD::CProcessorHD()
{
  m_pDXVAHD = NULL;
  m_pDXVAVP = NULL;
  g_Windowing.Register(this);

  m_context  = NULL;
}

CProcessorHD::~CProcessorHD()
{
  g_Windowing.Unregister(this);
  UnInit();
}

void CProcessorHD::UnInit()
{
  CSingleLock lock(m_section);
  Close();
  SAFE_RELEASE(m_pDXVAHD);
}

void CProcessorHD::Close()
{
  CSingleLock lock(m_section);
  SAFE_RELEASE(m_pDXVAVP);
  SAFE_RELEASE(m_context);
}

bool CProcessorHD::UpdateSize(const DXVA2_VideoDesc& dsc)
{
  return true;
}

bool CProcessorHD::PreInit()
{
  if (!LoadSymbols())
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - DXVAHD not loaded.");
    return false;
  }

  UnInit();

  CSingleLock lock(m_section);

  DXVAHD_RATIONAL fps = { 60, 1 }; 
  DXVAHD_CONTENT_DESC desc;
  desc.InputFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;
  desc.InputFrameRate = fps;
  desc.InputWidth = 640;
  desc.InputHeight = 480;
  desc.OutputFrameRate = fps;
  desc.OutputWidth = 640;
  desc.OutputHeight = 480;

  HRESULT cvres = m_DXVAHDCreateVideoService( (IDirect3DDevice9Ex*)g_Windowing.Get3DDevice()
                                              , &desc
                                              , DXVAHD_DEVICE_USAGE_OPTIMAL_QUALITY
                                              , NULL
                                              , &m_pDXVAHD );

  if(FAILED(cvres))
  {
    if(cvres == E_NOINTERFACE)
      CLog::Log(LOGNOTICE, __FUNCTION__" - The Direct3d device doesn't support DXVA-HD.");
    else
      CLog::Log(LOGERROR, __FUNCTION__" - failed to create DXVAHD device %x", cvres);

    return false;
  }

  CHECK(m_pDXVAHD->GetVideoProcessorDeviceCaps( &m_VPDevCaps ));

  if (m_VPDevCaps.VideoProcessorCount == 0)
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - unable to find any video processor. GPU drivers doesn't support DXVA-HD.");
    return false;
  }

  // Create the array of video processor caps. 
  DXVAHD_VPCAPS* pVPCaps = new (std::nothrow) DXVAHD_VPCAPS[ m_VPDevCaps.VideoProcessorCount ];
  if (pVPCaps == NULL)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - unable to create video processor caps array. Out of memory.");
    return false;
  }

  HRESULT hr = m_pDXVAHD->GetVideoProcessorCaps( m_VPDevCaps.VideoProcessorCount, pVPCaps );
  if(FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed get processor caps with error %x.", hr);

    delete [] pVPCaps;
    return false;
  }

  m_max_back_refs = 0;
  m_max_fwd_refs = 0;

  for (unsigned int i = 0; i < m_VPDevCaps.VideoProcessorCount; i++)
  {
    if (pVPCaps[i].FutureFrames > m_max_fwd_refs)
    {
      m_max_fwd_refs = pVPCaps[i].FutureFrames;
    }

    if (pVPCaps[i].PastFrames > m_max_back_refs)
    {
      m_max_back_refs = pVPCaps[i].PastFrames;
    }
  }

  m_size = m_max_back_refs + 1 + m_max_fwd_refs + 2;  // refs + 1 display + 2 safety frames

  // Get the image filtering capabilities.
  for (long i = 0; i < NUM_FILTERS; i++)
  {
    if (m_VPDevCaps.FilterCaps & (1 << i))
    {
      m_pDXVAHD->GetVideoProcessorFilterRange(PROCAMP_FILTERS[i], &m_Filters[i].Range);
      m_Filters[i].bSupported = true;
    }
    else
    {
      m_Filters[i].bSupported = false;
    }
  }

  m_VPCaps = pVPCaps[0];
  m_device = m_VPCaps.VPGuid;

  delete [] pVPCaps;

  return true;
}

bool CProcessorHD::Open(UINT width, UINT height, unsigned int flags, unsigned int format, unsigned int extended_format)
{
  Close();

  CSingleLock lock(m_section);

  if (!m_pDXVAHD)
  {
    return false;
  }

  m_width = width;
  m_height = height;
  m_flags = flags;
  m_renderFormat = format;

  if (g_advancedSettings.m_DXVANoDeintProcForProgressive)
  {
    CLog::Log(LOGNOTICE, __FUNCTION__" - Auto deinterlacing mode workaround activated. Deinterlacing processor will be used only for interlaced frames.");
  }

  if (format == RENDER_FMT_DXVA)
  {
    m_format = (D3DFORMAT)extended_format;
  }
  else
  {
    // Only NV12 software colorspace conversion is implemented for now
    m_format = (D3DFORMAT)MAKEFOURCC('N','V','1','2');
    if (!CreateSurfaces())
      return false;
  }

  if (!OpenProcessor())
  {
    return false;
  }

  return true;
}

bool CProcessorHD::ReInit()
{
  return PreInit() && (m_renderFormat == RENDER_FMT_DXVA || CreateSurfaces());
}

bool CProcessorHD::OpenProcessor()
{
  // restore the device if it was lost
  if (!m_pDXVAHD && !ReInit())
  {
    return false;
  }

  SAFE_RELEASE(m_pDXVAVP);

  CLog::Log(LOGDEBUG, __FUNCTION__" - processor selected %s.", GUIDToString(m_device).c_str());

  CHECK(m_pDXVAHD->CreateVideoProcessor(&m_device, &m_pDXVAVP));

  DXVAHD_STREAM_STATE_D3DFORMAT_DATA d3dformat = { m_format };
  CHECK(m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_D3DFORMAT
                                             , sizeof(d3dformat), &d3dformat ));

  DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE_DATA data =
  {
    0,                                          // Type: 0=Video, 1=Graphics
    0,                                          // RGB_Range: 0=Full, 1=Limited
    m_flags & CONF_FLAGS_YUVCOEF_BT709 ? 1 : 0, // YCbCr_Matrix: 0=BT.601, 1=BT.709
    m_flags & CONF_FLAGS_YUV_FULLRANGE ? 1 : 0  // YCbCr_xvYCC: 0=Conventional YCbCr, 1=xvYCC
  };
  CHECK(m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE
                                             , sizeof(data), &data ));

  DXVAHD_COLOR_YCbCrA bgColor = { 0.0625f, 0.5f, 0.5f, 1.0f }; // black color
  DXVAHD_COLOR backgroundColor;
  backgroundColor.YCbCr = bgColor; 
  DXVAHD_BLT_STATE_BACKGROUND_COLOR_DATA backgroundData = { true, backgroundColor }; // {YCbCr, DXVAHD_COLOR}
  CHECK(m_pDXVAVP->SetVideoProcessBltState( DXVAHD_BLT_STATE_BACKGROUND_COLOR
                                          , sizeof (backgroundData), &backgroundData ));

  DXVAHD_STREAM_STATE_ALPHA_DATA alpha = { true, 1.0f };
  CHECK(m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_ALPHA
                                             , sizeof(alpha), &alpha ));

  return true;
}

bool CProcessorHD::CreateSurfaces()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  LPDIRECT3DSURFACE9 surfaces[32];
  for (unsigned idx = 0; idx < m_size; idx++)
  {
    CHECK(pD3DDevice->CreateOffscreenPlainSurface(
      (m_width + 15) & ~15,
      (m_height + 15) & ~15,
      m_format,
      m_VPDevCaps.InputPool,
      &surfaces[idx],
      NULL));
  }

  m_context = new CSurfaceContext();
  for (int i = 0; i < m_size; i++)
  {
    m_context->AddSurface(surfaces[i]);
  }

  return true;
}

bool CProcessorHD::ApplyFilter(DXVAHD_FILTER filter, int value, int min, int max, int def)
{
  if (filter >= NUM_FILTERS)
    return false;

  // Unsupported filter. Ignore.
  if (!m_Filters[filter].bSupported)
  {
    return false;
  }

  DXVAHD_FILTER_RANGE_DATA range = m_Filters[filter].Range;
  int val;

  if(value > def)
  {
    val = range.Default + (range.Maximum - range.Default) * (value - def) / (max - def);
  }
  else if(value < def)
  {
    val = range.Default + (range.Minimum - range.Default) * (value - def) / (min - def);
  }
  else
  {
    val = range.Default;
  }

  DXVAHD_STREAM_STATE_FILTER_DATA data = { true, val };
  DXVAHD_STREAM_STATE state = static_cast<DXVAHD_STREAM_STATE>(DXVAHD_STREAM_STATE_FILTER_BRIGHTNESS + filter);

  return !FAILED( m_pDXVAVP->SetVideoProcessStreamState( 0, state, sizeof(data), &data ) );
}

bool CProcessorHD::Render(CRect src, CRect dst, IDirect3DSurface9* target, IDirect3DSurface9** source, DWORD flags, UINT frameIdx)
{
  CSingleLock lock(m_section);

  // restore processor if it was lost
  if(!m_pDXVAVP && !OpenProcessor())
  {
    return false;
  }
  
  if (!source[2])
    return false;

  EDEINTERLACEMODE deinterlace_mode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
  if (g_advancedSettings.m_DXVANoDeintProcForProgressive)
    deinterlace_mode = (flags & RENDER_FLAG_FIELD0 || flags & RENDER_FLAG_FIELD1) ? VS_DEINTERLACEMODE_FORCE : VS_DEINTERLACEMODE_OFF;
  EINTERLACEMETHOD interlace_method = g_renderManager.AutoInterlaceMethod(CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod);

  bool progressive = deinterlace_mode == VS_DEINTERLACEMODE_OFF
                  || (   interlace_method != VS_INTERLACEMETHOD_DXVA_BOB
                      && interlace_method != VS_INTERLACEMETHOD_DXVA_BEST);

  D3DSURFACE_DESC desc;
  CHECK(target->GetDesc(&desc));
  CRect rectTarget(0, 0, desc.Width, desc.Height);
  CWIN32Util::CropSource(src, dst, rectTarget);
  RECT sourceRECT = { src.x1, src.y1, src.x2, src.y2 };
  RECT dstRECT    = { dst.x1, dst.y1, dst.x2, dst.y2 };

  DXVAHD_FRAME_FORMAT dxvaFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;

  unsigned int providedPast = 0;
  for (int i = 3; i < 8; i++)
  {
    if (source[i])
      providedPast++;
  }
  unsigned int providedFuture = 0;
  for (int i = 1; i >= 0; i--)
  {
    if (source[i])
      providedFuture++;
  }
  int futureFrames = std::min(providedFuture, m_VPCaps.FutureFrames);
  int pastFrames = std::min(providedPast, m_VPCaps.PastFrames);

  DXVAHD_STREAM_DATA stream_data = { 0 };
  stream_data.Enable = TRUE;
  stream_data.PastFrames = pastFrames;
  stream_data.FutureFrames = futureFrames;
  stream_data.ppPastSurfaces = new IDirect3DSurface9*[pastFrames];
  stream_data.ppFutureSurfaces = new IDirect3DSurface9*[futureFrames];

  int start = 2 - futureFrames;
  int end = 2 + pastFrames;

  for (int i = start; i <= end; i++)
  {
    if (!source[i])
      continue;

    if (i > 2)
    {
      // frames order should be { ?, T-3, T-2, T-1 }
      stream_data.ppPastSurfaces[2+pastFrames-i] = source[i];
    }
    else if (i == 2)
    {
      stream_data.pInputSurface = source[2];
    }
    else if (i < 2)
    {
      // frames order should be { T+1, T+2, T+3, .. }
      stream_data.ppFutureSurfaces[1-i] = source[i];
    }
  }

  if (flags & RENDER_FLAG_FIELD0 && flags & RENDER_FLAG_TOP)
    dxvaFrameFormat = DXVAHD_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  else if (flags & RENDER_FLAG_FIELD1 && flags & RENDER_FLAG_BOT)
    dxvaFrameFormat = DXVAHD_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  if (flags & RENDER_FLAG_FIELD0 && flags & RENDER_FLAG_BOT)
    dxvaFrameFormat = DXVAHD_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;
  if (flags & RENDER_FLAG_FIELD1 && flags & RENDER_FLAG_TOP)
    dxvaFrameFormat = DXVAHD_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;

  // Override the sample format when the processor doesn't need to deinterlace or when deinterlacing is forced and flags are missing.
  if (progressive)
  {
    dxvaFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;
  }
  else if (deinterlace_mode == VS_DEINTERLACEMODE_FORCE 
        && dxvaFrameFormat  == DXVAHD_FRAME_FORMAT_PROGRESSIVE)
  {
    dxvaFrameFormat = DXVAHD_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  }

  bool frameProgressive = dxvaFrameFormat == DXVAHD_FRAME_FORMAT_PROGRESSIVE;

  // Progressive or Interlaced video at normal rate.
  stream_data.InputFrameOrField = frameIdx;
  stream_data.OutputIndex = flags & RENDER_FLAG_FIELD1 && !frameProgressive ? 1 : 0;

  DXVAHD_STREAM_STATE_FRAME_FORMAT_DATA frame_format = { dxvaFrameFormat };
  LOGIFERROR( m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_FRAME_FORMAT
                                                   , sizeof(frame_format), &frame_format ) );

  DXVAHD_STREAM_STATE_DESTINATION_RECT_DATA dstRect = { true, dstRECT };
  LOGIFERROR( m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_DESTINATION_RECT
                                                   , sizeof(dstRect), &dstRect));

  DXVAHD_STREAM_STATE_SOURCE_RECT_DATA srcRect = { true, sourceRECT };
  LOGIFERROR( m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_SOURCE_RECT
                                                   , sizeof(srcRect), &srcRect));

  ApplyFilter( DXVAHD_FILTER_BRIGHTNESS, CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness
                                             , 0, 100, 50);
  ApplyFilter( DXVAHD_FILTER_CONTRAST, CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast
                                             , 0, 100, 50);

  unsigned int uiRange = g_Windowing.UseLimitedColor() ? 1 : 0;
  DXVAHD_BLT_STATE_OUTPUT_COLOR_SPACE_DATA colorData = 
  {
    0,        // 0 = playback, 1 = video processing
    uiRange,  // 0 = 0-255, 1 = 16-235
    1,        // 0 = BT.601, 1 = BT.709
    1         // 0 = Conventional YCbCr, 1 = xvYCC
  };

  LOGIFERROR( m_pDXVAVP->SetVideoProcessBltState( DXVAHD_BLT_STATE_OUTPUT_COLOR_SPACE
                                                , sizeof(colorData), &colorData ));

  DXVAHD_BLT_STATE_TARGET_RECT_DATA targetRect = { true, dstRECT };
  LOGIFERROR( m_pDXVAVP->SetVideoProcessBltState( DXVAHD_BLT_STATE_TARGET_RECT
                                                , sizeof(targetRect), &targetRect ) );

  HRESULT hr = m_pDXVAVP->VideoProcessBltHD(target, frameIdx, 1, &stream_data);
  if(FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing VideoProcessBltHD with error %x", hr);
  }

  delete [] stream_data.ppPastSurfaces;
  delete [] stream_data.ppFutureSurfaces;

  return !FAILED(hr);
}

bool CProcessorHD::LoadSymbols()
{
  CSingleLock lock(m_dlSection);
  if(m_dlHandle == NULL)
    m_dlHandle = LoadLibraryEx("dxva2.dll", NULL, 0);
  if(m_dlHandle == NULL)
    return false;
  m_DXVAHDCreateVideoService = (DXVAHDCreateVideoServicePtr)GetProcAddress(m_dlHandle, "DXVAHD_CreateDevice");
  if(m_DXVAHDCreateVideoService == NULL)
    return false;
  return true;
}

#endif
