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

#pragma once

#if defined(HAVE_LCMS2)
#include "lcms2.h"
#endif

#include <string>

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
  bool IsEnabled();

  /*!
   \brief Get a 3D LUT for video color correction
   \param primaries video primaries (see CONF_FLAGS_COLPRI)
   \param cmsToken pointer to a color manager configuration token
   \param clutSize pointer to CLUT resolution
   \param clutData pointer to CLUT data (caller to free memory afterwards)
   \return true on success, false otherwise
   */
  bool GetVideo3dLut(int primaries, int *cmsToken, int *clutSize, uint16_t **clutData);

  /*!
   \brief Check if a 3D LUT is still valid
   \param cmsToken pointer to a color manager configuration token
   \param flags video renderer flags (see CONF_FLAGS_COLPRI)
   \return true on valid, false if 3D LUT should be reloaded
   */
  bool CheckConfiguration(int cmsToken, int flags);

private:
  /*! \brief Check .3dlut file validity
   \param filename full path and filename
   \return true if the file can be loaded, false otherwise
   */
  bool Probe3dLut(const std::string filename);

  /*! \brief Load a .3dlut file
   \param filename full path and filename
   \param clutSize pointer to CLUT resolution
   \param clutData pointer to CLUT data
   \return true on success, false otherwise
   */
  bool Load3dLut(const std::string filename, uint16_t **clutData, int *clutSize);


#if defined(HAVE_LCMS2)
  // ProbeIccDisplayProfile

  // ProbeIccDeviceLink (?)


  /* \brief Load an ICC display profile
   \param filename full path and filename
   \return display profile (cmsHPROFILE)
   */
  cmsHPROFILE LoadIccDisplayProfile(const std::string filename);

  /* \brief Load an ICC device link
   \param filename full path and filename
   \return device link (cmsHTRANSFORM)
   */
  // LoadIccDeviceLink (?)


  // create a gamma curve
  cmsToneCurve* CreateToneCurve(CMS_TRC_TYPE gammaType, float gammaValue, cmsCIEXYZ blackPoint);

  // create a source profile
  cmsHPROFILE CreateSourceProfile(CMS_PRIMARIES primaries, cmsToneCurve *gamma, CMS_WHITEPOINT whitepoint);


  /* \brief Create 3D LUT
   Samples a cmsHTRANSFORM object to create a 3D LUT of specified resolution
   \param transform cmsHTRANSFORM object to sample
   \param resolution size of the 3D LUT to create
   \param clut pointer to LUT data
   */
  void Create3dLut(cmsHTRANSFORM transform, uint16_t **clutData, int *clutSize);

  // keep current display profile loaded here
  cmsHPROFILE m_hProfile;
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


