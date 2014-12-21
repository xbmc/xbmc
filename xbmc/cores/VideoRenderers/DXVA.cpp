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

#include <windows.h>
#include <d3d9.h>
#include <Initguid.h>
#include <dxva.h>
#include <dxva2api.h>

#include "DXVA.h"
#include "windowing/WindowingFactory.h"
#include "WinRenderer.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "utils/AutoPtrHandle.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "win32/WIN32Util.h"
#include "utils/Log.h"

#include <memory>

using namespace DXVA;
using namespace AUTOPTR;
using namespace std;

DEFINE_GUID(DXVA2_VideoProcATIVectorAdaptiveDevice,   0x3C5323C1,0x6fb7,0x44f5,0x90,0x81,0x05,0x6b,0xf2,0xee,0x44,0x9d);
DEFINE_GUID(DXVA2_VideoProcATIMotionAdaptiveDevice,   0x552C0DAD,0xccbc,0x420b,0x83,0xc8,0x74,0x94,0x3c,0xf9,0xf1,0xa6);
DEFINE_GUID(DXVA2_VideoProcATIAdaptiveDevice,         0x6E8329FF,0xb642,0x418b,0xbc,0xf0,0xbc,0xb6,0x59,0x1e,0x25,0x5f);
DEFINE_GUID(DXVA2_VideoProcNVidiaAdaptiveDevice,      0x6CB69578,0x7617,0x4637,0x91,0xE5,0x1C,0x02,0xDB,0x81,0x02,0x85);
DEFINE_GUID(DXVA2_VideoProcIntelEdgeDevice,           0xBF752EF6,0x8CC4,0x457A,0xBE,0x1B,0x08,0xBD,0x1C,0xAE,0xEE,0x9F);
DEFINE_GUID(DXVA2_VideoProcNVidiaUnknownDevice,       0xF9F19DA5,0x3B09,0x4B2F,0x9D,0x89,0xC6,0x47,0x53,0xE3,0xEA,0xAB);

typedef struct {
    const char   *name;
    const GUID   *guid;
} dxva2_device_t;

static const dxva2_device_t dxva2_devices[] = {
  { "Progressive Device",           &DXVA2_VideoProcProgressiveDevice         },
  { "Bob Device",                   &DXVA2_VideoProcBobDevice                 },
  { "Vector Adaptative Device",     &DXVA2_VideoProcATIVectorAdaptiveDevice   },
  { "Motion Adaptative Device",     &DXVA2_VideoProcATIMotionAdaptiveDevice   },
  { "Adaptative Device",            &DXVA2_VideoProcATIAdaptiveDevice         },
  { "Spatial-temporal device",      &DXVA2_VideoProcNVidiaAdaptiveDevice      },
  { "Edge directed device",         &DXVA2_VideoProcIntelEdgeDevice           },
  { "Unknown device (nVidia)",      &DXVA2_VideoProcNVidiaUnknownDevice       },
  { NULL, NULL }
};

typedef struct {
    const char   *name;
    unsigned      flags;
} dxva2_deinterlacetech_t;

static const dxva2_deinterlacetech_t dxva2_deinterlacetechs[] = {
  { "Inverse Telecine",                   DXVA2_DeinterlaceTech_InverseTelecine        },
  { "Motion vector steered",              DXVA2_DeinterlaceTech_MotionVectorSteered    },
  { "Pixel adaptive",                     DXVA2_DeinterlaceTech_PixelAdaptive          },
  { "Field adaptive",                     DXVA2_DeinterlaceTech_FieldAdaptive          },
  { "Edge filtering",                     DXVA2_DeinterlaceTech_EdgeFiltering          },
  { "Median filtering",                   DXVA2_DeinterlaceTech_MedianFiltering        },
  { "Bob vertical stretch 4-tap",         DXVA2_DeinterlaceTech_BOBVerticalStretch4Tap },
  { "Bob vertical stretch",               DXVA2_DeinterlaceTech_BOBVerticalStretch     },
  { "Bob line replicate",                 DXVA2_DeinterlaceTech_BOBLineReplicate       },
  { "Unknown",                            DXVA2_DeinterlaceTech_Unknown                },
  { NULL, 0 }
};

// Prefered targets must be first
static const D3DFORMAT render_targets[] = {
    (D3DFORMAT)MAKEFOURCC('N','V','1','2'),
    (D3DFORMAT)MAKEFOURCC('Y','V','1','2'),
    D3DFMT_UNKNOWN
};

typedef struct {
    DWORD VendorID;
    DWORD DeviceID;
} pci_device;

// List of devices that drop frames with a deinterlacing processor for progressive material.
static const pci_device NoDeintProcForProgDevices[] = {
  { PCIV_nVidia, 0x0865 }, // ION
  { PCIV_nVidia, 0x0874 }, // ION
  { PCIV_nVidia, 0x0876 }, // ION
  { PCIV_nVidia, 0x087D }, // ION
  { PCIV_nVidia, 0x087E }, // ION LE
  { PCIV_nVidia, 0x087F }, // ION LE
  { 0          , 0x0000 }
};

static CStdString GUIDToString(const GUID& guid)
{
  CStdString buffer = StringUtils::Format("%08X-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
              , guid.Data1, guid.Data2, guid.Data3
              , guid.Data4[0], guid.Data4[1]
              , guid.Data4[2], guid.Data4[3], guid.Data4[4]
              , guid.Data4[5], guid.Data4[6], guid.Data4[7]);
  return buffer;
}

static const dxva2_device_t *dxva2_find_device(const GUID *guid)
{
    for (unsigned i = 0; dxva2_devices[i].name; i++) {
        if (IsEqualGUID(*dxva2_devices[i].guid, *guid))
            return &dxva2_devices[i];
    }
    return NULL;
}

static const dxva2_deinterlacetech_t *dxva2_find_deinterlacetech(unsigned flags)
{
    for (unsigned i = 0; dxva2_deinterlacetechs[i].name; i++) {
        if (dxva2_deinterlacetechs[i].flags == flags)
            return &dxva2_deinterlacetechs[i];
    }
    return NULL;
}

#define SCOPE(type, var) std::shared_ptr<type> var##_holder(var, CoTaskMemFree);

CCriticalSection CProcessor::m_dlSection;
HMODULE CProcessor::m_dlHandle = NULL;
DXVA2CreateVideoServicePtr CProcessor::m_DXVA2CreateVideoService = NULL;

CProcessor::CProcessor()
{
  m_service = NULL;
  m_process = NULL;
  g_Windowing.Register(this);

  m_context = NULL;
  m_progressive = true;
}

CProcessor::~CProcessor()
{
  g_Windowing.Unregister(this);
  UnInit();
}

void CProcessor::UnInit()
{
  CSingleLock lock(m_section);
  Close();
  SAFE_RELEASE(m_service);
}

void CProcessor::Close()
{
  CSingleLock lock(m_section);
  SAFE_RELEASE(m_process);
  SAFE_RELEASE(m_context);
}

bool CProcessor::UpdateSize(const DXVA2_VideoDesc& dsc)
{
  // TODO: print the D3FORMAT text version in log
  CLog::Log(LOGDEBUG, "DXVA - checking samples array size using %d render target", dsc.Format);

  GUID* deint_guid_list = NULL;
  unsigned guid_count = 0;
  if (FAILED(m_service->GetVideoProcessorDeviceGuids(&dsc, &guid_count, &deint_guid_list)))
    return false;

  SCOPE(GUID, deint_guid_list);
  
  for (unsigned i = 0; i < guid_count; i++)
  {
    DXVA2_VideoProcessorCaps caps;
    CHECK(m_service->GetVideoProcessorCaps(deint_guid_list[i], &dsc, D3DFMT_X8R8G8B8, &caps));
    if (caps.NumBackwardRefSamples + caps.NumForwardRefSamples > m_size)
    {
      m_size = caps.NumBackwardRefSamples + caps.NumForwardRefSamples;
      CLog::Log(LOGDEBUG, "DXVA - updated maximum samples count to %d", m_size);
    }
    m_max_back_refs = std::max(caps.NumBackwardRefSamples, m_max_back_refs);
    m_max_fwd_refs = std::max(caps.NumForwardRefSamples, m_max_fwd_refs);
  }

  return true;
}

bool CProcessor::PreInit()
{
  if (!LoadSymbols())
    return false;

  UnInit();

  CSingleLock lock(m_section);

  if (FAILED(m_DXVA2CreateVideoService(g_Windowing.Get3DDevice(), IID_IDirectXVideoProcessorService, (void**)&m_service)))
    return false;

  m_size = 0;

  // We try to find the maximum count of reference frames using a standard resolution and all known render target formats
  DXVA2_VideoDesc dsc = {};
  dsc.SampleWidth = 640;
  dsc.SampleHeight = 480;
  dsc.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedOddFirst;

  m_max_back_refs = 0;
  m_max_fwd_refs = 0;

  for (unsigned i = 0; render_targets[i] != D3DFMT_UNKNOWN; i++)
  {
    dsc.Format = render_targets[i];
    if (!UpdateSize(dsc))
      CLog::Log(LOGDEBUG, "DXVA - render target not supported by processor");
  }

  m_size = m_max_back_refs + 1 + m_max_fwd_refs + 2;  // refs + 1 display + 2 safety frames

  return true;
}

bool CProcessor::Open(UINT width, UINT height, unsigned int flags, unsigned int format, unsigned int extended_format)
{
  Close();

  CSingleLock lock(m_section);

  if (!m_service)
    return false;

  DXVA2_VideoDesc dsc;
  memset(&dsc, 0, sizeof(DXVA2_VideoDesc));

  dsc.SampleWidth = width;
  dsc.SampleHeight = height;
  dsc.SampleFormat.VideoLighting = DXVA2_VideoLighting_dim;

  switch (CONF_FLAGS_CHROMA_MASK(flags))
  {
    case CONF_FLAGS_CHROMA_LEFT:
      dsc.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Horizontally_Cosited
                                              | DXVA2_VideoChromaSubsampling_Vertically_AlignedChromaPlanes;
      break;
    case CONF_FLAGS_CHROMA_CENTER:
      dsc.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Vertically_AlignedChromaPlanes;
      break;
    case CONF_FLAGS_CHROMA_TOPLEFT:
      dsc.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Horizontally_Cosited
                                              | DXVA2_VideoChromaSubsampling_Vertically_Cosited;
      break;
    default:
      dsc.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Unknown;
  }

  if (flags & CONF_FLAGS_YUV_FULLRANGE)
    dsc.SampleFormat.NominalRange = DXVA2_NominalRange_0_255;
  else
    dsc.SampleFormat.NominalRange = DXVA2_NominalRange_16_235;

  switch (CONF_FLAGS_YUVCOEF_MASK(flags))
  {
    case CONF_FLAGS_YUVCOEF_240M:
      dsc.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_SMPTE240M;
      break;
    case CONF_FLAGS_YUVCOEF_BT601:
      dsc.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT601;
      break;
    case CONF_FLAGS_YUVCOEF_BT709:
      dsc.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT709;
      break;
    default:
      dsc.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_Unknown;
  }

  switch (CONF_FLAGS_COLPRI_MASK(flags))
  {
    case CONF_FLAGS_COLPRI_BT709:
      dsc.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_BT709;
      break;
    case CONF_FLAGS_COLPRI_BT470M:
      dsc.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_BT470_2_SysM;
      break;
    case CONF_FLAGS_COLPRI_BT470BG:
      dsc.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_BT470_2_SysBG;
      break;
    case CONF_FLAGS_COLPRI_170M:
      dsc.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_SMPTE170M;
      break;
    case CONF_FLAGS_COLPRI_240M:
      dsc.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_SMPTE240M;
      break;
    default:
      dsc.SampleFormat.VideoPrimaries = DXVA2_VideoPrimaries_Unknown;
  }

  switch (CONF_FLAGS_TRC_MASK(flags))
  {
    case CONF_FLAGS_TRC_BT709:
      dsc.SampleFormat.VideoTransferFunction = DXVA2_VideoTransFunc_709;
      break;
    case CONF_FLAGS_TRC_GAMMA22:
      dsc.SampleFormat.VideoTransferFunction = DXVA2_VideoTransFunc_22;
      break;
    case CONF_FLAGS_TRC_GAMMA28:
      dsc.SampleFormat.VideoTransferFunction = DXVA2_VideoTransFunc_28;
      break;
    default:
      dsc.SampleFormat.VideoTransferFunction = DXVA2_VideoTransFunc_Unknown;
  }

  m_desc = dsc;

  if (format == RENDER_FMT_DXVA)
    m_desc.Format = (D3DFORMAT)extended_format;
  else
  {
    // Only NV12 software colorspace conversion is implemented for now
    m_desc.Format = (D3DFORMAT)MAKEFOURCC('N','V','1','2');
    if (!CreateSurfaces())
      return false;
  }

  // frame flags are not available to do the complete calculation of the deinterlacing mode, as done in Render()
  // It's OK, as it doesn't make any difference for all hardware except the few GPUs on the quirk list.
  // And for those GPUs, the correct values will be calculated with the first Render() and the correct processor
  // will replace the one allocated here, before the user sees anything.
  // It's a bit inefficient, that's all.
  m_deinterlace_mode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
  m_interlace_method = g_renderManager.AutoInterlaceMethod(CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod);;

  EvaluateQuirkNoDeintProcForProg();

  if (g_advancedSettings.m_DXVANoDeintProcForProgressive || m_quirk_nodeintprocforprog)
    CLog::Log(LOGNOTICE, "DXVA: Auto deinterlacing mode workaround activated. Deinterlacing processor will be used only for interlaced frames.");

  if (!OpenProcessor())
    return false;

  return true;
}

void CProcessor::EvaluateQuirkNoDeintProcForProg()
{
  D3DADAPTER_IDENTIFIER9 AIdentifier = g_Windowing.GetAIdentifier();

  for (unsigned idx = 0; NoDeintProcForProgDevices[idx].VendorID != 0; idx++)
  {
    if(NoDeintProcForProgDevices[idx].VendorID == AIdentifier.VendorId
    && NoDeintProcForProgDevices[idx].DeviceID == AIdentifier.DeviceId)
    {
      m_quirk_nodeintprocforprog = true;
      return;
    }
  }
  m_quirk_nodeintprocforprog = false;
}

bool CProcessor::SelectProcessor()
{
  // The CProcessor can be run after dxva or software decoding, possibly after software deinterlacing.

  // Deinterlace mode off: force progressive
  // Deinterlace mode auto or force, with a dxva deinterlacing method: create an deinterlacing capable processor. The frame flags will tell it to deinterlace or not.
  m_progressive = m_deinterlace_mode == VS_DEINTERLACEMODE_OFF
                  || (   m_interlace_method != VS_INTERLACEMETHOD_DXVA_BOB
                      && m_interlace_method != VS_INTERLACEMETHOD_DXVA_BEST);

  if (m_progressive)
    m_desc.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
  else
    m_desc.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;

  GUID*    guid_list;
  unsigned guid_count;
  CHECK(m_service->GetVideoProcessorDeviceGuids(&m_desc, &guid_count, &guid_list));
  SCOPE(GUID, guid_list);

  if(guid_count == 0)
  {
    CLog::Log(LOGDEBUG, "DXVA - unable to find any processors");
    return false;
  }

  for(unsigned i = 0; i < guid_count; i++)
  {
    const GUID* g = &guid_list[i];
    const dxva2_device_t* device = dxva2_find_device(g);

    if (device)
    {
      CLog::Log(LOGDEBUG, "DXVA - processor found %s", device->name);
    }
    else
    {
      CHECK(m_service->GetVideoProcessorCaps(*g, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
      const dxva2_deinterlacetech_t* tech = dxva2_find_deinterlacetech(m_caps.DeinterlaceTechnology);
      if (tech != NULL)
        CLog::Log(LOGDEBUG, "DXVA - unknown processor %s found, deinterlace technology %s", GUIDToString(*g).c_str(), tech->name);
      else
        CLog::Log(LOGDEBUG, "DXVA - unknown processor %s found, unknown technology", GUIDToString(*g).c_str());
    }
  }

  if (m_progressive)
    m_device = DXVA2_VideoProcProgressiveDevice;
  else if(m_interlace_method == VS_INTERLACEMETHOD_DXVA_BEST)
    m_device = guid_list[0];
  else
    m_device = DXVA2_VideoProcBobDevice;

  return true;
}

bool CProcessor::OpenProcessor()
{
  if (!SelectProcessor())
    return false;

  SAFE_RELEASE(m_process);

  const dxva2_device_t* device = dxva2_find_device(&m_device);
  if (device)
    CLog::Log(LOGDEBUG, "DXVA - processor selected %s", device->name);
  else
    CLog::Log(LOGDEBUG, "DXVA - processor selected %s", GUIDToString(m_device).c_str());

  D3DFORMAT rtFormat = D3DFMT_X8R8G8B8;
  CHECK(m_service->GetVideoProcessorCaps(m_device, &m_desc, rtFormat, &m_caps))

  /* HACK for Intel Egde Device. 
   * won't work if backward refs is equals value from the capabilities *
   * Possible reasons are:                                             *
   * 1) The device capabilities are incorrectly reported               *
   * 2) The device is broken                                           */
  if (IsEqualGUID(m_device, DXVA2_VideoProcIntelEdgeDevice))
    m_caps.NumBackwardRefSamples = 0;

  if (m_caps.DeviceCaps & DXVA2_VPDev_SoftwareDevice)
    CLog::Log(LOGDEBUG, "DXVA - processor is software device");

  if (m_caps.DeviceCaps & DXVA2_VPDev_EmulatedDXVA1)
    CLog::Log(LOGDEBUG, "DXVA - processor is emulated dxva1");

  CLog::Log(LOGDEBUG, "DXVA - processor requires %d past frames and %d future frames", m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples);

  if (m_caps.NumBackwardRefSamples + m_caps.NumForwardRefSamples + 3 > m_size)
  {
    CLog::Log(LOGERROR, "DXVA - used an incorrect number of reference frames creating processor");
    return false;
  }

  CHECK(m_service->CreateVideoProcessor(m_device, &m_desc, rtFormat, 0, &m_process));

  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, rtFormat, DXVA2_ProcAmp_Brightness, &m_brightness));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, rtFormat, DXVA2_ProcAmp_Contrast  , &m_contrast));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, rtFormat, DXVA2_ProcAmp_Hue       , &m_hue));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, rtFormat, DXVA2_ProcAmp_Saturation, &m_saturation));

  return true;
}

bool CProcessor::CreateSurfaces()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  LPDIRECT3DSURFACE9 surfaces[32];
  for (unsigned idx = 0; idx < m_size; idx++)
  {
    CHECK(pD3DDevice->CreateOffscreenPlainSurface(
      (m_desc.SampleWidth + 15) & ~15,
      (m_desc.SampleHeight + 15) & ~15,
      m_desc.Format,
      D3DPOOL_DEFAULT,
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

CRenderPicture *CProcessor::Convert(DVDVideoPicture* picture)
{
  if (picture->format != RENDER_FMT_YUV420P)
  {
    CLog::Log(LOGERROR, "%s - colorspace not supported by processor, skipping frame.", __FUNCTION__);
    return NULL;
  }

  IDirect3DSurface9* surface = m_context->GetFree(NULL);
  if (!surface)
  {
    CLog::Log(LOGERROR, "%s - no free video surface", __FUNCTION__);
    return NULL;
  }

  D3DLOCKED_RECT rectangle;
  if (FAILED(surface->LockRect(&rectangle, NULL, 0)))
  {
    CLog::Log(LOGERROR, "%s - could not lock rect", __FUNCTION__);
    m_context->ClearReference(surface);
    return NULL;
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
    CLog::Log(LOGERROR, "%s - could not get surface descriptor", __FUNCTION__);
    m_context->ClearReference(surface);
    return NULL;
  }
  // Convert to NV12 - Chroma
  uint8_t *s_u, *s_v, *d_uv;
  for (unsigned y = 0; y < picture->iHeight / 2; y++)
  {
    s_u = picture->data[1] + (y * picture->iLineSize[1]);
    s_v = picture->data[2] + (y * picture->iLineSize[2]);
    d_uv = ((uint8_t*)(rectangle.pBits)) + (desc.Height + y) * rectangle.Pitch;
    for (unsigned x = 0; x < picture->iWidth / 2; x++)
    {
      *d_uv++ = *s_u++;
      *d_uv++ = *s_v++;
    }
  }
  if (FAILED(surface->UnlockRect()))
  {
    CLog::Log(LOGERROR, "%s - failed to unlock surface", __FUNCTION__);
    m_context->ClearReference(surface);
    return NULL;
  }

  m_context->ClearReference(surface);
  m_context->MarkRender(surface);
  CRenderPicture *pic = new CRenderPicture(m_context);
  pic->surface = surface;
  return pic;
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
                            * (value - def) / (min - def) );
  else
    return range.DefaultValue;
}

bool CProcessor::Render(CRect src, CRect dst, IDirect3DSurface9* target, IDirect3DSurface9** source, DWORD flags, UINT frameIdx)
{
  CSingleLock lock(m_section);

  if (!source[2])
    return false;

  // With auto deinterlacing, the Ion Gen. 1 drops some frames with deinterlacing processor + progressive flags for progressive material.
  // For that GPU (or when specified by an advanced setting), use the progressive processor.
  // This is at the expense of the switch speed when video interlacing flags change and a deinterlacing processor is actually required.
  EDEINTERLACEMODE mode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
  if (g_advancedSettings.m_DXVANoDeintProcForProgressive || m_quirk_nodeintprocforprog)
    mode = (flags & RENDER_FLAG_FIELD0 || flags & RENDER_FLAG_FIELD1) ? VS_DEINTERLACEMODE_FORCE : VS_DEINTERLACEMODE_OFF;
  EINTERLACEMETHOD method = g_renderManager.AutoInterlaceMethod(CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod);
  if(m_interlace_method != method
  || m_deinterlace_mode != mode
  || !m_process)
  {
    m_deinterlace_mode = mode;
    m_interlace_method = method;

    if (!OpenProcessor())
      return false;
  }
  
  D3DSURFACE_DESC desc;
  CHECK(target->GetDesc(&desc));
  CRect rectTarget(0, 0, desc.Width, desc.Height);
  CWIN32Util::CropSource(src, dst, rectTarget);
  RECT sourceRECT = { src.x1, src.y1, src.x2, src.y2 };
  RECT dstRECT    = { dst.x1, dst.y1, dst.x2, dst.y2 };

  // set sample format for progressive and interlaced
  UINT sampleFormat = DXVA2_SampleProgressiveFrame;
  if (flags & RENDER_FLAG_FIELD0 && flags & RENDER_FLAG_TOP)
    sampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
  else if (flags & RENDER_FLAG_FIELD1 && flags & RENDER_FLAG_BOT)
    sampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
  if (flags & RENDER_FLAG_FIELD0 && flags & RENDER_FLAG_BOT)
    sampleFormat = DXVA2_SampleFieldInterleavedOddFirst;
  if (flags & RENDER_FLAG_FIELD1 && flags & RENDER_FLAG_TOP)
    sampleFormat = DXVA2_SampleFieldInterleavedOddFirst;

  // How to prepare the samples array for VideoProcessBlt
  // - always provide current picture + the number of forward and backward references required by the current processor.
  // - provide the surfaces in the array in increasing temporal order
  // - at the start of playback, there may not be enough samples available. Use SampleFormat.SampleFormat = DXVA2_SampleUnknown for the missing samples.

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
  int futureFrames = std::min(providedFuture, m_caps.NumForwardRefSamples);
  int pastFrames = std::min(providedPast, m_caps.NumBackwardRefSamples);

  int count = 1 + pastFrames + futureFrames;
  auto_aptr<DXVA2_VideoSample> samp(new DXVA2_VideoSample[count]);

  int start = 2 - futureFrames;
  int end = 2 + pastFrames;
  int sampIdx = 0;
  for (int i = end; i >= start; i--)
  {
    if (!source[i])
      continue;

    DXVA2_VideoSample& vs = samp[sampIdx];
    vs.SrcSurface = source[i];
    vs.SrcRect = sourceRECT;
    vs.DstRect = dstRECT;
    vs.SampleData = 0;
    vs.Start = frameIdx + (sampIdx - pastFrames) * 2;
    vs.End = vs.Start + 2;
    vs.PlanarAlpha = DXVA2_Fixed32OpaqueAlpha();
    vs.SampleFormat = m_desc.SampleFormat;
    vs.SampleFormat.SampleFormat = sampleFormat;
    
    // Override the sample format when the processor doesn't need to deinterlace or when deinterlacing is forced and flags are missing.
    if (m_progressive)
      vs.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
    else if (m_deinterlace_mode == VS_DEINTERLACEMODE_FORCE && vs.SampleFormat.SampleFormat == DXVA2_SampleProgressiveFrame)
      vs.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;

    sampIdx++;
  }

 
  DXVA2_VideoProcessBltParams blt = {};
  blt.TargetFrame = frameIdx;
  if (flags & RENDER_FLAG_FIELD1)
    blt.TargetFrame += 1;
  blt.TargetRect  = dstRECT;
  blt.ConstrictionSize.cx = 0;
  blt.ConstrictionSize.cy = 0;

  blt.DestFormat.VideoTransferFunction = DXVA2_VideoTransFunc_sRGB;
  blt.DestFormat.SampleFormat          = DXVA2_SampleProgressiveFrame;
  if(g_Windowing.UseLimitedColor())
    blt.DestFormat.NominalRange          = DXVA2_NominalRange_16_235;
  else
    blt.DestFormat.NominalRange          = DXVA2_NominalRange_0_255;
  blt.Alpha = DXVA2_Fixed32OpaqueAlpha();

  blt.ProcAmpValues.Brightness = ConvertRange( m_brightness, CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness
                                             , 0, 100, 50);
  blt.ProcAmpValues.Contrast   = ConvertRange( m_contrast, CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast
                                             , 0, 100, 50);
  blt.ProcAmpValues.Hue        = m_hue.DefaultValue;
  blt.ProcAmpValues.Saturation = m_saturation.DefaultValue;

  blt.BackgroundColor.Y     = 0x1000;
  blt.BackgroundColor.Cb    = 0x8000;
  blt.BackgroundColor.Cr    = 0x8000;
  blt.BackgroundColor.Alpha = 0xffff;

  /* HACK to kickstart certain DXVA drivers (poulsbo) which oddly  *
   * won't render anything until someting else have been rendered. */
  g_Windowing.Get3DDevice()->SetFVF( D3DFVF_XYZ );
  float verts[2][3]= {};
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 1, verts, 3*sizeof(float));

  CHECK(m_process->VideoProcessBlt(target, &blt, &samp[0], count, NULL));
  return true;
}

bool CProcessor::LoadSymbols()
{
  CSingleLock lock(m_dlSection);

  if(m_dlHandle == NULL)
    m_dlHandle = LoadLibraryEx("dxva2.dll", NULL, 0);
  if(m_dlHandle == NULL)
    return false;
  m_DXVA2CreateVideoService = (DXVA2CreateVideoServicePtr)GetProcAddress(m_dlHandle, "DXVA2CreateVideoService");
  if(m_DXVA2CreateVideoService == NULL)
    return false;
  return true;
}

#endif
