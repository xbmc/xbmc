/*
 *  Copyright (C) 2016 Lauri Mylläri
 *      http://kodi.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ColorManager.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"

#include <math.h>
#include <vector>

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
  m_hProfile = nullptr;
#endif  //defined(HAVE_LCMS2)
}

#if defined(HAVE_LCMS2)
CColorManager::~CColorManager()
{
  if (m_hProfile)
  {
    cmsCloseProfile(m_hProfile);
    m_hProfile = nullptr;
  }
}
#else
CColorManager::~CColorManager() = default;
#endif //defined(HAVE_LCMS2)

bool CColorManager::IsEnabled() const
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("videoscreen.cmsenabled") && IsValid();
}

bool CColorManager::IsValid() const
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  if (!settings->GetBool("videoscreen.cmsenabled"))
    return true;

  int cmsmode = settings->GetInt("videoscreen.cmsmode");
  switch (cmsmode)
  {
  case CMS_MODE_3DLUT:
  {
    std::string fileName = settings->GetString("videoscreen.cms3dlut");
    if (fileName.empty())
      return false;
    if (!CFile::Exists(fileName))
      return false;
    return true;
  }
#if defined(HAVE_LCMS2)
  case CMS_MODE_PROFILE:
  {
    int cmslutsize = settings->GetInt("videoscreen.cmslutsize");
    if (cmslutsize <= 0)
      return false;
    return true;
  }
#endif
  default:
    return false;
  }
}

CMS_PRIMARIES videoFlagsToPrimaries(int flags)
{
  if (flags & CONF_FLAGS_COLPRI_BT709)
    return CMS_PRIMARIES_BT709;
  if (flags & CONF_FLAGS_COLPRI_BT2020)
    return CMS_PRIMARIES_BT2020;
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

bool CColorManager::Get3dLutSize(CMS_DATA_FORMAT format, int *clutSize, int *dataSize)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  int cmsmode = settings->GetInt("videoscreen.cmsmode");
  switch (cmsmode)
  {
  case CMS_MODE_3DLUT:
  {
    std::string fileName = settings->GetString("videoscreen.cms3dlut");
    if (fileName.empty())
      return false;

    int clutDimention;
    if (!Probe3dLut(fileName, &clutDimention))
      return false;

    if (clutSize)
      *clutSize = clutDimention;

    if (dataSize)
    {
      int bytesInSample = format == CMS_DATA_FMT_RGBA ? 4 : 3;
      *dataSize = sizeof(uint16_t) * clutDimention * clutDimention * clutDimention * bytesInSample;
    }
    return true;
  }
  case CMS_MODE_PROFILE:
  {
    int cmslutsize = settings->GetInt("videoscreen.cmslutsize");
    if (cmslutsize <= 0)
      return false;

    int clutDimention = 1 << cmslutsize;
    if (clutSize)
      *clutSize = clutDimention;

    if (dataSize)
    {
      int bytesInSample = format == CMS_DATA_FMT_RGBA ? 4 : 3;
      *dataSize = sizeof(uint16_t) * clutDimention * clutDimention * clutDimention * bytesInSample;
    }
    return true;
  }
  default:
    CLog::Log(LOGDEBUG, "ColorManager: unknown CMS mode %d", cmsmode);
    return false;
  }
}

bool CColorManager::GetVideo3dLut(int videoFlags, int *cmsToken, CMS_DATA_FORMAT format, int clutSize, uint16_t *clutData)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CMS_PRIMARIES videoPrimaries = videoFlagsToPrimaries(videoFlags);
  CLog::Log(LOGDEBUG, "ColorManager: video primaries: %d", (int)videoPrimaries);
  switch (settings->GetInt("videoscreen.cmsmode"))
  {
  case CMS_MODE_3DLUT:
    CLog::Log(LOGDEBUG, "ColorManager: CMS_MODE_3DLUT");
    m_cur3dlutFile = settings->GetString("videoscreen.cms3dlut");
    if (!Load3dLut(m_cur3dlutFile, format, clutSize, clutData))
      return false;
    m_curCmsMode = CMS_MODE_3DLUT;
    break;

  case CMS_MODE_PROFILE:
    CLog::Log(LOGDEBUG, "ColorManager: CMS_MODE_PROFILE");
#if defined(HAVE_LCMS2)
    {
      // check if display profile is not loaded, or has changed
      if (m_curIccProfile != settings->GetString("videoscreen.displayprofile"))
      {
        // free old profile if there is one
        if (m_hProfile)
          cmsCloseProfile(m_hProfile);
        // load profile
        m_hProfile = LoadIccDisplayProfile(settings->GetString("videoscreen.displayprofile"));
        if (!m_hProfile)
          return false;
        // detect blackpoint
        if (cmsDetectBlackPoint(&m_blackPoint, m_hProfile, INTENT_PERCEPTUAL, 0))
        {
          CLog::Log(LOGDEBUG, "ColorManager: black point: %f", m_blackPoint.Y);
        }
        m_curIccProfile = settings->GetString("videoscreen.displayprofile");
      }
      // create gamma curve
      cmsToneCurve* gammaCurve;
      m_m_curIccGammaMode = static_cast<CMS_TRC_TYPE>(settings->GetInt("videoscreen.cmsgammamode"));
      m_curIccGamma = settings->GetInt("videoscreen.cmsgamma");
      gammaCurve =
        CreateToneCurve(m_m_curIccGammaMode, m_curIccGamma/100.0f, m_blackPoint);

      // create source profile
      m_curIccWhitePoint = static_cast<CMS_WHITEPOINT>(settings->GetInt("videoscreen.cmswhitepoint"));
      m_curIccPrimaries = static_cast<CMS_PRIMARIES>(settings->GetInt("videoscreen.cmsprimaries"));
      CLog::Log(LOGDEBUG, "ColorManager: primaries setting: %d", (int)m_curIccPrimaries);
      if (m_curIccPrimaries == CMS_PRIMARIES_AUTO)
        m_curIccPrimaries = videoPrimaries;
      CLog::Log(LOGDEBUG, "ColorManager: source profile primaries: %d", (int)m_curIccPrimaries);
      cmsHPROFILE sourceProfile = CreateSourceProfile(m_curIccPrimaries, gammaCurve, m_curIccWhitePoint);

      // link profiles
      // TODO: intent selection, switch output to 16 bits?
      cmsSetAdaptationState(0.0);
      uint32_t fmt = format == CMS_DATA_FMT_RGBA ? TYPE_RGBA_FLT : TYPE_RGB_FLT;
      cmsHTRANSFORM deviceLink =  cmsCreateTransform(sourceProfile, fmt, m_hProfile, fmt, INTENT_ABSOLUTE_COLORIMETRIC, 0);

      // sample the transformation
      if (deviceLink)
        Create3dLut(deviceLink, format, clutSize, clutData);

      // free gamma curve, source profile and transformation
      if (deviceLink)
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
    CLog::Log(LOGDEBUG, "ColorManager: unknown CMS mode %d", settings->GetInt("videoscreen.cmsmode"));
    return false;
  }

  // set current state
  m_curVideoPrimaries = videoPrimaries;
  m_curClutSize = clutSize;
  *cmsToken = ++m_curCmsToken;
  return true;
}

bool CColorManager::CheckConfiguration(int cmsToken, int flags)
{
  if (cmsToken != m_curCmsToken)
    return false;
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (m_curCmsMode != settings->GetInt("videoscreen.cmsmode"))
    return false;   // CMS mode has changed
  switch (m_curCmsMode)
  {
  case CMS_MODE_3DLUT:
    if (m_cur3dlutFile != settings->GetString("videoscreen.cms3dlut"))
      return false; // different 3dlut file selected
    break;
  case CMS_MODE_PROFILE:
#if defined(HAVE_LCMS2)
    if (m_curIccProfile != settings->GetString("videoscreen.displayprofile"))
      return false; // different ICC profile selected
    if (m_curIccWhitePoint != settings->GetInt("videoscreen.cmswhitepoint"))
      return false; // whitepoint changed
    {
      CMS_PRIMARIES primaries = static_cast<CMS_PRIMARIES>(settings->GetInt("videoscreen.cmsprimaries"));
      if (primaries == CMS_PRIMARIES_AUTO) primaries = videoFlagsToPrimaries(flags);
      if (m_curIccPrimaries != primaries)
        return false; // primaries changed
    }
    if (m_m_curIccGammaMode != static_cast<CMS_TRC_TYPE>(settings->GetInt("videoscreen.cmsgammamode")))
      return false; // gamma mode changed
    if (m_curIccGamma != settings->GetInt("videoscreen.cmsgamma"))
      return false; // effective gamma changed
    if (m_curClutSize != 1 << settings->GetInt("videoscreen.cmslutsize"))
      return false; // CLUT size changed
    // TODO: check other parameters
#else   //defined(HAVE_LCMS2)
    return true;
#endif  //defined(HAVE_LCMS2)
    break;
  default:
    CLog::Log(LOGERROR, "ColorManager: unexpected CMS mode: %d", m_curCmsMode);
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

bool CColorManager::Probe3dLut(const std::string filename, int *clutSize)
{
  struct H3DLUT header;
  CFile lutFile;

  if (!lutFile.Open(filename))
  {
    CLog::Log(LOGERROR, "%s: Could not open 3DLUT file: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  if (lutFile.Read(&header, sizeof(header)) < static_cast<ssize_t>(sizeof(header)))
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

  int rSize = 1 << header.inputBitDepth[0];
  int gSize = 1 << header.inputBitDepth[1];
  int bSize = 1 << header.inputBitDepth[2];
  if (rSize != gSize || rSize != bSize)
  {
    CLog::Log(LOGERROR, "%s: Different channel resolutions unsupported: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  if (clutSize)
    *clutSize = rSize;

  lutFile.Close();
  return true;
}

bool CColorManager::Load3dLut(const std::string filename, CMS_DATA_FORMAT format, int CLUTsize, uint16_t *clutData)
{
  struct H3DLUT header;
  CFile lutFile;

  if (!lutFile.Open(filename))
  {
    CLog::Log(LOGERROR, "%s: Could not open 3DLUT file: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  if (lutFile.Read(&header, sizeof(header)) < static_cast<ssize_t>(sizeof(header)))
  {
    CLog::Log(LOGERROR, "%s: Could not read 3DLUT header: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  int rSize = 1 << header.inputBitDepth[0];
  int gSize = 1 << header.inputBitDepth[1];
  int bSize = 1 << header.inputBitDepth[2];

  if ( rSize != CLUTsize || rSize != gSize || rSize != bSize)
  {
    CLog::Log(LOGERROR, "%s: Different channel resolutions unsupported: %s", __FUNCTION__, filename.c_str());
    return false;
  }

  lutFile.Seek(header.lutFileOffset, SEEK_SET);

  int components = format == CMS_DATA_FMT_RGBA ? 4 : 3;
  for (int rIndex = 0; rIndex < rSize; rIndex++)
  {
    for (int gIndex = 0; gIndex < gSize; gIndex++)
    {
      std::vector<uint16_t> input(bSize * 3); // always 3 components
      lutFile.Read(input.data(), input.size() * sizeof(input[0]));
      int index = (rIndex + gIndex * rSize) * components;
      for (int bIndex = 0; bIndex < bSize; bIndex++)
      {
        int offset = index + bIndex * rSize * gSize * components;
        clutData[offset + 0] = input[bIndex * 3 + 2];
        clutData[offset + 1] = input[bIndex * 3 + 1];
        clutData[offset + 2] = input[bIndex * 3 + 0];
        if (format == CMS_DATA_FMT_RGBA)
          clutData[offset + 3] = 0xFFFF;
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
    CLog::Log(LOGERROR, "ICC profile not found");
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
      CLog::Log(LOGINFO, "calculated technical gamma %0.3f (50%% target %0.4f, output %0.4f)",
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
    CLog::Log(LOGERROR, "gamma type %d not implemented", gammaType);
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
      0.155, 0.070, 1.0 },
    { 0.708, 0.292, 1.0,        // BT.2020 UHDTV
      0.170, 0.797, 1.0,
      0.131, 0.046, 1.0 }
  };

  Gamma3[0] = Gamma3[1] = Gamma3[2] = gamma;
  hProfile = cmsCreateRGBProfile(&whiteCoords[whitepoint],
      &primaryCoords[primaries],
      Gamma3);
  return hProfile;
}


void CColorManager::Create3dLut(cmsHTRANSFORM transform, CMS_DATA_FORMAT format, int clutSize, uint16_t *clutData)
{
  const int lutResolution = clutSize;
  int components = format == CMS_DATA_FMT_RGBA ? 4 : 3;

  cmsFloat32Number *input = new cmsFloat32Number[components*lutResolution];
  cmsFloat32Number *output = new cmsFloat32Number[components*lutResolution];

#define clamp(x, l, h) ( ((x) < (l)) ? (l) : ( ((x) > (h)) ? (h) : (x) ) )
#define videoToPC(x) ( clamp((((x)*255)-16)/219,0,1) )
#define PCToVideo(x) ( (((x)*219)+16)/255 )

  for (int bIndex=0; bIndex<lutResolution; bIndex++)
  {
    for (int gIndex=0; gIndex<lutResolution; gIndex++)
    {
      for (int rIndex=0; rIndex<lutResolution; rIndex++)
      {
        int offset = rIndex * components;
        input[offset + 0] = videoToPC(rIndex / (lutResolution-1.0));
        input[offset + 1] = videoToPC(gIndex / (lutResolution-1.0));
        input[offset + 2] = videoToPC(bIndex / (lutResolution-1.0));
        if (format == CMS_DATA_FMT_RGBA)
          input[offset + 3] = 0.0f;
      }
      int index = (bIndex*lutResolution*lutResolution + gIndex*lutResolution)*components;
      cmsDoTransform(transform, input, output, lutResolution);
      for (int i=0; i < lutResolution * components; i++)
      {
        clutData[index + i] = PCToVideo(output[i]) * 65535;
      }
    }
  }

  for (int y=0; y<lutResolution; y+=1)
  {
    int index = components*(y*lutResolution*lutResolution + y*lutResolution + y);
    CLog::Log(LOGDEBUG, "  %d (%d): %d %d %d",
        (int)round(y * 255 / (lutResolution-1.0)), y,
        (int)round(clutData[index+0]),
        (int)round(clutData[index+1]),
        (int)round(clutData[index+2]));
  }
  delete[] input;
  delete[] output;
}

#endif //defined(HAVE_LCMS2)
