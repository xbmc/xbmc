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

#include "XMLUtils.h"
#include "URL.h"
#include "filesystem/SpecialProtocol.h"
#include "StringUtils.h"
#ifdef _WIN32
#include "PlatformDefs.h" //for strcasecmp
#endif

bool XMLUtils::GetHex(const TiXmlNode* pRootNode, const char* strTag, uint32_t& hexValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  sscanf(pNode->FirstChild()->Value(), "%x", (uint32_t*) &hexValue );
  return true;
}


bool XMLUtils::GetUInt(const TiXmlNode* pRootNode, const char* strTag, uint32_t& uintValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  uintValue = atol(pNode->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  lLongValue = atol(pNode->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  iIntValue = atoi(pNode->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetInt(const TiXmlNode* pRootNode, const char* strTag, int &value, const int min, const int max)
{
  if (GetInt(pRootNode, strTag, value))
  {
    if (value < min) value = min;
    if (value > max) value = max;
    return true;
  }
  return false;
}

bool XMLUtils::GetDouble(const TiXmlNode *root, const char *tag, double &value)
{
  const TiXmlNode* node = root->FirstChild(tag);
  if (!node || !node->FirstChild()) return false;
  value = atof(node->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  value = (float)atof(pNode->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetFloat(const TiXmlNode* pRootElement, const char *tagName, float& fValue, const float fMin, const float fMax)
{
  if (GetFloat(pRootElement, tagName, fValue))
  { // check range
    if (fValue < fMin) fValue = fMin;
    if (fValue > fMax) fValue = fMax;
    return true;
  }
  return false;
}

bool XMLUtils::GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  CStdString strEnabled = pNode->FirstChild()->Value();
  strEnabled.ToLower();
  if (strEnabled == "off" || strEnabled == "no" || strEnabled == "disabled" || strEnabled == "false" || strEnabled == "0" )
    bBoolValue = false;
  else
  {
    bBoolValue = true;
    if (strEnabled != "on" && strEnabled != "yes" && strEnabled != "enabled" && strEnabled != "true")
      return false; // invalid bool switch - it's probably some other string.
  }
  return true;
}

bool XMLUtils::GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag );
  if (!pElement) return false;
  const char* encoded = pElement->Attribute("urlencoded");
  const TiXmlNode* pNode = pElement->FirstChild();
  if (pNode != NULL)
  {
    strStringValue = pNode->Value();
    if (encoded && strcasecmp(encoded,"yes") == 0)
      CURL::Decode(strStringValue);
    return true;
  }
  strStringValue.Empty();
  return false;
}

bool XMLUtils::HasChild(const TiXmlNode* pRootNode, const char* strTag)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement) return false;
  const TiXmlNode* pNode = pElement->FirstChild();
  return (pNode != NULL);
}

bool XMLUtils::GetAdditiveString(const TiXmlNode* pRootNode, const char* strTag,
                                 const CStdString& strSeparator, CStdString& strStringValue,
                                 bool clear)
{
  CStdString strTemp;
  const TiXmlElement* node = pRootNode->FirstChildElement(strTag);
  bool bResult=false;
  if (node && node->FirstChild() && clear)
    strStringValue.clear();
  while (node)
  {
    if (node->FirstChild())
    {
      bResult = true;
      strTemp = node->FirstChild()->Value();
      const char* clear=node->Attribute("clear");
      if (strStringValue.IsEmpty() || (clear && strcasecmp(clear,"true")==0))
        strStringValue = strTemp;
      else
        strStringValue += strSeparator+strTemp;
    }
    node = node->NextSiblingElement(strTag);
  }

  return bResult;
}

/*!
  Returns true if the encoding of the document is other then UTF-8.
  /param strEncoding Returns the encoding of the document. Empty if UTF-8
*/
bool XMLUtils::GetEncoding(const TiXmlDocument* pDoc, CStdString& strEncoding)
{
  const TiXmlNode* pNode=NULL;
  while ((pNode=pDoc->IterateChildren(pNode)) && pNode->Type()!=TiXmlNode::DECLARATION) {}
  if (!pNode) return false;
  const TiXmlDeclaration* pDecl=pNode->ToDeclaration();
  if (!pDecl) return false;
  strEncoding=pDecl->Encoding();
  if (strEncoding.Equals("UTF-8") || strEncoding.Equals("UTF8")) strEncoding.Empty();
  strEncoding.MakeUpper();
  return !strEncoding.IsEmpty(); // Other encoding then UTF8?
}

/*!
  Returns true if the encoding of the document is specified as as UTF-8
  /param strXML The XML file (embedded in a string) to check.
*/
bool XMLUtils::HasUTF8Declaration(const CStdString &strXML)
{
  CStdString test = strXML;
  test.ToLower();
  // test for the encoding="utf-8" string
  if (test.Find("encoding=\"utf-8\"") >= 0)
    return true;
  // TODO: test for plain UTF8 here?
  return false;
}

bool XMLUtils::GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement) return false;

  int pathVersion = 0;
  pElement->Attribute("pathversion", &pathVersion);
  const char* encoded = pElement->Attribute("urlencoded");
  const TiXmlNode* pNode = pElement->FirstChild();
  if (pNode != NULL)
  {
    strStringValue = pNode->Value();
    if (encoded && strcasecmp(encoded,"yes") == 0)
      CURL::Decode(strStringValue);
    strStringValue = CSpecialProtocol::ReplaceOldPath(strStringValue, pathVersion);
    return true;
  }
  strStringValue.Empty();
  return false;
}

void XMLUtils::SetAdditiveString(TiXmlNode* pRootNode, const char *strTag, const CStdString& strSeparator, const CStdString& strValue)
{
  CStdStringArray list;
  StringUtils::SplitString(strValue,strSeparator,list);
  for (unsigned int i=0;i<list.size() && !list[i].IsEmpty();++i)
    SetString(pRootNode,strTag,list[i]);
}

void XMLUtils::SetString(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue)
{
  TiXmlElement newElement(strTag);
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    TiXmlText value(strValue);
    pNewNode->InsertEndChild(value);
  }
}

void XMLUtils::SetInt(TiXmlNode* pRootNode, const char *strTag, int value)
{
  CStdString strValue;
  strValue.Format("%i", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetLong(TiXmlNode* pRootNode, const char *strTag, long value)
{
  CStdString strValue;
  strValue.Format("%l", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetFloat(TiXmlNode* pRootNode, const char *strTag, float value)
{
  CStdString strValue;
  strValue.Format("%f", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetBoolean(TiXmlNode* pRootNode, const char *strTag, bool value)
{
  SetString(pRootNode, strTag, value ? "true" : "false");
}

void XMLUtils::SetHex(TiXmlNode* pRootNode, const char *strTag, uint32_t value)
{
  CStdString strValue;
  strValue.Format("%x", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetPath(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue)
{
  TiXmlElement newElement(strTag);
  newElement.SetAttribute("pathversion", CSpecialProtocol::path_version);
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    TiXmlText value(strValue);
    pNewNode->InsertEndChild(value);
  }
}
