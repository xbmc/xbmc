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
// MadvrSettings.h: interface for the CMadvrSettings class.
//
//////////////////////////////////////////////////////////////////////

//#if !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
//#define AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#pragma once

typedef struct
{
  const std::string name;
  const int label;
  bool chromaUp;
  bool lumaUp;
  bool lumaDown;
} MADVR_SCALING;

typedef struct
{
  const std::string name;
  const int label;
} MADVR_SETTINGS;

typedef struct
{
  const std::string name;
  const std::string algo;
  const int id;
  const int label;
} MADVR_DOUBLE_QUALITY;


static const int ChromaUpDef = 7; // BICUBIC75
static const int LumaUpDef = 14; // LANCZOS3
static const int LumaDownDef = 4; // CATMUL-ROM
static const MADVR_SCALING MadvrScaling[] =
{
  { "Nearest Neighbor", 70001, true, true, true },
  { "Bilinear", 70002, true, true, true },
  { "Dvxa", 70003, false, true, true },
  { "Mitchell-Netravali", 70004, true, true, true },
  { "Catmull-Rom", 70005, true, true, true },
  { "Bicubic50", 70006, true, true, true },
  { "Bicubic60", 70007, true, true, true },
  { "Bicubic75", 70008, true, true, true },
  { "Bicubic100", 70009, true, true, true },
  { "SoftCubic50", 70010, true, true, true },
  { "SoftCubic60", 70011, true, true, true },
  { "SoftCubic70", 70012, true, true, true },
  { "SoftCubic80", 70013, true, true, true },
  { "SoftCubic100", 70014, true, true, true },
  { "Lanczos3", 70015, true, true, true },
  { "Lanczos4", 70016, true, true, true },
  { "Lanczos8", 70017, false, false, false },
  { "Spline36", 70018, true, true, true },
  { "Spline64", 70019, true, true, true },
  { "Jinc3", 70020, true, true, false },
  { "Jinc4", 70021, false, false, false },
  { "Jinc8", 70022, false, false, false },
  { "Bilateral", 70033, true, false, false },
  { "SuperXbr25", 70034, true, false, false },
  { "SuperXbr50", 70035, true, false, false },
  { "SuperXbr75", 70036, true, false, false },
  { "SuperXbr100", 70037, true, false, false },
  { "SuperXbr125", 70038, true, false, false },
  { "SuperXbr150", 70039, true, false, false },
  { "Nedi", 70040, true, false, false },
  { "Nnedi16", 70023, true, false, false },
  { "Nnedi32", 70024, true, false, false },
  { "Nnedi64", 70025, true, false, false },
  { "Nnedi128", 70026, true, false, false },
  { "Nnedi256", 70027, true, false, false },
};

static const MADVR_DOUBLE_QUALITY MadvrDoubleQuality[] =
{
  { "SuperXbr25", "SuperXbr25", 0, 70034 },
  { "SuperXbr50", "SuperXbr50", 1, 70035 },
  { "SuperXbr75", "SuperXbr75", 2, 70036 },
  { "SuperXbr100", "SuperXbr100", 3, 70037 },
  { "SuperXbr125", "SuperXbr125", 4, 70038 },
  { "SuperXbr150", "SuperXbr150", 5, 70039 },
  { "SuperXbr150", "SuperXbr150", 6, 70039 },
  { "NEDI", "NEDI", 0, 70040 },
  { "16Neurons", "NNEDI3", 0, 70023 },
  { "32Neurons", "NNEDI3", 1, 70024 },
  { "64Neurons", "NNEDI3", 2, 70025 },
  { "128Neurons", "NNEDI3", 3, 70026 },
  { "256Neurons", "NNEDI3", 4, 70027 },
};

static const int MadvrDoubleFactorDef = 1; //1.5x
static const MADVR_SETTINGS MadvrDoubleFactor[] =
{
  { "2.0x", 70109 },
  { "1.5x", 70110 },
  { "1.2x", 70111 },
  { "always", 70112 }
};

static const int MadvrQuadrupleFactorDef = 1; //3.0x
static const MADVR_SETTINGS MadvrQuadrupleFactor[] =
{
  { "4.0x", 70113 },
  { "3.0x", 70114 },
  { "2.4x", 70115 },
  { "always", 70112 }
};

static const int MadvrDeintForceDef = 0; //AUTO
static const MADVR_SETTINGS MadvrDeintForce[] =
{
  { "auto", 70202 },
  { "film", 70203 },
  { "video", 70204 }
};

static const int MadvrDeintActiveDef = 1; //AUTO deactive
static const MADVR_SETTINGS MadvrDeintActive[] =
{
  { "ifdoubt_active", 70205 },
  { "ifdoubt_deactive", 70206 }
};

static const MADVR_SETTINGS MadvrSmoothMotion[] =
{
  { "avoidJudder", 70301 },
  { "almostAlways", 70302 },
  { "always", 70303 }
};

static const int MadvrDitheringDef = 1; //ORDERED
static const MADVR_SETTINGS MadvrDithering[] =
{
  { "random", 70401 },
  { "ordered", 70402 },
  { "errorDifMedNoise", 70403 },
  { "errorDifLowNoise", 70404 }
};

static const int MadvrDebandLevelDef = 0; //LOW
static const int MadvrDebandFadeLevelDef = 2; //HIGH
static const MADVR_SETTINGS MadvrDeband[] =
{
  { "debandlow", 70503 },
  { "debandmedium", 70504 },
  { "debandhigh", 70505 }
};

enum MADVR_RES_SETTINGS
{
  MADVR_RES_SD,
  MADVR_RES_720,
  MADVR_RES_1080,
  MADVR_RES_2160,
  MADVR_RES_ALL
};

enum MADVR_LOAD_TYPE
{
  MADVR_LOAD_GENERAL,
  MADVR_LOAD_SCALING
};


class CMadvrSettings
{
public:
  CMadvrSettings();
  ~CMadvrSettings() {};

  bool operator!=(const CMadvrSettings &right) const;

  static int GetSettingId(const MADVR_SETTINGS *setsArray, std::string sValue);
  static int GetScalingId(std::string sValue);
  static int GeDoubleAlgo(std::string sValue);
  static int GetDoubleId(int iValue);

  int m_Resolution;

  int m_ChromaUpscaling;
  bool m_ChromaAntiRing;
  bool m_ChromaSuperRes;

  int m_ImageUpscaling;
  bool m_ImageUpAntiRing;
  bool m_ImageUpLinear;

  int m_ImageDownscaling;
  bool m_ImageDownAntiRing;
  bool m_ImageDownLinear;

  int m_ImageDoubleLuma;
  int m_ImageDoubleChroma;
  int m_ImageQuadrupleLuma;
  int m_ImageQuadrupleChroma;

  int m_ImageDoubleLumaFactor;
  int m_ImageDoubleChromaFactor;
  int m_ImageQuadrupleLumaFactor;
  int m_ImageQuadrupleChromaFactor;

  int m_deintactive;
  int m_deintforce;
  bool m_deintlookpixels;

  int m_smoothMotion;

  int m_dithering;
  bool m_ditheringColoredNoise;
  bool m_ditheringEveryFrame;

  bool m_deband;
  int m_debandLevel;
  int m_debandFadeLevel;

  bool m_fineSharp;
  float m_fineSharpStrength;
  bool m_lumaSharpen;
  float m_lumaSharpenStrength;
  bool m_adaptiveSharpen;
  float m_adaptiveSharpenStrength;

  bool m_UpRefFineSharp;
  float m_UpRefFineSharpStrength;
  bool m_UpRefLumaSharpen;
  float m_UpRefLumaSharpenStrength;
  bool m_UpRefAdaptiveSharpen;
  float m_UpRefAdaptiveSharpenStrength;
  bool m_superRes;
  float m_superResStrength;

  bool m_refineOnce;
  bool m_superResFirst;

private:
};

//#endif // !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
