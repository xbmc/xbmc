#ifndef KEYBOARDLAYOUTCONFIGURATION_H
#define KEYBOARDLAYOUTCONFIGURATION_H

#ifdef _LINUX
#include "linux/PlatformDefs.h"
#elif defined (WIN32)
#include "windows.h"
#else
#include "xtl.h"
#endif

#include <map>
#include "utils/log.h"
#include "../guilib/StdString.h"
#include "../guilib/tinyXML/tinyxml.h"

//
// C++ Interface: CKeyboardLayoutConfiguration
//
// Description: Loads several keyboard layout configuration maps from an XML file and 
// answers questions about them.
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

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
  void readByteMapFromXML(const TiXmlElement* pXMLMap, map<BYTE, WCHAR>& charToCharMap, const char* mapRootElement);
};

extern CKeyboardLayoutConfiguration g_keyboardLayoutConfiguration;

#endif

