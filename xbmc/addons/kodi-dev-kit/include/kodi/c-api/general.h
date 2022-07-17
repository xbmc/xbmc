/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GENERAL_H
#define C_API_GENERAL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// \ingroup cpp_kodi_Defs
  /// @brief For kodi::CurrentKeyboardLayout used defines
  ///
  typedef enum StdKbButtons
  {
    /// The quantity of buttons per row on Kodi's standard keyboard
    STD_KB_BUTTONS_PER_ROW = 20,
    /// The quantity of rows on Kodi's standard keyboard
    STD_KB_BUTTONS_MAX_ROWS = 4,
    /// Keyboard layout type, this for initial standard
    STD_KB_MODIFIER_KEY_NONE = 0x00,
    /// Keyboard layout type, this for shift controlled layout (uppercase)
    STD_KB_MODIFIER_KEY_SHIFT = 0x01,
    /// Keyboard layout type, this to show symbols
    STD_KB_MODIFIER_KEY_SYMBOL = 0x02
  } StdKbButtons;
  //----------------------------------------------------------------------------

  //============================================================================
  /// \ingroup cpp_kodi_Defs
  /// @brief For kodi::QueueNotification() used message types
  ///
  typedef enum QueueMsg
  {
    /// Show info notification message
    QUEUE_INFO,
    /// Show warning notification message
    QUEUE_WARNING,
    /// Show error notification message
    QUEUE_ERROR,
    /// Show with own given image and parts if set on values
    QUEUE_OWN_STYLE
  } QueueMsg;
  //----------------------------------------------------------------------------

  //============================================================================
  /// \ingroup cpp_kodi_Defs
  /// @brief Format codes to get string from them.
  ///
  /// Used on kodi::GetLanguage().
  ///
  typedef enum LangFormats
  {
    /// two letter code as defined in ISO 639-1
    LANG_FMT_ISO_639_1,
    /// three letter code as defined in ISO 639-2/T or ISO 639-2/B
    LANG_FMT_ISO_639_2,
    /// full language name in English
    LANG_FMT_ENGLISH_NAME
  } LangFormats;
  //----------------------------------------------------------------------------

  /*
   * For interface between add-on and kodi.
   *
   * This structure defines the addresses of functions stored inside Kodi which
   * are then available for the add-on to call
   *
   * All function pointers there are used by the C++ interface functions below.
   * You find the set of them on xbmc/addons/interfaces/General.cpp
   *
   * Note: For add-on development itself this is not needed
   */
  typedef struct AddonKeyboardKeyTable
  {
    char* keys[STD_KB_BUTTONS_MAX_ROWS][STD_KB_BUTTONS_PER_ROW];
  } AddonKeyboardKeyTable;
  typedef struct AddonToKodiFuncTable_kodi
  {
    char* (*unknown_to_utf8)(void* kodiBase, const char* source, bool* ret, bool failOnBadChar);
    char* (*get_language)(void* kodiBase, int format, bool region);
    bool (*queue_notification)(void* kodiBase,
                               int type,
                               const char* header,
                               const char* message,
                               const char* imageFile,
                               unsigned int displayTime,
                               bool withSound,
                               unsigned int messageTime);
    void (*get_md5)(void* kodiBase, const char* text, char* md5);
    char* (*get_region)(void* kodiBase, const char* id);
    void (*get_free_mem)(void* kodiBase, long* free, long* total, bool as_bytes);
    int (*get_global_idle_time)(void* kodiBase);
    bool (*is_addon_avilable)(void* kodiBase, const char* id, char** version, bool* enabled);
    void (*kodi_version)(void* kodiBase,
                         char** compile_name,
                         int* major,
                         int* minor,
                         char** revision,
                         char** tag,
                         char** tagversion);
    char* (*get_current_skin_id)(void* kodiBase);
    bool (*get_keyboard_layout)(void* kodiBase,
                                char** layout_name,
                                int modifier_key,
                                struct AddonKeyboardKeyTable* layout);
    bool (*change_keyboard_layout)(void* kodiBase, char** layout_name);
  } AddonToKodiFuncTable_kodi;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GENERAL_H */
