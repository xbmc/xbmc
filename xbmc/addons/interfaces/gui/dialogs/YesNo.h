/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/gui/dialogs/yes_no.h"

extern "C"
{

  struct AddonGlobalInterface;

  namespace ADDON
  {

  /*!
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/gui/dialogs/YesNo.h"
   */
  struct Interface_GUIDialogYesNo
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note To add a new function use the "_" style to directly identify an
     * add-on callback function. Everything with CamelCase is only to be used
     * in Kodi.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static bool show_and_get_input_single_text(KODI_HANDLE kodiBase,
                                               const char* heading,
                                               const char* text,
                                               bool* canceled,
                                               const char* noLabel,
                                               const char* yesLabel);

    static bool show_and_get_input_line_text(KODI_HANDLE kodiBase,
                                             const char* heading,
                                             const char* line0,
                                             const char* line1,
                                             const char* line2,
                                             const char* noLabel,
                                             const char* yesLabel);

    static bool show_and_get_input_line_button_text(KODI_HANDLE kodiBase,
                                                    const char* heading,
                                                    const char* line0,
                                                    const char* line1,
                                                    const char* line2,
                                                    bool* canceled,
                                                    const char* noLabel,
                                                    const char* yesLabel);
    //@}
  };

  } /* namespace ADDON */
} /* extern "C" */
