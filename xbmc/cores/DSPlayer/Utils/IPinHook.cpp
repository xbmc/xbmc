/*
 * $Id: IPinHook.cpp 1221 2009-08-12 19:26:38Z casimir666 $
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2007 see AUTHORS
 * (C) 2010 Team XBMC
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAS_DS_PLAYER

#include <d3dx9.h>
#include <dxva.h>
#include <dxva2api.h>
#include <moreuuids.h>

#include "IPinHook.h"
#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"
#define LOG_FILE        _T("dxva.log")
#include "StreamsManager.h"

//#define LOG_BITSTREAM
//#define LOG_MATRIX

REFERENCE_TIME    g_tSegmentStart = 0;
REFERENCE_TIME    g_tSampleStart = 0;
REFERENCE_TIME    g_rtTimePerFrame = 0;
GUID              g_guidDXVADecoder = GUID_NULL;
int               g_nDXVAVersion = 0;

IPinCVtbl*      g_pPinCVtbl = NULL;
IMemInputPinCVtbl*  g_pMemInputPinCVtbl = NULL;

static bool      bFirst = true;


typedef struct
{
  const int        Format;
  const LPCTSTR      Description;
} D3DFORMAT_TYPE;

const D3DFORMAT_TYPE  D3DFormatType[] =
{
  { D3DFMT_UNKNOWN, _T("D3DFMT_UNKNOWN      ") },
  { D3DFMT_R8G8B8, _T("D3DFMT_R8G8B8       ") },
  { D3DFMT_A8R8G8B8, _T("D3DFMT_A8R8G8B8     ") },
  { D3DFMT_X8R8G8B8, _T("D3DFMT_X8R8G8B8     ") },
  { D3DFMT_R5G6B5, _T("D3DFMT_R5G6B5       ") },
  { D3DFMT_X1R5G5B5, _T("D3DFMT_X1R5G5B5     ") },
  { D3DFMT_A1R5G5B5, _T("D3DFMT_A1R5G5B5     ") },
  { D3DFMT_A4R4G4B4, _T("D3DFMT_A4R4G4B4     ") },
  { D3DFMT_R3G3B2, _T("D3DFMT_R3G3B2       ") },
  { D3DFMT_A8, _T("D3DFMT_A8           ") },
  { D3DFMT_A8R3G3B2, _T("D3DFMT_A8R3G3B2     ") },
  { D3DFMT_X4R4G4B4, _T("D3DFMT_X4R4G4B4     ") },
  { D3DFMT_A2B10G10R10, _T("D3DFMT_A2B10G10R10  ") },
  { D3DFMT_A8B8G8R8, _T("D3DFMT_A8B8G8R8     ") },
  { D3DFMT_X8B8G8R8, _T("D3DFMT_X8B8G8R8     ") },
  { D3DFMT_G16R16, _T("D3DFMT_G16R16       ") },
  { D3DFMT_A2R10G10B10, _T("D3DFMT_A2R10G10B10  ") },
  { D3DFMT_A16B16G16R16, _T("D3DFMT_A16B16G16R16 ") },
  { D3DFMT_A8P8, _T("D3DFMT_A8P8         ") },
  { D3DFMT_P8, _T("D3DFMT_P8           ") },
  { D3DFMT_L8, _T("D3DFMT_L8           ") },
  { D3DFMT_A8L8, _T("D3DFMT_A8L8         ") },
  { D3DFMT_A4L4, _T("D3DFMT_A4L4         ") },
  { D3DFMT_X8L8V8U8, _T("D3DFMT_X8L8V8U8     ") },
  { D3DFMT_Q8W8V8U8, _T("D3DFMT_Q8W8V8U8     ") },
  { D3DFMT_V16U16, _T("D3DFMT_V16U16       ") },
  { D3DFMT_A2W10V10U10, _T("D3DFMT_A2W10V10U10  ") },
  { D3DFMT_UYVY, _T("D3DFMT_UYVY         ") },
  { D3DFMT_R8G8_B8G8, _T("D3DFMT_R8G8_B8G8    ") },
  { D3DFMT_YUY2, _T("D3DFMT_YUY2         ") },
  { D3DFMT_G8R8_G8B8, _T("D3DFMT_G8R8_G8B8    ") },
  { D3DFMT_DXT1, _T("D3DFMT_DXT1         ") },
  { D3DFMT_DXT2, _T("D3DFMT_DXT2         ") },
  { D3DFMT_DXT3, _T("D3DFMT_DXT3         ") },
  { D3DFMT_DXT4, _T("D3DFMT_DXT4         ") },
  { D3DFMT_DXT5, _T("D3DFMT_DXT5         ") },
  { D3DFMT_D16_LOCKABLE, _T("D3DFMT_D16_LOCKABLE ") },
  { D3DFMT_D32, _T("D3DFMT_D32          ") },
  { D3DFMT_D15S1, _T("D3DFMT_D15S1        ") },
  { D3DFMT_D24S8, _T("D3DFMT_D24S8        ") },
  { D3DFMT_D24X8, _T("D3DFMT_D24X8        ") },
  { D3DFMT_D24X4S4, _T("D3DFMT_D24X4S4      ") },
  { D3DFMT_D16, _T("D3DFMT_D16          ") },
  { D3DFMT_D32F_LOCKABLE, _T("D3DFMT_D32F_LOCKABLE") },
  { D3DFMT_D24FS8, _T("D3DFMT_D24FS8       ") },
  { D3DFMT_L16, _T("D3DFMT_L16          ") },
  { D3DFMT_VERTEXDATA, _T("D3DFMT_VERTEXDATA   ") },
  { D3DFMT_INDEX16, _T("D3DFMT_INDEX16      ") },
  { D3DFMT_INDEX32, _T("D3DFMT_INDEX32      ") },
  { D3DFMT_Q16W16V16U16, _T("D3DFMT_Q16W16V16U16 ") },

  { MAKEFOURCC('N', 'V', '1', '2'), _T("D3DFMT_NV12") },
  { MAKEFOURCC('N', 'V', '2', '4'), _T("D3DFMT_NV24") },
};

const LPCTSTR    DXVAVersion[] = { _T("DXVA "), _T("DXVA1"), _T("DXVA2") };

LPCTSTR GetDXVADecoderDescription()
{
  return GetDXVAMode(&g_guidDXVADecoder);
}

LPCTSTR GetDXVAVersion()
{
  return DXVAVersion[g_nDXVAVersion];
}

LPCTSTR FindD3DFormat(const D3DFORMAT Format)
{
  for (int i = 0; i < countof(D3DFormatType); i++)
  {
    if (Format == D3DFormatType[i].Format)
      return D3DFormatType[i].Description;
  }

  return D3DFormatType[0].Description;
}

// === DirectShow hooks
static HRESULT(STDMETHODCALLTYPE * NewSegmentOrg)(IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate) = NULL;

static HRESULT STDMETHODCALLTYPE NewSegmentMine(IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate)
{
  g_tSegmentStart = tStart;
  return NewSegmentOrg(This, tStart, tStop, dRate);
}

static HRESULT(STDMETHODCALLTYPE *ReceiveOrg)(IMemInputPinC * This, IMediaSample *pSample) = NULL;

static HRESULT STDMETHODCALLTYPE ReceiveMineI(IMemInputPinC * This, IMediaSample *pSample)
{
  REFERENCE_TIME rtStart, rtStop;
  if (pSample && SUCCEEDED(pSample->GetTime(&rtStart, &rtStop)))
  {
    g_tSampleStart = rtStart;
    if (CStreamsManager::Get() && CStreamsManager::Get()->SubtitleManager)
      CStreamsManager::Get()->SubtitleManager->SetTimePerFrame(rtStop - rtStart);
  }
  return ReceiveOrg(This, pSample);
}

static HRESULT STDMETHODCALLTYPE ReceiveMine(IMemInputPinC * This, IMediaSample *pSample)
{
  // Support ffdshow queueing.
  // To avoid black out on pause, we have to lock g_ffdshowReceive to synchronize with CMainFrame::OnPlayPause.
  return ReceiveMineI(This, pSample);
}


void UnhookNewSegmentAndReceive()
{
  BOOL res;
  DWORD flOldProtect = 0;

  // Casimir666 : unhook previous VTables
  if (g_pPinCVtbl && g_pMemInputPinCVtbl)
  {
    res = VirtualProtect(g_pPinCVtbl, sizeof(IPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
    if (g_pPinCVtbl->NewSegment == NewSegmentMine)
      g_pPinCVtbl->NewSegment = NewSegmentOrg;
    res = VirtualProtect(g_pPinCVtbl, sizeof(IPinCVtbl), flOldProtect, &flOldProtect);

    res = VirtualProtect(g_pMemInputPinCVtbl, sizeof(IMemInputPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
    if (g_pMemInputPinCVtbl->Receive == ReceiveMine)
      g_pMemInputPinCVtbl->Receive = ReceiveOrg;
    res = VirtualProtect(g_pMemInputPinCVtbl, sizeof(IMemInputPinCVtbl), flOldProtect, &flOldProtect);

    g_pPinCVtbl = NULL;
    g_pMemInputPinCVtbl = NULL;
    NewSegmentOrg = NULL;
    ReceiveOrg = NULL;
  }
}

bool HookNewSegmentAndReceive(IPinC* pPinC, IMemInputPinC* pMemInputPinC)
{
  if (!pPinC || !pMemInputPinC || (GetVersion() & 0x80000000))
    return false;

  g_tSegmentStart = 0;
  g_tSampleStart = 0;

  BOOL res;
  DWORD flOldProtect = 0;

  UnhookNewSegmentAndReceive();

  // Casimir666 : change sizeof(IPinC) to sizeof(IPinCVtbl) to fix crash with EVR hack on Vista!
  res = VirtualProtect(pPinC->lpVtbl, sizeof(IPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
  if (NewSegmentOrg == NULL) NewSegmentOrg = pPinC->lpVtbl->NewSegment;
  pPinC->lpVtbl->NewSegment = NewSegmentMine;
  res = VirtualProtect(pPinC->lpVtbl, sizeof(IPinCVtbl), flOldProtect, &flOldProtect);

  // Casimir666 : change sizeof(IMemInputPinC) to sizeof(IMemInputPinCVtbl) to fix crash with EVR hack on Vista!
  res = VirtualProtect(pMemInputPinC->lpVtbl, sizeof(IMemInputPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
  if (ReceiveOrg == NULL) ReceiveOrg = pMemInputPinC->lpVtbl->Receive;
  pMemInputPinC->lpVtbl->Receive = ReceiveMine;
  res = VirtualProtect(pMemInputPinC->lpVtbl, sizeof(IMemInputPinCVtbl), flOldProtect, &flOldProtect);

  g_pPinCVtbl = pPinC->lpVtbl;
  g_pMemInputPinCVtbl = pMemInputPinC->lpVtbl;

  return true;
}


// === DXVA1 hooks

#define MAX_BUFFER_TYPE    15
BYTE*    g_ppBuffer[MAX_BUFFER_TYPE];


static HRESULT(STDMETHODCALLTYPE *GetVideoAcceleratorGUIDsOrg)(IAMVideoAcceleratorC * This,/* [out][in] */ LPDWORD pdwNumGuidsSupported,/* [out][in] */ LPGUID pGuidsSupported) = NULL;
static HRESULT(STDMETHODCALLTYPE *GetUncompFormatsSupportedOrg)(IAMVideoAcceleratorC * This,/* [in] */ const GUID *pGuid,/* [out][in] */ LPDWORD pdwNumFormatsSupported,/* [out][in] */ LPDDPIXELFORMAT pFormatsSupported) = NULL;
static HRESULT(STDMETHODCALLTYPE *GetInternalMemInfoOrg)(IAMVideoAcceleratorC * This,/* [in] */ const GUID *pGuid,/* [in] */ const AMVAUncompDataInfo *pamvaUncompDataInfo,/* [out][in] */ LPAMVAInternalMemInfo pamvaInternalMemInfo) = NULL;
static HRESULT(STDMETHODCALLTYPE *GetCompBufferInfoOrg)(IAMVideoAcceleratorC * This,/* [in] */ const GUID *pGuid,/* [in] */ const AMVAUncompDataInfo *pamvaUncompDataInfo,/* [out][in] */ LPDWORD pdwNumTypesCompBuffers,/* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo) = NULL;
static HRESULT(STDMETHODCALLTYPE *GetInternalCompBufferInfoOrg)(IAMVideoAcceleratorC * This,/* [out][in] */ LPDWORD pdwNumTypesCompBuffers,/* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo) = NULL;
static HRESULT(STDMETHODCALLTYPE *BeginFrameOrg)(IAMVideoAcceleratorC * This,/* [in] */ const AMVABeginFrameInfo *amvaBeginFrameInfo) = NULL;
static HRESULT(STDMETHODCALLTYPE *EndFrameOrg)(IAMVideoAcceleratorC * This,/* [in] */ const AMVAEndFrameInfo *pEndFrameInfo) = NULL;
static HRESULT(STDMETHODCALLTYPE *GetBufferOrg)(IAMVideoAcceleratorC * This,/* [in] */ DWORD dwTypeIndex,/* [in] */ DWORD dwBufferIndex,/* [in] */ BOOL bReadOnly,/* [out] */ LPVOID *ppBuffer,/* [out] */ LONG *lpStride) = NULL;
static HRESULT(STDMETHODCALLTYPE *ReleaseBufferOrg)(IAMVideoAcceleratorC * This,/* [in] */ DWORD dwTypeIndex,/* [in] */ DWORD dwBufferIndex) = NULL;
static HRESULT(STDMETHODCALLTYPE *ExecuteOrg)(IAMVideoAcceleratorC * This,/* [in] */ DWORD dwFunction,/* [in] */ LPVOID lpPrivateInputData,/* [in] */ DWORD cbPrivateInputData,/* [in] */ LPVOID lpPrivateOutputDat,/* [in] */ DWORD cbPrivateOutputData,/* [in] */ DWORD dwNumBuffers,/* [in] */ const AMVABUFFERINFO *pamvaBufferInfo) = NULL;
static HRESULT(STDMETHODCALLTYPE *QueryRenderStatusOrg)(IAMVideoAcceleratorC * This,/* [in] */ DWORD dwTypeIndex,/* [in] */ DWORD dwBufferIndex,/* [in] */ DWORD dwFlags) = NULL;
static HRESULT(STDMETHODCALLTYPE *DisplayFrameOrg)(IAMVideoAcceleratorC * This,/* [in] */ DWORD dwFlipToIndex,/* [in] */ IMediaSample *pMediaSample) = NULL;

static void LOG_TOFILE(LPCTSTR FileName, LPCTSTR fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int    nCount = _vsctprintf(fmt, args) + 1;
  if (TCHAR* buff = new TCHAR[nCount])
  {
    FILE*  f;
    _vstprintf_s(buff, nCount, fmt, args);
    if (_tfopen_s(&f, FileName, _T("at")) == 0)
    {
      fseek(f, 0, 2);
      _ftprintf(f, _T("%s\n"), buff);
      fclose(f);
    }
    delete[] buff;
  }
  va_end(args);
}

#ifdef _DEBUG
static void LOG(LPCTSTR fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  //  int    nCount  = _vsctprintf(fmt, args) + 1;
  TCHAR  buff[3000];
  FILE*  f;
  _vstprintf_s(buff, countof(buff), fmt, args);
  if (_tfopen_s(&f, LOG_FILE, _T("at")) == 0)
  {
    fseek(f, 0, 2);
    _ftprintf(f, _T("%s\n"), buff);
    fclose(f);
  }

  va_end(args);
}
static void LOGPF(LPCTSTR prefix, const DDPIXELFORMAT* p, int n)
{
  for (int i = 0; i < n; i++)
  {
    LOG(_T("%s[%d].dwSize = %d"), prefix, i, p[i].dwSize);
    LOG(_T("%s[%d].dwFlags = %08x"), prefix, i, p[i].dwFlags);
    LOG(_T("%s[%d].dwFourCC = %4.4hs"), prefix, i, &p[i].dwFourCC);
    LOG(_T("%s[%d].dwRGBBitCount = %08x"), prefix, i, &p[i].dwRGBBitCount);
    LOG(_T("%s[%d].dwRBitMask = %08x"), prefix, i, &p[i].dwRBitMask);
    LOG(_T("%s[%d].dwGBitMask = %08x"), prefix, i, &p[i].dwGBitMask);
    LOG(_T("%s[%d].dwBBitMask = %08x"), prefix, i, &p[i].dwBBitMask);
    LOG(_T("%s[%d].dwRGBAlphaBitMask = %08x"), prefix, i, &p[i].dwRGBAlphaBitMask);
  }
}

static void LOGUDI(LPCTSTR prefix, const AMVAUncompDataInfo* p, int n)
{
  for (int i = 0; i < n; i++)
  {
    LOG(_T("%s[%d].dwUncompWidth = %d"), prefix, i, p[i].dwUncompWidth);
    LOG(_T("%s[%d].dwUncompHeight = %d"), prefix, i, p[i].dwUncompHeight);

    CStdString prefix2;
    prefix2.Format(_T("%s[%d]"), prefix, i);
    LOGPF(prefix2, &p[i].ddUncompPixelFormat, 1);
  }
}


static void LogDXVA_PicParams_H264(DXVA_PicParams_H264* pPic)
{
  CStdString    strRes;
  int      i;

  if (bFirst)
  {
    LOG_TOFILE(_T("picture.log"), _T("RefPicFlag,wFrameWidthInMbsMinus1,wFrameHeightInMbsMinus1,CurrPic.Index7Bits,num_ref_frames,wBitFields,bit_depth_luma_minus8,bit_depth_chroma_minus8,Reserved16Bits,StatusReportFeedbackNumber,RFL.Index7Bits[0],") \
      _T("RFL.Index7Bits[1],RFL.Index7Bits[2],RFL.Index7Bits[3],RFL.Index7Bits[4],RFL.Index7Bits[5],") \
      _T("RFL.Index7Bits[6],RFL.Index7Bits[7],RFL.Index7Bits[8],RFL.Index7Bits[9],RFL.Index7Bits[10],") \
      _T("RFL.Index7Bits[11],RFL.Index7Bits[12],RFL.Index7Bits[13],RFL.Index7Bits[14],RFL.Index7Bits[15],") \
      _T("CurrFieldOrderCnt[0], CurrFieldOrderCnt[1],FieldOrderCntList[0][0], FieldOrderCntList[0][1],FieldOrderCntList[1][0], FieldOrderCntList[1][1],FieldOrderCntList[2][0], FieldOrderCntList[2][1],FieldOrderCntList[3][0], FieldOrderCntList[3][1],FieldOrderCntList[4][0], FieldOrderCntList[4][1],FieldOrderCntList[5][0],") \
      _T("FieldOrderCntList[5][1],FieldOrderCntList[6][0], FieldOrderCntList[6][1],FieldOrderCntList[7][0], FieldOrderCntList[7][1],FieldOrderCntList[8][0], FieldOrderCntList[8][1],FieldOrderCntList[9][0], FieldOrderCntList[9][1],FieldOrderCntList[10][0], FieldOrderCntList[10][1],FieldOrderCntList[11][0],")\
      _T("FieldOrderCntList[11][1],FieldOrderCntList[12][0], FieldOrderCntList[12][1],FieldOrderCntList[13][0], FieldOrderCntList[13][1],FieldOrderCntList[14][0], FieldOrderCntList[14][1],FieldOrderCntList[15][0], FieldOrderCntList[15][1],pic_init_qs_minus26,chroma_qp_index_offset,second_chroma_qp_index_offset,")\
      _T("ContinuationFlag,pic_init_qp_minus26,num_ref_idx_l0_active_minus1,num_ref_idx_l1_active_minus1,Reserved8BitsA,FrameNumList[0],FrameNumList[1],FrameNumList[2],FrameNumList[3],FrameNumList[4],FrameNumList[5],FrameNumList[6],FrameNumList[7],FrameNumList[8],FrameNumList[9],FrameNumList[10],FrameNumList[11],")\
      _T("FrameNumList[12],FrameNumList[13],FrameNumList[14],FrameNumList[15],UsedForReferenceFlags,NonExistingFrameFlags,frame_num,log2_max_frame_num_minus4,pic_order_cnt_type,log2_max_pic_order_cnt_lsb_minus4,delta_pic_order_always_zero_flag,direct_8x8_inference_flag,entropy_coding_mode_flag,pic_order_present_flag,")\
      _T("num_slice_groups_minus1,slice_group_map_type,deblocking_filter_control_present_flag,redundant_pic_cnt_present_flag,Reserved8BitsB,slice_group_change_rate_minus1"));

  }
  bFirst = false;

  strRes.AppendFormat(_T("%d,"), pPic->RefPicFlag);
  strRes.AppendFormat(_T("%d,"), pPic->wFrameWidthInMbsMinus1);
  strRes.AppendFormat(_T("%d,"), pPic->wFrameHeightInMbsMinus1);

  //      DXVA_PicEntry_H264  CurrPic)); /* flag is bot field flag */
  //  strRes.AppendFormat(_T("%d,"), pPic->CurrPic.AssociatedFlag);
  //  strRes.AppendFormat(_T("%d,"), pPic->CurrPic.bPicEntry);
  strRes.AppendFormat(_T("%d,"), pPic->CurrPic.Index7Bits);


  strRes.AppendFormat(_T("%d,"), pPic->num_ref_frames);
  strRes.AppendFormat(_T("%d,"), pPic->wBitFields);
  strRes.AppendFormat(_T("%d,"), pPic->bit_depth_luma_minus8);
  strRes.AppendFormat(_T("%d,"), pPic->bit_depth_chroma_minus8);

  strRes.AppendFormat(_T("%d,"), pPic->Reserved16Bits);
  strRes.AppendFormat(_T("%d,"), pPic->StatusReportFeedbackNumber);

  for (i = 0; i < 16; i++)
  {
    //    strRes.AppendFormat(_T("%d,"), pPic->RefFrameList[i].AssociatedFlag);
    //    strRes.AppendFormat(_T("%d,"), pPic->RefFrameList[i].bPicEntry);
    strRes.AppendFormat(_T("%d,"), pPic->RefFrameList[i].Index7Bits);
  }

  strRes.AppendFormat(_T("%d, %d,"), pPic->CurrFieldOrderCnt[0], pPic->CurrFieldOrderCnt[1]);

  for (int i = 0; i < 16; i++)
    strRes.AppendFormat(_T("%d, %d,"), pPic->FieldOrderCntList[i][0], pPic->FieldOrderCntList[i][1]);
  //      strRes.AppendFormat(_T("%d,"), pPic->FieldOrderCntList[16][2]);

  strRes.AppendFormat(_T("%d,"), pPic->pic_init_qs_minus26);
  strRes.AppendFormat(_T("%d,"), pPic->chroma_qp_index_offset);   /* also used for QScb */
  strRes.AppendFormat(_T("%d,"), pPic->second_chroma_qp_index_offset); /* also for QScr */
  strRes.AppendFormat(_T("%d,"), pPic->ContinuationFlag);

  /* remainder for parsing */
  strRes.AppendFormat(_T("%d,"), pPic->pic_init_qp_minus26);
  strRes.AppendFormat(_T("%d,"), pPic->num_ref_idx_l0_active_minus1);
  strRes.AppendFormat(_T("%d,"), pPic->num_ref_idx_l1_active_minus1);
  strRes.AppendFormat(_T("%d,"), pPic->Reserved8BitsA);

  for (int i = 0; i < 16; i++)
    strRes.AppendFormat(_T("%d,"), pPic->FrameNumList[i]);

  //      strRes.AppendFormat(_T("%d,"), pPic->FrameNumList[16]);
  strRes.AppendFormat(_T("%d,"), pPic->UsedForReferenceFlags);
  strRes.AppendFormat(_T("%d,"), pPic->NonExistingFrameFlags);
  strRes.AppendFormat(_T("%d,"), pPic->frame_num);

  strRes.AppendFormat(_T("%d,"), pPic->log2_max_frame_num_minus4);
  strRes.AppendFormat(_T("%d,"), pPic->pic_order_cnt_type);
  strRes.AppendFormat(_T("%d,"), pPic->log2_max_pic_order_cnt_lsb_minus4);
  strRes.AppendFormat(_T("%d,"), pPic->delta_pic_order_always_zero_flag);

  strRes.AppendFormat(_T("%d,"), pPic->direct_8x8_inference_flag);
  strRes.AppendFormat(_T("%d,"), pPic->entropy_coding_mode_flag);
  strRes.AppendFormat(_T("%d,"), pPic->pic_order_present_flag);
  strRes.AppendFormat(_T("%d,"), pPic->num_slice_groups_minus1);

  strRes.AppendFormat(_T("%d,"), pPic->slice_group_map_type);
  strRes.AppendFormat(_T("%d,"), pPic->deblocking_filter_control_present_flag);
  strRes.AppendFormat(_T("%d,"), pPic->redundant_pic_cnt_present_flag);
  strRes.AppendFormat(_T("%d,"), pPic->Reserved8BitsB);

  strRes.AppendFormat(_T("%d,"), pPic->slice_group_change_rate_minus1);

  //for (int i=0; i<810; i++)
  //  strRes.AppendFormat(_T("%d,"), pPic->SliceGroupMap[i]);
  //      strRes.AppendFormat(_T("%d,"), pPic->SliceGroupMap[810]);

  // SABOTAGE !!!
  //for (int i=0; i<16; i++)
  //{
  //  pPic->FieldOrderCntList[i][0] =  pPic->FieldOrderCntList[i][1] = 0;
  //  pPic->RefFrameList[i].AssociatedFlag = 1;
  //  pPic->RefFrameList[i].bPicEntry = 255;
  //  pPic->RefFrameList[i].Index7Bits = 127;
  //}

  // === Dump PicParams!
  //static FILE*  hPict = NULL;
  //if (!hPict) hPict = fopen ("PicParam.bin", "wb");
  //if (hPict)
  //{
  //  fwrite (pPic, sizeof (DXVA_PicParams_H264), 1, hPict);
  //}

  LOG_TOFILE(_T("picture.log"), strRes);
}

static void LogH264SliceShort(DXVA_Slice_H264_Short* pSlice, int nCount)
{
  CStdString    strRes;
  static bool  bFirstSlice = true;

  if (bFirstSlice)
  {
    strRes = _T("nCnt, BSNALunitDataLocation, SliceBytesInBuffer, wBadSliceChopping");
    LOG_TOFILE(_T("sliceshort.log"), strRes);
    strRes = "";
    bFirstSlice = false;
  }

  for (int i = 0; i < nCount; i++)
  {
    strRes.AppendFormat(_T("%d,"), i);
    strRes.AppendFormat(_T("%d,"), pSlice[i].BSNALunitDataLocation);
    strRes.AppendFormat(_T("%d,"), pSlice[i].SliceBytesInBuffer);
    strRes.AppendFormat(_T("%d"), pSlice[i].wBadSliceChopping);

    LOG_TOFILE(_T("sliceshort.log"), strRes);
    strRes = "";
  }
}

static void LogH264SliceLong(DXVA_Slice_H264_Long* pSlice, int nCount)
{
  static bool  bFirstSlice = true;
  CStdString    strRes;

  if (bFirstSlice)
  {
    strRes = _T("nCnt, BSNALunitDataLocation, SliceBytesInBuffer, wBadSliceChopping,") \
      _T("first_mb_in_slice, NumMbsForSlice, BitOffsetToSliceData, slice_type,luma_log2_weight_denom,chroma_log2_weight_denom,") \
      _T("num_ref_idx_l0_active_minus1,num_ref_idx_l1_active_minus1,slice_alpha_c0_offset_div2,slice_beta_offset_div2,") \
      _T("Reserved8Bits,slice_qs_delta,slice_qp_delta,redundant_pic_cnt,direct_spatial_mv_pred_flag,cabac_init_idc,") \
      _T("disable_deblocking_filter_idc,slice_id,");

    for (int i = 0; i < 2; i++)    /* L0 & L1 */
    {
      for (int j = 0; j < 32; j++)
      {
        strRes.AppendFormat(_T("R[%d][%d].AssociatedFlag,"), i, j);
        strRes.AppendFormat(_T("R[%d][%d].bPicEntry,"), i, j);
        strRes.AppendFormat(_T("R[%d][%d].Index7Bits,"), i, j);
      }
    }

    for (int a = 0; a < 2; a++)    /* L0 & L1; Y, Cb, Cr */
    {
      for (int b = 0; b < 32; b++)
      {
        for (int c = 0; c < 3; c++)
        {
          for (int d = 0; d < 2; d++)
          {
            strRes.AppendFormat(_T("W[%d][%d][%d][%d],"), a, b, c, d);
          }
        }
      }
    }


    LOG_TOFILE(_T("slicelong.log"), strRes);
    strRes = "";
  }
  bFirstSlice = false;

  for (int i = 0; i < nCount; i++)
  {
    strRes.AppendFormat(_T("%d,"), i);
    strRes.AppendFormat(_T("%d,"), pSlice[i].BSNALunitDataLocation);
    strRes.AppendFormat(_T("%d,"), pSlice[i].SliceBytesInBuffer);
    strRes.AppendFormat(_T("%d,"), pSlice[i].wBadSliceChopping);

    strRes.AppendFormat(_T("%d,"), pSlice[i].first_mb_in_slice);
    strRes.AppendFormat(_T("%d,"), pSlice[i].NumMbsForSlice);

    strRes.AppendFormat(_T("%d,"), pSlice[i].BitOffsetToSliceData);

    strRes.AppendFormat(_T("%d,"), pSlice[i].slice_type);
    strRes.AppendFormat(_T("%d,"), pSlice[i].luma_log2_weight_denom);
    strRes.AppendFormat(_T("%d,"), pSlice[i].chroma_log2_weight_denom);
    strRes.AppendFormat(_T("%d,"), pSlice[i].num_ref_idx_l0_active_minus1);
    strRes.AppendFormat(_T("%d,"), pSlice[i].num_ref_idx_l1_active_minus1);
    strRes.AppendFormat(_T("%d,"), pSlice[i].slice_alpha_c0_offset_div2);
    strRes.AppendFormat(_T("%d,"), pSlice[i].slice_beta_offset_div2);
    strRes.AppendFormat(_T("%d,"), pSlice[i].Reserved8Bits);

    strRes.AppendFormat(_T("%d,"), pSlice[i].slice_qs_delta);

    strRes.AppendFormat(_T("%d,"), pSlice[i].slice_qp_delta);
    strRes.AppendFormat(_T("%d,"), pSlice[i].redundant_pic_cnt);
    strRes.AppendFormat(_T("%d,"), pSlice[i].direct_spatial_mv_pred_flag);
    strRes.AppendFormat(_T("%d,"), pSlice[i].cabac_init_idc);
    strRes.AppendFormat(_T("%d,"), pSlice[i].disable_deblocking_filter_idc);
    strRes.AppendFormat(_T("%d,"), pSlice[i].slice_id);

    for (int a = 0; a < 2; a++)    /* L0 & L1 */
    {
      for (int b = 0; b < 32; b++)
      {
        strRes.AppendFormat(_T("%d,"), pSlice[i].RefPicList[a][b].AssociatedFlag);
        strRes.AppendFormat(_T("%d,"), pSlice[i].RefPicList[a][b].bPicEntry);
        strRes.AppendFormat(_T("%d,"), pSlice[i].RefPicList[a][b].Index7Bits);
      }
    }

    for (int a = 0; a < 2; a++)    /* L0 & L1; Y, Cb, Cr */
    {
      for (int b = 0; b < 32; b++)
      {
        for (int c = 0; c < 3; c++)
        {
          for (int d = 0; d < 2; d++)
          {
            strRes.AppendFormat(_T("%d,"), pSlice[i].Weights[a][b][c][d]);
          }
        }
      }
    }

    LOG_TOFILE(_T("slicelong.log"), strRes);
    strRes = "";
  }
}

static void LogDXVA_PictureParameters(DXVA_PictureParameters* pPic)
{
  CStdString    strRes;

  if (bFirst)
    LOG_TOFILE(_T("picture.log"), _T("wDecodedPictureIndex,wDeblockedPictureIndex,wForwardRefPictureIndex,wBackwardRefPictureIndex,wPicWidthInMBminus1,wPicHeightInMBminus1,bMacroblockWidthMinus1,bMacroblockHeightMinus1,bBlockWidthMinus1,bBlockHeightMinus1,bBPPminus1,bPicStructure,bSecondField,bPicIntra,bPicBackwardPrediction,bBidirectionalAveragingMode,bMVprecisionAndChromaRelation,bChromaFormat,bPicScanFixed,bPicScanMethod,bPicReadbackRequests,bRcontrol,bPicSpatialResid8,bPicOverflowBlocks,bPicExtrapolation,bPicDeblocked,bPicDeblockConfined,bPic4MVallowed,bPicOBMC,bPicBinPB,bMV_RPS,bReservedBits,wBitstreamFcodes,wBitstreamPCEelements,bBitstreamConcealmentNeed,bBitstreamConcealmentMethod"));
  bFirst = false;

  strRes.Format(_T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"),
    pPic->wDecodedPictureIndex,
    pPic->wDeblockedPictureIndex,
    pPic->wForwardRefPictureIndex,
    pPic->wBackwardRefPictureIndex,
    pPic->wPicWidthInMBminus1,
    pPic->wPicHeightInMBminus1,
    pPic->bMacroblockWidthMinus1,
    pPic->bMacroblockHeightMinus1,
    pPic->bBlockWidthMinus1,
    pPic->bBlockHeightMinus1,
    pPic->bBPPminus1,
    pPic->bPicStructure,
    pPic->bSecondField,
    pPic->bPicIntra,
    pPic->bPicBackwardPrediction);
  strRes.AppendFormat(_T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"),
    pPic->bBidirectionalAveragingMode,
    pPic->bMVprecisionAndChromaRelation,
    pPic->bChromaFormat,
    pPic->bPicScanFixed,
    pPic->bPicScanMethod,
    pPic->bPicReadbackRequests,
    pPic->bRcontrol,
    pPic->bPicSpatialResid8,
    pPic->bPicOverflowBlocks,
    pPic->bPicExtrapolation,
    pPic->bPicDeblocked,
    pPic->bPicDeblockConfined,
    pPic->bPic4MVallowed,
    pPic->bPicOBMC,
    pPic->bPicBinPB,
    pPic->bMV_RPS,
    pPic->bReservedBits,
    pPic->wBitstreamFcodes,
    pPic->wBitstreamPCEelements,
    pPic->bBitstreamConcealmentNeed,
    pPic->bBitstreamConcealmentMethod);

  LOG_TOFILE(_T("picture.log"), strRes);
}
#else
inline static void LOG(...) { }
inline static void LOGPF(LPCTSTR prefix, const DDPIXELFORMAT* p, int n) {}
inline static void LOGUDI(LPCTSTR prefix, const AMVAUncompDataInfo* p, int n) {}
inline static void LogDXVA_PicParams_H264(DXVA_PicParams_H264* pPic) {}
inline static void LogDXVA_PictureParameters(DXVA_PictureParameters* pPic) {}
#endif



static HRESULT STDMETHODCALLTYPE GetVideoAcceleratorGUImine(IAMVideoAcceleratorC * This, LPDWORD pdwNumGuidsSupported, LPGUID pGuidsSupported)
{
  LOG(_T("\nGetVideoAcceleratorGUIDs"));

  if (pdwNumGuidsSupported)
  {
    LOG(_T("[in] *pdwNumGuidsSupported = %d"), *pdwNumGuidsSupported);
  }

  HRESULT hr = GetVideoAcceleratorGUIDsOrg(This, pdwNumGuidsSupported, pGuidsSupported);

  LOG(_T("hr = %08x"), hr);

  if (pdwNumGuidsSupported)
  {
    LOG(_T("[out] *pdwNumGuidsSupported = %d"), *pdwNumGuidsSupported);

    if (pGuidsSupported)
    {
      for (DWORD i = 0; i < *pdwNumGuidsSupported; i++)
      {
        LOG(_T("[out] pGuidsSupported[%d] = %s"), i, StringFromGUID(pGuidsSupported[i]));
      }
    }
  }

  return hr;
}

static HRESULT STDMETHODCALLTYPE GetUncompFormatsSupportedMine(IAMVideoAcceleratorC * This, const GUID *pGuid, LPDWORD pdwNumFormatsSupported, LPDDPIXELFORMAT pFormatsSupported)
{
  LOG(_T("\nGetUncompFormatsSupported"));

  if (pGuid)
  {
    LOG(_T("[in] *pGuid = %s"), StringFromGUID(*pGuid));
  }

  if (pdwNumFormatsSupported)
  {
    LOG(_T("[in] *pdwNumFormatsSupported = %d"), *pdwNumFormatsSupported);
  }

  HRESULT hr = GetUncompFormatsSupportedOrg(This, pGuid, pdwNumFormatsSupported, pFormatsSupported);

  LOG(_T("hr = %08x"), hr);

  if (pdwNumFormatsSupported)
  {
    LOG(_T("[out] *pdwNumFormatsSupported = %d"), *pdwNumFormatsSupported);

    if (pFormatsSupported)
    {
      LOGPF(_T("[out] pFormatsSupported"), pFormatsSupported, *pdwNumFormatsSupported);
    }
  }

  return hr;
}

static HRESULT STDMETHODCALLTYPE GetInternalMemInfoMine(IAMVideoAcceleratorC * This, const GUID *pGuid, const AMVAUncompDataInfo *pamvaUncompDataInfo, LPAMVAInternalMemInfo pamvaInternalMemInfo)
{
  LOG(_T("\nGetInternalMemInfo"));

  HRESULT hr = GetInternalMemInfoOrg(This, pGuid, pamvaUncompDataInfo, pamvaInternalMemInfo);

  LOG(_T("hr = %08x"), hr);

  return hr;
}

static HRESULT STDMETHODCALLTYPE GetCompBufferInfoMine(IAMVideoAcceleratorC * This, const GUID *pGuid, const AMVAUncompDataInfo *pamvaUncompDataInfo, LPDWORD pdwNumTypesCompBuffers, LPAMVACompBufferInfo pamvaCompBufferInfo)
{
  LOG(_T("\nGetCompBufferInfo"));

  if (pGuid)
  {
    g_guidDXVADecoder = *pGuid;
    g_nDXVAVersion = 1;
    bFirst = true;
    LOG(_T("[in] *pGuid = %s"), StringFromGUID(*pGuid));

    if (pdwNumTypesCompBuffers)
    {
      LOG(_T("[in] *pdwNumTypesCompBuffers = %d"), *pdwNumTypesCompBuffers);
    }
  }

  HRESULT hr = GetCompBufferInfoOrg(This, pGuid, pamvaUncompDataInfo, pdwNumTypesCompBuffers, pamvaCompBufferInfo);

  LOG(_T("hr = %08x"), hr);

  //if(pdwNumTypesCompBuffers)
  //{
  //  LOG(_T("[out] *pdwNumTypesCompBuffers = %d"), *pdwNumTypesCompBuffers);

  //  if(pamvaUncompDataInfo)
  //  {
  //    LOGUDI(_T("[out] pamvaUncompDataInfo"), pamvaUncompDataInfo, *pdwNumTypesCompBuffers);
  //  }
  //}

  return hr;
}

static HRESULT STDMETHODCALLTYPE GetInternalCompBufferInfoMine(IAMVideoAcceleratorC * This, LPDWORD pdwNumTypesCompBuffers, LPAMVACompBufferInfo pamvaCompBufferInfo)
{
  LOG(_T("\nGetInternalCompBufferInfo"));

  HRESULT hr = GetInternalCompBufferInfoOrg(This, pdwNumTypesCompBuffers, pamvaCompBufferInfo);

  LOG(_T("hr = %08x"), hr);

  return hr;
}

static HRESULT STDMETHODCALLTYPE BeginFrameMine(IAMVideoAcceleratorC * This, const AMVABeginFrameInfo *amvaBeginFrameInfo)
{
  LOG(_T("\nBeginFrame"));

  if (amvaBeginFrameInfo)
  {
    LOG(_T("[in] amvaBeginFrameInfo->dwDestSurfaceIndex = %08x"), amvaBeginFrameInfo->dwDestSurfaceIndex);
    LOG(_T("[in] amvaBeginFrameInfo->pInputData = %08x"), amvaBeginFrameInfo->pInputData);
    LOG(_T("[in] amvaBeginFrameInfo->dwSizeInputData = %08x"), amvaBeginFrameInfo->dwSizeInputData);
    LOG(_T("[in] amvaBeginFrameInfo->pOutputData = %08x"), amvaBeginFrameInfo->pOutputData);
    LOG(_T("[in] amvaBeginFrameInfo->dwSizeOutputData = %08x"), amvaBeginFrameInfo->dwSizeOutputData);
  }
  if (amvaBeginFrameInfo && (amvaBeginFrameInfo->dwSizeInputData == 4))
  {
    LOG(_T("[in] amvaBeginFrameInfo->pInputData => dwDestSurfaceIndex = %ld "),
      ((DWORD*)amvaBeginFrameInfo->pInputData)[0]);
  }


  HRESULT hr = BeginFrameOrg(This, amvaBeginFrameInfo);

  LOG(_T("hr = %08x"), hr);

  return hr;
}

static HRESULT STDMETHODCALLTYPE EndFrameMine(IAMVideoAcceleratorC * This, const AMVAEndFrameInfo *pEndFrameInfo)
{
  LOG(_T("\nEndFrame"));

  if (pEndFrameInfo)
  {
    LOG(_T("[in] pEndFrameInfo->dwSizeMiscData = %08x"), pEndFrameInfo->dwSizeMiscData);
    LOG(_T("[in] pEndFrameInfo->pMiscData = %08x"), pEndFrameInfo->pMiscData);

    if (pEndFrameInfo->dwSizeMiscData >= 4)
      LOG(_T("[out] pEndFrameInfo->pMiscData = %02x %02x %02x %02x "),
      ((BYTE*)pEndFrameInfo->pMiscData)[0],
      ((BYTE*)pEndFrameInfo->pMiscData)[1],
      ((BYTE*)pEndFrameInfo->pMiscData)[2],
      ((BYTE*)pEndFrameInfo->pMiscData)[3]);

  }

  HRESULT hr = EndFrameOrg(This, pEndFrameInfo);

  LOG(_T("hr = %08x"), hr);

  return hr;
}

static HRESULT STDMETHODCALLTYPE GetBufferMine(IAMVideoAcceleratorC * This, DWORD dwTypeIndex, DWORD dwBufferIndex, BOOL bReadOnly, LPVOID *ppBuffer, LONG *lpStride)
{

  HRESULT hr = GetBufferOrg(This, dwTypeIndex, dwBufferIndex, bReadOnly, ppBuffer, lpStride);

  LOG(_T("\nGetBuffer"));

  LOG(_T("[in] dwTypeIndex = %08x"), dwTypeIndex);
  LOG(_T("[in] dwBufferIndex = %08x"), dwBufferIndex);
  LOG(_T("[in] bReadOnly = %08x"), bReadOnly);
  LOG(_T("[in] ppBuffer = %08x"), ppBuffer);
  LOG(_T("[out] *lpStride = %08x"), *lpStride);
  LOG(_T("hr = %08x"), hr);

  g_ppBuffer[dwTypeIndex] = (BYTE*)*ppBuffer;
  //LOG(_T("[out] *ppBuffer = %02x %02x %02x %02x ..."), ((BYTE*)*ppBuffer)[0], ((BYTE*)*ppBuffer)[1], ((BYTE*)*ppBuffer)[2], ((BYTE*)*ppBuffer)[3]);

  return hr;
}

static HRESULT STDMETHODCALLTYPE ReleaseBufferMine(IAMVideoAcceleratorC * This, DWORD dwTypeIndex, DWORD dwBufferIndex)
{
  LOG(_T("\nReleaseBuffer"));

  LOG(_T("[in] dwTypeIndex = %08x"), dwTypeIndex);
  LOG(_T("[in] dwBufferIndex = %08x"), dwBufferIndex);

  HRESULT hr = ReleaseBufferOrg(This, dwTypeIndex, dwBufferIndex);

  LOG(_T("hr = %08x"), hr);

  return hr;
}

static HRESULT STDMETHODCALLTYPE ExecuteMine(IAMVideoAcceleratorC* This, DWORD dwFunction, LPVOID lpPrivateInputData, DWORD cbPrivateInputData,
  LPVOID lpPrivateOutputData, DWORD cbPrivateOutputData, DWORD dwNumBuffers, const AMVABUFFERINFO *pamvaBufferInfo)
{
#ifdef _DEBUG
  LOG(_T("\nExecute"));
  LOG(_T("[in] dwFunction = %08x"), dwFunction);
  if (lpPrivateInputData)
  {
    if (dwFunction == 0x01000000)
    {
      DXVA_BufferDescription*    pBuffDesc = (DXVA_BufferDescription*)lpPrivateInputData;

      for (DWORD i = 0; i < dwNumBuffers; i++)
      {
        LOG(_T("[in] lpPrivateInputData, buffer description %d"), i);
        LOG(_T("   pBuffDesc->dwTypeIndex      = %d"), pBuffDesc[i].dwTypeIndex);
        LOG(_T("   pBuffDesc->dwBufferIndex    = %d"), pBuffDesc[i].dwBufferIndex);
        LOG(_T("   pBuffDesc->dwDataOffset    = %d"), pBuffDesc[i].dwDataOffset);
        LOG(_T("   pBuffDesc->dwDataSize      = %d"), pBuffDesc[i].dwDataSize);
        LOG(_T("   pBuffDesc->dwFirstMBaddress  = %d"), pBuffDesc[i].dwFirstMBaddress);
        LOG(_T("   pBuffDesc->dwHeight      = %d"), pBuffDesc[i].dwHeight);
        LOG(_T("   pBuffDesc->dwStride      = %d"), pBuffDesc[i].dwStride);
        LOG(_T("   pBuffDesc->dwWidth        = %d"), pBuffDesc[i].dwWidth);
        LOG(_T("   pBuffDesc->dwNumMBsInBuffer  = %d"), pBuffDesc[i].dwNumMBsInBuffer);
        LOG(_T("   pBuffDesc->dwReservedBits    = %d"), pBuffDesc[i].dwReservedBits);
      }
    }
    else if ((dwFunction == 0xfffff101) || (dwFunction == 0xfffff501))
    {
      DXVA_ConfigPictureDecode*    ConfigRequested = (DXVA_ConfigPictureDecode*)lpPrivateInputData;
      LOG(_T("[in] lpPrivateInputData, config requested"));
      LOG(_T("   pBuffDesc->dwTypeIndex      = %d"), ConfigRequested->bConfig4GroupedCoefs);
      LOG(_T("   ConfigRequested->bConfigBitstreamRaw      = %d"), ConfigRequested->bConfigBitstreamRaw);
      LOG(_T("   ConfigRequested->bConfigHostInverseScan    = %d"), ConfigRequested->bConfigHostInverseScan);
      LOG(_T("   ConfigRequested->bConfigIntraResidUnsigned    = %d"), ConfigRequested->bConfigIntraResidUnsigned);
      LOG(_T("   ConfigRequested->bConfigMBcontrolRasterOrder  = %d"), ConfigRequested->bConfigMBcontrolRasterOrder);
      LOG(_T("   ConfigRequested->bConfigResid8Subtraction    = %d"), ConfigRequested->bConfigResid8Subtraction);
      LOG(_T("   ConfigRequested->bConfigResidDiffAccelerator  = %d"), ConfigRequested->bConfigResidDiffAccelerator);
      LOG(_T("   ConfigRequested->bConfigResidDiffHost      = %d"), ConfigRequested->bConfigResidDiffHost);
      LOG(_T("   ConfigRequested->bConfigSpatialHost8or9Clipping= %d"), ConfigRequested->bConfigSpatialHost8or9Clipping);
      LOG(_T("   ConfigRequested->bConfigSpatialResid8      = %d"), ConfigRequested->bConfigSpatialResid8);
      LOG(_T("   ConfigRequested->bConfigSpatialResidInterleaved= %d"), ConfigRequested->bConfigSpatialResidInterleaved);
      LOG(_T("   ConfigRequested->bConfigSpecificIDCT      = %d"), ConfigRequested->bConfigSpecificIDCT);
      LOG(_T("   ConfigRequested->dwFunction          = %d"), ConfigRequested->dwFunction);
      LOG(_T("   ConfigRequested->guidConfigBitstreamEncryption  = %s"), StringFromGUID(ConfigRequested->guidConfigBitstreamEncryption));
      LOG(_T("   ConfigRequested->guidConfigMBcontrolEncryption  = %s"), StringFromGUID(ConfigRequested->guidConfigMBcontrolEncryption));
      LOG(_T("   ConfigRequested->guidConfigResidDiffEncryption  = %s"), StringFromGUID(ConfigRequested->guidConfigResidDiffEncryption));
    }
    else
      LOG(_T("[in] lpPrivateInputData = %02x %02x %02x %02x ..."),
      ((BYTE*)lpPrivateInputData)[0],
      ((BYTE*)lpPrivateInputData)[1],
      ((BYTE*)lpPrivateInputData)[2],
      ((BYTE*)lpPrivateInputData)[3]);
  }
  LOG(_T("[in] cbPrivateInputData = %08x"), cbPrivateInputData);
  LOG(_T("[in] lpPrivateOutputData = %08x"), lpPrivateOutputData);
  LOG(_T("[in] cbPrivateOutputData = %08x"), cbPrivateOutputData);
  LOG(_T("[in] dwNumBuffers = %08x"), dwNumBuffers);
  if (pamvaBufferInfo)
  {
    for (DWORD i = 0; i < dwNumBuffers; i++)
    {
      LOG(_T("[in] pamvaBufferInfo, buffer description %d"), i);
      LOG(_T("[in] pamvaBufferInfo->dwTypeIndex = %08x"), pamvaBufferInfo[i].dwTypeIndex);
      LOG(_T("[in] pamvaBufferInfo->dwBufferIndex = %08x"), pamvaBufferInfo[i].dwBufferIndex);
      LOG(_T("[in] pamvaBufferInfo->dwDataOffset = %08x"), pamvaBufferInfo[i].dwDataOffset);
      LOG(_T("[in] pamvaBufferInfo->dwDataSize = %08x"), pamvaBufferInfo[i].dwDataSize);
    }
  }


  for (DWORD i = 0; i < dwNumBuffers; i++)
  {
    if (pamvaBufferInfo[i].dwTypeIndex == DXVA_PICTURE_DECODE_BUFFER)
    {
      if (g_guidDXVADecoder == DXVA2_ModeH264_E || g_guidDXVADecoder == DXVA_Intel_H264_ClearVideo)
        LogDXVA_PicParams_H264((DXVA_PicParams_H264*)g_ppBuffer[pamvaBufferInfo[i].dwTypeIndex]);
      else if (g_guidDXVADecoder == DXVA2_ModeVC1_D)
        LogDXVA_PictureParameters((DXVA_PictureParameters*)g_ppBuffer[pamvaBufferInfo[i].dwTypeIndex]);
    }
    else if (pamvaBufferInfo[i].dwTypeIndex == DXVA_SLICE_CONTROL_BUFFER && (pamvaBufferInfo[i].dwDataSize % sizeof(DXVA_Slice_H264_Short)) == 0)
    {
      for (WORD j = 0; j < pamvaBufferInfo[i].dwDataSize / sizeof(DXVA_Slice_H264_Short); j++)
      {
        DXVA_Slice_H264_Short*  pSlice = &(((DXVA_Slice_H264_Short*)g_ppBuffer[pamvaBufferInfo[i].dwTypeIndex])[j]);
        LOG(_T("  - BSNALunitDataLocation  %d"), pSlice->BSNALunitDataLocation);
        LOG(_T("  - SliceBytesInBuffer     %d"), pSlice->SliceBytesInBuffer);
        LOG(_T("  - wBadSliceChopping      %d"), pSlice->wBadSliceChopping);
      }
    }
    else if (pamvaBufferInfo[i].dwTypeIndex == DXVA_BITSTREAM_DATA_BUFFER)
    {
#if defined(LOG_BITSTREAM) && defined(_DEBUG)
      char    strFile[MAX_PATH];
      static  int  nNb = 1;
      sprintf(strFile, "BitStream%d.bin", nNb++);
      FILE*    hFile = fopen(strFile, "wb");
      if (hFile)
      {
        fwrite(g_ppBuffer[pamvaBufferInfo[i].dwTypeIndex],
          1,
          pamvaBufferInfo[i].dwDataSize,
          hFile);
        fclose(hFile);
      }
#endif
    }
  }
#endif

  HRESULT hr = ExecuteOrg(This, dwFunction, lpPrivateInputData, cbPrivateInputData, lpPrivateOutputData, cbPrivateOutputData, dwNumBuffers, pamvaBufferInfo);

  LOG(_T("hr = %08x"), hr);

  if (lpPrivateOutputData && (dwFunction == 0x01000000))
  {
    LOG(_T("[out] *lpPrivateOutputData : Result = %08x"), ((DWORD*)lpPrivateOutputData)[0]);
  }

  return hr;
}

static HRESULT STDMETHODCALLTYPE QueryRenderStatusMine(IAMVideoAcceleratorC * This, DWORD dwTypeIndex, DWORD dwBufferIndex, DWORD dwFlags)
{

  HRESULT hr = QueryRenderStatusOrg(This, dwTypeIndex, dwBufferIndex, dwFlags);
  LOG(_T("\nQueryRenderStatus  Type=%d   Index=%d  hr = %08x"), dwTypeIndex, dwBufferIndex, hr);

  return hr;
}

static HRESULT STDMETHODCALLTYPE DisplayFrameMine(IAMVideoAcceleratorC * This, DWORD dwFlipToIndex, IMediaSample *pMediaSample)
{
  LOG(_T("\nEnter DisplayFrame  : %d"), dwFlipToIndex);

  HRESULT hr = DisplayFrameOrg(This, dwFlipToIndex, pMediaSample);

  LOG(_T("Leave DisplayFrame  : hr = %08x"), hr);

  return hr;
}

void HookAMVideoAccelerator(IAMVideoAcceleratorC* pAMVideoAcceleratorC)
{
  BOOL res;
  DWORD flOldProtect = 0;
  res = VirtualProtect(pAMVideoAcceleratorC->lpVtbl, sizeof(IAMVideoAcceleratorC), PAGE_WRITECOPY, &flOldProtect);

  g_guidDXVADecoder = GUID_NULL;
  g_nDXVAVersion = 0;
  bFirst = true;

  if (GetVideoAcceleratorGUIDsOrg == NULL) GetVideoAcceleratorGUIDsOrg = pAMVideoAcceleratorC->lpVtbl->GetVideoAcceleratorGUIDs;
  if (GetUncompFormatsSupportedOrg == NULL) GetUncompFormatsSupportedOrg = pAMVideoAcceleratorC->lpVtbl->GetUncompFormatsSupported;
  if (GetInternalMemInfoOrg == NULL) GetInternalMemInfoOrg = pAMVideoAcceleratorC->lpVtbl->GetInternalMemInfo;
  if (GetCompBufferInfoOrg == NULL) GetCompBufferInfoOrg = pAMVideoAcceleratorC->lpVtbl->GetCompBufferInfo;
  if (GetInternalCompBufferInfoOrg == NULL) GetInternalCompBufferInfoOrg = pAMVideoAcceleratorC->lpVtbl->GetInternalCompBufferInfo;
  if (BeginFrameOrg == NULL) BeginFrameOrg = pAMVideoAcceleratorC->lpVtbl->BeginFrame;
  if (EndFrameOrg == NULL) EndFrameOrg = pAMVideoAcceleratorC->lpVtbl->EndFrame;
  if (GetBufferOrg == NULL) GetBufferOrg = pAMVideoAcceleratorC->lpVtbl->GetBuffer;
  if (ReleaseBufferOrg == NULL) ReleaseBufferOrg = pAMVideoAcceleratorC->lpVtbl->ReleaseBuffer;
  if (ExecuteOrg == NULL) ExecuteOrg = pAMVideoAcceleratorC->lpVtbl->Execute;
  if (QueryRenderStatusOrg == NULL) QueryRenderStatusOrg = pAMVideoAcceleratorC->lpVtbl->QueryRenderStatus;
  if (DisplayFrameOrg == NULL) DisplayFrameOrg = pAMVideoAcceleratorC->lpVtbl->DisplayFrame;

  pAMVideoAcceleratorC->lpVtbl->GetVideoAcceleratorGUIDs = GetVideoAcceleratorGUImine;
  pAMVideoAcceleratorC->lpVtbl->GetUncompFormatsSupported = GetUncompFormatsSupportedMine;
  pAMVideoAcceleratorC->lpVtbl->GetInternalMemInfo = GetInternalMemInfoMine;
  pAMVideoAcceleratorC->lpVtbl->GetCompBufferInfo = GetCompBufferInfoMine;
  pAMVideoAcceleratorC->lpVtbl->GetInternalCompBufferInfo = GetInternalCompBufferInfoMine;
  pAMVideoAcceleratorC->lpVtbl->BeginFrame = BeginFrameMine;
  pAMVideoAcceleratorC->lpVtbl->EndFrame = EndFrameMine;
  pAMVideoAcceleratorC->lpVtbl->GetBuffer = GetBufferMine;
  pAMVideoAcceleratorC->lpVtbl->ReleaseBuffer = ReleaseBufferMine;
  pAMVideoAcceleratorC->lpVtbl->Execute = ExecuteMine;
  pAMVideoAcceleratorC->lpVtbl->QueryRenderStatus = QueryRenderStatusMine;
  pAMVideoAcceleratorC->lpVtbl->DisplayFrame = DisplayFrameMine;

  res = VirtualProtect(pAMVideoAcceleratorC->lpVtbl, sizeof(IAMVideoAcceleratorC), PAGE_EXECUTE, &flOldProtect);

#ifdef _DEBUG
  ::DeleteFile(LOG_FILE);
  ::DeleteFile(_T("picture.log"));
  ::DeleteFile(_T("slicelong.log"));
  ::DeleteFile(_T("sliceshort.log"));
#endif
}



// === Hook for DXVA2


static void LogDecodeBufferDesc(DXVA2_DecodeBufferDesc* pDecodeBuff)
{
  LOG(_T("DecodeBufferDesc type : %d   Size=%d   NumMBsInBuffer=%d"), pDecodeBuff->CompressedBufferType, pDecodeBuff->DataSize, pDecodeBuff->NumMBsInBuffer);
  //LOG(_T("  - BufferIndex                       %d"), pDecodeBuff->BufferIndex);
  //LOG(_T("  - DataOffset                        %d"), pDecodeBuff->DataOffset);
  //LOG(_T("  - DataSize                          %d"), pDecodeBuff->DataSize);
  //LOG(_T("  - FirstMBaddress                    %d"), pDecodeBuff->FirstMBaddress);
  //LOG(_T("  - NumMBsInBuffer                    %d"), pDecodeBuff->NumMBsInBuffer);
  //LOG(_T("  - Width                             %d"), pDecodeBuff->Width);
  //LOG(_T("  - Height                            %d"), pDecodeBuff->Height);
  //LOG(_T("  - Stride                            %d"), pDecodeBuff->Stride);
  //LOG(_T("  - ReservedBits                      %d"), pDecodeBuff->ReservedBits);
  //LOG(_T("  - pvPVPState                        %d"), pDecodeBuff->pvPVPState);
}

class CFakeDirectXVideoDecoder : public CUnknown, public IDirectXVideoDecoder
{
private:
  Com::SmartPtr<IDirectXVideoDecoder>  m_pDec;
  BYTE* m_ppBuffer[MAX_BUFFER_TYPE];
  UINT m_ppBufferLen[MAX_BUFFER_TYPE];

public:
  CFakeDirectXVideoDecoder(LPUNKNOWN pUnk, IDirectXVideoDecoder* pDec) : CUnknown(_T("Fake DXVA2 Dec"), pUnk)
  {
    m_pDec.Attach(pDec);
    memset(m_ppBuffer, 0, sizeof(m_ppBuffer));
  }

  ~CFakeDirectXVideoDecoder()
  {
    LOG(_T("CFakeDirectXVideoDecoder destroyed !\n"));
  }

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
  {
    if (riid == __uuidof(IDirectXVideoDecoder))
      return GetInterface((IDirectXVideoDecoder*)this, ppv);
    else
      return __super::NonDelegatingQueryInterface(riid, ppv);
  }

  virtual HRESULT STDMETHODCALLTYPE GetVideoDecoderService(IDirectXVideoDecoderService **ppService)
  {
    HRESULT    hr = m_pDec->GetVideoDecoderService(ppService);
    LOG(_T("IDirectXVideoDecoder::GetVideoDecoderService  hr = %08x\n"), hr);
    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE GetCreationParameters(GUID *pDeviceGuid, DXVA2_VideoDesc *pVideoDesc, DXVA2_ConfigPictureDecode *pConfig, IDirect3DSurface9 ***pDecoderRenderTargets, UINT *pNumSurfaces)
  {
    HRESULT    hr = m_pDec->GetCreationParameters(pDeviceGuid, pVideoDesc, pConfig, pDecoderRenderTargets, pNumSurfaces);
    LOG(_T("IDirectXVideoDecoder::GetCreationParameters hr = %08x\n"), hr);
    return hr;
  }


  virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT BufferType, void **ppBuffer, UINT *pBufferSize)
  {
    HRESULT    hr = m_pDec->GetBuffer(BufferType, ppBuffer, pBufferSize);

    if (BufferType < MAX_BUFFER_TYPE)
    {
      m_ppBuffer[BufferType] = (BYTE*)*ppBuffer;
      m_ppBufferLen[BufferType] = *pBufferSize;
    }
    //      LOG(_T("IDirectXVideoDecoder::GetBuffer Type = %d,  hr = %08x"), BufferType, hr);

    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT BufferType)
  {
    HRESULT    hr = m_pDec->ReleaseBuffer(BufferType);
    //      LOG(_T("IDirectXVideoDecoder::ReleaseBuffer Type = %d,  hr = %08x"), BufferType, hr);
    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE BeginFrame(IDirect3DSurface9 *pRenderTarget, void *pvPVPData)
  {
    HRESULT    hr = m_pDec->BeginFrame(pRenderTarget, pvPVPData);
    LOG(_T("IDirectXVideoDecoder::BeginFrame pRenderTarget = %08x,  hr = %08x"), pRenderTarget, hr);
    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE EndFrame(HANDLE *pHandleComplete)
  {
    HRESULT    hr = m_pDec->EndFrame(pHandleComplete);
    LOG(_T("IDirectXVideoDecoder::EndFrame  Handle=0x%08x  hr = %08x\n"), pHandleComplete, hr);
    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE Execute(const DXVA2_DecodeExecuteParams *pExecuteParams)
  {
    for (DWORD i = 0; i < pExecuteParams->NumCompBuffers; i++)
    {
      CStdString    strBuffer;

      LogDecodeBufferDesc(&pExecuteParams->pCompressedBuffers[i]);
      /*
      for (int j=0; j<4000 && j<pExecuteParams->pCompressedBuffers[i].DataSize; j++)
      strBuffer.AppendFormat (_T("%02x "), m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType][j]);

      LOG (_T(" - Buffer type=%d,  offset=%d, size=%d"),
      pExecuteParams->pCompressedBuffers[i].CompressedBufferType,
      pExecuteParams->pCompressedBuffers[i].DataOffset,
      pExecuteParams->pCompressedBuffers[i].DataSize);

      LOG (strBuffer);*/

      if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_PictureParametersBufferType)
      {
        if (g_guidDXVADecoder == DXVA2_ModeH264_E || g_guidDXVADecoder == DXVA_Intel_H264_ClearVideo)
          LogDXVA_PicParams_H264((DXVA_PicParams_H264*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType]);
        else if (g_guidDXVADecoder == DXVA2_ModeVC1_D)
          LogDXVA_PictureParameters((DXVA_PictureParameters*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType]);
      }

#if defined(_DEBUG)
      if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_SliceControlBufferType)
      {
        if (pExecuteParams->pCompressedBuffers[i].DataSize % sizeof(DXVA_Slice_H264_Long) == 0)
        {
          DXVA_Slice_H264_Long*  pSlice = (DXVA_Slice_H264_Long*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType];
          LogH264SliceLong(pSlice, pExecuteParams->pCompressedBuffers[i].DataSize / sizeof(DXVA_Slice_H264_Long));
        }
        else if (pExecuteParams->pCompressedBuffers[i].DataSize % sizeof(DXVA_Slice_H264_Short) == 0)
        {
          DXVA_Slice_H264_Short*  pSlice = (DXVA_Slice_H264_Short*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType];
          LogH264SliceShort(pSlice, pExecuteParams->pCompressedBuffers[i].DataSize / sizeof(DXVA_Slice_H264_Short));
        }
      }
#endif

#if defined(LOG_MATRIX) && defined(_DEBUG)
      if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_InverseQuantizationMatrixBufferType)
      {
        char    strFile[MAX_PATH];
        static  int  nNb = 1;
        sprintf(strFile, "Matrix%d.bin", nNb++);
        FILE*    hFile = fopen(strFile, "wb");
        if (hFile)
        {
          fwrite(m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType],
            1,
            pExecuteParams->pCompressedBuffers[i].DataSize,
            hFile);
          fclose(hFile);
        }
      }
#endif

#if defined(LOG_BITSTREAM) && defined(_DEBUG)
      if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_BitStreamDateBufferType)
      {
        char    strFile[MAX_PATH];
        static  int  nNb = 1;
        sprintf(strFile, "BitStream%d.bin", nNb++);
        FILE*    hFile = fopen(strFile, "wb");
        if (hFile)
        {
          fwrite(m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType],
            1,
            pExecuteParams->pCompressedBuffers[i].DataSize,
            hFile);
          fclose(hFile);
        }
      }
#endif
    }

    HRESULT    hr = m_pDec->Execute(pExecuteParams);

    if (pExecuteParams->pExtensionData)
      LOG(_T("IDirectXVideoDecoder::Execute  %d buffer, fct = %d  (in=%d, out=%d),  hr = %08x"),
      pExecuteParams->NumCompBuffers,
      pExecuteParams->pExtensionData->Function,
      pExecuteParams->pExtensionData->PrivateInputDataSize,
      pExecuteParams->pExtensionData->PrivateOutputDataSize,
      hr);
    else
      LOG(_T("IDirectXVideoDecoder::Execute  %d buffer, hr = %08x"), pExecuteParams->NumCompBuffers, hr);
    return hr;
  }
};


interface IDirectXVideoDecoderServiceC;
struct IDirectXVideoDecoderServiceCVtbl
{
  BEGIN_INTERFACE
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirectXVideoDecoderServiceC* pThis, /* [in] */ REFIID riid, /* [iid_is][out] */ void **ppvObject);
    ULONG(STDMETHODCALLTYPE *AddRef)(IDirectXVideoDecoderServiceC* pThis);
    ULONG(STDMETHODCALLTYPE *Release)(IDirectXVideoDecoderServiceC*  pThis);
    HRESULT(STDMETHODCALLTYPE* CreateSurface)(IDirectXVideoDecoderServiceC* pThis, __in  UINT Width, __in  UINT Height, __in  UINT BackBuffers, __in  D3DFORMAT Format, __in  D3DPOOL Pool, __in  DWORD Usage, __in  DWORD DxvaType, __out_ecount(BackBuffers + 1)  IDirect3DSurface9 **ppSurface, __inout_opt  HANDLE *pSharedHandle);

    HRESULT(STDMETHODCALLTYPE* GetDecoderDeviceGuids)(
      IDirectXVideoDecoderServiceC*          pThis,
      __out  UINT*                  pCount,
      __deref_out_ecount_opt(*pCount)  GUID**      pGuids);

    HRESULT(STDMETHODCALLTYPE* GetDecoderRenderTargets)(
      IDirectXVideoDecoderServiceC*          pThis,
      __in  REFGUID                  Guid,
      __out  UINT*                  pCount,
      __deref_out_ecount_opt(*pCount)  D3DFORMAT**  pFormats);

    HRESULT(STDMETHODCALLTYPE* GetDecoderConfigurations)(
      IDirectXVideoDecoderServiceC*          pThis,
      __in  REFGUID                  Guid,
      __in  const DXVA2_VideoDesc*          pVideoDesc,
      __reserved  void*                pReserved,
      __out  UINT*                  pCount,
      __deref_out_ecount_opt(*pCount)  DXVA2_ConfigPictureDecode **ppConfigs);

    HRESULT(STDMETHODCALLTYPE* CreateVideoDecoder)(
      IDirectXVideoDecoderServiceC*          pThis,
      __in  REFGUID                  Guid,
      __in  const DXVA2_VideoDesc*          pVideoDesc,
      __in  const DXVA2_ConfigPictureDecode*      pConfig,
      __in_ecount(NumRenderTargets)  IDirect3DSurface9 **ppDecoderRenderTargets,
      __in  UINT                    NumRenderTargets,
      __deref_out  IDirectXVideoDecoder**        ppDecode);

  END_INTERFACE
};

interface IDirectXVideoDecoderServiceC
{
  CONST_VTBL struct IDirectXVideoDecoderServiceCVtbl *lpVtbl;
};


IDirectXVideoDecoderServiceCVtbl*  g_pIDirectXVideoDecoderServiceCVtbl;
static HRESULT(STDMETHODCALLTYPE* CreateVideoDecoderOrg)  (IDirectXVideoDecoderServiceC* pThis, __in  REFGUID Guid, __in  const DXVA2_VideoDesc* pVideoDesc, __in  const DXVA2_ConfigPictureDecode* pConfig, __in_ecount(NumRenderTargets)  IDirect3DSurface9 **ppDecoderRenderTargets, __in  UINT NumRenderTargets, __deref_out  IDirectXVideoDecoder** ppDecode) = NULL;
static HRESULT(STDMETHODCALLTYPE* GetDecoderDeviceGuidsOrg)(IDirectXVideoDecoderServiceC* pThis, __out  UINT* pCount, __deref_out_ecount_opt(*pCount)  GUID** pGuids) = NULL;
static HRESULT(STDMETHODCALLTYPE* GetDecoderConfigurationsOrg) (IDirectXVideoDecoderServiceC* pThis, __in  REFGUID Guid, __in const DXVA2_VideoDesc* pVideoDesc, __reserved void* pReserved, __out UINT* pCount, __deref_out_ecount_opt(*pCount)  DXVA2_ConfigPictureDecode **ppConfigs) = NULL;



static void LogDXVA2Config(const DXVA2_ConfigPictureDecode* pConfig)
{
  LOG(_T("Config"));
  LOG(_T("  - Config4GroupedCoefs               %d"), pConfig->Config4GroupedCoefs);
  LOG(_T("  - ConfigBitstreamRaw                %d"), pConfig->ConfigBitstreamRaw);
  LOG(_T("  - ConfigDecoderSpecific             %d"), pConfig->ConfigDecoderSpecific);
  LOG(_T("  - ConfigHostInverseScan             %d"), pConfig->ConfigHostInverseScan);
  LOG(_T("  - ConfigIntraResidUnsigned          %d"), pConfig->ConfigIntraResidUnsigned);
  LOG(_T("  - ConfigMBcontrolRasterOrder        %d"), pConfig->ConfigMBcontrolRasterOrder);
  LOG(_T("  - ConfigMinRenderTargetBuffCount    %d"), pConfig->ConfigMinRenderTargetBuffCount);
  LOG(_T("  - ConfigResid8Subtraction           %d"), pConfig->ConfigResid8Subtraction);
  LOG(_T("  - ConfigResidDiffAccelerator        %d"), pConfig->ConfigResidDiffAccelerator);
  LOG(_T("  - ConfigResidDiffHost               %d"), pConfig->ConfigResidDiffHost);
  LOG(_T("  - ConfigSpatialHost8or9Clipping     %d"), pConfig->ConfigSpatialHost8or9Clipping);
  LOG(_T("  - ConfigSpatialResid8               %d"), pConfig->ConfigSpatialResid8);
  LOG(_T("  - ConfigSpatialResidInterleaved     %d"), pConfig->ConfigSpatialResidInterleaved);
  LOG(_T("  - ConfigSpecificIDCT                %d"), pConfig->ConfigSpecificIDCT);
  LOG(_T("  - guidConfigBitstreamEncryption     %s"), StringFromGUID(pConfig->guidConfigBitstreamEncryption));
  LOG(_T("  - guidConfigMBcontrolEncryption     %s"), StringFromGUID(pConfig->guidConfigMBcontrolEncryption));
  LOG(_T("  - guidConfigResidDiffEncryption     %s"), StringFromGUID(pConfig->guidConfigResidDiffEncryption));
}

static void LogDXVA2VideoDesc(const DXVA2_VideoDesc* pVideoDesc)
{
  LOG(_T("VideoDesc"));
  LOG(_T("  - Format                            %s  (0x%08x)"), FindD3DFormat(pVideoDesc->Format), pVideoDesc->Format);
  LOG(_T("  - InputSampleFreq                   %d/%d"), pVideoDesc->InputSampleFreq.Numerator, pVideoDesc->InputSampleFreq.Denominator);
  LOG(_T("  - OutputFrameFreq                   %d/%d"), pVideoDesc->OutputFrameFreq.Numerator, pVideoDesc->OutputFrameFreq.Denominator);
  LOG(_T("  - SampleFormat                      %d"), pVideoDesc->SampleFormat.value);
  LOG(_T("  - SampleHeight                      %d"), pVideoDesc->SampleHeight);
  LOG(_T("  - SampleWidth                       %d"), pVideoDesc->SampleWidth);
  LOG(_T("  - UABProtectionLevel                %d"), pVideoDesc->UABProtectionLevel);
}

static void LogVideoCardCaps(IDirectXVideoDecoderService* pDecoderService)
{
  HRESULT      hr;
  UINT      cDecoderGuids = 0;
  GUID*      pDecoderGuids = NULL;

  hr = pDecoderService->GetDecoderDeviceGuids(&cDecoderGuids, &pDecoderGuids);
  if (SUCCEEDED(hr))
  {
    // Look for the decoder GUIDs we want.
    for (UINT iGuid = 0; iGuid < cDecoderGuids; iGuid++)
    {
      LOG(_T("=== New mode : %s"), GetDXVAMode(&pDecoderGuids[iGuid]));

      // Find a configuration that we support. 
      UINT            cFormats = 0;
      UINT            cConfigurations = 0;
      D3DFORMAT*          pFormats = NULL;
      DXVA2_ConfigPictureDecode*  pConfig = NULL;
      DXVA2_VideoDesc        m_VideoDesc;

      hr = pDecoderService->GetDecoderRenderTargets(pDecoderGuids[iGuid], &cFormats, &pFormats);

      if (SUCCEEDED(hr))
      {
        // Look for a format that matches our output format.
        for (UINT iFormat = 0; iFormat < cFormats; iFormat++)
        {
          LOG(_T("Direct 3D format : %s"), FindD3DFormat(pFormats[iFormat]));
          // Fill in the video description. Set the width, height, format, and frame rate.
          memset(&m_VideoDesc, 0, sizeof(m_VideoDesc));
          m_VideoDesc.SampleWidth = 1280;
          m_VideoDesc.SampleHeight = 720;
          m_VideoDesc.Format = pFormats[iFormat];

          // Get the available configurations.
          hr = pDecoderService->GetDecoderConfigurations(pDecoderGuids[iGuid], &m_VideoDesc, NULL, &cConfigurations, &pConfig);

          if (SUCCEEDED(hr))
          {

            // Find a supported configuration.
            for (UINT iConfig = 0; iConfig < cConfigurations; iConfig++)
            {
              LogDXVA2Config(&pConfig[iConfig]);
            }

            CoTaskMemFree(pConfig);
          }
        }
      }

      LOG(_T("\n"));
      CoTaskMemFree(pFormats);
    }
  }
}

static HRESULT STDMETHODCALLTYPE CreateVideoDecoderMine(
  IDirectXVideoDecoderServiceC*          pThis,
  __in  REFGUID                  Guid,
  __in  const DXVA2_VideoDesc*          pVideoDesc,
  __in  const DXVA2_ConfigPictureDecode*      pConfig,
  __in_ecount(NumRenderTargets)  IDirect3DSurface9 **ppDecoderRenderTargets,
  __in  UINT                    NumRenderTargets,
  __deref_out  IDirectXVideoDecoder**        ppDecode)
{
  //  DebugBreak();
  //  ((DXVA2_VideoDesc*)pVideoDesc)->Format = (D3DFORMAT)0x3231564E;

  g_guidDXVADecoder = Guid;
  g_nDXVAVersion = 2;


#ifdef _DEBUG
  LOG(_T("\n\n"));
  LogDXVA2VideoDesc(pVideoDesc);
  LogDXVA2Config(pConfig);
#endif

  HRESULT hr = CreateVideoDecoderOrg(pThis, Guid, pVideoDesc, pConfig, ppDecoderRenderTargets, NumRenderTargets, ppDecode);

  if (FAILED(hr))
    g_guidDXVADecoder = GUID_NULL;
  else
  {
#ifdef _DEBUG
    if ((Guid == DXVA2_ModeH264_E) || (Guid == DXVA2_ModeVC1_D) || (Guid == DXVA_Intel_H264_ClearVideo))
    {
      *ppDecode = new CFakeDirectXVideoDecoder(NULL, *ppDecode);
      (*ppDecode)->AddRef();
    }

    for (DWORD i = 0; i < NumRenderTargets; i++)
      LOG(_T(" - Surf %d : %08x"), i, ppDecoderRenderTargets[i]);
#endif
  }

  LOG(_T("IDirectXVideoDecoderService::CreateVideoDecoder  %s  (%d render targets) hr = %08x"), GetDXVAMode(&g_guidDXVADecoder), NumRenderTargets, hr);

  return hr;
}



static HRESULT STDMETHODCALLTYPE GetDecoderDeviceGuimine(IDirectXVideoDecoderServiceC*          pThis,
  __out  UINT*                  pCount,
  __deref_out_ecount_opt(*pCount)  GUID**      pGuids)
{
  HRESULT hr = GetDecoderDeviceGuidsOrg(pThis, pCount, pGuids);
  LOG(_T("IDirectXVideoDecoderService::GetDecoderDeviceGuids  hr = %08x\n"), hr);

  return hr;
}

static HRESULT STDMETHODCALLTYPE GetDecoderConfigurationsMine(IDirectXVideoDecoderServiceC*    pThis,
  __in  REFGUID            Guid,
  __in  const DXVA2_VideoDesc*    pVideoDesc,
  __reserved  void*          pReserved,
  __out  UINT*            pCount,
  __deref_out_ecount_opt(*pCount)    DXVA2_ConfigPictureDecode **ppConfigs)
{
  HRESULT hr = GetDecoderConfigurationsOrg(pThis, Guid, pVideoDesc, pReserved, pCount, ppConfigs);

  // Force LongSlice decoding !
  //  memcpy (&(*ppConfigs)[1], &(*ppConfigs)[0], sizeof(DXVA2_ConfigPictureDecode));

  return hr;
}

void HookDirectXVideoDecoderService(void* pIDirectXVideoDecoderService)
{
  IDirectXVideoDecoderServiceC*  pIDirectXVideoDecoderServiceC = (IDirectXVideoDecoderServiceC*)pIDirectXVideoDecoderService;

  BOOL res;
  DWORD flOldProtect = 0;

  // Casimir666 : unhook previous VTables
  if (g_pIDirectXVideoDecoderServiceCVtbl)
  {
    res = VirtualProtect(g_pIDirectXVideoDecoderServiceCVtbl, sizeof(g_pIDirectXVideoDecoderServiceCVtbl), PAGE_WRITECOPY, &flOldProtect);
    if (g_pIDirectXVideoDecoderServiceCVtbl->CreateVideoDecoder == CreateVideoDecoderMine)
      g_pIDirectXVideoDecoderServiceCVtbl->CreateVideoDecoder = CreateVideoDecoderOrg;

#ifdef _DEBUG
    if (g_pIDirectXVideoDecoderServiceCVtbl->GetDecoderConfigurations == GetDecoderConfigurationsMine)
      g_pIDirectXVideoDecoderServiceCVtbl->GetDecoderConfigurations = GetDecoderConfigurationsOrg;

    //if (g_pIDirectXVideoDecoderServiceCVtbl->GetDecoderDeviceGuids == GetDecoderDeviceGuimine)
    //  g_pIDirectXVideoDecoderServiceCVtbl->GetDecoderDeviceGuids = GetDecoderDeviceGuidsOrg;
#endif

    res = VirtualProtect(g_pIDirectXVideoDecoderServiceCVtbl, sizeof(g_pIDirectXVideoDecoderServiceCVtbl), flOldProtect, &flOldProtect);

    g_pIDirectXVideoDecoderServiceCVtbl = NULL;
    CreateVideoDecoderOrg = NULL;
    GetDecoderConfigurationsOrg = NULL;
    g_guidDXVADecoder = GUID_NULL;
    g_nDXVAVersion = 0;
  }

  // TODO : remove log file !!
#ifdef _DEBUG
  ::DeleteFile(LOG_FILE);
  ::DeleteFile(_T("picture.log"));
  ::DeleteFile(_T("slicelong.log"));
#endif

  if (pIDirectXVideoDecoderService)
  {
    res = VirtualProtect(pIDirectXVideoDecoderServiceC->lpVtbl, sizeof(IDirectXVideoDecoderServiceCVtbl), PAGE_WRITECOPY, &flOldProtect);

    CreateVideoDecoderOrg = pIDirectXVideoDecoderServiceC->lpVtbl->CreateVideoDecoder;
    pIDirectXVideoDecoderServiceC->lpVtbl->CreateVideoDecoder = CreateVideoDecoderMine;

#ifdef _DEBUG
    GetDecoderConfigurationsOrg = pIDirectXVideoDecoderServiceC->lpVtbl->GetDecoderConfigurations;
    pIDirectXVideoDecoderServiceC->lpVtbl->GetDecoderConfigurations = GetDecoderConfigurationsMine;

    //GetDecoderDeviceGuidsOrg = pIDirectXVideoDecoderServiceC->lpVtbl->GetDecoderDeviceGuids;
    //pIDirectXVideoDecoderServiceC->lpVtbl->GetDecoderDeviceGuids = GetDecoderDeviceGuimine;
#endif

    res = VirtualProtect(pIDirectXVideoDecoderServiceC->lpVtbl, sizeof(IDirectXVideoDecoderServiceCVtbl), flOldProtect, &flOldProtect);

    g_pIDirectXVideoDecoderServiceCVtbl = pIDirectXVideoDecoderServiceC->lpVtbl;
  }
}

#endif