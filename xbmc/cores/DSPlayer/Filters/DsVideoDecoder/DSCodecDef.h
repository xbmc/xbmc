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

#pragma once

#define MAX_SUPPORTED_MODE 5
#define MPCVD_CAPTION _T("XBMC Internal Decoder")

#define ROUND_FRAMERATE(var,FrameRate)  if (labs ((long)(var - FrameRate)) < FrameRate*1/100) var = FrameRate;

typedef struct
{
  const int      PicEntryNumber;
  const UINT      PreferedConfigBitstream;
  const GUID*      Decoder[MAX_SUPPORTED_MODE];
  const WORD      RestrictedMode[MAX_SUPPORTED_MODE];
} DXVA_PARAMS;

typedef struct
{
  const CLSID*      clsMinorType;
  const enum CodecID  nFFCodec;
  const int        fourcc;
  const DXVA_PARAMS*  DXVAModes;

  int DXVAModeCount()    
  {
    if (!DXVAModes) return 0;
    for (int i=0; i<MAX_SUPPORTED_MODE; i++)
    {
      if (DXVAModes->Decoder[i] == &GUID_NULL) return i;
    }
    return MAX_SUPPORTED_MODE;
  }
} FFMPEG_CODECS;


typedef enum
{
 ffYCbCr_RGB_coeff_ITUR_BT601    = 0,
 ffYCbCr_RGB_coeff_ITUR_BT709    = 1,
 ffYCbCr_RGB_coeff_SMPTE240M     = 2,
} ffYCbCr_RGB_MatrixCoefficientsType;

struct TYCbCr2RGB_coeffs {
    double Kr;
    double Kg;
    double Kb;
    double chr_range;
    double y_mul;
    double vr_mul;
    double ug_mul;
    double vg_mul;
    double ub_mul;
    int Ysub;
    int RGB_add1;
    int RGB_add3;

    TYCbCr2RGB_coeffs(ffYCbCr_RGB_MatrixCoefficientsType cspOptionsIturBt,
                      int cspOptionsWhiteCutoff,
                      int cspOptionsBlackCutoff,
                      int cspOptionsChromaCutoff,
                      double cspOptionsRGB_WhiteLevel,
                      double cspOptionsRGB_BlackLevel)
    {
        if (cspOptionsIturBt == ffYCbCr_RGB_coeff_ITUR_BT601) {
             Kr = 0.299;
             Kg = 0.587;
             Kb = 0.114;
        } else if (cspOptionsIturBt == ffYCbCr_RGB_coeff_SMPTE240M) {
             Kr = 0.2122;
             Kg = 0.7013;
             Kb = 0.0865;
        } else {
             Kr = 0.2125;
             Kg = 0.7154;
             Kb = 0.0721;
        }

        double in_y_range   = cspOptionsWhiteCutoff - cspOptionsBlackCutoff;
        chr_range = 128 - cspOptionsChromaCutoff;

        double cspOptionsRGBrange = cspOptionsRGB_WhiteLevel - cspOptionsRGB_BlackLevel;
        y_mul =cspOptionsRGBrange / in_y_range;
        vr_mul=(cspOptionsRGBrange / chr_range) * (1.0 - Kr);
        ug_mul=(cspOptionsRGBrange / chr_range) * (1.0 - Kb) * Kb / Kg;
        vg_mul=(cspOptionsRGBrange / chr_range) * (1.0 - Kr) * Kr / Kg;   
        ub_mul=(cspOptionsRGBrange / chr_range) * (1.0 - Kb);
        int sub = std::min((int)cspOptionsRGB_BlackLevel, cspOptionsBlackCutoff);
        Ysub = cspOptionsBlackCutoff - sub;
        RGB_add1 = (int)cspOptionsRGB_BlackLevel - sub;
        RGB_add3 = (RGB_add1 << 8) + (RGB_add1 << 16) + RGB_add1;
    }
};


// DXVA modes supported for Mpeg2
DXVA_PARAMS    DXVA_Mpeg2 =
{
  9,    // PicEntryNumber
  1,    // PreferedConfigBitstream
  { &DXVA2_ModeMPEG2_VLD, &GUID_NULL },
  { DXVA_RESTRICTED_MODE_UNRESTRICTED, 0 }  // Restricted mode for DXVA1?
};

// DXVA modes supported for H264
DXVA_PARAMS    DXVA_H264 =
{
  16,    // PicEntryNumber
  2,    // PreferedConfigBitstream
  { &DXVA2_ModeH264_E, &DXVA2_ModeH264_F, &DXVA_Intel_H264_ClearVideo, &GUID_NULL },
  { DXVA_RESTRICTED_MODE_H264_E,   0}
};

DXVA_PARAMS    DXVA_H264_VISTA =
{
  22,    // PicEntryNumber
  2,    // PreferedConfigBitstream
  { &DXVA2_ModeH264_E, &DXVA2_ModeH264_F, &DXVA_Intel_H264_ClearVideo, &GUID_NULL },
  { DXVA_RESTRICTED_MODE_H264_E,   0}
};

// DXVA modes supported for VC1
DXVA_PARAMS    DXVA_VC1 =
{
  14,    // PicEntryNumber
  1,    // PreferedConfigBitstream
  { &DXVA2_ModeVC1_D,        &GUID_NULL },
  { DXVA_RESTRICTED_MODE_VC1_D,   0}
};

FFMPEG_CODECS    ffCodecs[] =
{
  // Flash video
  { &MEDIASUBTYPE_FLV1, CODEC_ID_FLV1, MAKEFOURCC('F','L','V','1'),  NULL },
  { &MEDIASUBTYPE_flv1, CODEC_ID_FLV1, MAKEFOURCC('f','l','v','1'),  NULL },
  { &MEDIASUBTYPE_FLV4, CODEC_ID_VP6F, MAKEFOURCC('F','L','V','4'),  NULL },
  { &MEDIASUBTYPE_flv4, CODEC_ID_VP6F, MAKEFOURCC('f','l','v','4'),  NULL },
  { &MEDIASUBTYPE_VP6F, CODEC_ID_VP6F, MAKEFOURCC('V','P','6','F'),  NULL },
  { &MEDIASUBTYPE_vp6f, CODEC_ID_VP6F, MAKEFOURCC('v','p','6','f'),  NULL },

  // VP5
  { &MEDIASUBTYPE_VP50, CODEC_ID_VP5,  MAKEFOURCC('V','P','5','0'),  NULL },
  { &MEDIASUBTYPE_vp50, CODEC_ID_VP5,  MAKEFOURCC('v','p','5','0'),  NULL },

  // VP6
  { &MEDIASUBTYPE_VP60, CODEC_ID_VP6,  MAKEFOURCC('V','P','6','0'),  NULL },
  { &MEDIASUBTYPE_vp60, CODEC_ID_VP6,  MAKEFOURCC('v','p','6','0'),  NULL },
  { &MEDIASUBTYPE_VP61, CODEC_ID_VP6,  MAKEFOURCC('V','P','6','1'),  NULL },
  { &MEDIASUBTYPE_vp61, CODEC_ID_VP6,  MAKEFOURCC('v','p','6','1'),  NULL },
  { &MEDIASUBTYPE_VP62, CODEC_ID_VP6,  MAKEFOURCC('V','P','6','2'),  NULL },
  { &MEDIASUBTYPE_vp62, CODEC_ID_VP6,  MAKEFOURCC('v','p','6','2'),  NULL },
  { &MEDIASUBTYPE_VP6A, CODEC_ID_VP6A, MAKEFOURCC('V','P','6','A'),  NULL },
  { &MEDIASUBTYPE_vp6a, CODEC_ID_VP6A, MAKEFOURCC('v','p','6','a'),  NULL },

  // Xvid
  { &MEDIASUBTYPE_XVID, CODEC_ID_MPEG4,  MAKEFOURCC('X','V','I','D'),  NULL },
  { &MEDIASUBTYPE_xvid, CODEC_ID_MPEG4,  MAKEFOURCC('x','v','i','d'),  NULL },
  { &MEDIASUBTYPE_XVIX, CODEC_ID_MPEG4,  MAKEFOURCC('X','V','I','X'),  NULL },
  { &MEDIASUBTYPE_xvix, CODEC_ID_MPEG4,  MAKEFOURCC('x','v','i','x'),  NULL },

  // DivX
  { &MEDIASUBTYPE_DX50, CODEC_ID_MPEG4,  MAKEFOURCC('D','X','5','0'),  NULL },
  { &MEDIASUBTYPE_dx50, CODEC_ID_MPEG4,  MAKEFOURCC('d','x','5','0'),  NULL },
  { &MEDIASUBTYPE_DIVX, CODEC_ID_MPEG4,  MAKEFOURCC('D','I','V','X'),  NULL },
  { &MEDIASUBTYPE_divx, CODEC_ID_MPEG4,  MAKEFOURCC('d','i','v','x'),  NULL },

  // WMV1/2/3
  { &MEDIASUBTYPE_WMV1, CODEC_ID_WMV1,  MAKEFOURCC('W','M','V','1'),  NULL },
  { &MEDIASUBTYPE_wmv1, CODEC_ID_WMV1,  MAKEFOURCC('w','m','v','1'),  NULL },
  { &MEDIASUBTYPE_WMV2, CODEC_ID_WMV2,  MAKEFOURCC('W','M','V','2'),  NULL },
  { &MEDIASUBTYPE_wmv2, CODEC_ID_WMV2,  MAKEFOURCC('w','m','v','2'),  NULL },
  { &MEDIASUBTYPE_WMV3, CODEC_ID_WMV3,  MAKEFOURCC('W','M','V','3'),  NULL },
  { &MEDIASUBTYPE_wmv3, CODEC_ID_WMV3,  MAKEFOURCC('w','m','v','3'),  NULL },

  // Mpeg 2
  { &MEDIASUBTYPE_MPEG2_VIDEO, CODEC_ID_MPEG2VIDEO,  MAKEFOURCC('M','P','G','2'),  &DXVA_Mpeg2 },

  // MSMPEG-4
  { &MEDIASUBTYPE_DIV3, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('D','I','V','3'),  NULL },
  { &MEDIASUBTYPE_div3, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('d','i','v','3'),  NULL },
  { &MEDIASUBTYPE_DVX3, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('D','V','X','3'),  NULL },
  { &MEDIASUBTYPE_dvx3, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('d','v','x','3'),  NULL },
  { &MEDIASUBTYPE_MP43, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('M','P','4','3'),  NULL },
  { &MEDIASUBTYPE_mp43, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('m','p','4','3'),  NULL },
  { &MEDIASUBTYPE_COL1, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('C','O','L','1'),  NULL },
  { &MEDIASUBTYPE_col1, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('c','o','l','1'),  NULL },
  { &MEDIASUBTYPE_DIV4, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('D','I','V','4'),  NULL },
  { &MEDIASUBTYPE_div4, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('d','i','v','4'),  NULL },
  { &MEDIASUBTYPE_DIV5, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('D','I','V','5'),  NULL },
  { &MEDIASUBTYPE_div5, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('d','i','v','5'),  NULL },
  { &MEDIASUBTYPE_DIV6, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('D','I','V','6'),  NULL },
  { &MEDIASUBTYPE_div6, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('d','i','v','6'),  NULL },
  { &MEDIASUBTYPE_AP41, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('A','P','4','1'),  NULL },
  { &MEDIASUBTYPE_ap41, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('a','p','4','1'),  NULL },
  { &MEDIASUBTYPE_MPG3, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('M','P','G','3'),  NULL },
  { &MEDIASUBTYPE_mpg3, CODEC_ID_MSMPEG4V3,  MAKEFOURCC('m','p','g','3'),  NULL },
  { &MEDIASUBTYPE_DIV2, CODEC_ID_MSMPEG4V2,  MAKEFOURCC('D','I','V','2'),  NULL },
  { &MEDIASUBTYPE_div2, CODEC_ID_MSMPEG4V2,  MAKEFOURCC('d','i','v','2'),  NULL },
  { &MEDIASUBTYPE_MP42, CODEC_ID_MSMPEG4V2,  MAKEFOURCC('M','P','4','2'),  NULL },
  { &MEDIASUBTYPE_mp42, CODEC_ID_MSMPEG4V2,  MAKEFOURCC('m','p','4','2'),  NULL },
  { &MEDIASUBTYPE_MPG4, CODEC_ID_MSMPEG4V1,  MAKEFOURCC('M','P','G','4'),  NULL },
  { &MEDIASUBTYPE_mpg4, CODEC_ID_MSMPEG4V1,  MAKEFOURCC('m','p','g','4'),  NULL },
  { &MEDIASUBTYPE_DIV1, CODEC_ID_MSMPEG4V1,  MAKEFOURCC('D','I','V','1'),  NULL },
  { &MEDIASUBTYPE_div1, CODEC_ID_MSMPEG4V1,  MAKEFOURCC('d','i','v','1'),  NULL },
  { &MEDIASUBTYPE_MP41, CODEC_ID_MSMPEG4V1,  MAKEFOURCC('M','P','4','1'),  NULL },
  { &MEDIASUBTYPE_mp41, CODEC_ID_MSMPEG4V1,  MAKEFOURCC('m','p','4','1'),  NULL },

  // AMV Video
  { &MEDIASUBTYPE_AMVV, CODEC_ID_AMV,  MAKEFOURCC('A','M','V','V'),  NULL },

  // H264/AVC
  { &MEDIASUBTYPE_H264, CODEC_ID_H264, MAKEFOURCC('H','2','6','4'),  &DXVA_H264 },
  { &MEDIASUBTYPE_h264, CODEC_ID_H264, MAKEFOURCC('h','2','6','4'),  &DXVA_H264 },
  { &MEDIASUBTYPE_X264, CODEC_ID_H264, MAKEFOURCC('X','2','6','4'),  &DXVA_H264 },
  { &MEDIASUBTYPE_x264, CODEC_ID_H264, MAKEFOURCC('x','2','6','4'),  &DXVA_H264 },
  { &MEDIASUBTYPE_VSSH, CODEC_ID_H264, MAKEFOURCC('V','S','S','H'),  &DXVA_H264 },
  { &MEDIASUBTYPE_vssh, CODEC_ID_H264, MAKEFOURCC('v','s','s','h'),  &DXVA_H264 },
  { &MEDIASUBTYPE_DAVC, CODEC_ID_H264, MAKEFOURCC('D','A','V','C'),  &DXVA_H264 },
  { &MEDIASUBTYPE_davc, CODEC_ID_H264, MAKEFOURCC('d','a','v','c'),  &DXVA_H264 },
  { &MEDIASUBTYPE_PAVC, CODEC_ID_H264, MAKEFOURCC('P','A','V','C'),  &DXVA_H264 },
  { &MEDIASUBTYPE_pavc, CODEC_ID_H264, MAKEFOURCC('p','a','v','c'),  &DXVA_H264 },
  { &MEDIASUBTYPE_AVC1, CODEC_ID_H264, MAKEFOURCC('A','V','C','1'),  &DXVA_H264 },
  { &MEDIASUBTYPE_avc1, CODEC_ID_H264, MAKEFOURCC('a','v','c','1'),  &DXVA_H264 },
  { &MEDIASUBTYPE_H264_bis, CODEC_ID_H264, MAKEFOURCC('a','v','c','1'),  &DXVA_H264 },
  
  // SVQ3
  { &MEDIASUBTYPE_SVQ3, CODEC_ID_SVQ3, MAKEFOURCC('S','V','Q','3'),  NULL },

  // SVQ1
  { &MEDIASUBTYPE_SVQ1, CODEC_ID_SVQ1, MAKEFOURCC('S','V','Q','1'),  NULL },

  // H263
  { &MEDIASUBTYPE_H263, CODEC_ID_H263, MAKEFOURCC('H','2','6','3'),  NULL },
  { &MEDIASUBTYPE_h263, CODEC_ID_H263, MAKEFOURCC('h','2','6','3'),  NULL },

  { &MEDIASUBTYPE_S263, CODEC_ID_H263, MAKEFOURCC('S','2','6','3'),  NULL },
  { &MEDIASUBTYPE_s263, CODEC_ID_H263, MAKEFOURCC('s','2','6','3'),  NULL },  

  // Real video
  { &MEDIASUBTYPE_RV10, CODEC_ID_RV10, MAKEFOURCC('R','V','1','0'),  NULL },  
  { &MEDIASUBTYPE_RV20, CODEC_ID_RV20, MAKEFOURCC('R','V','2','0'),  NULL },  
  { &MEDIASUBTYPE_RV30, CODEC_ID_RV30, MAKEFOURCC('R','V','3','0'),  NULL },  
  { &MEDIASUBTYPE_RV40, CODEC_ID_RV40, MAKEFOURCC('R','V','4','0'),  NULL },  

  // Theora
  { &MEDIASUBTYPE_THEORA, CODEC_ID_THEORA, MAKEFOURCC('T','H','E','O'),  NULL },
  { &MEDIASUBTYPE_theora, CODEC_ID_THEORA, MAKEFOURCC('t','h','e','o'),  NULL },


  // WVC1
  { &MEDIASUBTYPE_WVC1, CODEC_ID_VC1,  MAKEFOURCC('W','V','C','1'),  &DXVA_VC1 },
  { &MEDIASUBTYPE_wvc1, CODEC_ID_VC1,  MAKEFOURCC('w','v','c','1'),  &DXVA_VC1 },

  // Other MPEG-4
  { &MEDIASUBTYPE_MP4V, CODEC_ID_MPEG4,  MAKEFOURCC('M','P','4','V'),  NULL },
  { &MEDIASUBTYPE_mp4v, CODEC_ID_MPEG4,  MAKEFOURCC('m','p','4','v'),  NULL },
  { &MEDIASUBTYPE_M4S2, CODEC_ID_MPEG4,  MAKEFOURCC('M','4','S','2'),  NULL },
  { &MEDIASUBTYPE_m4s2, CODEC_ID_MPEG4,  MAKEFOURCC('m','4','s','2'),  NULL },
  { &MEDIASUBTYPE_MP4S, CODEC_ID_MPEG4,  MAKEFOURCC('M','P','4','S'),  NULL },
  { &MEDIASUBTYPE_mp4s, CODEC_ID_MPEG4,  MAKEFOURCC('m','p','4','s'),  NULL },
  { &MEDIASUBTYPE_3IV1, CODEC_ID_MPEG4,  MAKEFOURCC('3','I','V','1'),  NULL },
  { &MEDIASUBTYPE_3iv1, CODEC_ID_MPEG4,  MAKEFOURCC('3','i','v','1'),  NULL },
  { &MEDIASUBTYPE_3IV2, CODEC_ID_MPEG4,  MAKEFOURCC('3','I','V','2'),  NULL },
  { &MEDIASUBTYPE_3iv2, CODEC_ID_MPEG4,  MAKEFOURCC('3','i','v','2'),  NULL },
  { &MEDIASUBTYPE_3IVX, CODEC_ID_MPEG4,  MAKEFOURCC('3','I','V','X'),  NULL },
  { &MEDIASUBTYPE_3ivx, CODEC_ID_MPEG4,  MAKEFOURCC('3','i','v','x'),  NULL },
  { &MEDIASUBTYPE_BLZ0, CODEC_ID_MPEG4,  MAKEFOURCC('B','L','Z','0'),  NULL },
  { &MEDIASUBTYPE_blz0, CODEC_ID_MPEG4,  MAKEFOURCC('b','l','z','0'),  NULL },
  { &MEDIASUBTYPE_DM4V, CODEC_ID_MPEG4,  MAKEFOURCC('D','M','4','V'),  NULL },
  { &MEDIASUBTYPE_dm4v, CODEC_ID_MPEG4,  MAKEFOURCC('d','m','4','v'),  NULL },
  { &MEDIASUBTYPE_FFDS, CODEC_ID_MPEG4,  MAKEFOURCC('F','F','D','S'),  NULL },
  { &MEDIASUBTYPE_ffds, CODEC_ID_MPEG4,  MAKEFOURCC('f','f','d','s'),  NULL },
  { &MEDIASUBTYPE_FVFW, CODEC_ID_MPEG4,  MAKEFOURCC('F','V','F','W'),  NULL },
  { &MEDIASUBTYPE_fvfw, CODEC_ID_MPEG4,  MAKEFOURCC('f','v','f','w'),  NULL },
  { &MEDIASUBTYPE_DXGM, CODEC_ID_MPEG4,  MAKEFOURCC('D','X','G','M'),  NULL },
  { &MEDIASUBTYPE_dxgm, CODEC_ID_MPEG4,  MAKEFOURCC('d','x','g','m'),  NULL },
  { &MEDIASUBTYPE_FMP4, CODEC_ID_MPEG4,  MAKEFOURCC('F','M','P','4'),  NULL },
  { &MEDIASUBTYPE_fmp4, CODEC_ID_MPEG4,  MAKEFOURCC('f','m','p','4'),  NULL },
  { &MEDIASUBTYPE_HDX4, CODEC_ID_MPEG4,  MAKEFOURCC('H','D','X','4'),  NULL },
  { &MEDIASUBTYPE_hdx4, CODEC_ID_MPEG4,  MAKEFOURCC('h','d','x','4'),  NULL },
  { &MEDIASUBTYPE_LMP4, CODEC_ID_MPEG4,  MAKEFOURCC('L','M','P','4'),  NULL },
  { &MEDIASUBTYPE_lmp4, CODEC_ID_MPEG4,  MAKEFOURCC('l','m','p','4'),  NULL },
  { &MEDIASUBTYPE_NDIG, CODEC_ID_MPEG4,  MAKEFOURCC('N','D','I','G'),  NULL },
  { &MEDIASUBTYPE_ndig, CODEC_ID_MPEG4,  MAKEFOURCC('n','d','i','g'),  NULL },
  { &MEDIASUBTYPE_RMP4, CODEC_ID_MPEG4,  MAKEFOURCC('R','M','P','4'),  NULL },
  { &MEDIASUBTYPE_rmp4, CODEC_ID_MPEG4,  MAKEFOURCC('r','m','p','4'),  NULL },
  { &MEDIASUBTYPE_SMP4, CODEC_ID_MPEG4,  MAKEFOURCC('S','M','P','4'),  NULL },
  { &MEDIASUBTYPE_smp4, CODEC_ID_MPEG4,  MAKEFOURCC('s','m','p','4'),  NULL },
  { &MEDIASUBTYPE_SEDG, CODEC_ID_MPEG4,  MAKEFOURCC('S','E','D','G'),  NULL },
  { &MEDIASUBTYPE_sedg, CODEC_ID_MPEG4,  MAKEFOURCC('s','e','d','g'),  NULL },
  { &MEDIASUBTYPE_UMP4, CODEC_ID_MPEG4,  MAKEFOURCC('U','M','P','4'),  NULL },
  { &MEDIASUBTYPE_ump4, CODEC_ID_MPEG4,  MAKEFOURCC('u','m','p','4'),  NULL },
  { &MEDIASUBTYPE_WV1F, CODEC_ID_MPEG4,  MAKEFOURCC('W','V','1','F'),  NULL },
  { &MEDIASUBTYPE_wv1f, CODEC_ID_MPEG4,  MAKEFOURCC('w','v','1','f'),  NULL },
};

/* Important: the order should be exactly the same as in ffCodecs[] */
const AMOVIESETUP_MEDIATYPE CXBMCVideoDecFilter::sudPinTypesIn[] =
{
  // Flash video
  { &MEDIATYPE_Video, &MEDIASUBTYPE_FLV1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_flv1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_FLV4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_flv4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_VP6F   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_vp6f   },

  // VP5
  { &MEDIATYPE_Video, &MEDIASUBTYPE_VP50   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_vp50   },

  // VP6
  { &MEDIATYPE_Video, &MEDIASUBTYPE_VP60   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_vp60   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_VP61   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_vp61   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_VP62   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_vp62   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_VP6A   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_vp6a   },

  // Xvid
  { &MEDIATYPE_Video, &MEDIASUBTYPE_XVID   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_xvid   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_XVIX   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_xvix   },

  // DivX
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DX50   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_dx50   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DIVX   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_divx   },

  // WMV1/2/3
  { &MEDIATYPE_Video, &MEDIASUBTYPE_WMV1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_wmv1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_WMV2   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_wmv2   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_WMV3   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_wmv3   },

  // Mpeg 2
  { &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG2_VIDEO },

  // MSMPEG-4
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DIV3   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_div3   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DVX3   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_dvx3   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_MP43   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_mp43   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_COL1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_col1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DIV4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_div4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DIV5   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_div5   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DIV6   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_div6   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_AP41   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_ap41   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_MPG3   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_mpg3   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DIV2   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_div2   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_MP42   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_mp42   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_MPG4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_mpg4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DIV1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_div1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_MP41   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_mp41   },

  // AMV Video
  { &MEDIATYPE_Video, &MEDIASUBTYPE_AMVV   },

  // H264/AVC
  { &MEDIATYPE_Video, &MEDIASUBTYPE_H264   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_h264   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_X264   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_x264   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_VSSH   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_vssh   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DAVC   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_davc   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_PAVC   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_pavc   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_AVC1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_avc1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_H264_bis },

  // SVQ3
  { &MEDIATYPE_Video, &MEDIASUBTYPE_SVQ3   },

  // SVQ1
  { &MEDIATYPE_Video, &MEDIASUBTYPE_SVQ1   },

  // H263
  { &MEDIATYPE_Video, &MEDIASUBTYPE_H263   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_h263   },

  { &MEDIATYPE_Video, &MEDIASUBTYPE_S263   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_s263   },
  
  // Real video
  { &MEDIATYPE_Video, &MEDIASUBTYPE_RV10   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_RV20   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_RV30   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_RV40   },

  // Theora
  { &MEDIATYPE_Video, &MEDIASUBTYPE_THEORA },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_theora },

  // VC1
  { &MEDIATYPE_Video, &MEDIASUBTYPE_WVC1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_wvc1   },
  
  // IMPORTANT : some of the last MediaTypes present in next group may be not available in
  // the standalone filter (workaround to prevent GraphEdit crash).
  // Other MPEG-4
  { &MEDIATYPE_Video, &MEDIASUBTYPE_MP4V   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_mp4v   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_M4S2   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_m4s2   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_MP4S   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_mp4s   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_3IV1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_3iv1   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_3IV2   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_3iv2   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_3IVX   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_3ivx   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_BLZ0   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_blz0   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DM4V   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_dm4v   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_FFDS   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_ffds   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_FVFW   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_fvfw   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_DXGM   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_dxgm   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_FMP4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_fmp4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_HDX4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_hdx4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_LMP4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_lmp4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_NDIG   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_ndig   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_RMP4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_rmp4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_SMP4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_smp4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_SEDG   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_sedg   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_UMP4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_ump4   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_WV1F   },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_wv1f   }
};