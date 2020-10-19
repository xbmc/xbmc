/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_CONTROLS_EDIT_H
#define C_API_GUI_CONTROLS_EDIT_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit_Defs
  /// @{
  /// @anchor AddonGUIInputType
  /// @brief Text input types used on kodi::gui::controls::CEdit
  enum AddonGUIInputType
  {
    /// Text inside edit control only readable
    ADDON_INPUT_TYPE_READONLY = -1,
    /// Normal text entries
    ADDON_INPUT_TYPE_TEXT = 0,
    /// To use on edit control only numeric numbers
    ADDON_INPUT_TYPE_NUMBER,
    /// To insert seconds
    ADDON_INPUT_TYPE_SECONDS,
    /// To insert time
    ADDON_INPUT_TYPE_TIME,
    /// To insert a date
    ADDON_INPUT_TYPE_DATE,
    /// Used for write in IP addresses
    ADDON_INPUT_TYPE_IPADDRESS,
    /// Text field used as password entry field with not visible text
    ADDON_INPUT_TYPE_PASSWORD,
    /// Text field used as password entry field with not visible text but
    /// returned as MD5 value
    ADDON_INPUT_TYPE_PASSWORD_MD5,
    /// Use text field for search purpose
    ADDON_INPUT_TYPE_SEARCH,
    /// Text field as filter
    ADDON_INPUT_TYPE_FILTER,
    ///
    ADDON_INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW
  };
  /// @}
  //----------------------------------------------------------------------------

  typedef struct AddonToKodiFuncTable_kodi_gui_control_edit
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_enabled)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool enabled);
    void (*set_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* label);
    char* (*get_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_text)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* text);
    char* (*get_text)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_cursor_position)(KODI_HANDLE kodiBase,
                                KODI_GUI_CONTROL_HANDLE handle,
                                unsigned int position);
    unsigned int (*get_cursor_position)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_input_type)(KODI_HANDLE kodiBase,
                           KODI_GUI_CONTROL_HANDLE handle,
                           int type,
                           const char* heading);
  } AddonToKodiFuncTable_kodi_gui_control_edit;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_CONTROLS_EDIT_H */
