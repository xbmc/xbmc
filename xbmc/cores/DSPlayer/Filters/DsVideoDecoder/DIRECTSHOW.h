#pragma once
/*
 *      Copyright (C) 2010 Team XBMC
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

#include "libavcodec/dxva2.h"
#include <dxva.h>
#include <dxva2api.h>
#include "DSUtil/DSUtil.h"
#include "dxva2_wrapper.h"

#include <videoacc.h>
#include "DVDPlayer/DVDCodecs/Video/DVDVideoCodecFFmpeg.h"

//#include "Event.h"
#include "utils/CriticalSection.h"


enum DxvaDecoderType {
  DECODER_TYPE_DXVA_1,   ///< IAMVideoAccelerator
  DECODER_TYPE_DXVA_2    ///< IDirectXVideoDecoder
};

enum PCI_Vendors
{
  PCIV_ATI        = 0x1002,
  PCIV_nVidia        = 0x10DE,
  PCIV_Intel        = 0x8086,
  PCIV_S3_Graphics    = 0x5333
};

typedef enum
{
	PICT_TOP_FIELD     = 1,
	PICT_BOTTOM_FIELD  = 2,
	PICT_FRAME         = 3
} FF_FIELD_TYPE;

typedef enum
{
	I_TYPE  = 1, ///< Intra
	P_TYPE  = 2, ///< Predicted
	B_TYPE  = 3, ///< Bi-dir predicted
	S_TYPE  = 4, ///< S(GMC)-VOP MPEG4
	SI_TYPE = 5, ///< Switching Intra
	SP_TYPE = 6, ///< Switching Predicted
	BI_TYPE = 7
} FF_SLICE_TYPE;

// Bitmasks for DXVA compatibility check
#define DXVA_UNSUPPORTED_LEVEL    1
#define DXVA_TOO_MUCH_REF_FRAMES  2
#define DXVA_INCOMPATIBLE_SAR    4

#define MAX_COM_BUFFER				6		// Max uncompressed buffer for an Execute command (DXVA1)
#define COMP_BUFFER_COUNT			18
#define NO_REF_FRAME			0xFFFF

#define NUM_OUTPUT_SURFACES                4
#define NUM_VIDEO_SURFACES_MPEG2           10  // (1 frame being decoded, 2 reference)
#define NUM_VIDEO_SURFACES_H264            32 // (1 frame being decoded, up to 16 references)
#define NUM_VIDEO_SURFACES_VC1             10  // (same as MPEG-2)
class CXBMCVideoDecFilter;
class CDXVADecoder
{
public:
  static CDXVADecoder* CreateDecoder(CXBMCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, const GUID* guidDecoder, int nPicEntryNumber);
	static CDXVADecoder* CreateDecoder(CXBMCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
protected:
  CDXVADecoder (CXBMCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, int nPicEntryNumber);
	CDXVADecoder (CXBMCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
public:
  CDXVADecoder* Acquire();
  long          Release();
 ~CDXVADecoder();
  virtual bool Open      (AVCodecContext* avctx, const enum PixelFormat);
  virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame);
  //virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, directshow_dxva_h264* picture);
  virtual int  Check     (AVCodecContext* avctx);
  virtual bool Flush();
  virtual void Close();

  void CreateDummySurface();
  bool  ConfigureDXVA1();
  bool DXVADisplayFrame(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
  bool GetDeliveryBuffer(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, IMediaSample** ppSampleToDeliver);
  void SetDirectXVideoDec (IDirectXVideoDecoder* pDirectXVideoDec)  { m_pDirectXVideoDec = pDirectXVideoDec; };
  void SetSurfaceArray(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets);
  /* ffmpeg callbacks*/
  int   GetBuffer(AVCodecContext *avctx, AVFrame *pic);
  void  RelBuffer(AVCodecContext *avctx, AVFrame *pic);
  /* AVHWAccel dxvadecoder callbacks*/
  int   DXVABeginFrame(dxva_context *ctx, unsigned index);
  int   DXVAEndFrame(dxva_context *ctx, unsigned index);
  int   DXVAExecute(dxva_context *ctx);
  int   DXVAGetBuffer(dxva_context *ctx, unsigned type, void **dxva_data, unsigned *dxva_size);
  int   DXVAReleaseBuffer(dxva_context *ctx, unsigned type);
  //Function used from ffmpeg dxva callbacks
  IAMVideoAccelerator* GetIAMVideoAccelerator(){return m_pAMVideoAccelerator;}
  CDXDecWrapper* GetCDXDecWrapper(){return m_pIDXVideoDecoder;}
  HRESULT FindFreeDXVA1Buffer(DWORD dwTypeIndex, DWORD& dwBufferIndex);
  DWORD GetCurrentBufferIndex()
  {
    return m_dwBufferIndex;
  }
  void SetCurrentBufferIndex(DWORD index)
  {
    m_dwBufferIndex = index;
  }
  unsigned GetBufferSize(unsigned index){ return m_ComBufferInfo[index].dwBytesToAllocate;}
  DWORD              m_dwNumBuffersInfo;
  static bool      Supports(enum PixelFormat fmt);
protected:
  struct SVideoBuffer
  {
    SVideoBuffer();
   ~SVideoBuffer();

    void Init(int index);
    void Clear();

    IDirect3DSurface9* surface;
    int                surface_index;
    bool               used;
    int                age;
    REFERENCE_TIME     rt_start;
    REFERENCE_TIME     rt_stop;
  };

  long                         m_references;

  static const unsigned        m_buffer_max = 32;
  SVideoBuffer                 m_buffer[m_buffer_max];
  unsigned                     m_buffer_age;
  int                          m_refs;
  CCriticalSection             m_section;

  struct dxva_context*         m_context;
  CXBMCVideoDecFilter* m_pFilter;
public:
  void SetTypeSpecificFlags(IMediaSample* pMS,FF_SLICE_TYPE slice_type, FF_FIELD_TYPE field_type);
  
private:
  void Init(CXBMCVideoDecFilter* pFilter, int nPicEntryNumber);
  

  DxvaDecoderType						m_nEngine;
  Com::SmartQIPtr<IAMVideoAccelerator> m_pAMVideoAccelerator;
  CDXDecWrapper* m_pIDXVideoDecoder;
  AMVABUFFERINFO          m_DXVA1BufferInfo[MAX_COM_BUFFER];
  DXVA_BufferDescription       m_DXVA1BufferDesc[MAX_COM_BUFFER];
  
  DXVA_ConfigPictureDecode    m_DXVA1Config;
  AMVACompBufferInfo        m_ComBufferInfo[COMP_BUFFER_COUNT];
  DWORD              m_dwBufferIndex;
  int								 m_nMaxWaiting;
  DWORD              m_dwDisplayCount;
  int								m_nPicEntryNumber;		// Total number of picture in store

  // === DXVA2 variables
  Com::SmartPtr<IDirectXVideoDecoder>  m_pDirectXVideoDec;
  DXVA2_ConfigPictureDecode    m_DXVA2Config;
  DXVA2_DecodeExecuteParams    m_ExecuteParams;
};

