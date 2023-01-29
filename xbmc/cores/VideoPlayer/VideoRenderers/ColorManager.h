/*
 *  Copyright (C) 2016 Lauri Mylläri
 *      http://kodi.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if defined(HAVE_LCMS2)
#include <lcms2.h>
#endif

#include <cstdint>
#include <string>

extern "C"
{
#include <libavutil/pixfmt.h>
}

enum CMS_DATA_FORMAT
{
  CMS_DATA_FMT_RGB,
  CMS_DATA_FMT_RGBA,
  CMS_DATA_FMT_COUNT
};

enum CMS_MODE
{
  CMS_MODE_3DLUT,
  CMS_MODE_PROFILE,
  CMS_MODE_COUNT
};

enum CMS_WHITEPOINT
{
  CMS_WHITEPOINT_D65,
  CMS_WHITEPOINT_D93,
  CMS_WHITEPOINT_COUNT
};

enum CMS_PRIMARIES
{
  CMS_PRIMARIES_AUTO,
  CMS_PRIMARIES_BT709,    // HDTV
  CMS_PRIMARIES_170M,     // SDTV
  CMS_PRIMARIES_BT470M,   // old NTSC (1953)
  CMS_PRIMARIES_BT470BG,  // old PAL/SECAM (1975)
  CMS_PRIMARIES_240M,     // old HDTV (1988)
  CMS_PRIMARIES_BT2020,   // UHDTV
  CMS_PRIMARIES_COUNT
};

enum CMS_TRC_TYPE
{
  CMS_TRC_BT1886,
  CMS_TRC_INPUT_OFFSET,
  CMS_TRC_OUTPUT_OFFSET,
  CMS_TRC_ABSOLUTE,
  CMS_TRC_COUNT
};

class CColorManager
{
public:
  CColorManager();
  virtual ~CColorManager();

  /*!
   \brief Check if user has requested color management
   \return true on enabled, false otherwise
   */
  bool IsEnabled() const;

  /*!
   \brief Check if configuration of color management is valid
   \return true on valid, false otherwise
   */
  bool IsValid() const;

  /*!
   \brief Get a 3D LUT for video color correction
   \param srcPrimaries video primaries (see AVColorPrimaries)
   \param cmsToken pointer to a color manager configuration token
   \param format of CLUT data
   \param clutSize CLUT resolution
   \param clutData pointer to CLUT data
   \return true on success, false otherwise
   */
  bool GetVideo3dLut(AVColorPrimaries srcPrimaries, int* cmsToken, CMS_DATA_FORMAT format,
                     int clutSize, uint16_t* clutData);

  /*!
   \brief Check if a 3D LUT is still valid
   \param cmsToken pointer to a color manager configuration token
   \param srcPrimaries video primaries (see AVColorPrimaries)
   \return true on valid, false if 3D LUT should be reloaded
   */
  bool CheckConfiguration(int cmsToken, AVColorPrimaries srcPrimaries);

  /*!
  \brief Get a 3D LUT dimension and data size for video color correction
  \param format required format of CLUT data
  \param clutSize pointer to CLUT resolution
  \param dataSize pointer to CLUT data size
  \return true on success, false otherwise
  */
  static bool Get3dLutSize(CMS_DATA_FORMAT format, int *clutSize, int *dataSize);

private:
  /*! \brief Check .3dlut file validity
   \param filename full path and filename
   \param clutSize pointer to CLUT resolution
   \return true if the file can be loaded, false otherwise
   */
  static bool Probe3dLut(const std::string& filename, int* clutSize);

  /*! \brief Load a .3dlut file
   \param filename full path and filename
   \param format of CLUT data
   \param clutSize CLUT resolution
   \param clutData pointer to CLUT data
   \return true on success, false otherwise
   */
  static bool Load3dLut(const std::string& filename,
                        CMS_DATA_FORMAT format,
                        int clutSize,
                        uint16_t* clutData);


#if defined(HAVE_LCMS2)
  // ProbeIccDisplayProfile

  // ProbeIccDeviceLink (?)


  /* \brief Load an ICC display profile
   \param filename full path and filename
   \return display profile (cmsHPROFILE)
   */
  cmsHPROFILE LoadIccDisplayProfile(const std::string& filename);

  /* \brief Load an ICC device link
   \param filename full path and filename
   \return device link (cmsHTRANSFORM)
   */
  // LoadIccDeviceLink (?)


  // create a gamma curve
  cmsToneCurve* CreateToneCurve(CMS_TRC_TYPE gammaType, double gammaValue, cmsCIEXYZ blackPoint);

  // create a source profile
  cmsHPROFILE CreateSourceProfile(CMS_PRIMARIES primaries, cmsToneCurve *gamma, CMS_WHITEPOINT whitepoint);


  /* \brief Create 3D LUT
   Samples a cmsHTRANSFORM object to create a 3D LUT of specified resolution
   \param transform cmsHTRANSFORM object to sample
   \param format of CLUT data
   \param resolution size of the 3D LUT to create
   \param clut pointer to LUT data
   */
  void Create3dLut(cmsHTRANSFORM transform, CMS_DATA_FORMAT format, int clutSize, uint16_t *clutData);

  // keep current display profile loaded here
  cmsHPROFILE m_hProfile = nullptr;
  cmsCIEXYZ   m_blackPoint = { 0, 0, 0 };

  // display parameters (gamma, input/output offset, primaries, whitepoint, intent?)
  CMS_WHITEPOINT m_curIccWhitePoint;
  CMS_PRIMARIES m_curIccPrimaries;
  CMS_TRC_TYPE m_m_curIccGammaMode;
  int m_curIccGamma;  // gamma multiplied by 100
#endif // defined(HAVE_LCMS2)

  // current configuration:
  CMS_PRIMARIES m_curVideoPrimaries;
  int m_curClutSize;
  int m_curCmsToken;
  // (compare the following to system settings to see if configuration is still valid)
  int m_curCmsMode;
  std::string m_cur3dlutFile;
  std::string m_curIccProfile;

};


