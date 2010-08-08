/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DS_PLAYER

#include "XBMCVideoDecFilter.h"
#include <math.h>
#include "DShowUtil/dshowutil.h"
#include "Cspinfo.h"
#include <mmreg.h>
#include "evr.h"
#include "mfidl.h"
#include "PODtypes.h"
#include <initguid.h>
#include "utils/SystemInfo.h"

#include "VideoDecOutputPin.h"
#include "utils/CPUInfo.h"

#include "DShowUtil/DShowUtil.h"
#include "DShowUtil/MediaTypes.h"

#include <moreuuids.h>
#include "DIRECTSHOW.h"



#include "DSCodecDef.h"

const int CXBMCVideoDecFilter::sudPinTypesInCount = countof(CXBMCVideoDecFilter::sudPinTypesIn);

UINT       CXBMCVideoDecFilter::FFmpegFilters = 0xFFFFFFFF;
UINT       CXBMCVideoDecFilter::DXVAFilters = 0xFFFFFFFF;
bool     CXBMCVideoDecFilter::m_ref_frame_count_check_skip = false;

const AMOVIESETUP_MEDIATYPE CXBMCVideoDecFilter::sudPinTypesOut[] =
{
  {&MEDIATYPE_Video, &MEDIASUBTYPE_NV12},
  {&MEDIATYPE_Video, &MEDIASUBTYPE_NV24}
};
const int CXBMCVideoDecFilter::sudPinTypesOutCount = countof(CXBMCVideoDecFilter::sudPinTypesOut);


BOOL CALLBACK EnumFindProcessWnd (HWND hwnd, LPARAM lParam)
{
  DWORD  procid = 0;
  TCHAR  WindowClass [40];
  GetWindowThreadProcessId (hwnd, &procid);
  GetClassName (hwnd, WindowClass, countof(WindowClass));
  
  if (procid == GetCurrentProcessId() && _tcscmp (WindowClass, _T("XBMC")) == 0)
  {
    HWND*    pWnd = (HWND*) lParam;
    *pWnd = hwnd;
    return FALSE;
  }
  return TRUE;
}

CXBMCVideoDecFilter::CXBMCVideoDecFilter(LPUNKNOWN lpunk, HRESULT* phr) 
  : CBaseVideoFilter(NAME("XBMC Internal Decoder"), lpunk, phr, __uuidof(this))
{
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load())
  {
    *phr = E_FAIL;
    return;
  }
  HWND    hWnd = NULL;
  for (int i=0; i<countof(ffCodecs); i++)
  {
    if(ffCodecs[i].nFFCodec == CODEC_ID_H264)
    {
      if(g_sysinfo.IsVistaOrHigher())
      {
        ffCodecs[i].DXVAModes = &DXVA_H264_VISTA;
      }
    }
  }

  if(phr) 
    *phr = S_OK;

  if (m_pOutput)
    delete m_pOutput;
  if(!(m_pOutput = new CVideoDecOutputPin(NAME("CVideoDecOutputPin"), this, phr, L"Output"))) 
    *phr = E_OUTOFMEMORY;

  m_pDXVADecoder       = NULL;
  m_pAVCodecID         = CODEC_ID_NONE;
  m_pCodecContext      = NULL;
  m_pFrame             = NULL;
  m_pConvertFrame      = NULL;
  m_nCodecNb           = -1;
  m_rtAvrTimePerFrame  = 0;
  m_bReorderBFrame     = true;
  m_DXVADecoderGUID    = GUID_NULL;
  m_nActiveCodecs      = 16383;
  m_rtLastStart        = 0;
  m_nCountEstimated    = 0;
  m_nOutCsp            = 0;

  m_nWorkaroundBug     = FF_BUG_AUTODETECT;
  m_nErrorConcealment  = FF_EC_DEBLOCK | FF_EC_GUESS_MVS;

  m_nDiscardMode       = AVDISCARD_DEFAULT;
  m_nErrorRecognition  = FF_ER_CAREFUL;
  m_nIDCTAlgo          = FF_IDCT_AUTO;
  m_bDXVACompatible    = true;
  m_pFFBuffer          = NULL;
  m_nFFBufferSize      = 0;
  ResetBuffer();
  m_nWidth             = 0;
  m_nHeight            = 0;
  m_pSwsContext        = NULL;
  m_bUseDXVA           = true;
  m_bUseFFmpeg         = true;

  m_nDXVAMode          = MODE_SOFTWARE;
  m_pDXVADecoder       = NULL;
  m_pVideoOutputFormat = NULL;
  m_nVideoOutputCount  = 0;
  m_hDevice            = INVALID_HANDLE_VALUE;

  m_nARMode            = 1;
  m_nDXVACheckCompatibility  = 0;
  m_nPosB              = 1;
  m_sar.SetSize(1,1);
  
  
  m_nDiscardMode = AVDISCARD_DEFAULT;
  m_nErrorRecognition = FF_ER_CAREFUL;
  m_nIDCTAlgo = FF_IDCT_AUTO;
  m_nActiveCodecs = 16383;
  m_nARMode = 0;
  m_nDXVACheckCompatibility = 3;
  if(m_nDXVACheckCompatibility>3) 
    m_nDXVACheckCompatibility = 0;

//TODO Add interupt cb
  EnumWindows(EnumFindProcessWnd, (LPARAM)&hWnd);
  DetectVideoCard(hWnd);

#ifdef _DEBUG
  // Check codec definition table
  int    nCodecs    = countof(ffCodecs);
  int    nPinTypes = countof(sudPinTypesIn);
  ASSERT (nCodecs == nPinTypes);
  for (int i=0; i<nPinTypes; i++)
    ASSERT (ffCodecs[i].clsMinorType == sudPinTypesIn[i].clsMinorType);
#endif
}

UINT CXBMCVideoDecFilter::GetAdapter(IDirect3D9* pD3D, HWND hWnd)
{
  if(hWnd == NULL || pD3D == NULL)
    return D3DADAPTER_DEFAULT;

  HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
  if(hMonitor == NULL) return D3DADAPTER_DEFAULT;

  for(UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp)
  {
    HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
    if(hAdpMon == hMonitor) return adp;
  }

  return D3DADAPTER_DEFAULT;
}

void CXBMCVideoDecFilter::DetectVideoCard(HWND hWnd)
{
  IDirect3D9* pD3D9;
  m_nPCIVendor = 0;
  m_nPCIDevice = 0;
  m_VideoDriverVersion.HighPart = 0;
  m_VideoDriverVersion.LowPart = 0;

  if (pD3D9 = Direct3DCreate9(D3D_SDK_VERSION)) 
  {
    D3DADAPTER_IDENTIFIER9 adapterIdentifier;
    if (pD3D9->GetAdapterIdentifier(GetAdapter(pD3D9, hWnd), 0, &adapterIdentifier) == S_OK) 
    {
      m_nPCIVendor = adapterIdentifier.VendorId;
      m_nPCIDevice = adapterIdentifier.DeviceId;
      m_VideoDriverVersion = adapterIdentifier.DriverVersion;
      m_strDeviceDescription = adapterIdentifier.Description;
      m_strDeviceDescription.AppendFormat (_T(" (%d)"), m_nPCIVendor);
    }
    pD3D9->Release();
  }
}


CXBMCVideoDecFilter::~CXBMCVideoDecFilter()
{
  Cleanup();
}

inline int LNKO(int a, int b)
{
  if(a == 0 || b == 0)
    return(1);
  while(a != b)
  {
    if(a < b) b -= a;
    else if(a > b) a -= b;
  }
  return(a);
}

bool CXBMCVideoDecFilter::IsVideoInterlaced()
{
  // NOT A BUG : always tell DirectShow it's interlaced (progressive flags set in 
  // SetTypeSpecificFlags function)
  return true;
};

void CXBMCVideoDecFilter::UpdateFrameTime (REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
  if (rtStart == _I64_MIN)
  {
    // If reference time has not been set by splitter, extrapolate start time
    // from last known start time already delivered
    rtStart = m_rtLastStart + m_rtAvrTimePerFrame*m_nCountEstimated;
    m_nCountEstimated++;
  }
  else
  {
    // Known start time, set as new reference
    m_rtLastStart    = rtStart;
    m_nCountEstimated  = 1;
  }

  rtStop  = rtStart + m_rtAvrTimePerFrame;
}


void CXBMCVideoDecFilter::GetOutputSize(int& w, int& h, int& arx, int& ary, int &RealWidth, int &RealHeight)
{
#if 1
  RealWidth = m_nWidth;
  RealHeight = m_nHeight;
  w = PictWidthRounded();
  h = PictHeightRounded();
#else
  if (m_nDXVAMode == MODE_SOFTWARE)
  {
    w = m_nWidth;
    h = m_nHeight;
  }
  else
  {
    // DXVA surface are multiple of 16 pixels!
    w = PictWidthRounded();
    h = PictHeightRounded();
  }
#endif
}

void CXBMCVideoDecFilter::Cleanup()
{
  m_nOutCsp = 0;
  SAFE_DELETE(m_pDXVADecoder);
  
  if (m_pFrame) m_dllAvUtil.av_free(m_pFrame);
  m_pFrame = NULL;

  if (m_pConvertFrame)
  {
    m_dllAvCodec.avpicture_free(m_pConvertFrame);
    m_dllAvUtil.av_free(m_pConvertFrame);
  }
  m_pConvertFrame = NULL;

  if (m_pFFBuffer)
    free(m_pFFBuffer);

  if (m_pCodecContext)
  {
    if (m_pCodecContext->codec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    if (m_pCodecContext->extradata)
    {
      m_dllAvUtil.av_free(m_pCodecContext->extradata);
      m_pCodecContext->extradata = NULL;
      m_pCodecContext->extradata_size = 0;
    }
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }
  SAFE_DELETE(m_pDXVADecoder);

  m_pCodecContext    = NULL;
  m_pFrame    = NULL;
  m_pFFBuffer    = NULL;
  m_nFFBufferSize  = 0;
  m_nFFBufferPos  = 0;
  m_nFFPicEnd    = INT_MIN;
  m_nCodecNb    = -1;
  SAFE_DELETE_ARRAY (m_pVideoOutputFormat);

  // Release DXVA ressources
  if (m_hDevice != INVALID_HANDLE_VALUE)
  {
    m_pDeviceManager->CloseDeviceHandle(m_hDevice);
    m_hDevice = INVALID_HANDLE_VALUE;
  }

  m_pDeviceManager    = NULL;
  m_pDecoderService    = NULL;
  m_pDecoderRenderTarget  = NULL;
  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
  m_dllSwScale.Unload();
}

STDMETHODIMP CXBMCVideoDecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  return
     __super::NonDelegatingQueryInterface(riid, ppv);
}




HRESULT CXBMCVideoDecFilter::CheckInputType(const CMediaType* mtIn)
{
  for (int i=0; i<sizeof(sudPinTypesIn)/sizeof(AMOVIESETUP_MEDIATYPE); i++)
  {
    if ((mtIn->majortype == *sudPinTypesIn[i].clsMajorType) && 
      (mtIn->subtype == *sudPinTypesIn[i].clsMinorType))
      return S_OK;
  }

  return VFW_E_TYPE_NOT_ACCEPTED;
}


bool CXBMCVideoDecFilter::IsMultiThreadSupported(int nCodec)
{
  return (nCodec==CODEC_ID_H264);
}

enum PixelFormat CXBMCVideoDecFilter::GetFormat( struct AVCodecContext * avctx, const PixelFormat * fmt )
{
  CXBMCVideoDecFilter* ctx  = (CXBMCVideoDecFilter*)avctx->opaque;

  if(!ctx->IsDXVASupported())
    return ctx->m_dllAvCodec.avcodec_default_get_format(avctx, fmt);

  const PixelFormat * cur = fmt;
  while(*cur != PIX_FMT_NONE)
  {
    
    if(ctx->GetDXVADecoder()->Supports(*cur))
    {
      if(ctx->GetDXVADecoder()->Open(avctx, *cur))
      {
        return *cur;
      }
      else
      {
        CLog::Log(LOGERROR,"Failed to open dxva context");
      }
    }
    cur++;
  }
  return ctx->m_dllAvCodec.avcodec_default_get_format(avctx, fmt);

}

HRESULT CXBMCVideoDecFilter::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
  AVCodec* pCodec;

  if (direction != PINDIR_INPUT)
    return __super::SetMediaType(direction,pmt);
  
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load())
    return E_FAIL;

  m_dllAvCodec.avcodec_register_all();
  
  CDSStreamInfo *hint = new CDSStreamInfo(*pmt);
  
  
  m_bUseFFmpeg = true;
  m_pAVCodecID = hint->codec_id;

  if (m_pAVCodecID == -1) 
    return VFW_E_TYPE_NOT_ACCEPTED;
  else if(m_pAVCodecID == CODEC_ID_H264)
    m_bUseDXVA = true;
  else
    m_bUseDXVA = false;

  m_pCodecContext  = m_dllAvCodec.avcodec_alloc_context();
  
  m_bReorderBFrame  = true;
  
  CheckPointerDbg(m_pCodecContext, E_POINTER, "failed to avcodec_alloc_context");


  if ( (hint->codec_tag == MAKEFOURCC('a','v','c','1')) || (hint->codec_tag == MAKEFOURCC('A','V','C','1')))
    m_bReorderBFrame = false;
  for (int i=0; i<countof(ffCodecs); i++)
  {
    if (pmt->subtype == *ffCodecs[i].clsMinorType)
    {
      m_nCodecNb = i;
      break;
    }
  }

  pCodec = m_dllAvCodec.avcodec_find_decoder(m_pAVCodecID);//ffCodecs[nNewCodec].nFFCodec);
  CheckPointerDbg(pCodec, VFW_E_UNSUPPORTED_VIDEO, "Codec not supported with the internal decoder");

  m_pCodecContext->opaque = (void*) this;
  m_pCodecContext->debug_mv        = 0;
  m_pCodecContext->debug           = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->get_format = GetFormat;
      
  if (m_pAVCodecID != CODEC_ID_H264 && pCodec->capabilities & CODEC_CAP_DR1)
    m_pCodecContext->flags |= CODEC_FLAG_EMU_EDGE;

  if (hint->profile != 0)
    m_pCodecContext->profile = hint->profile;
  if (hint->level != 0)
    m_pCodecContext->level = hint->level;
  m_nWidth = hint->width;
  m_nHeight = hint->height;
  m_pCodecContext->coded_width = hint->width;
  m_pCodecContext->coded_height = hint->height;
  m_pCodecContext->intra_matrix      = (uint16_t*)calloc(sizeof(uint16_t),64);
  m_pCodecContext->inter_matrix      = (uint16_t*)calloc(sizeof(uint16_t),64);
  /* The codectag is currently used in mpegvideo.c at MPV_common_init*/
  m_pCodecContext->codec_tag = hint->codec_tag;//ffCodecs[nNewCodec].fourcc;

  //m_pCodecContext->idct_algo        = m_nIDCTAlgo;
  //m_pCodecContext->skip_loop_filter    = (AVDiscard)m_nDiscardMode;
  m_pCodecContext->dsp_mask = FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;
      
  if (hint->extrasize > 0)
  {
    m_pCodecContext->extradata_size = hint->extrasize;
    m_pCodecContext->extradata = (uint8_t*)m_dllAvUtil.av_mallocz(hint->extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hint->extradata, hint->extrasize);
  }
  m_rtAvrTimePerFrame = std::max (1i64, hint->avgtimeperframe);

  int num_threads = std::min(8 /*MAX_THREADS*/, g_cpuInfo.getCPUCount());
  if( num_threads > 1 && IsDXVASupported() // thumbnail extraction fails when run threaded
  && ( pCodec->id != CODEC_ID_H264 || pCodec->id == CODEC_ID_MPEG4 ))
    m_dllAvCodec.avcodec_thread_init(m_pCodecContext, num_threads);
  else
    m_dllAvCodec.avcodec_thread_init(m_pCodecContext, 1);
      
  if (m_dllAvCodec.avcodec_open(m_pCodecContext, pCodec)<0)
  {
    CLog::Log(LOGERROR,"%s failed to open codec",__FUNCTION__);
    return VFW_E_INVALIDMEDIATYPE;
  }
      
  m_pFrame = m_dllAvCodec.avcodec_alloc_frame();
  CheckPointer (m_pFrame, E_POINTER);

      
  BuildDXVAOutputFormat();
  return __super::SetMediaType(direction, pmt);
}


VIDEO_OUTPUT_FORMATS DXVAFormats[] =
{
  {&MEDIASUBTYPE_NV12, 1, 12, 'avxd'},  // DXVA2
  {&MEDIASUBTYPE_NV12, 1, 12, 'AVXD'},
  {&MEDIASUBTYPE_NV12, 1, 12, 'AVxD'},
  {&MEDIASUBTYPE_NV12, 1, 12, 'AvXD'}
};

VIDEO_OUTPUT_FORMATS SoftwareFormats[] =
{
  {&MEDIASUBTYPE_YV12, 3, 12, '21VY'},
  {&MEDIASUBTYPE_YUY2, 1, 16, '2YUY'},  // Software
  {&MEDIASUBTYPE_I420, 3, 12, '024I'},
  {&MEDIASUBTYPE_IYUV, 3, 12, 'VUYI'}
};


bool CXBMCVideoDecFilter::IsDXVASupported()
{
  if (m_nCodecNb != -1) {
    // Does the codec suppport DXVA ?
    if (ffCodecs[m_nCodecNb].DXVAModes != NULL) {
      // Enabled by user ?
      if (m_bUseDXVA) {
        // is the file compatible ?
         if (m_bDXVACompatible) {
          return true;
        }
      }
    }
  }
  return false;
}


void CXBMCVideoDecFilter::BuildDXVAOutputFormat()
{
  int      nPos = 0;

  delete[] (m_pVideoOutputFormat);   
  (m_pVideoOutputFormat)=NULL;  
//SAFE_DELETE_ARRAY (m_pVideoOutputFormat);

  m_nVideoOutputCount = (IsDXVASupported() ? ffCodecs[m_nCodecNb].DXVAModeCount() + countof (DXVAFormats) : 0) +
              (m_bUseFFmpeg   ? countof(SoftwareFormats) : 0);

  m_pVideoOutputFormat  = DNew VIDEO_OUTPUT_FORMATS[m_nVideoOutputCount];

  if (IsDXVASupported())
  {
    // Dynamic DXVA media types for DXVA1
    for (nPos=0; nPos<ffCodecs[m_nCodecNb].DXVAModeCount(); nPos++)
    {
      m_pVideoOutputFormat[nPos].subtype      = ffCodecs[m_nCodecNb].DXVAModes->Decoder[nPos];
      m_pVideoOutputFormat[nPos].biCompression  = 'avxd';
      m_pVideoOutputFormat[nPos].biBitCount    = 12;
      m_pVideoOutputFormat[nPos].biPlanes      = 1;
    }

    // Static list for DXVA2
    memcpy (&m_pVideoOutputFormat[nPos], DXVAFormats, sizeof(DXVAFormats));
    nPos += countof (DXVAFormats);
  }

  // Software rendering
  if (m_bUseFFmpeg)
    memcpy (&m_pVideoOutputFormat[nPos], SoftwareFormats, sizeof(SoftwareFormats));
}


int CXBMCVideoDecFilter::GetPicEntryNumber()
{
  if (IsDXVASupported())
    return ffCodecs[m_nCodecNb].DXVAModes->PicEntryNumber;
  else
    return 0;
}


void CXBMCVideoDecFilter::GetOutputFormats (int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats)
{
  nNumber    = m_nVideoOutputCount;
  *ppFormats  = m_pVideoOutputFormat;
}

HRESULT CXBMCVideoDecFilter::CompleteConnect(PIN_DIRECTION direction, IPin* pReceivePin)
{
  //LOG(_T("CXBMCVideoDecFilter::CompleteConnect"));

  if (direction==PINDIR_INPUT && m_pOutput->IsConnected())
  {
    ReconnectOutput (m_nWidth, m_nHeight);
  }
  else if (direction==PINDIR_OUTPUT)     
  {
    if (IsDXVASupported())
    {
      if (m_nDXVAMode == MODE_DXVA1)
        m_pDXVADecoder->ConfigureDXVA1();
      else if (SUCCEEDED (ConfigureDXVA2 (pReceivePin)) &&  SUCCEEDED (SetEVRForDXVA2 (pReceivePin)) )
        m_nDXVAMode  = MODE_DXVA2;
    }
    if (m_nDXVAMode == MODE_SOFTWARE)
    {
      if (m_pCodecContext->codec_id == CODEC_ID_VC1)
	    {
		    //VC1Context* vc1 = (VC1Context*) m_pCodecContext->priv_data;
        //if (!vc1->interlace)
        //{
           //return VFW_E_INVALIDMEDIATYPE;
        //}
	}
      
      
     
    }

    CLSID  ClsidSourceFilter = DShowUtil::GetCLSID(m_pInput->GetConnected());
    //if((ClsidSourceFilter == __uuidof(CMpegSourceFilter)) || (ClsidSourceFilter == __uuidof(CMpegSplitterFilter)))
    //  m_bReorderBFrame = false;
  }

  // Cannot use YUY2 if horizontal or vertical resolution is not even
  if ( ((m_pOutput->CurrentMediaType().subtype == MEDIASUBTYPE_NV12) && (m_nDXVAMode == MODE_SOFTWARE)) ||
     ((m_pOutput->CurrentMediaType().subtype == MEDIASUBTYPE_YUY2) && (m_pCodecContext->width&1 || m_pCodecContext->height&1)) )
    return VFW_E_INVALIDMEDIATYPE;

  return __super::CompleteConnect (direction, pReceivePin);
}


HRESULT CXBMCVideoDecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
  if (UseDXVA2())
  {
    HRESULT          hr;
    ALLOCATOR_PROPERTIES  Actual;

    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

    pProperties->cBuffers = GetPicEntryNumber();

    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
      return hr;

    return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
      ? E_FAIL
      : NOERROR;
  }
  else
    return __super::DecideBufferSize (pAllocator, pProperties);
}


HRESULT CXBMCVideoDecFilter::NewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate)
{
  CAutoLock cAutoLock(&m_csReceive);
  m_nPosB = 1;
  memset (&m_BFrames, 0, sizeof(m_BFrames));
  m_rtLastStart    = 0;
  m_nCountEstimated  = 0;

  ResetBuffer();

  if (m_pCodecContext)
    m_dllAvCodec.avcodec_flush_buffers (m_pCodecContext);

  if (m_pDXVADecoder)
    m_pDXVADecoder->Flush();
  return __super::NewSegment (rtStart, rtStop, dRate);
}


HRESULT CXBMCVideoDecFilter::BreakConnect(PIN_DIRECTION dir)
{
  if (dir == PINDIR_INPUT)
  {
    Cleanup();
  }

  return __super::BreakConnect (dir);
}

void CXBMCVideoDecFilter::SetTypeSpecificFlags(IMediaSample* pMS)
{
  if(Com::SmartQIPtr<IMediaSample2> pMS2 = pMS)
  {
    AM_SAMPLE2_PROPERTIES props;
    if(SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props)))
    {
      props.dwTypeSpecificFlags &= ~0x7f;

      if(!m_pFrame->interlaced_frame)
        props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_WEAVE;
      else
      {
        if(m_pFrame->top_field_first)
          props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_FIELD1FIRST;
      }

      switch (m_pFrame->pict_type)
      {
      case FF_I_TYPE :
      case FF_SI_TYPE :
        props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_I_SAMPLE;
        break;
      case FF_P_TYPE :
      case FF_SP_TYPE :
        props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_P_SAMPLE;
        break;
      default :
        props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_B_SAMPLE;
        break;
      }

      pMS2->SetProperties(sizeof(props), (BYTE*)&props);
    }
  }
}

#if 1 //INCLUDE_MPC_VIDEO_DECODER
int CXBMCVideoDecFilter::GetCspFromMediaType(GUID& subtype)
{
  if (subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV || subtype == MEDIASUBTYPE_YV12)
    return FF_CSP_420P|FF_CSP_FLAGS_YUV_ADJ;
  else if (subtype == MEDIASUBTYPE_YUY2)
    return FF_CSP_YUY2;
//  else if (subtype == MEDIASUBTYPE_ARGB32 || subtype == MEDIASUBTYPE_RGB32 || subtype == MEDIASUBTYPE_RGB24 || subtype == MEDIASUBTYPE_RGB565)

  ASSERT (FALSE);
  return FF_CSP_NULL;
}


void CXBMCVideoDecFilter::InitSwscale()
{
  if (m_pSwsContext == NULL)
  {
    TYCbCr2RGB_coeffs  coeffs(ffYCbCr_RGB_coeff_ITUR_BT601,0, 235, 16, 255.0, 0.0);
    int32_t        swscaleTable[7];
    //SwsParams      params;

    //memset(&params,0,sizeof(params));
    //if (m_pCodecContext->dsp_mask & CCpuId::MPC_MM_MMX)  params.cpu |= SWS_CPU_CAPS_MMX|SWS_CPU_CAPS_MMX2;
    //if (m_pCodecContext->dsp_mask & CCpuId::MPC_MM_3DNOW)  params.cpu |= SWS_CPU_CAPS_3DNOW;

    //params.methodLuma.method=params.methodChroma.method=SWS_POINT;

    swscaleTable[0] = int32_t(coeffs.vr_mul * 65536 + 0.5);
    swscaleTable[1] = int32_t(coeffs.ub_mul * 65536 + 0.5);
    swscaleTable[2] = int32_t(coeffs.ug_mul * 65536 + 0.5);
    swscaleTable[3] = int32_t(coeffs.vg_mul * 65536 + 0.5);
    swscaleTable[4] = int32_t(coeffs.y_mul  * 65536 + 0.5);
    swscaleTable[5] = int32_t(coeffs.Ysub * 65536);
    swscaleTable[6] = coeffs.RGB_add1;

    BITMAPINFOHEADER bihOut;
    DShowUtil::ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

    m_nOutCsp    = GetCspFromMediaType(m_pOutput->CurrentMediaType().subtype);
    //Dvdplayer use SWS_FAST_BILINEAR
    struct SwsContext *context = m_dllSwScale.sws_getContext(m_pCodecContext->width, m_pCodecContext->height,
                                         m_pCodecContext->pix_fmt, m_pCodecContext->width, m_pCodecContext->height,
                                         PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    m_pSwsContext = context;
  //sws_getContext(m_pCodecContext->width, m_pCodecContext->height,(PixelFormat)csp_ffdshow2mplayer(csp_lavc2ffdshow(m_pCodecContext->pix_fmt)),
  //                            m_pCodecContext->width,m_pCodecContext->height, (PixelFormat)csp_ffdshow2mplayer(m_nOutCsp), SWS_POINT,
  //                            NULL, NULL, NULL);*/

    m_pOutSize.cx  = bihOut.biWidth;
    m_pOutSize.cy  = abs(bihOut.biHeight);
  }
}

template<class T> inline T odd2even(T x)
{
    return x&1 ?
        x + 1 :
        x;
}
#endif /* INCLUDE_MPC_VIDEO_DECODER */



bool CXBMCVideoDecFilter::FindPicture(int nIndex, int nStartCode)
{
  DWORD    dw      = 0;

  for (int i=0; i<m_nFFBufferPos-nIndex; i++)
  {
    dw = (dw<<8) + m_pFFBuffer[i+nIndex];
    if (i >= 4)
    {
      if (m_nFFPicEnd == INT_MIN)
      {
        if ( (dw & 0xffffff00) == 0x00000100 &&
           (dw & 0x000000FF) == nStartCode )
        {
          m_nFFPicEnd = i+nIndex-3;
        }
      }
      else
      {
        if ( (dw & 0xffffff00) == 0x00000100 &&
           ( (dw & 0x000000FF) == nStartCode ||  (dw & 0x000000FF) == 0xB3 ))
        {
          m_nFFPicEnd = i+nIndex-3;
          return true;
        }
      }
    }
    
  }

  return false;
}


void CXBMCVideoDecFilter::ResetBuffer()
{
  m_nFFBufferPos    = 0;
  m_nFFPicEnd      = INT_MIN;

  for (int i=0; i<MAX_BUFF_TIME; i++)
  {
    m_FFBufferTime[i].nBuffPos  = INT_MIN;
    m_FFBufferTime[i].rtStart  = _I64_MIN;
    m_FFBufferTime[i].rtStop  = _I64_MIN;
  }
}

void CXBMCVideoDecFilter::PushBufferTime(int nPos, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
  for (int i=0; i<MAX_BUFF_TIME; i++)
  {
    if (m_FFBufferTime[i].nBuffPos == INT_MIN)
    {
      m_FFBufferTime[i].nBuffPos  = nPos;
      m_FFBufferTime[i].rtStart  = rtStart;
      m_FFBufferTime[i].rtStop  = rtStop;
      break;
    }
  }
}

void CXBMCVideoDecFilter::PopBufferTime(int nPos)
{
  int    nDestPos = 0;
  int    i     = 0;

  // Shift buffer time list
  while (i<MAX_BUFF_TIME && m_FFBufferTime[i].nBuffPos!=INT_MIN)
  {
    if (m_FFBufferTime[i].nBuffPos >= nPos)
    {
      m_FFBufferTime[nDestPos].nBuffPos  = m_FFBufferTime[i].nBuffPos - nPos;
      m_FFBufferTime[nDestPos].rtStart  = m_FFBufferTime[i].rtStart;
      m_FFBufferTime[nDestPos].rtStop    = m_FFBufferTime[i].rtStop;
      nDestPos++;
    }
    i++;
  }

  // Free unused slots
  for (i=nDestPos; i<MAX_BUFF_TIME; i++)
  {
    m_FFBufferTime[i].nBuffPos  = INT_MIN;
    m_FFBufferTime[i].rtStart  = _I64_MIN;
    m_FFBufferTime[i].rtStop  = _I64_MIN;
  }
}

bool CXBMCVideoDecFilter::AppendBuffer (BYTE* pDataIn, int nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
  if (rtStart != _I64_MIN)
    PushBufferTime (m_nFFBufferPos, rtStart, rtStop);

  if (m_nFFBufferPos+nSize+FF_INPUT_BUFFER_PADDING_SIZE > m_nFFBufferSize)
  {
    m_nFFBufferSize = m_nFFBufferPos+nSize+FF_INPUT_BUFFER_PADDING_SIZE;
    m_pFFBuffer    = (BYTE*)realloc(m_pFFBuffer, m_nFFBufferSize);
  }

  memcpy(m_pFFBuffer+m_nFFBufferPos, pDataIn, nSize);

  m_nFFBufferPos += nSize;

  return true;
}

void CXBMCVideoDecFilter::ShrinkBuffer()
{
  int      nRemaining = m_nFFBufferPos-m_nFFPicEnd;

  ASSERT (m_nFFPicEnd != INT_MIN);

  PopBufferTime (m_nFFPicEnd);
  memcpy (m_pFFBuffer, m_pFFBuffer+m_nFFPicEnd, nRemaining);
  m_nFFBufferPos  = nRemaining;

  m_nFFPicEnd = (m_pFFBuffer[3] == 0x00) ?  0 : INT_MIN;
}

union pts_union
{
  double  pts_d;
  int64_t pts_i;
};

static int64_t pts_dtoi(double pts)
{
  pts_union u;
  u.pts_d = pts;
  return u.pts_i;
}

static double pts_itod(int64_t pts)
{
  pts_union u;
  u.pts_i = pts;
  return u.pts_d;
}

HRESULT CXBMCVideoDecFilter::Transform(IMediaSample* pIn)
{
  CAutoLock cAutoLock(&m_csReceive);
  HRESULT      hr;
  BYTE*      pData;
  int        nSize;
  REFERENCE_TIME  rtStart = _I64_MIN;
  REFERENCE_TIME  rtStop  = _I64_MIN;

  if(FAILED(hr = pIn->GetPointer(&pData)))
    return hr;

  nSize    = pIn->GetActualDataLength();
  pIn->GetTime(&rtStart, &rtStop);
  //double startpts;
  //startpts = pts_dtoi(((double)(rtStart/10)));
  if (rtStop <= rtStart && rtStop != _I64_MIN)
    rtStop = rtStart + (m_rtAvrTimePerFrame);
  

  m_pCodecContext->reordered_opaque  = rtStart;

  int        got_picture;
  int        used_bytes;

  
  if (nSize+FF_INPUT_BUFFER_PADDING_SIZE > m_nFFBufferSize)
  {
    m_nFFBufferSize = nSize+FF_INPUT_BUFFER_PADDING_SIZE;
    m_pFFBuffer    = (BYTE*)realloc(m_pFFBuffer, m_nFFBufferSize);
  }

  // Required number of additionally allocated bytes at the end of the input bitstream for decoding.
  // This is mainly needed because some optimized bitstream readers read
  // 32 or 64 bit at once and could read over the end.<br>
  // Note: If the first 23 bits of the additional bytes are not 0, then damaged
  // MPEG bitstreams could cause overread and segfault.
  
  memcpy(m_pFFBuffer, pData, nSize);
  memset(m_pFFBuffer+nSize,0,FF_INPUT_BUFFER_PADDING_SIZE);
  
  

  
  used_bytes = m_dllAvCodec.avcodec_decode_video(m_pCodecContext, m_pFrame, &got_picture, m_pFFBuffer, nSize);
  
  if (used_bytes < 0)
  {
    CLog::Log(LOGERROR, "%s - avcodec_decode_video returned failure", __FUNCTION__);
    return S_OK;
  }

  if (m_pCodecContext->has_b_frames)
  {
    m_BFrames[m_nPosB].rtStart  = rtStart;
    m_BFrames[m_nPosB].rtStop  = rtStop;
    m_nPosB      = 1 - m_nPosB;
  }
  
  
  if (used_bytes != nSize && !m_pCodecContext->hurry_up)
    CLog::DebugLog("%s - avcodec_decode_video didn't consume the full packet. size: %d, consumed: %d", __FUNCTION__, nSize, used_bytes);

  if (!got_picture || !m_pFrame->data[0]) 
    return S_OK;
  if (pIn->IsPreroll() == S_OK || rtStart < 0)
    return S_OK;

  Com::SmartPtr<IMediaSample>  pOut;

  if (m_nDXVAMode == MODE_SOFTWARE)
  {
    BYTE*          pDataOut = NULL;

    UpdateAspectRatio();
    if(FAILED(hr = GetDeliveryBuffer(m_pCodecContext->width, m_pCodecContext->height, &pOut)) || FAILED(hr = pOut->GetPointer(&pDataOut)))
      return hr;
    
    /*this is only used if it has a start*/
    if (m_pFrame->reordered_opaque > 0)
    {
      rtStart = m_pFrame->reordered_opaque;
      rtStop  = m_pFrame->reordered_opaque + m_rtAvrTimePerFrame;
    }
    ReorderBFrames(rtStart, rtStop);

    pOut->SetTime(&rtStart, &rtStop);
    pOut->SetMediaTime(NULL, NULL);

 /* this is the original from mpc-hc */    
    if (m_pSwsContext == NULL) 
      InitSwscale();
    
    // TODO : quick and dirty patch to fix convertion to YUY2 with swscale
    /*FF_CSP_YUY2 is PIX_FMT_YUYV422 */
    if (m_nOutCsp == FF_CSP_YUY2)
      CopyBuffer(pDataOut, m_pFrame->data, m_pCodecContext->width, m_pCodecContext->height, m_pFrame->linesize[0], MEDIASUBTYPE_I420, false);

    else if (m_pSwsContext != NULL)
    {
      uint8_t*  dst[4];
      stride_t  srcStride[4];
      stride_t  dstStride[4];

      const TcspInfo *outcspInfo=csp_getInfo(m_nOutCsp);
      for (int i=0;i<4;i++)
      {
        srcStride[i]=(stride_t)m_pFrame->linesize[i];
        dstStride[i]=m_pOutSize.cx>>outcspInfo->shiftX[i];
        if (i==0)
          dst[i]=pDataOut;
        else
          dst[i]=dst[i-1]+dstStride[i-1]*(m_pOutSize.cy>>outcspInfo->shiftY[i-1]);
      }

      int nTempCsp = m_nOutCsp;
      if(outcspInfo->id==FF_CSP_420P)
        csp_yuv_adj_to_plane(nTempCsp,outcspInfo,odd2even(m_pOutSize.cy),(unsigned char**)dst,dstStride);
      else
        csp_yuv_adj_to_plane(nTempCsp,outcspInfo,m_pCodecContext->height,(unsigned char**)dst,dstStride);

      
      m_dllSwScale.sws_scale(m_pSwsContext, m_pFrame->data, srcStride, 0, m_pCodecContext->height, dst, dstStride);
    }

    SetTypeSpecificFlags (pOut);
    hr = m_pOutput->Deliver(pOut);
    return hr;
  }
  else if (m_nDXVAMode == MODE_DXVA1)
  {
    //nSize  -= used_bytes;
    //pData += used_bytes;
    if (m_pDXVADecoder)//  || m_nDXVAMode == MODE_DXVA2 )
    {
      int result = -1;
      //CheckPointer (m_pDXVADecoder, E_UNEXPECTED);
      UpdateAspectRatio();

      // Change aspect ratio for DXVA1
      if ((m_nDXVAMode == MODE_DXVA1) && ReconnectOutput(PictWidthRounded(), PictHeightRounded(), true, PictWidth(), PictHeight()) == S_OK)
      {
        m_pDXVADecoder->ConfigureDXVA1();
        CLog::DebugLog("CDXVADecoder->ConfigureDXVA1");
      }

      if (m_pDXVADecoder->GetDeliveryBuffer(rtStart, rtStop,&pOut))
      {
        //This seem to be work but need to verify if the issyncpoint can help to set the fieldtype
        int field_type,slice_type;
        slice_type = m_pFrame->pict_type;
        /*Based on what happen in h264.c at switch (h->sei_pic_struct)*/
        if (m_pFrame->interlaced_frame == 0 && m_pFrame->repeat_pict == 0)
          field_type = PICT_TOP_FIELD;
        else if(m_pFrame->interlaced_frame == 0 && m_pFrame->repeat_pict == 1)
          field_type = PICT_BOTTOM_FIELD;
        else 
          field_type = PICT_FRAME;
        
        m_pDXVADecoder->SetTypeSpecificFlags(pOut, (FF_SLICE_TYPE)slice_type, (FF_FIELD_TYPE)field_type);
        //TODO add a structure with index, time start and stop to select the right one to display depending on the current time
        hr = m_pDXVADecoder->GetIAMVideoAccelerator()->DisplayFrame(m_pDXVADecoder->GetCurrentBufferIndex(),pOut);

      }
      
      //result = m_pDXVADecoder->Decode(m_pCodecContext, m_pFrame);
    }
  }
  
  

  return hr;
}


void CXBMCVideoDecFilter::UpdateAspectRatio()
{
  if(((m_nARMode) && (m_pCodecContext)) && ((m_pCodecContext->sample_aspect_ratio.num>0) && (m_pCodecContext->sample_aspect_ratio.den>0)))
  { 
    Com::SmartSize SAR(m_pCodecContext->sample_aspect_ratio.num, m_pCodecContext->sample_aspect_ratio.den); 
    if(m_sar != SAR) 
    { 
      m_sar = SAR; 
      Com::SmartSize aspect(m_nWidth * SAR.cx, m_nHeight * SAR.cy); 
      int lnko = LNKO(aspect.cx, aspect.cy); 
      if(lnko > 1) aspect.cx /= lnko, aspect.cy /= lnko; 
      SetAspect(aspect); 
    } 
  }
}

void CXBMCVideoDecFilter::ReorderBFrames(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
  // Re-order B-frames if needed
  if (m_pCodecContext->has_b_frames && m_bReorderBFrame)
  {
    rtStart  = m_BFrames [m_nPosB].rtStart;
    rtStop  = m_BFrames [m_nPosB].rtStop;
  }
}

void CXBMCVideoDecFilter::FillInVideoDescription(DXVA2_VideoDesc *pDesc)
{
  memset (pDesc, 0, sizeof(DXVA2_VideoDesc));
  pDesc->SampleWidth      = PictWidthRounded();
  pDesc->SampleHeight      = PictHeightRounded();
  pDesc->Format        = D3DFMT_A8R8G8B8;
  pDesc->UABProtectionLevel  = 1;
}

BOOL CXBMCVideoDecFilter::IsSupportedDecoderMode(const GUID& mode)
{
  if (IsDXVASupported())
  {
    for (int i=0; i<MAX_SUPPORTED_MODE; i++)
    {
      if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == GUID_NULL) 
        break;
      else if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == mode)
        return true;
    }
  }

  return false;
}

BOOL CXBMCVideoDecFilter::IsSupportedDecoderConfig(const D3DFORMAT nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered)
{
  bool  bRet = false;

  bRet = (nD3DFormat == MAKEFOURCC('N', 'V', '1', '2'));

  bIsPrefered = (config.ConfigBitstreamRaw == ffCodecs[m_nCodecNb].DXVAModes->PreferedConfigBitstream);
  //LOG (_T("IsSupportedDecoderConfig  0x%08x  %d"), nD3DFormat, bRet);
  return bRet;
}

HRESULT CXBMCVideoDecFilter::FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
                           const GUID& guidDecoder, 
                           DXVA2_ConfigPictureDecode *pSelectedConfig,
                           BOOL *pbFoundDXVA2Configuration)
{
    HRESULT hr = S_OK;
    UINT cFormats = 0;
    UINT cConfigurations = 0;
  bool bIsPrefered = false;

    D3DFORMAT                   *pFormats = NULL;           // size = cFormats
    DXVA2_ConfigPictureDecode   *pConfig = NULL;            // size = cConfigurations

    // Find the valid render target formats for this decoder GUID.
    hr = pDecoderService->GetDecoderRenderTargets(guidDecoder, &cFormats, &pFormats);
  //LOG (_T("GetDecoderRenderTargets => %d"), cFormats);

    if (SUCCEEDED(hr))
    {
        // Look for a format that matches our output format.
        for (UINT iFormat = 0; iFormat < cFormats;  iFormat++)
        {
      //LOG (_T("Try to negociate => 0x%08x"), pFormats[iFormat]);

            // Fill in the video description. Set the width, height, format, and frame rate.
            FillInVideoDescription(&m_VideoDesc); // Private helper function.
            m_VideoDesc.Format = pFormats[iFormat];

            // Get the available configurations.
            hr = pDecoderService->GetDecoderConfigurations(guidDecoder, &m_VideoDesc, NULL, &cConfigurations, &pConfig);

            if (FAILED(hr))
            {
                continue;
            }

            // Find a supported configuration.
            for (UINT iConfig = 0; iConfig < cConfigurations; iConfig++)
            {
                if (IsSupportedDecoderConfig(pFormats[iFormat], pConfig[iConfig], bIsPrefered))
                {
                    // This configuration is good.
          if (bIsPrefered || !*pbFoundDXVA2Configuration)
          {
            *pbFoundDXVA2Configuration = TRUE;
            *pSelectedConfig = pConfig[iConfig];
          }
                    
          if (bIsPrefered) break;
                }
            }

            CoTaskMemFree(pConfig);
        } // End of formats loop.
    }

    CoTaskMemFree(pFormats);

    // Note: It is possible to return S_OK without finding a configuration.
    return hr;
}


HRESULT CXBMCVideoDecFilter::ConfigureDXVA2(IPin *pPin)
{
  HRESULT hr             = S_OK;
  UINT  cDecoderGuids     = 0;
  BOOL  bFoundDXVA2Configuration = FALSE;
  GUID  guidDecoder     = GUID_NULL;

  DXVA2_ConfigPictureDecode config;
  ZeroMemory(&config, sizeof(config));

  Com::SmartPtr<IMFGetService>      pGetService;
  Com::SmartPtr<IDirect3DDeviceManager9>  pDeviceManager;
  Com::SmartPtr<IDirectXVideoDecoderService>  pDecoderService;
  GUID*          pDecoderGuids = NULL;
  HANDLE          hDevice = INVALID_HANDLE_VALUE;

  // Query the pin for IMFGetService.
  hr = pPin->QueryInterface(__uuidof(IMFGetService), (void**)&pGetService);

  // Get the Direct3D device manager.
  if (SUCCEEDED(hr))
    hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, __uuidof(IDirect3DDeviceManager9), (void**)&pDeviceManager);

  // Open a new device handle.
  if (SUCCEEDED(hr))
    hr = pDeviceManager->OpenDeviceHandle(&hDevice);

  // Get the video decoder service.
  if (SUCCEEDED(hr))
    hr = pDeviceManager->GetVideoService(hDevice, __uuidof(IDirectXVideoDecoderService), (void**)&pDecoderService);

  // Get the decoder GUIDs.
  if (SUCCEEDED(hr))
  {
    hr = pDecoderService->GetDecoderDeviceGuids(&cDecoderGuids, &pDecoderGuids);
  }

  if (SUCCEEDED(hr))
  {
    // Look for the decoder GUIDs we want.
    for (UINT iGuid = 0; iGuid < cDecoderGuids; iGuid++)
    {
      // Do we support this mode?
      if (!IsSupportedDecoderMode(pDecoderGuids[iGuid]))
        continue;

      // Find a configuration that we support. 
      hr = FindDXVA2DecoderConfiguration(pDecoderService, pDecoderGuids[iGuid], &config, &bFoundDXVA2Configuration);

      if (FAILED(hr))
        break;

      if (bFoundDXVA2Configuration)
        // Found a good configuration. Save the GUID.
        guidDecoder = pDecoderGuids[iGuid];
    }
  }

  if (pDecoderGuids) 
    CoTaskMemFree(pDecoderGuids);

  if (!bFoundDXVA2Configuration)
    hr = E_FAIL; // Unable to find a configuration.

  if (SUCCEEDED(hr))
  {
    // Store the things we will need later.
    m_pDeviceManager  = pDeviceManager;
    m_pDecoderService  = pDecoderService;
    m_DXVA2Config  = config;
    m_DXVADecoderGUID  = guidDecoder;
    m_hDevice    = hDevice;
  }

  if (FAILED(hr))
  {
    if (hDevice != INVALID_HANDLE_VALUE)
      pDeviceManager->CloseDeviceHandle(hDevice);
  }

  return hr;
}


HRESULT CXBMCVideoDecFilter::SetEVRForDXVA2(IPin *pPin)
{
  HRESULT hr = S_OK;
  Com::SmartPtr<IMFGetService>      pGetService;
  Com::SmartPtr<IDirectXVideoMemoryConfiguration>  pVideoConfig;
  Com::SmartPtr<IMFVideoDisplayControl>    pVdc;

  // Query the pin for IMFGetService.
  hr = pPin->QueryInterface(__uuidof(IMFGetService), (void**)&pGetService);

  // Get the IDirectXVideoMemoryConfiguration interface.
  if (SUCCEEDED(hr))
  {
    hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, __uuidof(IDirectXVideoMemoryConfiguration), (void**)&pVideoConfig);

    if (SUCCEEDED (pGetService->GetService(MR_VIDEO_RENDER_SERVICE, __uuidof(IMFVideoDisplayControl), (void**)&pVdc)))
    {
      HWND  hWnd;
      if (SUCCEEDED (pVdc->GetVideoWindow(&hWnd)))
        DetectVideoCard(hWnd);
    }
  }

  // Notify the EVR. 
  if (SUCCEEDED(hr))
  {
    DXVA2_SurfaceType surfaceType;
    for (DWORD iTypeIndex = 0; ; iTypeIndex++)
    {
      hr = pVideoConfig->GetAvailableSurfaceTypeByIndex(iTypeIndex, &surfaceType);
      if (FAILED(hr))
        break;

      if (surfaceType == DXVA2_SurfaceType_DecoderRenderTarget)
      {
        hr = pVideoConfig->SetSurfaceType(DXVA2_SurfaceType_DecoderRenderTarget);
        break;
      }
    }
  }

  return hr;
}





HRESULT CXBMCVideoDecFilter::FindDXVA1DecoderConfiguration(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DDPIXELFORMAT* pPixelFormat)
{
  HRESULT      hr        = E_FAIL;
  DWORD      dwFormats    = 0;
  DDPIXELFORMAT*  pPixelFormats  = NULL;


  pAMVideoAccelerator->GetUncompFormatsSupported (guidDecoder, &dwFormats, NULL);
  if (dwFormats > 0)
  {
      // Find the valid render target formats for this decoder GUID.
    pPixelFormats = DNew DDPIXELFORMAT[dwFormats];
    hr = pAMVideoAccelerator->GetUncompFormatsSupported (guidDecoder, &dwFormats, pPixelFormats);
    if (SUCCEEDED(hr))
    {
      // Look for a format that matches our output format.
      for (DWORD iFormat = 0; iFormat < dwFormats; iFormat++)
      {
        if (pPixelFormats[iFormat].dwFourCC == MAKEFOURCC ('N', 'V', '1', '2'))
        {
          memcpy (pPixelFormat, &pPixelFormats[iFormat], sizeof(DDPIXELFORMAT));
          SAFE_DELETE_ARRAY(pPixelFormats)
          return S_OK;
        }
      }

      SAFE_DELETE_ARRAY(pPixelFormats);
      hr = E_FAIL;
    }
  }

  return hr;
}

HRESULT CXBMCVideoDecFilter::CheckDXVA1Decoder(const GUID *pGuid)
{
  if (m_nCodecNb != -1)
  {
    for (int i=0; i<MAX_SUPPORTED_MODE; i++)
      if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == *pGuid)
        return S_OK;
  }

  return E_INVALIDARG;
}

void CXBMCVideoDecFilter::SetDXVA1Params(const GUID* pGuid, DDPIXELFORMAT* pPixelFormat)
{
  m_DXVADecoderGUID    = *pGuid;
  memcpy (&m_PixelFormat, pPixelFormat, sizeof (DDPIXELFORMAT));
}

WORD CXBMCVideoDecFilter::GetDXVA1RestrictedMode()
{
  if (m_nCodecNb != -1)
  {
    for (int i=0; i<MAX_SUPPORTED_MODE; i++)
      if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == m_DXVADecoderGUID)
        return ffCodecs[m_nCodecNb].DXVAModes->RestrictedMode [i];
  }

  return DXVA_RESTRICTED_MODE_UNRESTRICTED;
}

HRESULT CXBMCVideoDecFilter::CreateDXVA1Decoder(IAMVideoAccelerator*  pAMVideoAccelerator, const GUID* pDecoderGuid, DWORD dwSurfaceCount)
{
  if (m_pDXVADecoder && m_DXVADecoderGUID  == *pDecoderGuid) 
    return S_OK;
  SAFE_DELETE (m_pDXVADecoder);

  if (!m_bUseDXVA) 
    return E_FAIL;

  m_nDXVAMode      = MODE_DXVA1;
  m_DXVADecoderGUID  = *pDecoderGuid;
  m_pDXVADecoder		= CDXVADecoder::CreateDecoder (this, pAMVideoAccelerator, &m_DXVADecoderGUID, dwSurfaceCount);
  
  //m_pDXVADecoder->Open(m_pCodecContext,PIX_FMT_DXVA2_VLD);
  
  {
    CLog::Log(LOGINFO,"YEAH GOT IT");
  }//m_pDXVADecoder->SetExtraData ((BYTE*)m_pCodecContext->extradata, m_pCodecContext->extradata_size);

  return S_OK;
}

HRESULT CXBMCVideoDecFilter::CreateDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets)
{
  HRESULT              hr;
  Com::SmartPtr<IDirectXVideoDecoder>  pDirectXVideoDec;
  
  m_pDecoderRenderTarget  = NULL;

  if (m_pDXVADecoder) 
    m_pDXVADecoder->SetDirectXVideoDec (NULL);

  hr = m_pDecoderService->CreateVideoDecoder (m_DXVADecoderGUID, &m_VideoDesc, &m_DXVA2Config, 
                pDecoderRenderTargets, nNumRenderTargets, &pDirectXVideoDec);

  if (SUCCEEDED (hr))
  {
    if (!m_pDXVADecoder)
    {
		m_pDXVADecoder  = CDXVADecoder::CreateDecoder(this, pDirectXVideoDec, &m_DXVADecoderGUID, GetPicEntryNumber(), &m_DXVA2Config);
      if (m_pDXVADecoder) 
      {
        CLog::Log(LOGINFO,"YEAH GOT IT");
      }//m_pDXVADecoder->SetExtraData ((BYTE*)m_pCodecContext->extradata, m_pCodecContext->extradata_size);
    }

    m_pDXVADecoder->SetDirectXVideoDec (pDirectXVideoDec);
  }

  return hr;
}

#endif