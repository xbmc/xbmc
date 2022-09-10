/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_SCREENSAVER_H
#define C_API_ADDONINSTANCE_SCREENSAVER_H

#include "../addon_base.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef KODI_ADDON_INSTANCE_HDL KODI_ADDON_SCREENSAVER_HDL;

  struct KODI_ADDON_SCREENSAVER_PROPS
  {
    ADDON_HARDWARE_CONTEXT device;
    int x;
    int y;
    int width;
    int height;
    float pixelRatio;
  };

  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_SCREENSAVER_START_V1)(
      const KODI_ADDON_SCREENSAVER_HDL hdl);
  typedef void(ATTR_APIENTRYP PFN_KODI_ADDON_SCREENSAVER_STOP_V1)(
      const KODI_ADDON_SCREENSAVER_HDL hdl);
  typedef void(ATTR_APIENTRYP PFN_KODI_ADDON_SCREENSAVER_RENDER_V1)(
      const KODI_ADDON_SCREENSAVER_HDL hdl);

  typedef struct KodiToAddonFuncTable_Screensaver
  {
    PFN_KODI_ADDON_SCREENSAVER_START_V1 start;
    PFN_KODI_ADDON_SCREENSAVER_STOP_V1 stop;
    PFN_KODI_ADDON_SCREENSAVER_RENDER_V1 render;
  } KodiToAddonFuncTable_Screensaver;

  typedef struct AddonToKodiFuncTable_Screensaver
  {
    void (*get_properties)(const KODI_HANDLE hdl, struct KODI_ADDON_SCREENSAVER_PROPS* props);
  } AddonToKodiFuncTable_Screensaver;

  typedef struct AddonInstance_Screensaver
  {
    struct AddonToKodiFuncTable_Screensaver* toKodi;
    struct KodiToAddonFuncTable_Screensaver* toAddon;
  } AddonInstance_Screensaver;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_SCREENSAVER_H */
