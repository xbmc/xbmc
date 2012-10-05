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

#include "KeyboardLayoutConfiguration.h"
#include "utils/CharsetConverter.h"
#include "utils/XBMCTinyXML.h"

using namespace std;
CKeyboardLayoutConfiguration g_keyboardLayoutConfiguration;

CKeyboardLayoutConfiguration::CKeyboardLayoutConfiguration()
{
  SetDefaults();
}


CKeyboardLayoutConfiguration::~CKeyboardLayoutConfiguration()
{
  SetDefaults();
}

void CKeyboardLayoutConfiguration::SetDefaults()
{
  m_changeXbmcCharRegardlessModifiers.clear();
  m_changeXbmcCharWithRalt.clear();
  m_deriveXbmcCharFromVkeyRegardlessModifiers.clear();
  m_deriveXbmcCharFromVkeyWithShift.clear();
  m_deriveXbmcCharFromVkeyWithRalt.clear();
}

bool CKeyboardLayoutConfiguration::Load(const CStdString& strFileName)
{
  SetDefaults();

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strFileName))
  {
    CLog::Log(LOGINFO, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("keyboard_layout"))
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <keyboard_layout>", strFileName.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG, "reading char2char ");
  const TiXmlElement* pMapChangeXbmcCharRegardlessModifiers = pRootElement->FirstChildElement("char2char");
  readCharMapFromXML(pMapChangeXbmcCharRegardlessModifiers, m_changeXbmcCharRegardlessModifiers, ("char2char"));

  CLog::Log(LOGDEBUG, "reading char2char_ralt ");
  const TiXmlElement* pMapChangeXbmcCharWithRalt = pRootElement->FirstChildElement("char2char_ralt");
  readCharMapFromXML(pMapChangeXbmcCharWithRalt, m_changeXbmcCharWithRalt, ("char2char_ralt"));

  CLog::Log(LOGDEBUG, "reading vkey2char ");
  const TiXmlElement* pMapDeriveXbmcCharFromVkeyRegardlessModifiers = pRootElement->FirstChildElement("vkey2char");
  readByteMapFromXML(pMapDeriveXbmcCharFromVkeyRegardlessModifiers, m_deriveXbmcCharFromVkeyRegardlessModifiers, ("vkey2char"));

  CLog::Log(LOGDEBUG, "reading vkey2char_shift ");
  const TiXmlElement* pMapDeriveXbmcCharFromVkeyWithShift = pRootElement->FirstChildElement("vkey2char_shift");
  readByteMapFromXML(pMapDeriveXbmcCharFromVkeyWithShift, m_deriveXbmcCharFromVkeyWithShift, ("vkey2char_shift"));

  CLog::Log(LOGDEBUG, "reading vkey2char_ralt ");
  const TiXmlElement* pMapDeriveXbmcCharFromVkeyWithRalt = pRootElement->FirstChildElement("vkey2char_ralt");
  readByteMapFromXML(pMapDeriveXbmcCharFromVkeyWithRalt, m_deriveXbmcCharFromVkeyWithRalt, ("vkey2char_ralt"));

  return true;
}

void CKeyboardLayoutConfiguration::readCharMapFromXML(const TiXmlElement* pXMLMap, map<WCHAR, WCHAR>& charToCharMap, const char* mapRootElement)
{
  if (pXMLMap && !pXMLMap->NoChildren())
  { // map keys
    const TiXmlElement* pEntry = pXMLMap->FirstChildElement();
    while (pEntry)
    {
      CStdString strInChar = pEntry->Attribute("inchar");
      CStdString strOutChar = pEntry->Attribute("outchar");
      if (strInChar.length() > 0 && strOutChar.length() > 0)
      {
        CStdStringW fromStr;
        g_charsetConverter.utf8ToW(strInChar, fromStr);
        CStdStringW toStr;
        g_charsetConverter.utf8ToW(strOutChar, toStr);
        if (fromStr.size()==1 && toStr.size()==1)
        {
          charToCharMap.insert(pair<WCHAR, WCHAR>(fromStr[0], toStr[0]));
          CLog::Log(LOGDEBUG, "insert map entry from %c to %c ", fromStr[0], toStr[0]);
        }
        else
        {
          CLog::Log(LOGERROR, "String from %ls or to %ls does not have the expected length of 1", fromStr.c_str(), toStr.c_str());
        }
      }
      else
      {
        CLog::Log(LOGERROR, "map entry misses attribute <inchar> or <outchar> or content of them");
      }
      pEntry = pEntry->NextSiblingElement();
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "XML-Configuration doesn't contain expected map root element %s", mapRootElement);
  }
}

void CKeyboardLayoutConfiguration::readByteMapFromXML(const TiXmlElement* pXMLMap, map<BYTE, WCHAR>& charToCharMap, const char* mapRootElement)
{
  if (pXMLMap && !pXMLMap->NoChildren())
  { // map keys
    const TiXmlElement* pEntry = pXMLMap->FirstChildElement();
    while (pEntry)
    {
      CStdString strInHex = pEntry->Attribute("inhex");
      CStdString strOutChar = pEntry->Attribute("outchar");
      if (strInHex.length() > 0 && strOutChar.length() > 0)
      {
        CStdString hexValue = strInHex;
        CStdStringW toStr;
        g_charsetConverter.utf8ToW(strOutChar, toStr);

        int from;
        if (sscanf(hexValue.c_str(), "%x", (unsigned int *)&from))
        {
          if (from != 0) // eats nearly any typing error as 0: catch it:
          {
            if (from < 256)
            {
              if (toStr.size()==1)
              {
                    charToCharMap.insert(pair<BYTE, WCHAR>(from, toStr[0]));
                    CLog::Log(LOGDEBUG, "insert map entry from %d to %c ", from, toStr[0]);
              }
              else
              {
                CLog::Log(LOGERROR, "String to %ls does not have the expected length of >=1", toStr.c_str());
              }
            }
            else
            {
              CLog::Log(LOGERROR, "From value %d was greater than 255! ", from);
            }
          }
          else
          {
            CLog::Log(LOGERROR, "Scanned from-value %d as 0 probably a (typing?) error! ", from);
          }
        }
        else
        {
            CLog::Log(LOGERROR, "Could not scan from-value %s (was no valid hex value) ", hexValue.c_str());
        }
      }
      else
      {
        CLog::Log(LOGERROR, "map entry misses attribute <inhex> or <outchar> or content of them");
      }
      pEntry = pEntry->NextSiblingElement();
    }
  }
  else
  {
    CLog::Log(LOGERROR, "XML-Configuration doesn't contain expected map root element %s", mapRootElement);
  }
}

bool CKeyboardLayoutConfiguration::containsChangeXbmcCharRegardlessModifiers(WCHAR key)
{
  bool result = m_changeXbmcCharRegardlessModifiers.find(key) != m_changeXbmcCharRegardlessModifiers.end();
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found containsChangeXbmcCharRegardlessModifiers key char %c: bool: %d ", key, result);
#endif
  return result;
}

bool CKeyboardLayoutConfiguration::containsChangeXbmcCharWithRalt(WCHAR key)
{
  bool result = m_changeXbmcCharWithRalt.find(key) != m_changeXbmcCharWithRalt.end();
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found containsChangeXbmcCharWithRalt key char %c: bool: %d ", key, result);
#endif
  return result;
}

bool CKeyboardLayoutConfiguration::containsDeriveXbmcCharFromVkeyRegardlessModifiers(BYTE key)
{
  bool result = m_deriveXbmcCharFromVkeyRegardlessModifiers.find(key) != m_deriveXbmcCharFromVkeyRegardlessModifiers.end();
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found containsDeriveXbmcCharFromVkeyRegardlessModifiers key vkey %d: bool: %d ", key, result);
#endif
  return result;
}

bool CKeyboardLayoutConfiguration::containsDeriveXbmcCharFromVkeyWithShift(BYTE key)
{
  bool result = m_deriveXbmcCharFromVkeyWithShift.find(key) != m_deriveXbmcCharFromVkeyWithShift.end();
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found containsDeriveXbmcCharFromVkeyWithShift key vkey %d: bool: %d ", key, result);
#endif
  return result;
}

bool CKeyboardLayoutConfiguration::containsDeriveXbmcCharFromVkeyWithRalt(BYTE key)
{
  bool result = m_deriveXbmcCharFromVkeyWithRalt.find(key) != m_deriveXbmcCharFromVkeyWithRalt.end();
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found containsDeriveXbmcCharFromVkeyWithRalt key vkey %d: bool: %d ", key, result);
#endif
  return result;
}

WCHAR CKeyboardLayoutConfiguration::valueOfChangeXbmcCharRegardlessModifiers(WCHAR key)
{
  WCHAR result = (m_changeXbmcCharRegardlessModifiers.find(key))->second;
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found valueOfChangeXbmcCharRegardlessModifiers for char key %c: char %c ", key, result);
#endif
  return result;
}

WCHAR CKeyboardLayoutConfiguration::valueOfChangeXbmcCharWithRalt(WCHAR key)
{
  WCHAR result = (m_changeXbmcCharWithRalt.find(key))->second;
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found valueOfChangeXbmcCharWithRalt for char key %c: char %c ", key, result);
#endif
  return result;
}

WCHAR CKeyboardLayoutConfiguration::valueOfDeriveXbmcCharFromVkeyRegardlessModifiers(BYTE key)
{
  WCHAR result = (m_deriveXbmcCharFromVkeyRegardlessModifiers.find(key))->second;
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found valueOfDeriveXbmcCharFromVkeyRegardlessModifiers for key vkey %d: char %c ", key, result);
#endif
  return result;
}

WCHAR CKeyboardLayoutConfiguration::valueOfDeriveXbmcCharFromVkeyWithShift(BYTE key)
{
  WCHAR result = (m_deriveXbmcCharFromVkeyWithShift.find(key))->second;
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found valueOfDeriveXbmcCharFromVkeyWithShift for key vkey %d: char %c ", key, result);
#endif
  return result;
}

WCHAR CKeyboardLayoutConfiguration::valueOfDeriveXbmcCharFromVkeyWithRalt(BYTE key)
{
  WCHAR result = (m_deriveXbmcCharFromVkeyWithRalt.find(key))->second;
#ifdef DEBUG_KEYBOARD_GETCHAR
  CLog::Log(LOGDEBUG, "found valueOfDeriveXbmcCharFromVkeyWithRalt for key vkey %d: char %c ", key, result);
#endif
  return result;
}


