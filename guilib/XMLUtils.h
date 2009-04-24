#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

// forward
class TiXmlDocument;
class TiXmlNode;

class XMLUtils
{
public:
  static bool HasUTF8Declaration(const CStdString &strXML);

  static bool GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue);
  static bool GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue);
  static bool GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue);
  static bool GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value);
  static bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue);
  static bool GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue);
  static bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue);
  static bool GetEncoding(const TiXmlDocument* pDoc, CStdString& strEncoding);
  static bool GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue);

  static void SetString(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue);
  static void SetInt(TiXmlNode* pRootNode, const char *strTag, int value);
  static void SetFloat(TiXmlNode* pRootNode, const char *strTag, float value);
  static void SetBoolean(TiXmlNode* pRootNode, const char *strTag, bool value);
  static void SetHex(TiXmlNode* pRootNode, const char *strTag, DWORD value);
  static void SetPath(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue);
};

