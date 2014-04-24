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

#include "DXVAHD.h"
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

#define ALLOW_ADDING_SURFACES 0

using namespace DXVA;
using namespace AUTOPTR;
using namespace std;

typedef HRESULT (__stdcall *DXVAHDCreateVideoServicePtr)(IDirect3DDevice9Ex *pD3DDevice, const DXVAHD_CONTENT_DESC *pContentDesc, DXVAHD_DEVICE_USAGE Usage, PDXVAHDSW_Plugin pPlugin, IDXVAHD_Device **ppDevice);
static DXVAHDCreateVideoServicePtr g_DXVAHDCreateVideoService;

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

static bool LoadDXVAHD()
{
  static CCriticalSection g_section;
  static HMODULE          g_handle;

  CSingleLock lock(g_section);
  if(g_handle == NULL)
  {
    g_handle = LoadLibraryEx("dxva2.dll", NULL, 0);
  }
  if(g_handle == NULL)
  {
    return false;
  }
  g_DXVAHDCreateVideoService = (DXVAHDCreateVideoServicePtr)GetProcAddress(g_handle, "DXVAHD_CreateDevice");
  if(g_DXVAHDCreateVideoService == NULL)
  {
    return false;
  }
  return true;
}

static std::string GUIDToString(const GUID& guid)
{
  std::string buffer = StringUtils::Format("%08X-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
                                          , guid.Data1, guid.Data2, guid.Data3
                                          , guid.Data4[0], guid.Data4[1]
                                          , guid.Data4[2], guid.Data4[3], guid.Data4[4]
                                          , guid.Data4[5], guid.Data4[6], guid.Data4[7]);
  return buffer;
}

CProcessorHD::CProcessorHD()
{
  m_pDXVAHD = NULL;
  m_pDXVAVP = NULL;
  m_index   = 0;
  m_frame   = 0;
  g_Windowing.Register(this);

  m_surfaces = NULL;
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

  for(unsigned i = 0; i < m_frames.size(); i++)
  {
    SAFE_RELEASE(m_frames[i].context);
    SAFE_RELEASE(m_frames[i].pSurface);
  }
  m_frames.clear();

  SAFE_RELEASE(m_context);
  if (m_surfaces)
  {
    for (unsigned i = 0; i < m_size; i++)
    {
      SAFE_RELEASE(m_surfaces[i]);
    }
    free(m_surfaces);
    m_surfaces = NULL;
  }
}

bool CProcessorHD::UpdateSize(const DXVA2_VideoDesc& dsc)
{
  return true;
}

bool CProcessorHD::PreInit()
{
  if (!LoadDXVAHD())
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

  HRESULT cvres = g_DXVAHDCreateVideoService( (IDirect3DDevice9Ex*)g_Windowing.Get3DDevice()
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

  m_frame = 0;

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
  LOGIFERROR(m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_D3DFORMAT
                                                  , sizeof(d3dformat), &d3dformat ));

  DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE_DATA data =
  {
    0,                                          // Type: 0=Video, 1=Graphics
    0,                                          // RGB_Range: 0=Full, 1=Limited
    m_flags & CONF_FLAGS_YUVCOEF_BT709 ? 1 : 0, // YCbCr_Matrix: 0=BT.601, 1=BT.709
    m_flags & CONF_FLAGS_YUV_FULLRANGE ? 1 : 0  // YCbCr_xvYCC: 0=Conventional YCbCr, 1=xvYCC
  };
  LOGIFERROR(m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE
                                                  , sizeof(data), &data ));

  DXVAHD_COLOR_YCbCrA bgColor = { 0.0625f, 0.5f, 0.5f, 1.0f }; // black color
  DXVAHD_COLOR backgroundColor;
  backgroundColor.YCbCr = bgColor; 
  DXVAHD_BLT_STATE_BACKGROUND_COLOR_DATA backgroundData = { true, backgroundColor }; // {YCbCr, DXVAHD_COLOR}
  LOGIFERROR(m_pDXVAVP->SetVideoProcessBltState( DXVAHD_BLT_STATE_BACKGROUND_COLOR
                                               , sizeof (backgroundData), &backgroundData ));

  DXVAHD_STREAM_STATE_ALPHA_DATA alpha = { true, 1.0f };
  LOGIFERROR(m_pDXVAVP->SetVideoProcessStreamState( 0, DXVAHD_STREAM_STATE_ALPHA
                                                  , sizeof(alpha), &alpha ));

  return true;
}

bool CProcessorHD::CreateSurfaces()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  m_surfaces = (LPDIRECT3DSURFACE9*)calloc(m_size, sizeof(LPDIRECT3DSURFACE9));
  for (unsigned idx = 0; idx < m_size; idx++)
    CHECK(pD3DDevice->CreateOffscreenPlainSurface(
                                (m_width + 15) & ~15,
                                (m_height + 15) & ~15,
                                m_format,
                                m_VPDevCaps.InputPool,
                                &m_surfaces[idx],
                                NULL));

  m_context = new CSurfaceContext();

  return true;
}

REFERENCE_TIME CProcessorHD::Add(DVDVideoPicture* picture)
{
  CSingleLock lock(m_section);

  IDirect3DSurface9* surface = NULL;
  CSurfaceContext* context = NULL;

  if (picture->iFlags & DVP_FLAG_DROPPED)
  {
    return 0;
  }

  switch (picture->format)
  {
    case RENDER_FMT_DXVA:
    {
      surface = (IDirect3DSurface9*)picture->data[3];
      context = picture->context;
      break;
    }

    case RENDER_FMT_YUV420P:
    {
      if (!m_surfaces)
      {
        CLog::Log(LOGWARNING, __FUNCTION__" - not initialized.");
        return 0;
      }

      surface = m_surfaces[m_index];
      m_index = (m_index + 1) % m_size;

      context = m_context;
  
      D3DLOCKED_RECT rectangle;
      if (FAILED(surface->LockRect(&rectangle, NULL, 0)))
      {
        return 0;
      }

      // Convert to NV12 - Luma
      // TODO: Optimize this later using shaders/swscale/etc.
      uint8_t *s = picture->data[0];
      uint8_t* bits = (uint8_t*)(rectangle.pBits);
      for (unsigned y = 0; y < picture->iHeight; y++)
      {
        memcpy(bits, s, picture->iWidth);
        s += picture->iLineSize[0];
        bits += rectangle.Pitch;
      }

      D3DSURFACE_DESC desc;
      if (FAILED(surface->GetDesc(&desc)))
      {
        return 0;
      }

      // Convert to NV12 - Chroma
      uint8_t *s_u, *s_v, *d_uv;
      for (unsigned y = 0; y < picture->iHeight/2; y++)
      {
        s_u = picture->data[1] + (y * picture->iLineSize[1]);
        s_v = picture->data[2] + (y * picture->iLineSize[2]);
        d_uv = ((uint8_t*)(rectangle.pBits)) + (desc.Height + y) * rectangle.Pitch;
        for (unsigned x = 0; x < picture->iWidth/2; x++)
        {
          *d_uv++ = *s_u++;
          *d_uv++ = *s_v++;
        }
      }
  
      if (FAILED(surface->UnlockRect()))
      {
        return 0;
      }
      break;
    }
    
    default:
    {
      CLog::Log(LOGWARNING, __FUNCTION__" - colorspace not supported by processor, skipping frame.");
      return 0;
    }
  }

  if (!surface || !context)
  {
    return 0;
  }
  m_frame += 2;

  surface->AddRef();
  context->Acquire();

  SFrame frame = {};
  frame.index       = m_frame;
  frame.pSurface    = surface; 
  frame.context     = context;
  frame.format      = DXVAHD_FRAME_FORMAT_PROGRESSIVE;

  if (picture->iFlags & DVP_FLAG_INTERLACED)
  {
    frame.format = picture->iFlags & DVP_FLAG_TOP_FIELD_FIRST
                     ? DXVAHD_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST
                     : DXVAHD_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;
  }

  m_frames.push_back(frame);

  if (m_frames.size() > m_size)
  {
    SAFE_RELEASE(m_frames.front().context);
    SAFE_RELEASE(m_frames.front().pSurface);

    m_frames.pop_front();
  }

  return m_frame;
}

bool CProcessorHD::ApplyFilter(DXVAHD_FILTER filter, int value, int min, int max, int def)
{
  if (filter > NUM_FILTERS)
  {
    return false;
  }
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

bool CProcessorHD::Render(CRect src, CRect dst, IDirect3DSurface9* target, REFERENCE_TIME frame, DWORD flags)
{
  CSingleLock lock(m_section);

  // restore processor if it was lost
  if(!m_pDXVAVP && !OpenProcessor())
  {
    return false;
  }
  
  EDEINTERLACEMODE deinterlace_mode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
  if (g_advancedSettings.m_DXVANoDeintProcForProgressive)
    deinterlace_mode = (flags & RENDER_FLAG_FIELD0 || flags & RENDER_FLAG_FIELD1) ? VS_DEINTERLACEMODE_FORCE : VS_DEINTERLACEMODE_OFF;
  EINTERLACEMETHOD interlace_method = g_renderManager.AutoInterlaceMethod(CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod);

  bool progressive = deinterlace_mode == VS_DEINTERLACEMODE_OFF
                  || (   interlace_method != VS_INTERLACEMETHOD_DXVA_BOB
                      && interlace_method != VS_INTERLACEMETHOD_DXVA_BEST);

  // minFrame is the first samples to keep. Delete the rest.
  REFERENCE_TIME minFrame = frame - m_max_back_refs * 2;

  SFrames::iterator it = m_frames.begin();
  while (it != m_frames.end())
  {
    if (it->index < minFrame)
    {
      SAFE_RELEASE(it->context);
      SAFE_RELEASE(it->pSurface);
      it = m_frames.erase(it);
    }
    else
      ++it;
  }

  if(m_frames.empty())
  {
    return false;
  }

  D3DSURFACE_DESC desc;
  CHECK(target->GetDesc(&desc));
  CRect rectTarget(0, 0, desc.Width, desc.Height);
  CWIN32Util::CropSource(src, dst, rectTarget);
  RECT sourceRECT = { src.x1, src.y1, src.x2, src.y2 };
  RECT dstRECT    = { dst.x1, dst.y1, dst.x2, dst.y2 };

  // MinTime and MaxTime are now the first and last samples to feed the processor.
  minFrame = frame - m_VPCaps.PastFrames * 2;
  REFERENCE_TIME maxFrame = frame + m_VPCaps.FutureFrames * 2;

  bool isValid(false);
  DXVAHD_FRAME_FORMAT dxvaFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;

  DXVAHD_STREAM_DATA stream_data = { 0 };
  stream_data.Enable = TRUE;
  stream_data.PastFrames = 0;
  stream_data.FutureFrames = 0;
  stream_data.ppPastSurfaces = new IDirect3DSurface9*[m_VPCaps.PastFrames];
  stream_data.ppFutureSurfaces = new IDirect3DSurface9*[m_VPCaps.FutureFrames];

  for(it = m_frames.begin(); it != m_frames.end(); ++it)
  {
    if (it->index >= minFrame && it->index <= maxFrame)
    {
      if (it->index < frame)
      {
        // frames order should be { .., T-1, T-2, T-3 }
        stream_data.ppPastSurfaces[(frame - it->index)/2 - 1] = it->pSurface;
        stream_data.PastFrames++;
      }
      else if (it->index == frame)
      {
        stream_data.pInputSurface = it->pSurface;
        dxvaFrameFormat = (DXVAHD_FRAME_FORMAT) it->format;
        isValid = true;
      }
      else if (it->index > frame)
      {
        // frames order should be { T+1, T+2, T+3, .. }
        stream_data.ppFutureSurfaces[(it->index - frame)/2 - 1] = it->pSurface;
        stream_data.FutureFrames++;
      }
    }
  }

  // no present frame, skip
  if (!isValid)
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - uncomplete stream data, skipping frame.");
    return false;
  }

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
  stream_data.InputFrameOrField = frame + (flags & RENDER_FLAG_FIELD1 ? 1 : 0);
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

  HRESULT hr = m_pDXVAVP->VideoProcessBltHD(target, frame, 1, &stream_data);
  if(FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing VideoProcessBltHD with error %x", hr);
  }

  delete [] stream_data.ppPastSurfaces;
  delete [] stream_data.ppFutureSurfaces;

  return !FAILED(hr);
}

#endif
