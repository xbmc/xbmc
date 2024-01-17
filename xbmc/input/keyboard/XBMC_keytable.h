/*
 *  Copyright (C) 2007-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>

namespace KODI
{
namespace KEYBOARD
{
/// \ingroup keyboard
/// \{

struct XBMCKEYTABLE
{
  // The sym is a value that identifies which key was pressed. Note
  // that it specifies the key not the character so it is unaffected by
  // shift, control, etc.
  uint16_t sym;

  // If the keypress generates a printing character the unicode and
  // ascii member variables contain the character generated. If the
  // key is a non-printing character, e.g. a function or arrow key,
  // the unicode and ascii member variables are zero.
  uint16_t unicode;
  char ascii;

  // The following two member variables are used to specify the
  // action/function assigned to a key.
  // The keynames are used as tags in keyboard.xml. When reading keyboard.xml
  // TranslateKeyboardString uses the keyname to look up the vkey, and
  // this is used in the mapping table.
  uint32_t vkey;
  const char* keyname;
};

struct KeyTable
{
  static bool LookupName(std::string keyname, XBMCKEYTABLE* keytable);
  static bool LookupSym(uint16_t sym, XBMCKEYTABLE* keytable);
  static bool LookupUnicode(uint16_t unicode, XBMCKEYTABLE* keytable);
  static bool LookupSymAndUnicode(uint16_t sym, uint16_t unicode, XBMCKEYTABLE* keytable);
  static bool LookupVKeyName(uint32_t vkey, XBMCKEYTABLE* keytable);
};

/// \}
} // namespace KEYBOARD
} // namespace KODI
