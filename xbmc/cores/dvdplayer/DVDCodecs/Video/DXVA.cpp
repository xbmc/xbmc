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

#define ALLOW_ADDING_SURFACES 0

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

// Vendor-specific DXVA2 video processor GUIDs
DEFINE_GUID(DXVA2_VideoProcATIVectorAdaptiveDevice, 0x3C5323C1,0x6fb7,0x44f5,0x90,0x81,0x05,0x6b,0xf2,0xee,0x44,0x9d);
DEFINE_GUID(DXVA2_VideoProcATIMotionAdaptiveDevice, 0x552C0DAD,0xccbc,0x420b,0x83,0xc8,0x74,0x94,0x3c,0xf9,0xf1,0xa6);
DEFINE_GUID(DXVA2_VideoProcATIAdaptiveDevice,       0x6E8329FF,0xb642,0x418b,0xbc,0xf0,0xbc,0xb6,0x59,0x1e,0x25,0x5f);
DEFINE_GUID(DXVA2_VideoProcNVidiaAdaptiveDevice,    0x6CB69578,0x7617,0x4637,0x91,0xE5,0x1C,0x02,0xDB,0x81,0x02,0x85);
DEFINE_GUID(DXVA2_VideoProcIntelEdgeDevice,         0xBF752EF6,0x8CC4,0x457A,0xBE,0x1B,0x08,0xBD,0x1C,0xAE,0xEE,0x9F);

typedef struct {
  const char       *name;
  const GUID       *guid;
  EINTERLACEMETHOD method;
  unsigned         caps;
} dxva2_procdevice_t;

// List of all known DXVA2 processor devices, their deinterlace method assignments and its capability flags
static const dxva2_procdevice_t dxva2_procdevices[] = {
  { "Vector adaptive device (ATI) - High quality",      &DXVA2_VideoProcATIVectorAdaptiveDevice, VS_INTERLACEMETHOD_DXVA_ADAPTIVE_HIGH,   DXVA_DEINTERLACE_ADAPTIVE_HIGH   },
  { "Motion adaptive device (ATI) - Adaptive (medium)", &DXVA2_VideoProcATIMotionAdaptiveDevice, VS_INTERLACEMETHOD_DXVA_ADAPTIVE_MEDIUM, DXVA_DEINTERLACE_ADAPTIVE_MEDIUM },
  { "Adaptive device (ATI) - Adaptive (low)",           &DXVA2_VideoProcATIAdaptiveDevice,       VS_INTERLACEMETHOD_DXVA_ADAPTIVE_LOW,    DXVA_DEINTERLACE_ADAPTIVE_LOW    },
  { "Spatial-temporal device (nVidia) - High quality",  &DXVA2_VideoProcNVidiaAdaptiveDevice,    VS_INTERLACEMETHOD_DXVA_ADAPTIVE_HIGH,   DXVA_DEINTERLACE_ADAPTIVE_HIGH   },
  { "Edge directed device (Intel) - High quality",      &DXVA2_VideoProcIntelEdgeDevice,         VS_INTERLACEMETHOD_DXVA_ADAPTIVE_HIGH,   DXVA_DEINTERLACE_ADAPTIVE_HIGH   },
  { "Bob device (DXVA2 standard) - Bob",                &DXVA2_VideoProcBobDevice,               VS_INTERLACEMETHOD_DXVA_BOB,             DXVA_DEINTERLACE_BOB             },
  { "Progressive device (DXVA2 standard)",              &DXVA2_VideoProcProgressiveDevice,       VS_INTERLACEMETHOD_NONE,                 0                                },
  { NULL, NULL, VS_INTERLACEMETHOD_NONE, false }
};

typedef struct
{
  EINTERLACEMETHOD method;
  EINTERLACEMETHOD substitute;
} dxva2_deinterlace_subst_t;

// DXVA2-specific deinterlace method substitution table; if substitute == method then no substitute for that method
static const dxva2_deinterlace_subst_t dxva2_deinterlace_subst[] = {
  { VS_INTERLACEMETHOD_DXVA_ADAPTIVE_HIGH,   VS_INTERLACEMETHOD_DXVA_ADAPTIVE_MEDIUM },
  { VS_INTERLACEMETHOD_DXVA_ADAPTIVE_MEDIUM, VS_INTERLACEMETHOD_DXVA_ADAPTIVE_LOW    },
  { VS_INTERLACEMETHOD_DXVA_ADAPTIVE_LOW,    VS_INTERLACEMETHOD_DXVA_BOB             },
  { VS_INTERLACEMETHOD_DXVA_BOB,             VS_INTERLACEMETHOD_NONE                 },
  { VS_INTERLACEMETHOD_NONE,                 VS_INTERLACEMETHOD_NONE                 },
};

typedef struct {
  const char *name;
  UINT       deinterlacetech_flag;
  int        score;
} dxva2_deinterlacetech_t;

// List of all considered DXVA2 deinterlace technology flags and their scores for unknown processors, preferred must come first
static const dxva2_deinterlacetech_t dxva2_deinterlacetechs[] = {
  { "Motion vector steered",      DXVA2_DeinterlaceTech_MotionVectorSteered,    100 },
  { "Pixel adaptive",             DXVA2_DeinterlaceTech_PixelAdaptive,           90 },
  { "Field adaptive",             DXVA2_DeinterlaceTech_FieldAdaptive,           80 },
  { "Edge filtering",             DXVA2_DeinterlaceTech_EdgeFiltering,           70 },
  { "Median filtering",           DXVA2_DeinterlaceTech_MedianFiltering,         60 },
  { "Bob vertical stretch 4-tap", DXVA2_DeinterlaceTech_BOBVerticalStretch4Tap,  50 },
  { "Bob vertical stretch",       DXVA2_DeinterlaceTech_BOBVerticalStretch,      40 },
  { "Bob line replicate",         DXVA2_DeinterlaceTech_BOBLineReplicate,        30 },
  { NULL, 0, 0 }
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


#define SCOPE(type, var) boost::shared_ptr<type> var##_holder(var, CoTaskMemFree);
#define NEWMAX(a, b) a = a > b ? a : b;

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
  m_buffer_age   = 0;
  m_refs         = 0;
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
  if (!CheckCompatibility(avctx))
    return false;

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

  // find what decode configs are available
  UINT                       cfg_count = 0;
  DXVA2_ConfigPictureDecode *cfg_list  = NULL;
  CHECK(m_service->GetDecoderConfigurations(m_input
                                          , &m_format
                                          , NULL
                                          , &cfg_count
                                          , &cfg_list))
  SCOPE(DXVA2_ConfigPictureDecode, cfg_list);

  DXVA2_ConfigPictureDecode config = {};

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
    CLog::Log(LOGDEBUG, "DXVA - failed to find a raw input bitstream");
    return false;
  }
  *const_cast<DXVA2_ConfigPictureDecode*>(m_context->cfg) = config;

  if(!OpenProcessor())
    return false;

  if(!OpenDecoder())
    return false;

  // The front buffer seems to be the only one required and it seems to always be m_buffer[0] on ATI and nVidia
  // but how reliable is that information?
  // Not adding refs causes a crash at the end of playback because the surfaces are released with the decoder, despite the >0 D3D ref count.
  for(unsigned i = 0; i < m_buffer_count; i++)
    m_processor->HoldSurface(m_buffer[i].surface);

  avctx->get_buffer      = GetBufferS;
  avctx->release_buffer  = RelBufferS;
  avctx->hwaccel_context = m_context;

  if (IsL41LimitedATI())
  {
#ifdef FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG
    m_context->workaround |= FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG;
#else
    CLog::Log(LOGWARNING, "DXVA - video card with different scaling list zigzag order detected, but no support in libavcodec");
#endif
  }

  return true;
}

bool CDecoder::OpenProcessor()
{
  m_state = DXVA_OPEN;

  {
    CSingleExit leave(m_section);
    CProcessor* processor = new CProcessor();
    m_processor = processor;
  }

  if(m_state != DXVA_OPEN)
  {
    CLog::Log(LOGERROR, "DXVA - device was lost while trying to create a processor");
    return false;
  }

  if(!m_processor->Open(m_format))
    return false;

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
  ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(picture);
  CSingleLock lock(m_section);
  picture->format = DVDVideoPicture::FMT_DXVA;
  picture->proc   = m_processor;
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
      CLog::Log(LOGERROR, "CDecoder::Check - device didn't reset in reasonable time");
      return VC_ERROR;
    }
  }

  if(avctx->refs > m_refs)
  {
    CLog::Log(LOGWARNING, "CDecoder::Check - number of required reference frames increased, recreating decoder");
#if ALLOW_ADDING_SURFACES
    if(!OpenDecoder())
      return VC_ERROR;
#else
    Close();
    return VC_FLUSHED;
#endif
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
  if(FAILED( hr = m_decoder->Execute(&params)))
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
  CHECK(m_service->GetDecoderRenderTargets(guid, &output_count, &output_list))
  SCOPE(D3DFORMAT, output_list);

  for(unsigned k = 0; k < output_count; k++)
  {
    if(output_list[k] == MAKEFOURCC('Y','V','1','2')
    || output_list[k] == MAKEFOURCC('N','V','1','2'))
    {
      m_input         = guid;
      m_format.Format = output_list[k];
      if (output_list[k] == MAKEFOURCC('N','V','1','2'))
        return true; // Prefer NV12; Ati (at least) has better deinterlacing
      else
        output_found = true;
    }
  }
  return output_found;
}

bool CDecoder::OpenDecoder()
{
  SAFE_RELEASE(m_decoder);

  m_context->surface_count = m_refs + 1 + 1 + m_processor->Size(); // refs + 1 decode + 1 libavcodec safety + processor buffer

  if(m_context->surface_count > m_buffer_count)
  {
    CLog::Log(LOGDEBUG, "DXVA - allocating %d surfaces", m_context->surface_count - m_buffer_count);

    CHECK(m_service->CreateSurface( (m_format.SampleWidth  + 15) & ~15
                                  , (m_format.SampleHeight + 15) & ~15
                                  , m_context->surface_count - 1 - m_buffer_count
                                  , m_format.Format
                                  , D3DPOOL_DEFAULT
                                  , 0
                                  , DXVA2_VideoDecoderRenderTarget
                                  , m_context->surface + m_buffer_count, NULL ));

    for(unsigned i = m_buffer_count; i < m_context->surface_count; i++)
      m_buffer[i].surface = m_context->surface[i];

    m_buffer_count = m_context->surface_count;
  }

  CHECK(m_service->CreateVideoDecoder(m_input, &m_format
                                    , m_context->cfg
                                    , m_context->surface
                                    , m_context->surface_count
                                    , &m_decoder))

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
  g_Windowing.Register(this);

  m_surfaces = NULL;
  m_index = 0;
}

CProcessor::~CProcessor()
{
  g_Windowing.Unregister(this);
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
  for (vector<IDirect3DSurface9*>::iterator it = m_heldsurfaces.begin(); it != m_heldsurfaces.end(); it++)
    SAFE_RELEASE(*it);

  if (m_surfaces)
  {
    for (unsigned i = 0; i < m_surfaces_count; i++) SAFE_RELEASE(m_surfaces[i]);
    free(m_surfaces);
    m_surfaces = NULL;
  }
}

bool CProcessor::Open(UINT width, UINT height, unsigned int flags)
{
  // Only NV12 software colorspace conversion is implemented for now
  if (Open(width, height, flags, (D3DFORMAT)MAKEFOURCC('N','V','1','2')))
    return true;

  // Future...
  //if (Open(width, height, flags, (D3DFORMAT)MAKEFOURCC('Y','U','Y','2')))
  //  return true;

  //if (Open(width, height, flags, (D3DFORMAT)MAKEFOURCC('U','Y','V','Y')))
  //  return true;

  return false;
}

bool CProcessor::Open(UINT width, UINT height, unsigned int flags, D3DFORMAT format)
{
  DXVA2_VideoDesc dsc;
  memset(&dsc, 0, sizeof(DXVA2_VideoDesc));

  dsc.Format = format;
  dsc.SampleWidth = width;
  dsc.SampleHeight = height;
  dsc.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
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

  return Open(dsc);
}

bool CProcessor::Open(const DXVA2_VideoDesc& dsc)
{
  Close();

  if(!LoadDXVA())
    return false;

  CSingleLock lock(m_section);
  m_desc = dsc;

  CHECK(g_DXVA2CreateVideoService(g_Windowing.Get3DDevice(), IID_IDirectXVideoProcessorService, (void**)&m_service));

  m_processors.clear();
  m_defaultdevice = GUID_NULL;
  m_streamsampleformat = DXVA2_SampleUnknown;
  m_size = 0;

  GUID         unknowndevice = GUID_NULL;
  int          unknowndevicescore = 0;
  bool         hasknowndevice = false;
  vector<GUID> guid_list;
  GUID*        progressive_guid_list;
  GUID*        deinterlace_guid_list;
  unsigned     guid_count;

  // Get all progressive and deinterlacing processors: we do this in separate steps for progressive
  // and deinterlacing because drivers might filter the returned GUID list on input sample format
  UINT sampleformat = m_desc.SampleFormat.SampleFormat;
  m_desc.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
  CHECK(m_service->GetVideoProcessorDeviceGuids(&m_desc, &guid_count, &progressive_guid_list));
  SCOPE(GUID, progressive_guid_list);
  for (unsigned i = 0; i < guid_count; i++)
    guid_list.push_back(progressive_guid_list[i]);

  m_desc.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
  CHECK(m_service->GetVideoProcessorDeviceGuids(&m_desc, &guid_count, &deinterlace_guid_list));
  SCOPE(GUID, deinterlace_guid_list);
  for (unsigned i = 0; i < guid_count; i++)
    guid_list.push_back(deinterlace_guid_list[i]);
  m_desc.SampleFormat.SampleFormat = sampleformat;

  if (guid_list.size() == 0)
  {
    CLog::Log(LOGDEBUG, "DXVA - unable to find any processors");
    return false;
  }

  m_defaultdevice = guid_list[0];
  for (unsigned i = 0; i < guid_list.size(); i++)
  {
    GUID g = guid_list[i];
    unsigned j;

    // Check if we know this processor GUID
    for (j = 0; dxva2_procdevices[j].name; j++ )
    {
      if (IsEqualGUID(g, *dxva2_procdevices[j].guid))
      {
        if (SaveProcessor(dxva2_procdevices[j].method, &g, dxva2_procdevices[j].caps))
        {
          CLog::Log(LOGDEBUG, "DXVA - known processor found: %s, guid: %s", dxva2_procdevices[j].name, GUIDToString(g).c_str());
          if (DXVA_DEINTERLACE_MASK(dxva2_procdevices[j].caps))
            hasknowndevice = true;
        }
        break;
      }
      if (IsEqualGUID(g, DXVA2_VideoProcProgressiveDevice))
        m_defaultdevice = g;
    }

    // Check unknown device deinterlace capabilities and pick the best of them
    if (!dxva2_procdevices[j].name)
    {
      CHECK(m_service->GetVideoProcessorCaps(g, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
      for (j = 0; dxva2_deinterlacetechs[j].name; j++)
      {
        if (m_caps.DeinterlaceTechnology & dxva2_deinterlacetechs[j].deinterlacetech_flag)
        {
          int score = dxva2_deinterlacetechs[j].score * 100 + m_caps.NumBackwardRefSamples + m_caps.NumForwardRefSamples;
          CLog::Log(LOGDEBUG, "DXVA - unknown processor found, deinterlace technology: %s, deinterlace quality score: %d, guid: %s", dxva2_deinterlacetechs[j].name, score, GUIDToString(g).c_str());
          if (dxva2_deinterlacetechs[j].score > score)
          {
            unknowndevice = g;
            unknowndevicescore = score;
          }
          break;
        }
      }
    }

    if (!dxva2_deinterlacetechs[j].name)
      CLog::Log(LOGDEBUG, "DXVA - unknown processor found (ignored), guid: %s", GUIDToString(g).c_str());
  }

  // Unknown deinterlacing processor is only utilized if no known processor was found
  if (!hasknowndevice && !IsEqualGUID(unknowndevice, GUID_NULL))
    SaveProcessor(VS_INTERLACEMETHOD_DXVA_ADAPTIVE_HIGH, &unknowndevice, DXVA_DEINTERLACE_ADAPTIVE_HIGH);

  // Get the maximum number of required reference samples
  m_maxbackrefs = m_maxfwdrefs = 0;
  for (vector<dxva2_processor_t>::iterator it = m_processors.begin(); it != m_processors.end(); it++)
  {
    CHECK(m_service->GetVideoProcessorCaps(it->guid, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
    NEWMAX(m_maxbackrefs, m_caps.NumBackwardRefSamples);
    NEWMAX(m_maxfwdrefs, m_caps.NumForwardRefSamples);
    CLog::Log(LOGDEBUG, "DXVA - %d past frames and %d future frames required by processor %s", m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples, GUIDToString(it->guid).c_str());
  }
  CHECK(m_service->GetVideoProcessorCaps(m_defaultdevice, &m_desc, D3DFMT_X8R8G8B8, &m_caps));
  NEWMAX(m_maxbackrefs, m_caps.NumBackwardRefSamples);
  NEWMAX(m_maxfwdrefs, m_caps.NumForwardRefSamples);
  CLog::Log(LOGDEBUG, "DXVA - %d past frames and %d future frames required by fallback processor %s", m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples, GUIDToString(m_defaultdevice).c_str());
  m_size = 3 + m_maxbackrefs + m_maxfwdrefs;

  // Aggregate processor capabilities
  m_processorcaps = 0;
  for (vector<dxva2_processor_t>::iterator it = m_processors.begin(); it != m_processors.end(); it++)
    m_processorcaps |= DXVA_DEINTERLACE_MASK(it->caps);

  m_time = 0;

  return true;
}

bool CProcessor::SaveProcessor(EINTERLACEMETHOD method, GUID* guid, unsigned caps)
{
  // Prevent duplicates, keep the first processor found for a deinterlace method
  for (vector<dxva2_processor_t>::iterator it = m_processors.begin(); it != m_processors.end(); it++)
    if (it->method == method)
      return false;

  dxva2_processor_t processor = { method, *guid, caps };
  m_processors.push_back(processor);
  return true;
}

bool CProcessor::SelectProcessor()
{
  bool inited = true;

  // DXVA processor device not yet initialized
  if (!m_process)
    inited = false;

  // Deinterlace method changed
  if (m_currinterlacemethod != g_settings.m_currentVideoSettings.m_InterlaceMethod)
    inited = false;

  // Sample format changed
  if ((m_currinterlacemethod == VS_INTERLACEMETHOD_AUTO && m_desc.SampleFormat.SampleFormat != m_streamsampleformat)
   || (m_currinterlacemethod != VS_INTERLACEMETHOD_NONE && m_desc.SampleFormat.SampleFormat != m_streamsampleformat && m_streamsampleformat != DXVA2_SampleProgressiveFrame))
    inited = false;

  if (inited)
    return true;

  CSingleLock lock(m_section);
  EINTERLACEMETHOD method = m_currinterlacemethod = g_settings.m_currentVideoSettings.m_InterlaceMethod;

  // Synchronize sample type and render deinterlace method
  switch (m_currinterlacemethod)
  {
    case VS_INTERLACEMETHOD_AUTO:
      m_desc.SampleFormat.SampleFormat = m_streamsampleformat;
      if (m_streamsampleformat == DXVA2_SampleFieldInterleavedOddFirst
       || m_streamsampleformat == DXVA2_SampleFieldInterleavedEvenFirst)
        method = VS_INTERLACEMETHOD_DXVA_ADAPTIVE_HIGH;
      else
        method = VS_INTERLACEMETHOD_NONE;
      break;
    case VS_INTERLACEMETHOD_DXVA_BOB:
    case VS_INTERLACEMETHOD_DXVA_ADAPTIVE_LOW:
    case VS_INTERLACEMETHOD_DXVA_ADAPTIVE_MEDIUM:
    case VS_INTERLACEMETHOD_DXVA_ADAPTIVE_HIGH:
      if (m_streamsampleformat == DXVA2_SampleProgressiveFrame)
        m_desc.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst; // Assume TFF if we don't know better
      else
        m_desc.SampleFormat.SampleFormat = m_streamsampleformat;
      break;
    default:
      m_desc.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
      method = VS_INTERLACEMETHOD_NONE;
      break;
  }

  // Find a processor or a substitute for the selected deinterlace method
  GUID device = GUID_NULL;
  bool deinterlacing = false;
  while (IsEqualGUID(device, GUID_NULL))
  {
    for (vector<dxva2_processor_t>::iterator it = m_processors.begin(); it != m_processors.end(); it++)
    {
      if (it->method == method)
      {
        device = it->guid;
        deinterlacing = DXVA_DEINTERLACE_MASK(it->caps) ? true : false;
        break;
      }
    }

    if (IsEqualGUID(device, GUID_NULL))
    {
      // Find a substitute processor
      EINTERLACEMETHOD substitute = method;
      for (unsigned i = 0; dxva2_deinterlace_subst[i].method != VS_INTERLACEMETHOD_NONE; i++)
      {
        if (dxva2_deinterlace_subst[i].method == method)
        {
          substitute = dxva2_deinterlace_subst[i].substitute;
          break;
        }
      }

      if (substitute != method)
        method = substitute;
      else
        device = m_defaultdevice;
    }
  }

  // (Re)create processor
  m_device = device;
  SAFE_RELEASE(m_process);

  CLog::Log(LOGDEBUG, "DXVA - processor selected %s", GUIDToString(m_device).c_str());
  CHECK(m_service->GetVideoProcessorCaps(m_device, &m_desc, D3DFMT_X8R8G8B8, &m_caps));

  D3DFORMAT output = m_desc.Format;
  if(FAILED(m_service->CreateVideoProcessor(m_device, &m_desc, output, 0, &m_process)))
  {
    CLog::Log(LOGDEBUG, "DXVA - unable to use source format, use RGB output instead\n");
    output = D3DFMT_X8R8G8B8;
    CHECK(m_service->CreateVideoProcessor(m_device, &m_desc, output, 0, &m_process));
  }

  CLog::Log(LOGDEBUG, "DXVA - processor requires %d past frames and %d future frames", m_caps.NumBackwardRefSamples, m_caps.NumForwardRefSamples);
  CLog::Log(LOGDEBUG, "DXVA - processor capabilities: deinterlacing: %s, software device: %s, emulated dxva1: %s",
    deinterlacing ? "yes" : "no", m_caps.DeviceCaps & DXVA2_VPDev_SoftwareDevice ? "yes" : "no", m_caps.DeviceCaps & DXVA2_VPDev_EmulatedDXVA1 ? "yes" : "no");

  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Brightness, &m_brightness));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Contrast  , &m_contrast));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Hue       , &m_hue));
  CHECK(m_service->GetProcAmpRange(m_device, &m_desc, output, DXVA2_ProcAmp_Saturation, &m_saturation));

  // Reset field when switching progressive vs. deinterlacing processors
  if (m_deinterlacing != deinterlacing)
    m_previoustime = 0;

  m_deinterlacing = deinterlacing;

  return true;
}

void CProcessor::HoldSurface(IDirect3DSurface9* surface)
{
  surface->AddRef();
  m_heldsurfaces.push_back(surface);
}

bool CProcessor::CreateSurfaces()
{
  CSingleLock lock(m_section);

  m_surfaces_count = m_size;
  m_surfaces = (LPDIRECT3DSURFACE9*)calloc(m_surfaces_count, sizeof(LPDIRECT3DSURFACE9));
  CHECK(m_service->CreateSurface((m_desc.SampleWidth + 15) & ~15,
                                 (m_desc.SampleHeight + 15) & ~15,
                                  m_surfaces_count - 1,
                                  m_desc.Format,
                                  D3DPOOL_DEFAULT,
                                  0,
                                  DXVA2_VideoSoftwareRenderTarget,
                                  m_surfaces,
                                  NULL));

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
    m_sample.back().End = vs.Start;

  m_sample.push_back(vs);
  if(m_sample.size() > m_size)
  {
    SAFE_RELEASE(m_sample.front().SrcSurface);
    m_sample.pop_front();
  }

  return m_time;
}

bool CProcessor::ProcessPicture(DVDVideoPicture* picture)
{
  CSingleLock lock(m_section);

  if (picture->iFlags & DVP_FLAG_INTERLACED)
  {
    if (picture->iFlags & DVP_FLAG_TOP_FIELD_FIRST)
      m_streamsampleformat = DXVA2_SampleFieldInterleavedEvenFirst;
    else
      m_streamsampleformat = DXVA2_SampleFieldInterleavedOddFirst;
  }
  else
    m_streamsampleformat = DXVA2_SampleProgressiveFrame;

  IDirect3DSurface9* surface = NULL;

  switch (picture->format)
  {
    case DVDVideoPicture::FMT_DXVA:
    {
      surface = (IDirect3DSurface9*)picture->data[3];
      break;
    }

    case DVDVideoPicture::FMT_YUV420P:
    {
      surface = m_surfaces[m_index];
      m_index = (m_index + 1) % m_size;
  
      D3DLOCKED_RECT rectangle;
      CHECK(surface->LockRect(&rectangle, NULL, 0));

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
      CHECK(surface->GetDesc(&desc));

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
  
      CHECK(surface->UnlockRect());

      picture->proc = this;
      picture->format = DVDVideoPicture::FMT_DXVA;
      break;
    }
    
    default:
    {
      CLog::Log(LOGWARNING, "DXVA - colorspace not supported by processor, skipping frame");
      return false;
    }
  }

  if (!surface)
    return false;

  if (picture->iFlags & DVP_FLAG_DROPPED)
    picture->proc_id = 0;
  else
    picture->proc_id = Add(surface);

  return true;
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

bool CProcessor::Render(const RECT &dst, IDirect3DSurface9* target, REFERENCE_TIME time)
{
  CSingleLock lock(m_section);

  /* (re)create processor device */
  if (!SelectProcessor())
    return false;

  /* add a delay given number of forward references */
  REFERENCE_TIME requested_time = time;
  if (time > m_caps.NumForwardRefSamples * 2)
    time -= m_caps.NumForwardRefSamples * 2;
  else
    time = 0;

  if(m_sample.empty())
    return false;

  /* find oldest needed frame for current processor */
  SSamples::iterator it = m_sample.begin();
  for(; it != m_sample.end(); it++)
  {
    if(it->Start + m_caps.NumBackwardRefSamples * 2 >= time)
      break;
  }

  if(it == m_sample.end())
  {
    CLog::Log(LOGERROR, "DXVA - failed to find image, all images newer or no images");
    return false;
  }

  D3DSURFACE_DESC desc;
  CHECK(target->GetDesc(&desc));

  int count = 1 + m_caps.NumBackwardRefSamples + m_caps.NumForwardRefSamples;
  int valid = 0;

  auto_aptr<DXVA2_VideoSample> samp(new DXVA2_VideoSample[count]);
  for(; it != m_sample.end() && valid < count && it->Start <= time; it++, valid++)
  {
    DXVA2_VideoSample& vs = samp[valid];
    vs = *it;
    vs.DstRect = dst;
    if(vs.End == 0)
      vs.End = vs.Start + 2;
    CWinRenderer::CropSource(vs.SrcRect, vs.DstRect, desc);
  }

  if(time >= samp[valid-1].End)
  {
    CLog::Log(LOGWARNING, "CProcessor::Render - requested time %l64d is after last sample %l64d", time, samp[valid-1].End);
    time = samp[valid-1].Start;
  }
  
  if(time < samp[0].Start)
  {
    CLog::Log(LOGWARNING, "CProcessor::Render - requested time %l64d is before first sample %l64d", time, samp[0].Start);
    time = samp[0].Start;
  }  

  DXVA2_VideoProcessBltParams blt = {};
  blt.TargetFrame = time;
  blt.TargetRect  = samp[0].DstRect;
  blt.ConstrictionSize.cx = 0;
  blt.ConstrictionSize.cy = 0;

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

  // Render the in-between temporal frame for subsequent calls within the same timestamp when deinterlacing
  if (m_deinterlacing && (m_previoustime == time))
    blt.TargetFrame++;
  m_previoustime = time;

  /* HACK to kickstart certain DXVA drivers (poulsbo) which oddly  *
   * won't render anything until someting else have been rendered. */
  g_Windowing.Get3DDevice()->SetFVF( D3DFVF_XYZ );
  float verts[2][3]= {};
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 1, verts, 3*sizeof(float));

  CHECK(m_process->VideoProcessBlt(target, &blt, samp.get(), valid, NULL));

  /* add a delay given number of maximum forward references */
  if (requested_time > m_maxfwdrefs * 2)
    requested_time -= m_maxfwdrefs * 2;
  else
    requested_time = 0;

  /* find oldest needed frame for all processors */
  it = m_sample.begin();
  for(; it != m_sample.end(); it++)
  {
    if(it->Start + m_maxbackrefs * 2 >= requested_time)
      break;
  }

  /* erase anything older than this */
  if(it != m_sample.end())
  {
    for(SSamples::iterator it2 = m_sample.begin(); it2 != it; it2++)
      SAFE_RELEASE(it2->SrcSurface);
    m_sample.erase(m_sample.begin(), it);
  }

  return true;
}

#endif
