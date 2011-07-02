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
#include "boost/shared_ptr.hpp"
#include "utils/AutoPtrHandle.h"
#include "settings/AdvancedSettings.h"
#include "threads/Atomics.h"

using namespace DXVA;
using namespace AUTOPTR;
using namespace std;

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
DEFINE_GUID(DXVADDI_Intel_ModeH264_C, 0x604F8E66,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E, 0x604F8E68,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeVC1_E , 0xBCC5DB6D,0xA2B6,0x4AF0,0xAC,0xE4,0xAD,0xB1,0xF7,0x87,0xBC,0x89);

typedef struct {
    const char   *name;
    const GUID   *guid;
    int          codec;
} dxva2_mode_t;

/* XXX Prefered modes must come first */
static const dxva2_mode_t dxva2_modes[] = {
    { "MPEG2 VLD",    &DXVA2_ModeMPEG2_VLD,     CODEC_ID_MPEG2VIDEO },
    { "MPEG2 MoComp", &DXVA2_ModeMPEG2_MoComp,  0 },
    { "MPEG2 IDCT",   &DXVA2_ModeMPEG2_IDCT,    0 },

    { "H.264 variable-length decoder (VLD), FGT",               &DXVA2_ModeH264_F, CODEC_ID_H264 },
    { "H.264 VLD, no FGT",                                      &DXVA2_ModeH264_E, CODEC_ID_H264 },
    { "H.264 IDCT, FGT",                                        &DXVA2_ModeH264_D, 0,            },
    { "H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVA2_ModeH264_C, 0,            },
    { "H.264 MoComp, FGT",                                      &DXVA2_ModeH264_B, 0,            },
    { "H.264 motion compensation (MoComp), no FGT",             &DXVA2_ModeH264_A, 0,            },

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

    { "Intel H.264 VLD, no FGT",                                      &DXVADDI_Intel_ModeH264_E, CODEC_ID_H264 },
    { "Intel H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVADDI_Intel_ModeH264_C, 0 },
    { "Intel H.264 motion compensation (MoComp), no FGT",             &DXVADDI_Intel_ModeH264_A, 0 },
    { "Intel VC-1 VLD",                                               &DXVADDI_Intel_ModeVC1_E,  0 },

    { NULL, NULL, 0 }
};

DEFINE_GUID(DXVA2_VideoProcATIVectorAdaptiveDevice,   0x3C5323C1,0x6fb7,0x44f5,0x90,0x81,0x05,0x6b,0xf2,0xee,0x44,0x9d);
DEFINE_GUID(DXVA2_VideoProcATIMotionAdaptiveDevice,   0x552C0DAD,0xccbc,0x420b,0x83,0xc8,0x74,0x94,0x3c,0xf9,0xf1,0xa6);
DEFINE_GUID(DXVA2_VideoProcATIAdaptiveDevice,         0x6E8329FF,0xb642,0x418b,0xbc,0xf0,0xbc,0xb6,0x59,0x1e,0x25,0x5f);
DEFINE_GUID(DXVA2_VideoProcNVidiaAdaptiveDevice,      0x6CB69578,0x7617,0x4637,0x91,0xE5,0x1C,0x02,0xDB,0x81,0x02,0x85);
DEFINE_GUID(DXVA2_VideoProcIntelEdgeDevice,           0xBF752EF6,0x8CC4,0x457A,0xBE,0x1B,0x08,0xBD,0x1C,0xAE,0xEE,0x9F);

typedef struct {
  const char   *name;
  const GUID   *guid;
  int          score;
} dxva2_device_t;

// List of all known DXVA2 processor devices and their deinterlace scores (0 = progressive or standard bob)
static const dxva2_device_t dxva2_devices[] = {
  { "Vector adaptive device (ATI)",       &DXVA2_VideoProcATIVectorAdaptiveDevice,   100 },
  { "Motion adaptive device (ATI)",       &DXVA2_VideoProcATIMotionAdaptiveDevice,    90 },
  { "Adaptive device (ATI)",              &DXVA2_VideoProcATIAdaptiveDevice,          80 },
  { "Spatial-temporal device (nVidia)",   &DXVA2_VideoProcNVidiaAdaptiveDevice,      100 },
  { "Edge directed device (Intel)",       &DXVA2_VideoProcIntelEdgeDevice,           100 },
  { "Bob device (DXVA standard)",         &DXVA2_VideoProcBobDevice,                   0 },
  { "Progressive device (DXVA standard)", &DXVA2_VideoProcProgressiveDevice,           0 },
  { NULL, NULL, 0 }
};

typedef struct {
  const char   *name;
  unsigned     deinterlacetech_flag;
  int          score;
} dxva2_deinterlacetech_t;

// List of all considered DXVA2 deinterlace technology flags and their scores for unknown processor devices, preferred must come first
static const dxva2_deinterlacetech_t dxva2_deinterlacetechs[] = {
  { "Motion vector steered",              DXVA2_DeinterlaceTech_MotionVectorSteered,     100 },
  { "Pixel adaptive",                     DXVA2_DeinterlaceTech_PixelAdaptive,            90 },
  { "Field adaptive",                     DXVA2_DeinterlaceTech_FieldAdaptive,            80 },
  { "Edge filtering",                     DXVA2_DeinterlaceTech_EdgeFiltering,            70 },
  { "Median filtering",                   DXVA2_DeinterlaceTech_MedianFiltering,          60 },
  { "Bob vertical stretch 4-tap",         DXVA2_DeinterlaceTech_BOBVerticalStretch4Tap,   50 },
  { "Bob vertical stretch",               DXVA2_DeinterlaceTech_BOBVerticalStretch,       40 },
  { "Bob line replicate",                 DXVA2_DeinterlaceTech_BOBLineReplicate,         30 },
  { NULL, 0, 0 }
};

// List of PCI Device ID of ATI cards with UVD or UVD+ decoding block.
static DWORD UVDDeviceID [] = {
  0x95C0, // ATI Radeon HD 3400 Series (and others)
  0x95C4, // ATI Radeon HD 3400 Series (and others)
  0x95C5, // ATI Radeon HD 3400 Series (and others)
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

static CStdString GUIDToString(const GUID& guid)
{
  CStdString buffer;
  buffer.Format("%08X-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"
              , guid.Data1, guid.Data2, guid.Data3
              , guid.Data4[0], guid.Data4[1]
              , guid.Data4[2], guid.Data4[3], guid.Data4[4]
              , guid.Data4[5], guid.Data4[6], guid.Data4[7]);
  return buffer;
}

static const dxva2_mode_t *dxva2_find(const GUID *guid)
{
    for (unsigned i = 0; dxva2_modes[i].name; i++) {
        if (IsEqualGUID(*dxva2_modes[i].guid, *guid))
            return &dxva2_modes[i];
    }
    return NULL;
}

#define SCOPE(type, var) boost::shared_ptr<type> var##_holder(var, CoTaskMemFree)

#define NEWMAX(a, b, c) if ((a + b) > c) c = a + b

CDecoder::CDecoder()
 : m_event(true)
{
  m_event.Set();
  m_state     = DXVA_OPEN;
  m_service   = NULL;
  m_context          = (dxva_context*)calloc(1, sizeof(dxva_context));
  m_context->cfg     = (DXVA2_ConfigPictureDecode*)calloc(1, sizeof(DXVA2_ConfigPictureDecode));
  m_context->surface = NULL;
  m_context->decoder = NULL;

  m_surfaces = NULL;
  m_read = 0;
  m_write = 0;

  m_level = 0;

  g_Windowing.Register(this);
}

CDecoder::~CDecoder()
{
  g_Windowing.Unregister(this);
  Close();
  free(const_cast<DXVA2_ConfigPictureDecode*>(m_context->cfg)); // yes this is foobar
  free(m_context);
}

void CDecoder::Close()
{
  CSingleLock lock(m_section);

  SAFE_RELEASE(m_context->decoder);
  SAFE_RELEASE(m_service);

  if (m_context->surface)
  {
    free(m_context->surface);
    m_context->surface = NULL;
  }

  if (m_surfaces)
  {
    for (unsigned i = 0; i < m_context->surface_count; i++) SAFE_RELEASE(m_surfaces[i]);
    free(m_surfaces);
    m_surfaces = NULL;
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
} while(0)

static bool CheckH264L41(AVCodecContext *avctx)
{
    unsigned widthmbs  = (avctx->width + 15) / 16;  // width in macroblocks
    unsigned heightmbs = (avctx->height + 15) / 16; // height in macroblocks
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
  && !CDVDCodecUtils::IsVP3CompatibleWidth(avctx->width))
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
  if(avctx->codec_id != CODEC_ID_H264)
    return true;

  // Macroblock width incompatibility
  if (HasVP3WidthBug(avctx))
  {
    CLog::Log(LOGWARNING,"DXVA - width %i is not supported with nVidia VP3 hardware. DXVA will not be used", avctx->width);
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

bool CDecoder::Open(AVCodecContext *avctx, enum PixelFormat fmt)
{
  if (!g_Windowing.m_processor->Create())
    return false;

  if (!CheckCompatibility(avctx))
    return false;

  if(!LoadDXVA())
    return false;

    CSingleLock lock(m_section);

  if(m_state == DXVA_LOST)
  {
    CLog::Log(LOGDEBUG, "DXVA - device is in lost state, we can't start");
    return false;
  }

  // Only one instance for hardware decoder (this fixes problems creating VC-1 decoder)
  IHardwareDecoder* decoder = ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware();
  if (decoder != NULL)
  {
    decoder->Release();
    ((CDVDVideoCodecFFmpeg*)avctx->opaque)->SetHardware(this);
  }

  CHECK(g_DXVA2CreateVideoService(g_Windowing.Get3DDevice(), IID_IDirectXVideoDecoderService, (void**)&m_service));

  UINT  input_count;
  GUID *input_list;

  CHECK(m_service->GetDecoderDeviceGuids(&input_count, &input_list));
  SCOPE(GUID, input_list);

  for(unsigned i = 0; i < input_count; i++)
  {
    const GUID *g            = &input_list[i];
    const dxva2_mode_t *mode = dxva2_find(g);
    if(mode)
      CLog::Log(LOGDEBUG, "DXVA - supports '%s'", mode->name);
    else
      CLog::Log(LOGDEBUG, "DXVA - supports %s", GUIDToString(*g).c_str());
  }

  memset(&m_format, 0, sizeof(DXVA2_VideoDesc));

  m_format.Format = D3DFMT_UNKNOWN;
  for(const dxva2_mode_t* mode = dxva2_modes; mode->name && m_format.Format == D3DFMT_UNKNOWN; mode++)
  {
    if(mode->codec != avctx->codec_id)
      continue;

    for(unsigned j = 0; j < input_count; j++)
    {
      if(!IsEqualGUID(input_list[j], *mode->guid))
        continue;

      CLog::Log(LOGDEBUG, "DXVA - trying '%s'", mode->name);
      if(OpenTarget(input_list[j]))
        break;
    }
  }

  if(m_format.Format == D3DFMT_UNKNOWN)
  {
    CLog::Log(LOGDEBUG, "DXVA - unable to find an input/output format combination");
    return false;
  }

  m_format.SampleWidth  = avctx->width;
  m_format.SampleHeight = avctx->height;
  m_format.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
  m_format.SampleFormat.VideoLighting = DXVA2_VideoLighting_dim;

  if(avctx->color_range == AVCOL_RANGE_JPEG)
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

  // create decoding surfaces
  m_refs = avctx->refs;
  if (m_refs == 0)
  {
    if(avctx->codec_id == CODEC_ID_H264)
      m_refs = 16;
    else
      m_refs = 2;
  }
  CLog::Log(LOGDEBUG, "DXVA - source requires %d references", avctx->refs);

  unsigned count = 0;

  // Open DXVA video processor
  if (!g_Windowing.m_processor->Open(m_format.SampleWidth, m_format.SampleHeight, m_format.Format))
    return false;
  count = g_Windowing.m_processor->GetSize();

  m_context->surface_count = m_refs + 1 + 1 + count; // refs + 1 decode + 1 libavcodec safety + processor buffer
  
  CLog::Log(LOGDEBUG, "DXVA - allocating %d surfaces", m_context->surface_count);
  
  m_surfaces = (LPDIRECT3DSURFACE9*)calloc(m_context->surface_count, sizeof(LPDIRECT3DSURFACE9));
  CHECK(m_service->CreateSurface( (m_format.SampleWidth + 15) & ~15
                                , (m_format.SampleHeight + 15) & ~15
                                , m_context->surface_count - 1
                                , m_format.Format
                                , D3DPOOL_DEFAULT
                                , 0
                                , DXVA2_VideoDecoderRenderTarget
                                , m_surfaces, NULL ));

  g_Windowing.m_processor->LockSurfaces(m_surfaces, m_context->surface_count);

  m_context->surface = (LPDIRECT3DSURFACE9*)calloc(m_context->surface_count, sizeof(LPDIRECT3DSURFACE9));
  for (unsigned i = 0; i < m_context->surface_count; i++)
    m_context->surface[i] = m_surfaces[i];

  // find what decode configs are available
  UINT                       cfg_count = 0;
  DXVA2_ConfigPictureDecode *cfg_list  = NULL;
  CHECK(m_service->GetDecoderConfigurations(m_input
                                          , &m_format
                                          , NULL
                                          , &cfg_count
                                          , &cfg_list));
  SCOPE(DXVA2_ConfigPictureDecode, cfg_list);

  DXVA2_ConfigPictureDecode config = {};
  config.ConfigBitstreamRaw = 0;

  unsigned bitstream = 2; // ConfigBitstreamRaw = 2 is required for Poulsbo and handles skipping better with nVidia
  for(unsigned i = 0; i< cfg_count; i++)
  {
    CLog::Log(LOGDEBUG,
              "DXVA - config %d: bitstream type %d%s",
              i,
              cfg_list[i].ConfigBitstreamRaw,
              IsEqualGUID(cfg_list[i].guidConfigBitstreamEncryption, DXVA_NoEncrypt) ? "" : ", encrypted");

    // select first available
    if(config.ConfigBitstreamRaw == 0 && cfg_list[i].ConfigBitstreamRaw != 0)
      config = cfg_list[i];

    // overide with preferred if found
    if(config.ConfigBitstreamRaw != bitstream && cfg_list[i].ConfigBitstreamRaw == bitstream)
      config = cfg_list[i];
  }

  if(!config.ConfigBitstreamRaw)
  {
    CLog::Log(LOGWARNING, "DXVA - failed to find a raw input bitstream");
    return false;
  }
  *const_cast<DXVA2_ConfigPictureDecode*>(m_context->cfg) = config;

  CHECK(m_service->CreateVideoDecoder(m_input, &m_format
                                    , m_context->cfg
                                    , m_context->surface
                                    , m_context->surface_count
                                    , &m_context->decoder));

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
    for(unsigned i = 0; i < m_context->surface_count; i++)
    {
      if (m_context->surface[i] == (IDirect3DSurface9*)frame->data[3])
        return VC_BUFFER | VC_PICTURE;
    }
    CLog::Log(LOGWARNING, "DXVA - ignoring invalid surface");
    return VC_BUFFER;
  }

  return 0;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(picture);

  CSingleLock lock(m_section);

  picture->data[3] = frame->data[3];
  picture->format = DVDVideoPicture::FMT_DXPICT;

  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  CSingleLock lock(m_section);

  bool reset = false;

  if(m_state == DXVA_RESET)
  {
    Close();
    reset = true;
  }

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
    reset = true;
  }

  if (reset)
  {
    if(!Open(avctx, avctx->pix_fmt))
    {
      CLog::Log(LOGERROR, "CDecoder::Check - decoder was not able to reset");
      Close();
      return VC_ERROR;
    }
    return VC_FLUSHED;
  }

  if((unsigned)avctx->refs > m_refs)
  {
    CLog::Log(LOGWARNING, "CDecoder::Check - number of required reference frames increased, recreating decoder");
    Close();
    return VC_FLUSHED;
  }

  // Status reports are available only for the DXVA2_ModeH264 and DXVA2_ModeVC1 modes
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
  data.Function = DXVA_STATUS_REPORTING_FUNCTION;
  data.pPrivateOutputData    = &status;
  data.PrivateOutputDataSize = avctx->codec_id == CODEC_ID_H264 ? sizeof(DXVA_Status_H264) : sizeof(DXVA_Status_VC1);
  HRESULT hr;
  if(FAILED( hr = m_context->decoder->Execute(&params)))
  {
    CLog::Log(LOGWARNING, "DXVA - failed to get decoder status - 0x%08X", hr);
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

  return 0;
}

bool CDecoder::OpenTarget(const GUID &guid)
{
  bool       output_found = false;
  UINT       output_count = 0;
  D3DFORMAT *output_list  = NULL;
  CHECK(m_service->GetDecoderRenderTargets(guid, &output_count, &output_list));
  SCOPE(D3DFORMAT, output_list);

  for(unsigned k = 0; k < output_count; k++)
  {
    if(output_list[k] == MAKEFOURCC('Y','V','1','2')
    || output_list[k] == MAKEFOURCC('N','V','1','2'))
    {
      m_input         = guid;
      m_format.Format = output_list[k];
      if (output_list[k] == MAKEFOURCC('N','V','1','2'))
        return true;
      else
        output_found = true;
    }
  }
  return output_found;
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

  if (m_level > 0)
  {
    m_surfaces[m_write] = (IDirect3DSurface9*)pic->data[3];
    m_write = (m_write + 1) % m_context->surface_count;

    m_level--;

    for(unsigned i = 0; i < 4; i++)
    pic->data[i] = NULL;
  }
}

int CDecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CSingleLock lock(m_section);

  if(avctx->width  != m_format.SampleWidth
  || avctx->height != m_format.SampleHeight)
  {
    Close();
    if(!Open(avctx, avctx->pix_fmt))
    {
      Close();
      return -1;
    }
  }

  if(m_level > m_refs+2)
  {
    CLog::Log(LOGERROR, "DXVA - number of used surfaces exceeded");
    Close();
    return -1;
  }

  IDirect3DSurface9* surface = m_surfaces[m_read];
  m_read = (m_read + 1) % m_context->surface_count;

  m_level++;

  pic->reordered_opaque = avctx->reordered_opaque;
  pic->type = FF_BUFFER_TYPE_USER;
  pic->age  = 256*256*256*64; // as everybody else, i've got no idea about this one
  
  for (unsigned i = 0; i < 4; i++) pic->linesize[i] = 0;
  pic->data[0] = (uint8_t*)surface;
  pic->data[1] = NULL;
  pic->data[2] = NULL;
  pic->data[3] = (uint8_t*)surface;

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
  m_references = 1;
  g_Windowing.Register(this);

  m_surfaces = NULL;

  m_samples = NULL;
  m_index = 0;
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

  if (m_samples)
  {
    free(m_samples);
    m_samples = NULL;
  }
  
  if (m_surfaces)
  {
    for (unsigned i = 0; i < m_count; i++) SAFE_RELEASE(m_surfaces[i]);
    free(m_surfaces);
    m_surfaces = NULL;
  }

  m_opened = false;
}

bool CProcessor::Create()
{
  Close();

  if(!LoadDXVA())
    return false;

  CSingleLock lock(m_section);

  CHECK(g_DXVA2CreateVideoService(g_Windowing.Get3DDevice(), IID_IDirectXVideoProcessorService, (void**)&m_service));

  m_opened = false;

  return true;
}

bool CProcessor::Open(UINT width, UINT height)
{
  // Only NV12 software colorspace conversion is implemented for now
  if (Open(width, height, (D3DFORMAT)MAKEFOURCC('N','V','1','2')))
    return true;

  // Future...
  //if (Open(width, height, (D3DFORMAT)MAKEFOURCC('Y','U','Y','2')))
  //  return true;

  //if (Open(width, height, (D3DFORMAT)MAKEFOURCC('U','Y','V','Y')))
  //  return true;

  return false;
}

bool CProcessor::Open(UINT width, UINT height, D3DFORMAT format)
{
  CSingleLock lock(m_section);

  memset(&m_desc, 0, sizeof(DXVA2_VideoDesc));
  m_desc.SampleWidth = width;
  m_desc.SampleHeight = height;
  m_desc.Format  = format;
  m_desc.UABProtectionLevel = FALSE;

  m_StreamSampleFormat = DXVA2_SampleUnknown;
  
  // Reset detected processors
  m_progdevice  = GUID_NULL;
  m_bobdevice   = GUID_NULL;
  m_hqdevice    = GUID_NULL;

  m_device      = GUID_NULL;

  GUID*           deint_guid_list;
  GUID*           prog_guid_list;
  unsigned        guid_count;

  // Find deinterlacing processors
  m_desc.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedOddFirst;
  CHECK(m_service->GetVideoProcessorDeviceGuids(&m_desc, &guid_count, &deint_guid_list));
  SCOPE(GUID, deint_guid_list);

  if (guid_count == 0)
    CLog::Log(LOGDEBUG, "DXVA - unable to find any processors with deinterlace capabilities");
  else
  {
    GUID knownhqdevice = GUID_NULL;
    GUID unknownhqdevice = GUID_NULL;
    int  knownhqdevicescore = 0;
    int  unknownhqdevicescore = 0;

    for (unsigned i = 0; i < guid_count; i++)
    {
      GUID* g = &deint_guid_list[i];
      int   j;

      // Lookup known devices
      for (j = 0; dxva2_devices[j].name; j++ )
      {
        if (IsEqualGUID(*g, *dxva2_devices[j].guid))
        {
          CLog::Log(LOGDEBUG, "DXVA - known processor found: %s, deinterlace quality score: %d, guid: %s", dxva2_devices[j].name, dxva2_devices[j].score, GUIDToString(*g).c_str());
          if (dxva2_devices[j].score > knownhqdevicescore)
          {
            knownhqdevice = *g;
            knownhqdevicescore = dxva2_devices[j].score;
          }
          break;
        }
        else if (IsEqualGUID(*g, DXVA2_VideoProcBobDevice))
          m_bobdevice = *g;
      }

      // Check unknown device deinterlace capabilities
      if (!dxva2_devices[j].name)
      {
        CHECK(m_service->GetVideoProcessorCaps(*g, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
        for (j = 0; dxva2_deinterlacetechs[j].name; j++)
        {
          if (m_caps.DeinterlaceTechnology & dxva2_deinterlacetechs[j].deinterlacetech_flag)
          {
            int score = dxva2_deinterlacetechs[j].score * 100 + m_caps.NumBackwardRefSamples + m_caps.NumForwardRefSamples;
            CLog::Log(LOGDEBUG, "DXVA - unknown processor found, deinterlace technology: %s, deinterlace quality score: %d, guid: %s", dxva2_deinterlacetechs[j].name, score, GUIDToString(*g).c_str());
            if (dxva2_deinterlacetechs[j].score > score)
            {
              unknownhqdevice = *g;
              unknownhqdevicescore = score;
            }
            break;
          }
        }

        if (!dxva2_deinterlacetechs[j].name)
          CLog::Log(LOGDEBUG, "DXVA - unknown processor found (not considered), guid: %s", GUIDToString(*g).c_str());
      }
    }

    if (knownhqdevice != GUID_NULL) m_hqdevice = knownhqdevice;
    else if (unknownhqdevice != GUID_NULL) m_hqdevice = unknownhqdevice;
  }

  // Find progressive processor
  m_desc.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
  CHECK(m_service->GetVideoProcessorDeviceGuids(&m_desc, &guid_count, &prog_guid_list));
  SCOPE(GUID, prog_guid_list);

  m_default = GUID_NULL;

  if (guid_count > 0)
  {
    m_default = prog_guid_list[0];
    for (unsigned i = 0; i < guid_count; i++)
    {
      GUID* g = &prog_guid_list[i];
      if (IsEqualGUID(*g, DXVA2_VideoProcProgressiveDevice))
        m_progdevice = *g;
    }
  }

  if (m_default == GUID_NULL)
  {
    CLog::Log(LOGERROR, "DXVA - unable to find default processor device, DXVA will not be used");
    return false;
  }

  // This should never happen according to DXVA2 specification...
  if (m_hqdevice == GUID_NULL && m_bobdevice == GUID_NULL && m_progdevice == GUID_NULL)
  {
    CLog::Log(LOGDEBUG, "DXVA - unable to find a suitable processor, falling back to default, guid: %s", GUIDToString(m_default).c_str());
    m_progdevice = m_default;
  }

  // Save max. required number of reference samples
  m_size = 0;
  if (m_hqdevice != GUID_NULL)
  {
    CHECK(m_service->GetVideoProcessorCaps(m_hqdevice, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
    CLog::Log(LOGDEBUG, "DXVA - HQ processor requires %d past frames and %d future frames", m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples);
    NEWMAX(m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples, m_size);
  }
  if (m_bobdevice != GUID_NULL)
  {
    CHECK(m_service->GetVideoProcessorCaps(m_bobdevice, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
    CLog::Log(LOGDEBUG, "DXVA - Bob processor requires %d past frames and %d future frames", m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples);
    NEWMAX(m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples, m_size);
  }
  if (m_progdevice != GUID_NULL)
  {
    CHECK(m_service->GetVideoProcessorCaps(m_progdevice, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
    CLog::Log(LOGDEBUG, "DXVA - Progressive processor requires %d past frames and %d future frames", m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples);
    NEWMAX(m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples, m_size);
  }

  m_size += 3;  // safety frames

  m_samples = (VideoSample*)calloc(m_size, sizeof(VideoSample));
  for (unsigned i = 0; i < m_size; i++)
  {
    m_samples[i].Time = 0;
    m_samples[i].SrcSurface = NULL;
  }

  m_time = 0;

  if (!SelectProcessor()) return false;

  m_opened = true;

  return true;
}

bool CProcessor::CreateSurfaces()
{
  CSingleLock lock(m_section);

  m_count = m_size;
  m_surfaces = (LPDIRECT3DSURFACE9*)calloc(m_count, sizeof(LPDIRECT3DSURFACE9));
  CHECK(m_service->CreateSurface((m_desc.SampleWidth + 15) & ~15,
                                 (m_desc.SampleHeight + 15) & ~15,
                                  m_count - 1,
                                  m_desc.Format,
                                  D3DPOOL_DEFAULT,
                                  0,
                                  DXVA2_VideoSoftwareRenderTarget,
                                  m_surfaces,
                                  NULL));

  return true;
}

bool CProcessor::LockSurfaces(LPDIRECT3DSURFACE9* surfaces, unsigned count)
{
  CSingleLock lock(m_section);

  m_count = count;

  m_surfaces = (LPDIRECT3DSURFACE9*)calloc(m_count, sizeof(LPDIRECT3DSURFACE9));
  for (unsigned i = 0; i < m_count; i++)
  {
    m_surfaces[i] = surfaces[i];
    m_surfaces[i]->AddRef();
  }

  return true;
}

bool CProcessor::SelectProcessor()
{
  if (!m_service) return false;

  m_CurrInterlaceMethod = g_settings.m_currentVideoSettings.m_InterlaceMethod;
  bool useautobob = (((m_desc.SampleWidth > g_advancedSettings.m_DXVADeintAutoMaxWidth) || (m_desc.SampleHeight > g_advancedSettings.m_DXVADeintAutoMaxHeight))
                  && ((float)m_desc.InputSampleFreq.Denominator > g_advancedSettings.m_DXVADeintAutoMaxFps * (float)m_desc.InputSampleFreq.Numerator));

  // Synchronize sample type and render deinterlace method
  EINTERLACEMETHOD deint = m_CurrInterlaceMethod;
  switch (m_CurrInterlaceMethod)
  {
    case VS_INTERLACEMETHOD_NONE:
      m_desc.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
      break;
    case VS_INTERLACEMETHOD_DXVA_BOB:
    case VS_INTERLACEMETHOD_DXVA_HQ:
      m_desc.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
      break;
    case VS_INTERLACEMETHOD_DXVA_BOB_INVERTED:
    case VS_INTERLACEMETHOD_DXVA_HQ_INVERTED:
      m_desc.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedOddFirst;
      break;
    case VS_INTERLACEMETHOD_AUTO:
  default:
      m_desc.SampleFormat.SampleFormat = m_StreamSampleFormat;
      if (m_desc.SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedOddFirst) deint = useautobob ? VS_INTERLACEMETHOD_DXVA_BOB_INVERTED : VS_INTERLACEMETHOD_DXVA_HQ_INVERTED;
      else if (m_desc.SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedEvenFirst) deint = useautobob? VS_INTERLACEMETHOD_DXVA_BOB : VS_INTERLACEMETHOD_DXVA_HQ;
      else deint = VS_INTERLACEMETHOD_NONE;
      break;
  }
  m_BFF = (m_desc.SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedOddFirst ? true : false);

  // Find out which processor to use based on render deinterlace method
  GUID device = m_default;
  switch (deint)
  {
    case VS_INTERLACEMETHOD_NONE:
      // Fallback: progressive -> bob -> hq
      if (m_progdevice != GUID_NULL) device = m_progdevice;
      else if (m_bobdevice != GUID_NULL) device = m_bobdevice;
      else if (m_hqdevice != GUID_NULL) device = m_hqdevice;
      break;
    case VS_INTERLACEMETHOD_DXVA_BOB:
    case VS_INTERLACEMETHOD_DXVA_BOB_INVERTED:
      // Fallback: bob -> hq -> progressive
      if (m_bobdevice != GUID_NULL) device = m_bobdevice;
      else if (m_hqdevice != GUID_NULL) device = m_hqdevice;
      else if (m_progdevice != GUID_NULL) device = m_progdevice;
      break;
    case VS_INTERLACEMETHOD_DXVA_HQ:
    case VS_INTERLACEMETHOD_DXVA_HQ_INVERTED:
      // Fallback: hq -> bob -> progressive
      if (m_hqdevice != GUID_NULL) device = m_hqdevice;
      else if (m_bobdevice != GUID_NULL) device = m_bobdevice;
      else if (m_progdevice != GUID_NULL) device = m_progdevice;
      break;
  }

  if (!IsEqualGUID(device, m_device))
  {
    // Try to switch processor mid-stream
    m_device = device;
    CLog::Log(LOGDEBUG, "DXVA - processor selected %s", GUIDToString(m_device).c_str());
    CHECK(m_service->GetVideoProcessorCaps(m_device, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
    if (m_caps.DeviceCaps & DXVA2_VPDev_SoftwareDevice)
      CLog::Log(LOGDEBUG, "DXVA - processor is software device");
    if (m_caps.DeviceCaps & DXVA2_VPDev_EmulatedDXVA1)
      CLog::Log(LOGDEBUG, "DXVA - processor is emulated dxva1");
    CLog::Log(LOGDEBUG, "DXVA - processor requires %d past frames and %d future frames", m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples);

    // Release old processor and try to initialize new one
    SAFE_RELEASE(m_process);
    D3DFORMAT output = m_desc.Format;
    if(FAILED(m_service->CreateVideoProcessor(m_device, &m_desc, output, 0, &m_process)))
    {
      CLog::Log(LOGDEBUG, "DXVA - unable to use source format, use RGB output instead\n");
      output = D3DFMT_X8R8G8B8;
      CHECK(m_service->CreateVideoProcessor(m_device, &m_desc, output, 0, &m_process));
    }

    CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Brightness, &m_brightness));
    CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Contrast  , &m_contrast));
    CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Hue       , &m_hue));
    CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Saturation, &m_saturation));
  }

  return true;
}

void CProcessor::StillFrame()
{
  CSingleLock lock(m_section);

  IDirect3DSurface9* surface = m_samples[(m_index + m_size - 1) % m_size].SrcSurface;

  // we make sure to present the same frame even if deinterlacing is on
  for (unsigned i = 0; i < m_size; i++)
    m_samples[i].SrcSurface = surface;
}

REFERENCE_TIME CProcessor::Add(IDirect3DSurface9* source)
{
  do
  {
    m_time += 2;
    
    m_samples[m_index].Time = m_time;
    m_samples[m_index].SrcSurface = source;
    
    m_index = (m_index + 1) % m_size;
  }
  while (m_samples[m_index].SrcSurface == NULL);  // workaround to present a frame without past or future reference frames

  return m_time;
}

bool CProcessor::ProcessPicture(DVDVideoPicture* picture)
{
  CSingleLock lock(m_section);

  IDirect3DSurface9* surface = NULL;
  m_StreamSampleFormat = picture->iFlags & DVP_FLAG_INTERLACED ? (picture->iFlags & DVP_FLAG_TOP_FIELD_FIRST ? DXVA2_SampleFieldInterleavedEvenFirst : DXVA2_SampleFieldInterleavedOddFirst) : DXVA2_SampleProgressiveFrame;

  switch (picture->format)
  {
    case DVDVideoPicture::FMT_DXVA:
      return true;

    case DVDVideoPicture::FMT_DXPICT:
      surface = (IDirect3DSurface9*)picture->data[3];
      break;

    case DVDVideoPicture::FMT_YUV420P:
      surface = m_surfaces[m_index];
  
      D3DLOCKED_RECT rectangle;
      CHECK(surface->LockRect(&rectangle, NULL, 0));

      // copy luma
      uint8_t *s = picture->data[0];
      uint8_t* bits = (uint8_t*)(rectangle.pBits);
      for (unsigned y = 0; y < picture->iHeight; y++)
      {
        memcpy(bits, s, picture->iWidth);
        s += picture->iLineSize[0];
        bits += rectangle.Pitch;
      }

      D3DSURFACE_DESC desc;
      CHECK(surface->GetDesc(&desc));

      //copy chroma
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
  
      CHECK(surface->UnlockRect());
      break;
  }

  if (!surface)
  {
    CLog::Log(LOGWARNING, "DXVA - colorspace not supported by processor");
    return false;
  }
  
  picture->proc = this;
  picture->proc_id = picture->iFlags & DVP_FLAG_DROPPED ? 0 : Add(surface);
  
  picture->format = DVDVideoPicture::FMT_DXVA;

  return true;
}

static DXVA2_Fixed32 ConvertRange(const DXVA2_ValueRange& range, int value, int min, int max, int def)
{
  if(value > def)
    return DXVA2FloatToFixed( DXVA2FixedToFloat(range.DefaultValue)
                            + (DXVA2FixedToFloat(range.MaxValue) - DXVA2FixedToFloat(range.DefaultValue))
                            * (value - def) / (max - def) );
  if(value < def)
    return DXVA2FloatToFixed( DXVA2FixedToFloat(range.DefaultValue)
                            + (DXVA2FixedToFloat(range.MinValue) - DXVA2FixedToFloat(range.DefaultValue)) 
                            * (value - def) / (min - def) );
  return range.DefaultValue;
}

bool CProcessor::Render(const RECT &dst, IDirect3DSurface9* target, REFERENCE_TIME time, int fieldflag)
{
  CSingleLock lock(m_section);

  // If deinterlace method or stream interlace format has changed, switch to another DXVA processor
  if (m_CurrInterlaceMethod != g_settings.m_currentVideoSettings.m_InterlaceMethod)
  {
    CLog::Log(LOGDEBUG,"CProcessor::Render - deinterlace method changed, switching DXVA processor");
    if (!SelectProcessor()) return false;
  }
  if (m_CurrInterlaceMethod == VS_INTERLACEMETHOD_AUTO && m_desc.SampleFormat.SampleFormat != m_StreamSampleFormat)
  {
    CLog::Log(LOGDEBUG,"CProcessor::Render - stream interlace format changed, switching DXVA processor");
    if (!SelectProcessor()) return false;
  }
  if (!m_process || !m_samples)
  {
    CLog::Log(LOGWARNING, "CProcessor::Render - processor not properly set up, skipping frame");
    return false;
  }

  if ((time > m_time) || (time - (m_caps.NumBackwardRefSamples + m_caps.NumForwardRefSamples) * 2 <= m_time - m_size * 2))
  {
    CLog::Log(LOGDEBUG, "CProcessor::Render - Samples required to render frame %d not present in sample buffer, skipping frame", time);
    return false;
  }

  unsigned count = 1 + m_caps.NumBackwardRefSamples + m_caps.NumForwardRefSamples;
  unsigned index = (m_index + m_size - ((m_time - time) >> 1) - count) % m_size;

  D3DSURFACE_DESC desc;
  CHECK(target->GetDesc(&desc));

  RECT dest = dst, src = { 0, 0, m_desc.SampleWidth, m_desc.SampleHeight };
  CWinRenderer::CropSource(src, dest, desc);

  auto_aptr<DXVA2_VideoSample> samples(new DXVA2_VideoSample[count]);
  for (unsigned i = 0; i < count; i++)
  {
    samples[i].Start = m_samples[index].Time;
    samples[i].End = m_samples[index].Time + 2;
    samples[i].SampleFormat.SampleFormat = m_desc.SampleFormat.SampleFormat;
    samples[i].SrcSurface = m_samples[index].SrcSurface;
    samples[i].SrcRect = src;
    samples[i].DstRect = dest;
    samples[i].PlanarAlpha = DXVA2_Fixed32OpaqueAlpha();
    samples[i].SampleData = 0;
    index = (index + 1) % m_size;
  }

  DXVA2_VideoProcessBltParams blt = {};
  blt.TargetFrame = time - m_caps.NumForwardRefSamples * 2 + (m_BFF ? 1 - fieldflag : fieldflag);
  blt.TargetRect.left     = 0;
  blt.TargetRect.top      = 0;
  blt.TargetRect.right    = desc.Width;
  blt.TargetRect.bottom   = desc.Height;

  blt.DestFormat.VideoTransferFunction = DXVA2_VideoTransFunc_sRGB;
  blt.DestFormat.SampleFormat          = m_desc.SampleFormat.SampleFormat;
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

  /* HACK to kickstart certain DXVA drivers (poulsbo) which oddly  *
   * won't render anything until someting else have been rendered. */
  g_Windowing.Get3DDevice()->SetFVF( D3DFVF_XYZ );
  float verts[2][3]= {};
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 1, verts, 3*sizeof(float));

  CHECK(m_process->VideoProcessBlt(target, &blt, samples.get(), count, NULL));

  return true;
}

CProcessor* CProcessor::Acquire()
{
  AtomicIncrement(&m_references);
  return this;
}

long CProcessor::Release()
{
  long count = AtomicDecrement(&m_references);
  ASSERT(count >= 0);
  if (count == 0) delete this;
  return count;
}

#endif
