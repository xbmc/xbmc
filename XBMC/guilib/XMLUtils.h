#pragma once

#include "include.h"

class XMLUtils
{
public:
  static bool GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue);
  static bool GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue);
  static bool GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue);
  static bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue);
  static bool GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue);
  static bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue);
};