/*
 *      Copyright (C) 2007-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef _XBMC_keytable_h
#define _XBMC_keytable_h

typedef struct struct_XBMCKEYTABLE
{

  // The scancode is the key scan code returned by the keyboard driver.
  // The only use for this is to detect some multimedia keys that do
  // not set the sym member in Linux.
  uint8_t scancode;

  // The sym is a value that identifies which key was pressed. Note
  // that it specifies the key not the character so it is unaffected by
  // shift, control, etc.
  uint16_t sym;

  // If the keypress generates a printing character the unicode and
  // ascii member variables contain the character generated. If the
  // key is a non-printing character, e.g. a function or arrow key,
  // the unicode and ascii member variables are zero.
  uint16_t unicode;
  char     ascii;

  // The following two member variables are used to specify the
  // action/function assigned to a key.
  // The keynames are used as tags in keyboard.xml. When reading keyboard.xml
  // TranslateKeyboardString uses the keyname to look up the vkey, and
  // this is used in the mapping table.
  uint32_t    vkey;
  const char* keyname;

} XBMCKEYTABLE;

bool KeyTableLookupName(const char* keyname, XBMCKEYTABLE* keytable);
bool KeyTableLookupSym(uint16_t sym, XBMCKEYTABLE* keytable);
bool KeyTableLookupUnicode(uint16_t unicode, XBMCKEYTABLE* keytable);
bool KeyTableLookupVKeyName(uint32_t vkey, XBMCKEYTABLE* keytable);

#endif /* _XBMC_keytable_h */
