#ifndef KEYBOARDLAYOUTCONFIGURATION_H
#define KEYBOARDLAYOUTCONFIGURATION_H

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// Comment OUT, if not really debugging!!!
// #define DEBUG_KEYBOARD_GETCHAR

#ifdef _LINUX
#include "linux/PlatformDefs.h"
#elif defined (WIN32)
#include "windows.h"
#else
#include "xtl.h"
#endif

#include <map>
#include "utils/log.h"
#include "utils/StdString.h"

class TiXmlElement;

class CKeyboardLayoutConfiguration
{
public:
  CKeyboardLayoutConfiguration();
  ~CKeyboardLayoutConfiguration();

  bool Load(const CStdString& strFileName);

  bool containsChangeXbmcCharRegardlessModifiers(WCHAR key);
  bool containsChangeXbmcCharWithRalt(WCHAR key);
  bool containsDeriveXbmcCharFromVkeyRegardlessModifiers(BYTE key);
  bool containsDeriveXbmcCharFromVkeyWithShift(BYTE key);
  bool containsDeriveXbmcCharFromVkeyWithRalt(BYTE key);

  WCHAR valueOfChangeXbmcCharRegardlessModifiers(WCHAR key);
  WCHAR valueOfChangeXbmcCharWithRalt(WCHAR key);
  WCHAR valueOfDeriveXbmcCharFromVkeyRegardlessModifiers(BYTE key);
  WCHAR valueOfDeriveXbmcCharFromVkeyWithShift(BYTE key);
  WCHAR valueOfDeriveXbmcCharFromVkeyWithRalt(BYTE key);

private:
  std::map<WCHAR, WCHAR> m_changeXbmcCharRegardlessModifiers;
  std::map<WCHAR, WCHAR> m_changeXbmcCharWithRalt;
  std::map<BYTE, WCHAR> m_deriveXbmcCharFromVkeyRegardlessModifiers;
  std::map<BYTE, WCHAR> m_deriveXbmcCharFromVkeyWithShift;
  std::map<BYTE, WCHAR> m_deriveXbmcCharFromVkeyWithRalt;

  void SetDefaults();
  void readCharMapFromXML(const TiXmlElement* pXMLMap, std::map<WCHAR, WCHAR>& charToCharMap, const char* mapRootElement);
  void readByteMapFromXML(const TiXmlElement* pXMLMap, std::map<BYTE, WCHAR>& charToCharMap, const char* mapRootElement);
};

extern CKeyboardLayoutConfiguration g_keyboardLayoutConfiguration;

#endif
