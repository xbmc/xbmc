/*
 *      Copyright (C) 2016 Lauri Mylläri
 *      http://kodi.org
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

#include <math.h>
#include <string>
#include <vector>

#include "system.h"
#include "ColorManager.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

using namespace XFILE;

CColorManager::CColorManager()
{
  m_curVideoPrimaries = CMS_PRIMARIES_AUTO;
  m_curClutSize = 0;
  m_curCmsToken = 0;
  m_curCmsMode = 0;
  m_cur3dlutFile = "";
  m_curIccProfile = "";
#if defined(HAVE_LCMS2)
  m_hProfile = NULL;
#endif  //defined(HAVE_LCMS2)
}

CColorManager::~CColorManager()
{
#if defined(HAVE_LCMS2)
  if (m_hProfile)
  {
    cmsCloseProfile(m_hProfile);
    m_hProfile = NULL;
  }
#endif  //defined(HAVE_LCMS2)
}

bool CColorManager::IsEnabled()
{
  //TODO: check that the configuration is valid here (files exist etc)

  return CSettings::GetInstance().GetBool("videoscreen.cmsenabled");
}

CMS_PRIMARIES videoFlagsToPrimaries(int flags)
{
  if (flags & CONF_FLAGS_COLPRI_BT709)
    return CMS_PRIMARIES_BT709;
  if (flags & CONF_FLAGS_COLPRI_170M)
    return CMS_PRIMARIES_170M;
  if (flags & CONF_FLAGS_COLPRI_BT470M)
    return CMS_PRIMARIES_BT470M;
  if (flags & CONF_FLAGS_COLPRI_BT470BG)
    return CMS_PRIMARIES_BT470BG;
  if (flags & CONF_FLAGS_COLPRI_240M)
    return CMS_PRIMARIES_240M;
  return CMS_PRIMARIES_BT709; // default to bt.709
}

bool CColorManager::GetVideo3dLut(int videoFlags, int *cmsToken, int *clutSize, uint16_t **clutData)
{
  CMS_PRIMARIES videoPrimaries = videoFlagsToPrimaries(videoFlags);
  CLog::Log(LOGDEBUG, "video primaries: %d\n", (int)videoPrimaries);
  switch (CSettings::GetInstance().GetInt("videoscreen.cmsmode"))
  {
  case CMS_MODE_3DLUT:
    CLog::Log(LOGDEBUG, "ColorManager: CMS_MODE_3DLUT\n");
    m_cur3dlutFile = CSettings::GetInstance().GetString("videoscreen.cms3dlut");
    if (!Load3dLut(m_cur3dlutFile, clutData, clutSize))
      return false;
    m_curCmsMode = CMS_MODE_3DLUT;
    break;

  case CMS_MODE_PROFILE:
    CLog::Log(LOGDEBUG, "ColorManager: CMS_MODE_PROFILE\n");
#if defined(HAVE_LCMS2)
    {
      bool changed = false;
      // check if display profile is not loaded, or has changed
      if (m_curIccProfile != CSettings::GetInstance().GetString("videoscreen.displayprofile"))
      {
        changed = true;
        // free old profile if there is one
        if (m_hProfile)
          cmsCloseProfile(m_hProfile);
        // load profile
        m_hProfile = LoadIccDisplayProfile(CSettings::GetInstance().GetString("videoscreen.displayprofile"));
        if (!m_hProfile)
          return false;
        // detect blackpoint
        if (cmsDetectBlackPoint(&m_blackPoint, m_hProfile, INTENT_PERCEPTUAL, 0))
        {
          CLog::Log(LOGDEBUG, "black point: %f\n", m_blackPoint.Y);
        }
        m_curIccProfile = CSettings::GetInstance().GetString("videoscreen.displayprofile");
      }
      // create gamma curve
      cmsToneCurve* gammaCurve;
      m_m_curIccGammaMode = (CMS_TRC_TYPE)CSettings::GetInstance().GetInt("videoscreen.cmsgammamode");
      m_curIccGamma = CSettings::GetInstance().GetInt("videoscreen.cmsgamma");
      gammaCurve =
        CreateToneCurve(m_m_curIccGammaMode, m_curIccGamma/100.0f, m_blackPoint);

      // create source profile
      m_curIccWhitePoint = (CMS_WHITEPOINT)CSettings::GetInstance().GetInt("videoscreen.cmswhitepoint");
      m_curIccPrimaries = (CMS_PRIMARIES)CSettings::GetInstance().GetInt("videoscreen.cmsprimaries");
      CLog::Log(LOGDEBUG, "primaries setting: %d\n", (int)m_curIccPrimaries);
      if (m_curIccPrimaries == CMS_PRIMARIES_AUTO) m_curIccPrimaries = videoPrimaries;
      CLog::Log(LOGDEBUG, "source profile primaries: %d\n", (int)m_curIccPrimaries);
      cmsHPROFILE sourceProfile =
        CreateSourceProfile(m_curIccPrimaries, gammaCurve, m_curIccWhitePoint);

      // link profiles
      // TODO: intent selection, switch output to 16 bits?
      cmsSetAdaptationState(0.0);
      cmsHTRANSFORM deviceLink =
        cmsCreateTransform(sourceProfile, TYPE_RGB_FLT,
            m_hProfile, TYPE_RGB_FLT,
            INTENT_ABSOLUTE_COLORIMETRIC, 0);

      // sample the transformation
      *clutSize = 1 << CSettings::GetInstance().GetInt("videoscreen.cmslutsize");
      Create3dLut(deviceLink, clutData, clutSize);

      // free gamma curve, source profile and transformation
      cmsDeleteTransform(deviceLink);
      cmsCloseProfile(sourceProfile);
      cmsFreeToneCurve(gammaCurve);
    }

    m_curCmsMode = CMS_MODE_PROFILE;
    break;
#else   //defined(HAVE_LCMS2)
    return false;
#endif  //defined(HAVE_LCMS2)

  default:
    CLog::Log(LOGDEBUG, "ColorManager: unknown CMS mode %d\n", CSettings::GetInstance().GetInt("videoscreen.cmsmode"));
    return false;
  }

  // set current state
  m_curVideoPrimaries = videoPrimaries;
  m_curClutSize = *clutSize;
  *cmsToken = ++m_curCmsToken;
  return true;
}

bool CColorManager::CheckConfiguration(int cmsToken, int flags)
{
  if (cmsToken != m_curCmsToken)
    return false;
  if (m_curCmsMode != CSettings::GetInstance().GetInt("videoscreen.cmsmode"))
    return false;   // CMS mode has changed
  switch (m_curCmsMode)
  {
  case CMS_MODE_3DLUT:
    if (m_cur3dlutFile != CSettings::GetInstance().GetString("videoscreen.cms3dlut"))
      return false; // different 3dlut file selected
    break;
  case CMS_MODE_PROFILE:
#if defined(HAVE_LCMS2)
    if (m_curIccProfile != CSettings::GetInstance().GetString("videoscreen.displayprofile"))
      return false; // different ICC profile selected
    if (m_curIccWhitePoint != CSettings::GetInstance().GetInt("videoscreen.cmswhitepoint"))
      return false; // whitepoint changed
    {
      CMS_PRIMARIES primaries = (CMS_PRIMARIES)CSettings::GetInstance().GetInt("videoscreen.cmsprimaries");
      if (primaries == CMS_PRIMARIES_AUTO) primaries = videoFlagsToPrimaries(flags);
      if (m_curIccPrimaries != primaries)
        return false; // primaries changed
    }
    if (m_m_curIccGammaMode != (CMS_TRC_TYPE)CSettings::GetInstance().GetInt("videoscreen.cmsgammamode"))
      return false; // gamma mode changed
    if (m_curIccGamma != CSettings::GetInstance().GetInt("videoscreen.cmsgamma"))
      return false; // effective gamma changed
    if (m_curClutSize != 1 << CSettings::GetInstance().GetInt("videoscreen.cmslutsize"))
      return false; // CLUT size changed
    // TODO: check other parameters
#else   //defined(HAVE_LCMS2)
    return true;
#endif  //defined(HAVE_LCMS2)
    break;
  default:
    CLog::Log(LOGERROR, "%s: unexpected CMS mode: %d", __FUNCTION__, m_curCmsMode);
    return false;
  }
  return true;
}



// madvr 3dlut file format support
struct H3DLUT
{
  char signature[4];            // file signature; must be: '3DLT'
  uint32_t fileVersion;         // file format version number (currently "1")
  char programName[32];         // name of the program that created the file
  uint64_t programVersion;      // version number of the program that created the file
  uint32_t inputBitDepth[3];    // input bit depth per component (Y,Cb,Cr or R,G,B)
  uint32_t inputColorEncoding;  // input color encoding standard
  uint32_t outputBitDepth;      // output bit depth for all components (valid values are 8, 16 and 32)
  uint32_t outputColorEncoding; // output color encoding standard
  uint32_t parametersFileOffset;// number of bytes between the beginning of the file and array parametersData
  uint32_t parametersSize;      // size in bytes of the array parametersData
  uint32_t lutFileOffset;       // number of bytes between the beginning of the file and array lutData
  uint32_t lutCompressionMethod;// type of compression used if any (0 = none, ...)
  uint32_t lutCompressedSize;   // size in bytes of the array lutData inside the file, whether compressed or not
  uint32_t lutUncompressedSize; // true size in bytes of the array lutData when in memory for usage (outside the file)
  // This header is followed by the char array 'parametersData', of length 'parametersSize',
  // and by the array 'lutDataxx', of length 'lutCompressedSize'.
};

bool CColorManager::Probe3dLut(const std::string filename)
{
  struct H3DLUT header;
  CFile lutFile;

  if (!lutFile.Open(filename))
  {
    CLog::Log(LOGERROR, "%s: Could not open 3DLUT file: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  if (lutFile.Read(&header, sizeof(header)) < sizeof(header))
  {
    CLog::Log(LOGERROR, "%s: Could not read 3DLUT header: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  if ( !(header.signature[0]=='3'
        && header.signature[1]=='D'
        && header.signature[2]=='L'
        && header.signature[3]=='T') )
  {
    CLog::Log(LOGERROR, "%s: Not a 3DLUT file: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  if ( header.fileVersion != 1
      || header.lutCompressionMethod != 0
      || header.inputColorEncoding != 0
      || header.outputColorEncoding != 0 )
  {
    CLog::Log(LOGERROR, "%s: Unsupported 3DLUT file: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  lutFile.Close();
  return true;
}

bool CColorManager::Load3dLut(const std::string filename, uint16_t **CLUT, int *CLUTsize)
{
  struct H3DLUT header;
  CFile lutFile;

  if (!lutFile.Open(filename))
  {
    CLog::Log(LOGERROR, "%s: Could not open 3DLUT file: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  if (lutFile.Read(&header, sizeof(header)) < sizeof(header))
  {
    CLog::Log(LOGERROR, "%s: Could not read 3DLUT header: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  int rSize = 1 << header.inputBitDepth[0];
  int gSize = 1 << header.inputBitDepth[1];
  int bSize = 1 << header.inputBitDepth[2];

  if ( !((rSize == gSize) && (rSize == bSize)) )
  {
    CLog::Log(LOGERROR, "%s: Different channel resolutions unsupported: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  int lutsamples = rSize * gSize * bSize * 3;
  *CLUTsize = rSize; // TODO: assumes cube
  *CLUT = (uint16_t*)malloc(lutsamples * sizeof(uint16_t));

  lutFile.Seek(header.lutFileOffset, SEEK_SET);

  for (int rIndex=0; rIndex<rSize; rIndex++) {
    for (int gIndex=0; gIndex<gSize; gIndex++) {
      std::vector<uint16_t> input(bSize*3);
      lutFile.Read(input.data(), input.size()*sizeof(input[0]));
      int index = (rIndex + gIndex*rSize)*3;
      for (int bIndex=0; bIndex<bSize; bIndex++) {
        (*CLUT)[index+bIndex*rSize*gSize*3+0] = input[bIndex*3+2];
        (*CLUT)[index+bIndex*rSize*gSize*3+1] = input[bIndex*3+1];
        (*CLUT)[index+bIndex*rSize*gSize*3+2] = input[bIndex*3+0];
      }
    }
  }

  lutFile.Close();

  return true;
}



#if defined(HAVE_LCMS2)
// ICC profile support

cmsHPROFILE CColorManager::LoadIccDisplayProfile(const std::string filename)
{
  cmsHPROFILE hProfile;

  hProfile = cmsOpenProfileFromFile(filename.c_str(), "r");
  if (!hProfile)
  {
    CLog::Log(LOGERROR, "ICC profile not found\n");
  }
  return hProfile;
}


cmsToneCurve* CColorManager::CreateToneCurve(CMS_TRC_TYPE gammaType, float gammaValue, cmsCIEXYZ blackPoint)
{
  const int tableSize = 1024;
  cmsFloat32Number gammaTable[tableSize];

  switch (gammaType)
  {
  case CMS_TRC_INPUT_OFFSET:
    // calculate gamma to match effective gamma provided, then fall through to bt.1886
    {
      double effectiveGamma = gammaValue;
      double gammaLow = effectiveGamma;  // low limit for infinite contrast ratio
      double gammaHigh = 3.2;            // high limit for 2.4 gamma on 200:1 contrast ratio
      double gammaGuess = 0.0;
#define TARGET(gamma) (pow(0.5, (gamma)))
#define GAIN(bkpt, gamma) (pow(1-pow((bkpt), 1/(gamma)), (gamma)))
#define LIFT(bkpt, gamma) (pow((bkpt), 1/(gamma)) / (1-pow((bkpt), 1/(gamma))))
#define HALFPT(bkpt, gamma) (GAIN(bkpt, gamma)*pow(0.5+LIFT(bkpt, gamma), gamma))
      for (int i=0; i<3; i++)
      {
        // calculate 50% output for gammaLow and gammaHigh, compare to target 50% output
        gammaGuess = gammaLow + (gammaHigh-gammaLow)
            * ((HALFPT(blackPoint.Y, gammaLow)-TARGET(effectiveGamma))
                / (HALFPT(blackPoint.Y, gammaLow)-HALFPT(blackPoint.Y, gammaHigh)));
        if (HALFPT(blackPoint.Y, gammaGuess) < TARGET(effectiveGamma))
        {
          // guess is too high
          // move low limit half way to guess
          gammaLow = gammaLow + (gammaGuess-gammaLow)/2;
          // set high limit to guess
          gammaHigh = gammaGuess;
        }
        else
        {
          // guess is too low
          // set low limit to guess
          gammaLow = gammaGuess;
          // move high limit half way to guess
          gammaHigh = gammaHigh + (gammaGuess-gammaLow)/2;
        }
      }
      gammaValue = gammaGuess;
      CLog::Log(LOGINFO, "calculated technical gamma %0.3f (50%% target %0.4f, output %0.4f)\n",
        gammaValue,
        TARGET(effectiveGamma),
        HALFPT(blackPoint.Y, gammaValue));
#undef TARGET
#undef GAIN
#undef LIFT
#undef HALFPT
    }
    // fall through to bt.1886 with calculated technical gamma

  case CMS_TRC_BT1886:
    {
      double bkipow = pow(blackPoint.Y, 1.0/gammaValue);
      double wtipow = 1.0;
      double lift = bkipow / (wtipow - bkipow);
      double gain = pow(wtipow - bkipow, gammaValue);
      for (int i=0; i<tableSize; i++)
      {
        gammaTable[i] = gain * pow(((double) i)/(tableSize-1) + lift, gammaValue);
      }
    }
    break;

  case CMS_TRC_OUTPUT_OFFSET:
    {
      double gain = 1-blackPoint.Y;
      // TODO: here gamma is adjusted to match absolute gamma output at 50%
      //  - is it a good idea or should the provided gamma be kept?
      double adjustedGamma = log(gain/(gain+pow(2,-gammaValue)-1))/log(2);
      for (int i=0; i<tableSize; i++)
      {
        gammaTable[i] = gain * pow(((double) i)/(tableSize-1), adjustedGamma) + blackPoint.Y;
      }
    }
    break;

  case CMS_TRC_ABSOLUTE:
    {
      for (int i=0; i<tableSize; i++)
      {
        gammaTable[i] = fmax(blackPoint.Y, pow(((double) i)/(tableSize-1), gammaValue));
      }
    }
    break;

  default:
    CLog::Log(LOGERROR, "gamma type %d not implemented\n", gammaType);
  }

  cmsToneCurve* result = cmsBuildTabulatedToneCurveFloat(0,
      tableSize,
      gammaTable);
  return result;
}


cmsHPROFILE CColorManager::CreateSourceProfile(CMS_PRIMARIES primaries, cmsToneCurve *gamma, CMS_WHITEPOINT whitepoint)
{
  cmsToneCurve*  Gamma3[3];
  cmsHPROFILE hProfile;
  cmsCIExyY whiteCoords[] = {
    { 0.3127, 0.3290, 1.0 },    // D65 as specified in BT.709
    { 0.2830, 0.2980, 1.0 }     // Japanese D93 - is there a definitive source? NHK? ARIB TR-B9?
  };
  cmsCIExyYTRIPLE primaryCoords[] = {
    { 0.640, 0.330, 1.0,        // auto setting, these should not be used (BT.709 just in case)
      0.300, 0.600, 1.0,
      0.150, 0.060, 1.0 },
    { 0.640, 0.330, 1.0,        // BT.709 (HDTV, sRGB)
      0.300, 0.600, 1.0,
      0.150, 0.060, 1.0 },
    { 0.630, 0.340, 1.0,        // SMPTE 170M (SDTV)
      0.310, 0.595, 1.0,
      0.155, 0.070, 1.0 },
    { 0.670, 0.330, 1.0,        // BT.470 M (obsolete NTSC 1953)
      0.210, 0.710, 1.0,
      0.140, 0.080, 1.0 },
    { 0.640, 0.330, 1.0,        // BT.470 B/G (obsolete PAL/SECAM 1975)
      0.290, 0.600, 1.0,
      0.150, 0.060, 1.0 },
    { 0.630, 0.340, 1.0,        // SMPTE 240M (obsolete HDTV 1988)
      0.310, 0.595, 1.0,
      0.155, 0.070, 1.0 }
  };

  Gamma3[0] = Gamma3[1] = Gamma3[2] = gamma;
  hProfile = cmsCreateRGBProfile(&whiteCoords[whitepoint],
      &primaryCoords[primaries],
      Gamma3);
  return hProfile;
}


void CColorManager::Create3dLut(cmsHTRANSFORM transform, uint16_t **clutData, int *clutSize)
{
  const int lutResolution = *clutSize;
  int lutsamples = lutResolution * lutResolution * lutResolution * 3;
  *clutData = (uint16_t*)malloc(lutsamples * sizeof(uint16_t));

  cmsFloat32Number input[3*lutResolution];
  cmsFloat32Number output[3*lutResolution];

#define clamp(x, l, h) ( ((x) < (l)) ? (l) : ( ((x) > (h)) ? (h) : (x) ) )
#define videoToPC(x) ( clamp((((x)*255)-16)/219,0,1) )
#define PCToVideo(x) ( (((x)*219)+16)/255 )
// #define videoToPC(x) ( x )
// #define PCToVideo(x) ( x )
  for (int bIndex=0; bIndex<lutResolution; bIndex++) {
    for (int gIndex=0; gIndex<lutResolution; gIndex++) {
      for (int rIndex=0; rIndex<lutResolution; rIndex++) {
        input[rIndex*3+0] = videoToPC(rIndex / (lutResolution-1.0));
        input[rIndex*3+1] = videoToPC(gIndex / (lutResolution-1.0));
        input[rIndex*3+2] = videoToPC(bIndex / (lutResolution-1.0));
      }
      int index = (bIndex*lutResolution*lutResolution + gIndex*lutResolution)*3;
      cmsDoTransform(transform, input, output, lutResolution);
      for (int i=0; i<lutResolution*3; i++) {
        (*clutData)[index+i] = PCToVideo(output[i]) * 65535;
      }
    }
  }

  for (int y=0; y<lutResolution; y+=1)
  {
    int index = 3*(y*lutResolution*lutResolution + y*lutResolution + y);
    CLog::Log(LOGDEBUG, "  %d (%d): %d %d %d\n",
        (int)round(y * 255 / (lutResolution-1.0)), y,
        (int)round((*clutData)[index+0]),
        (int)round((*clutData)[index+1]),
        (int)round((*clutData)[index+2]));
  }
}


#endif //defined(HAVE_LCMS2)
