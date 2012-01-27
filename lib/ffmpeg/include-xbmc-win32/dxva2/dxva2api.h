/*****************************************************************************
 * dxva2api.h: DXVA 2 interface
 *****************************************************************************
 * Copyright (C) 2009 Geoffroy Couprie
 * Copyright (C) 2009 Laurent Aimar
 * $Id$
 *
 * Authors: Geoffroy Couprie <geal _AT_ videolan _DOT_ org>
 *          Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef _DXVA2API_H
#define _DXVA2API_H

#define MINGW_DXVA2API_H_VERSION (2)

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <objbase.h>
#include <d3d9.h>

/* Define it to allow using nameless struct/union (non C99 compliant) to match
 * the official documentation. */
//#define DXVA2API_USE_BITFIELDS

/****************STRUCTURES******************/
#pragma pack(push, 1)

typedef struct _DXVA2_ExtendedFormat {
#ifdef DXVA2API_USE_BITFIELDS
  union {
    struct {
      UINT SampleFormat           : 8;
      UINT VideoChromaSubsampling : 4;
      UINT NominalRange           : 3;
      UINT VideoTransferMatrix    : 3;
      UINT VideoLighting          : 4;
      UINT VideoPrimaries         : 5;
      UINT VideoTransferFunction  : 5;
    };
    UINT value;
  };
#else
    UINT value;
#endif
} DXVA2_ExtendedFormat;

typedef struct _DXVA2_Frequency {
  UINT Numerator;
  UINT Denominator;
} DXVA2_Frequency;

typedef struct _DXVA2_VideoDesc {
  UINT SampleWidth;
  UINT SampleHeight;
  DXVA2_ExtendedFormat SampleFormat;
  D3DFORMAT Format;
  DXVA2_Frequency InputSampleFreq;
  DXVA2_Frequency OutputFrameFreq;
  UINT UABProtectionLevel;
  UINT Reserved;
} DXVA2_VideoDesc;

typedef struct _DXVA2_ConfigPictureDecode {
  GUID guidConfigBitstreamEncryption;
  GUID guidConfigMBcontrolEncryption;
  GUID guidConfigResidDiffEncryption;
  UINT ConfigBitstreamRaw;
  UINT ConfigMBcontrolRasterOrder;
  UINT ConfigResidDiffHost;
  UINT ConfigSpatialResid8;
  UINT ConfigResid8Subtraction;
  UINT ConfigSpatialHost8or9Clipping;
  UINT ConfigSpatialResidInterleaved;
  UINT ConfigIntraResidUnsigned;
  UINT ConfigResidDiffAccelerator;
  UINT ConfigHostInverseScan;
  UINT ConfigSpecificIDCT;
  UINT Config4GroupedCoefs;
  USHORT ConfigMinRenderTargetBuffCount;
  USHORT ConfigDecoderSpecific;
} DXVA2_ConfigPictureDecode;

typedef struct _DXVA2_DecodeBufferDesc {
  DWORD CompressedBufferType;
  UINT BufferIndex;
  UINT DataOffset;
  UINT DataSize;
  UINT FirstMBaddress;
  UINT NumMBsInBuffer;
  UINT Width;
  UINT Height;
  UINT Stride;
  UINT ReservedBits;
  PVOID pvPVPState;
} DXVA2_DecodeBufferDesc;

typedef struct _DXVA2_DecodeExtensionData {
  UINT Function;
  PVOID pPrivateInputData;
  UINT PrivateInputDataSize;
  PVOID pPrivateOutputData;
  UINT PrivateOutputDataSize;
} DXVA2_DecodeExtensionData;

typedef struct _DXVA2_DecodeExecuteParams {
  UINT NumCompBuffers;
  DXVA2_DecodeBufferDesc *pCompressedBuffers;
  DXVA2_DecodeExtensionData *pExtensionData;
} DXVA2_DecodeExecuteParams;

enum {
    DXVA2_VideoDecoderRenderTarget	= 0,
	DXVA2_VideoProcessorRenderTarget	= 1,
	DXVA2_VideoSoftwareRenderTarget	= 2
};

enum {
    DXVA2_PictureParametersBufferType	= 0,
	DXVA2_MacroBlockControlBufferType	= 1,
	DXVA2_ResidualDifferenceBufferType	= 2,
	DXVA2_DeblockingControlBufferType	= 3,
	DXVA2_InverseQuantizationMatrixBufferType	= 4,
	DXVA2_SliceControlBufferType	= 5,
	DXVA2_BitStreamDateBufferType	= 6,
	DXVA2_MotionVectorBuffer	= 7,
	DXVA2_FilmGrainBuffer	= 8
};

/* DXVA MPEG-I/II and VC-1 */
typedef struct _DXVA_PictureParameters {
    USHORT  wDecodedPictureIndex;
    USHORT  wDeblockedPictureIndex;
    USHORT  wForwardRefPictureIndex;
    USHORT  wBackwardRefPictureIndex;
    USHORT  wPicWidthInMBminus1;
    USHORT  wPicHeightInMBminus1;
    UCHAR   bMacroblockWidthMinus1;
    UCHAR   bMacroblockHeightMinus1;
    UCHAR   bBlockWidthMinus1;
    UCHAR   bBlockHeightMinus1;
    UCHAR   bBPPminus1;
    UCHAR   bPicStructure;
    UCHAR   bSecondField;
    UCHAR   bPicIntra;
    UCHAR   bPicBackwardPrediction;
    UCHAR   bBidirectionalAveragingMode;
    UCHAR   bMVprecisionAndChromaRelation;
    UCHAR   bChromaFormat;
    UCHAR   bPicScanFixed;
    UCHAR   bPicScanMethod;
    UCHAR   bPicReadbackRequests;
    UCHAR   bRcontrol;
    UCHAR   bPicSpatialResid8;
    UCHAR   bPicOverflowBlocks;
    UCHAR   bPicExtrapolation;
    UCHAR   bPicDeblocked;
    UCHAR   bPicDeblockConfined;
    UCHAR   bPic4MVallowed;
    UCHAR   bPicOBMC;
    UCHAR   bPicBinPB;
    UCHAR   bMV_RPS;
    UCHAR   bReservedBits;
    USHORT  wBitstreamFcodes;
    USHORT  wBitstreamPCEelements;
    UCHAR   bBitstreamConcealmentNeed;
    UCHAR   bBitstreamConcealmentMethod;
} DXVA_PictureParameters, *LPDXVA_PictureParameters;

typedef struct _DXVA_QmatrixData {
    BYTE    bNewQmatrix[4];
    WORD    Qmatrix[4][8 * 8];
} DXVA_QmatrixData, *LPDXVA_QmatrixData;

typedef struct _DXVA_SliceInfo {
    USHORT  wHorizontalPosition;
    USHORT  wVerticalPosition;
    UINT    dwSliceBitsInBuffer;
    UINT    dwSliceDataLocation;
    UCHAR   bStartCodeBitOffset;
    UCHAR   bReservedBits;
    USHORT  wMBbitOffset;
    USHORT  wNumberMBsInSlice;
    USHORT  wQuantizerScaleCode;
    USHORT  wBadSliceChopping;
} DXVA_SliceInfo, *LPDXVA_SliceInfo;

/* DXVA H264 */
typedef struct {
#ifdef DXVA2API_USE_BITFIELDS
    union {
        struct {
            UCHAR Index7Bits     : 7;
            UCHAR AssociatedFlag : 1;
        };
        UCHAR bPicEntry;
    };
#else
    UCHAR bPicEntry;
#endif
} DXVA_PicEntry_H264;


typedef struct {
    USHORT wFrameWidthInMbsMinus1;
    USHORT wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264 CurrPic;
    UCHAR  num_ref_frames;
#ifdef DXVA2API_USE_BITFIELDS
    union {
        struct {
            USHORT field_pic_flag           : 1;
            USHORT MbaffFrameFlag           : 1;
            USHORT residual_colour_transform_flag : 1;
            USHORT sp_for_switch_flag       : 1;
            USHORT chroma_format_idc        : 2;
            USHORT RefPicFlag               : 1;
            USHORT constrained_intra_pred_flag : 1;
            USHORT weighted_pred_flag       : 1;
            USHORT weighted_bipred_idc      : 2;
            USHORT MbsConsecutiveFlag       : 1;
            USHORT frame_mbs_only_flag      : 1;
            USHORT transform_8x8_mode_flag  : 1;
            USHORT MinLumaBipredSize8x8Flag : 1;
            USHORT IntraPicFlag             : 1;
        };
        USHORT wBitFields;
    };
#else
    USHORT wBitFields;
#endif
    UCHAR   bit_depth_luma_minus8;
    UCHAR   bit_depth_chroma_minus8;
    USHORT  Reserved16Bits;
    UINT    StatusReportFeedbackNumber;
    DXVA_PicEntry_H264 RefFrameList[16];
    INT     CurrFieldOrderCnt[2];
    INT     FieldOrderCntList[16][2];
    CHAR    pic_init_qs_minus26;
    CHAR    chroma_qp_index_offset;
    CHAR    second_chroma_qp_index_offset;
    UCHAR   ContinuationFlag;
    CHAR    pic_init_qp_minus26;
    UCHAR   num_ref_idx_l0_active_minus1;
    UCHAR   num_ref_idx_l1_active_minus1;
    UCHAR   Reserved8BitsA;
    USHORT  FrameNumList[16];

    UINT    UsedForReferenceFlags;
    USHORT  NonExistingFrameFlags;
    USHORT  frame_num;
    UCHAR   log2_max_frame_num_minus4;
    UCHAR   pic_order_cnt_type;
    UCHAR   log2_max_pic_order_cnt_lsb_minus4;
    UCHAR   delta_pic_order_always_zero_flag;
    UCHAR   direct_8x8_inference_flag;
    UCHAR   entropy_coding_mode_flag;
    UCHAR   pic_order_present_flag;
    UCHAR   num_slice_groups_minus1;
    UCHAR   slice_group_map_type;
    UCHAR   deblocking_filter_control_present_flag;
    UCHAR   redundant_pic_cnt_present_flag;
    UCHAR   Reserved8BitsB;
    USHORT  slice_group_change_rate_minus1;
    UCHAR   SliceGroupMap[810];
} DXVA_PicParams_H264;

typedef struct {
    UCHAR   bScalingLists4x4[6][16];
    UCHAR   bScalingLists8x8[2][64];
} DXVA_Qmatrix_H264;


typedef struct {
    UINT    BSNALunitDataLocation;
    UINT    SliceBytesInBuffer;
    USHORT  wBadSliceChopping;
    USHORT  first_mb_in_slice;
    USHORT  NumMbsForSlice;
    USHORT  BitOffsetToSliceData;
    UCHAR   slice_type;
    UCHAR   luma_log2_weight_denom;
    UCHAR   chroma_log2_weight_denom;

    UCHAR   num_ref_idx_l0_active_minus1;
    UCHAR   num_ref_idx_l1_active_minus1;
    CHAR    slice_alpha_c0_offset_div2;
    CHAR    slice_beta_offset_div2;
    UCHAR   Reserved8Bits;
    DXVA_PicEntry_H264 RefPicList[2][32];
    SHORT   Weights[2][32][3][2];
    CHAR    slice_qs_delta;
    CHAR    slice_qp_delta;
    UCHAR   redundant_pic_cnt;
    UCHAR   direct_spatial_mv_pred_flag;
    UCHAR   cabac_init_idc;
    UCHAR   disable_deblocking_filter_idc;
    USHORT  slice_id;
} DXVA_Slice_H264_Long;

typedef struct {
    UINT    BSNALunitDataLocation;
    UINT    SliceBytesInBuffer;
    USHORT  wBadSliceChopping;
} DXVA_Slice_H264_Short;

typedef struct {
    USHORT  wFrameWidthInMbsMinus1;
    USHORT  wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264 InPic;
    DXVA_PicEntry_H264 OutPic;
    USHORT  PicOrderCnt_offset;
    INT     CurrPicOrderCnt;
    UINT    StatusReportFeedbackNumber;
    UCHAR   model_id;
    UCHAR   separate_colour_description_present_flag;
    UCHAR   film_grain_bit_depth_luma_minus8;
    UCHAR   film_grain_bit_depth_chroma_minus8;
    UCHAR   film_grain_full_range_flag;
    UCHAR   film_grain_colour_primaries;
    UCHAR   film_grain_transfer_characteristics;
    UCHAR   film_grain_matrix_coefficients;
    UCHAR   blending_mode_id;
    UCHAR   log2_scale_factor;
    UCHAR   comp_model_present_flag[4];
    UCHAR   num_intensity_intervals_minus1[4];
    UCHAR   num_model_values_minus1[4];
    UCHAR   intensity_interval_lower_bound[3][16];
    UCHAR   intensity_interval_upper_bound[3][16];
    SHORT   comp_model_value[3][16][8];
} DXVA_FilmGrainChar_H264;

#pragma pack(pop)

/*************INTERFACES************/
#ifdef __cplusplus
extern "C" {
#endif

typedef _COM_interface IDirectXVideoDecoderService IDirectXVideoDecoderService;
typedef _COM_interface IDirectXVideoDecoder IDirectXVideoDecoder;

#undef INTERFACE
#define INTERFACE IDirectXVideoDecoder
DECLARE_INTERFACE_(IDirectXVideoDecoder,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetVideoDecoderService)(THIS_ IDirectXVideoDecoderService**) PURE;
	STDMETHOD(GetCreationParameters)(THIS_ GUID*,DXVA2_VideoDesc*,DXVA2_ConfigPictureDecode*,IDirect3DSurface9***,UINT*) PURE;
	STDMETHOD(GetBuffer)(THIS_ UINT,void**,UINT*) PURE;
	STDMETHOD(ReleaseBuffer)(THIS_ UINT) PURE;
	STDMETHOD(BeginFrame)(THIS_ IDirect3DSurface9 *,void*) PURE;
	STDMETHOD(EndFrame)(THIS_ HANDLE *) PURE;
	STDMETHOD(Execute)(THIS_ const DXVA2_DecodeExecuteParams*) PURE;


};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoDecoder_QueryInterface(p,a,b)	(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectXVideoDecoder_AddRef(p)	(p)->lpVtbl->AddRef(p)
#define IDirectXVideoDecoder_Release(p)	(p)->lpVtbl->Release(p)
#define IDirectXVideoDecoder_BeginFrame(p,a,b)	(p)->lpVtbl->BeginFrame(p,a,b)
#define IDirectXVideoDecoder_EndFrame(p,a)	(p)->lpVtbl->EndFrame(p,a)
#define IDirectXVideoDecoder_Execute(p,a)	(p)->lpVtbl->Execute(p,a)
#define IDirectXVideoDecoder_GetBuffer(p,a,b,c)	(p)->lpVtbl->GetBuffer(p,a,b,c)
#define IDirectXVideoDecoder_GetCreationParameters(p,a,b,c,d,e)	(p)->lpVtbl->GetCreationParameters(p,a,b,c,d,e)
#define IDirectXVideoDecoder_GetVideoDecoderService(p,a)	(p)->lpVtbl->GetVideoDecoderService(p,a)
#define IDirectXVideoDecoder_ReleaseBuffer(p,a)	(p)->lpVtbl->ReleaseBuffer(p,a)
#else
#define IDirectXVideoDecoder_QueryInterface(p,a,b)	(p)->QueryInterface(a,b)
#define IDirectXVideoDecoder_AddRef(p)	(p)->AddRef()
#define IDirectXVideoDecoder_Release(p)	(p)->Release()
#define IDirectXVideoDecoder_BeginFrame(p,a,b)	(p)->BeginFrame(a,b)
#define IDirectXVideoDecoder_EndFrame(p,a)	(p)->EndFrame(a)
#define IDirectXVideoDecoder_Execute(p,a)	(p)->Execute(a)
#define IDirectXVideoDecoder_GetBuffer(p,a,b,c)	(p)->GetBuffer(a,b,c)
#define IDirectXVideoDecoder_GetCreationParameters(p,a,b,c,d,e)	(p)->GetCreationParameters(a,b,c,d,e)
#define IDirectXVideoDecoder_GetVideoDecoderService(p,a)	(p)->GetVideoDecoderService(a)
#define IDirectXVideoDecoder_ReleaseBuffer(p,a)	(p)->ReleaseBuffer(a)
#endif

#undef INTERFACE
#define INTERFACE IDirectXVideoAccelerationService
DECLARE_INTERFACE_(IDirectXVideoAccelerationService,IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(CreateSurface)(THIS_ UINT,UINT,UINT,D3DFORMAT,D3DPOOL,DWORD,DWORD,IDirect3DSurface9**,HANDLE*) PURE;

};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoAccelerationService_QueryInterface(p,a,b)	(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectXVideoAccelerationService_AddRef(p)	(p)->lpVtbl->AddRef(p)
#define IDirectXVideoAccelerationService_Release(p)	(p)->lpVtbl->Release(p)
#define IDirectXVideoAccelerationService_CreateSurface(p,a,b,c,d,e,f,g,h,i)	(p)->lpVtbl->CreateSurface(p,a,b,c,d,e,f,g,h,i)
#else
#define IDirectXVideoAccelerationService_QueryInterface(p,a,b)	(p)->QueryInterface(a,b)
#define IDirectXVideoAccelerationService_AddRef(p)	(p)->AddRef()
#define IDirectXVideoAccelerationService_Release(p)	(p)->Release()
#define IDirectXVideoAccelerationService_CreateSurface(p,a,b,c,d,e,f,g,h,i)	(p)->CreateSurface(a,b,c,d,e,f,g,h,i)
#endif

#undef INTERFACE
#define INTERFACE IDirectXVideoDecoderService
DECLARE_INTERFACE_(IDirectXVideoDecoderService,IDirectXVideoAccelerationService)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(CreateSurface)(THIS_ UINT,UINT,UINT,D3DFORMAT,D3DPOOL,DWORD,DWORD,IDirect3DSurface9**,HANDLE*) PURE;
    STDMETHOD(GetDecoderDeviceGuids)(THIS_ UINT*,GUID **) PURE;
    STDMETHOD(GetDecoderRenderTargets)(THIS_ REFGUID,UINT*,D3DFORMAT**) PURE;
    STDMETHOD(GetDecoderConfigurations)(THIS_ REFGUID,const DXVA2_VideoDesc*,IUnknown*,UINT*,DXVA2_ConfigPictureDecode**) PURE;
    STDMETHOD(CreateVideoDecoder)(THIS_ REFGUID,const DXVA2_VideoDesc*,DXVA2_ConfigPictureDecode*,IDirect3DSurface9**,UINT,IDirectXVideoDecoder**) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoDecoderService_QueryInterface(p,a,b)	(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectXVideoDecoderService_AddRef(p)	(p)->lpVtbl->AddRef(p)
#define IDirectXVideoDecoderService_Release(p)	(p)->lpVtbl->Release(p)
#define IDirectXVideoDecoderService_CreateSurface(p,a,b,c,d,e,f,g,h,i)	(p)->lpVtbl->CreateSurface(p,a,b,c,d,e,f,g,h,i)
#define IDirectXVideoDecoderService_CreateVideoDecoder(p,a,b,c,d,e,f)	(p)->lpVtbl->CreateVideoDecoder(p,a,b,c,d,e,f)
#define IDirectXVideoDecoderService_GetDecoderConfigurations(p,a,b,c,d,e)	(p)->lpVtbl->GetDecoderConfigurations(p,a,b,c,d,e)
#define IDirectXVideoDecoderService_GetDecoderDeviceGuids(p,a,b)	(p)->lpVtbl->GetDecoderDeviceGuids(p,a,b)
#define IDirectXVideoDecoderService_GetDecoderRenderTargets(p,a,b,c)	(p)->lpVtbl->GetDecoderRenderTargets(p,a,b,c)
#else
#define IDirectXVideoDecoderService_QueryInterface(p,a,b)	(p)->QueryInterface(a,b)
#define IDirectXVideoDecoderService_AddRef(p)	(p)->AddRef()
#define IDirectXVideoDecoderService_Release(p)	(p)->Release()
#define IDirectXVideoDecoderService_CreateSurface(p,a,b,c,d,e,f,g,h,i)	(p)->CreateSurface(a,b,c,d,e,f,g,h,i)
#define IDirectXVideoDecoderService_CreateVideoDecoder(p,a,b,c,d,e,f)	(p)->CreateVideoDecoder(a,b,c,d,e,f)
#define IDirectXVideoDecoderService_GetDecoderConfigurations(p,a,b,c,d,e)	(p)->GetDecoderConfigurations(a,b,c,d,e)
#define IDirectXVideoDecoderService_GetDecoderDeviceGuids(p,a,b)	(p)->GetDecoderDeviceGuids(a,b)
#define IDirectXVideoDecoderService_GetDecoderRenderTargets(p,a,b,c)	(p)->GetDecoderRenderTargets(a,b,c)
#endif

#undef INTERFACE
#define INTERFACE IDirect3DDeviceManager9
DECLARE_INTERFACE_(IDirect3DDeviceManager9,IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(ResetDevice)(THIS_ IDirect3DDevice9*,UINT) PURE;
    STDMETHOD(OpenDeviceHandle)(THIS_ HANDLE*) PURE;
    STDMETHOD(CloseDeviceHandle)( THIS_ HANDLE) PURE;
    STDMETHOD(TestDevice)( THIS_ HANDLE) PURE;
    STDMETHOD(LockDevice)( THIS_ HANDLE,IDirect3DDevice9**,BOOL) PURE;
    STDMETHOD(UnlockDevice)( THIS_ HANDLE,BOOL) PURE;
    STDMETHOD(GetVideoService)( THIS_ HANDLE,REFIID,void**) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirect3DDeviceManager9_QueryInterface(p,a,b)	(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DDeviceManager9_AddRef(p)	(p)->lpVtbl->AddRef(p)
#define IDirect3DDeviceManager9_Release(p)	(p)->lpVtbl->Release(p)
#define IDirect3DDeviceManager9_ResetDevice(p,a,b) (p)->lpVtbl->ResetDevice(p,a,b)
#define IDirect3DDeviceManager9_OpenDeviceHandle(p,a) (p)->lpVtbl->OpenDeviceHandle(p,a)
#define IDirect3DDeviceManager9_CloseDeviceHandle(p,a) (p)->lpVtbl->CloseDeviceHandle(p,a)
#define IDirect3DDeviceManager9_TestDevice(p,a) (p)->lpVtbl->TestDevice(p,a)
#define IDirect3DDeviceManager9_LockDevice(p,a,b,c) (p)->lpVtbl->LockDevice(p,a,b,c)
#define IDirect3DDeviceManager9_UnlockDevice(p,a,b) (p)->lpVtbl->UnlockDevice(p,a,b)
#define IDirect3DDeviceManager9_GetVideoService(p,a,b,c) (p)->lpVtbl->GetVideoService(p,a,b,c)
#else
#define IDirect3DDeviceManager9_QueryInterface(p,a,b)	(p)->QueryInterface(a,b)
#define IDirect3DDeviceManager9_AddRef(p)	(p)->AddRef()
#define IDirect3DDeviceManager9_Release(p)	(p)->Release()
#define IDirect3DDeviceManager9_ResetDevice(p,a,b) (p)->ResetDevice(a,b)
#define IDirect3DDeviceManager9_OpenDeviceHandle(p,a) (p)->OpenDeviceHandle(a)
#define IDirect3DDeviceManager9_CloseDeviceHandle(p,a) (p)->CloseDeviceHandle(a)
#define IDirect3DDeviceManager9_TestDevice(p,a) (p)->TestDevice(a)
#define IDirect3DDeviceManager9_LockDevice(p,a,b,c) (p)->LockDevice(a,b,c)
#define IDirect3DDeviceManager9_UnlockDevice(p,a,b) (p)->UnlockDevice(a,b)
#define IDirect3DDeviceManager9_GetVideoService(p,a,b,c) (p)->GetVideoService(a,b,c)
#endif

#ifdef __cplusplus
};
#endif

#endif //_DXVA2API_H
