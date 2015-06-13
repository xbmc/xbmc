/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "XMLUtils.h"
#include "URL.h"
#include "StringUtils.h"
#ifdef TARGET_WINDOWS
#include "PlatformDefs.h" //for strcasecmp
#endif

bool XMLUtils::GetHex(const TiXmlNode* pRootNode, const char* strTag, uint32_t& hexValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  return sscanf(pNode->FirstChild()->Value(), "%x", (uint32_t*)&hexValue) == 1;
}


bool XMLUtils::GetUInt(const TiXmlNode* pRootNode, const char* strTag, uint32_t& uintValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  uintValue = atol(pNode->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetUInt(const TiXmlNode* pRootNode, const char* strTag, uint32_t &value, const uint32_t min, const uint32_t max)
{
  if (GetUInt(pRootNode, strTag, value))
  {
    if (value < min) value = min;
    if (value > max) value = max;
    return true;
  }
  return false;
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
  std::string strEnabled = pNode->FirstChild()->ValueStr();
  StringUtils::ToLower(strEnabled);
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

bool XMLUtils::GetString(const TiXmlNode* pRootNode, const char* strTag, std::string& strStringValue)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement) return false;

  const char* encoded = pElement->Attribute("urlencoded");
  const TiXmlNode* pNode = pElement->FirstChild();
  if (pNode != NULL)
  {
    strStringValue = pNode->ValueStr();
    if (encoded && strcasecmp(encoded,"yes") == 0)
      strStringValue = CURL::Decode(strStringValue);
    return true;
  }
  strStringValue.clear();
  return true;
}

std::string XMLUtils::GetString(const TiXmlNode* pRootNode, const char* strTag)
{
  std::string temp;
  GetString(pRootNode, strTag, temp);
  return temp;
}

bool XMLUtils::HasChild(const TiXmlNode* pRootNode, const char* strTag)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement) return false;
  const TiXmlNode* pNode = pElement->FirstChild();
  return (pNode != NULL);
}

bool XMLUtils::GetAdditiveString(const TiXmlNode* pRootNode, const char* strTag,
                                 const std::string& strSeparator, std::string& strStringValue,
                                 bool clear)
{
  std::string strTemp;
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
      if (strStringValue.empty() || (clear && strcasecmp(clear,"true")==0))
        strStringValue = strTemp;
      else
        strStringValue += strSeparator+strTemp;
    }
    node = node->NextSiblingElement(strTag);
  }

  return bResult;
}

/*!
  Parses the XML for multiple tags of the given name.
  Does not clear the array to support chaining.
*/
bool XMLUtils::GetStringArray(const TiXmlNode* pRootNode, const char* strTag, std::vector<std::string>& arrayValue, bool clear /* = false */, const std::string& separator /* = "" */)
{
  std::string strTemp;
  const TiXmlElement* node = pRootNode->FirstChildElement(strTag);
  bool bResult=false;
  if (node && node->FirstChild() && clear)
    arrayValue.clear();
  while (node)
  {
    if (node->FirstChild())
    {
      bResult = true;
      strTemp = node->FirstChild()->ValueStr();

      const char* clearAttr = node->Attribute("clear");
      if (clearAttr && strcasecmp(clearAttr, "true") == 0)
        arrayValue.clear();

      if (strTemp.empty())
        continue;

      if (separator.empty())
        arrayValue.push_back(strTemp);
      else
      {
        std::vector<std::string> tempArray = StringUtils::Split(strTemp, separator);
        arrayValue.insert(arrayValue.end(), tempArray.begin(), tempArray.end());
      }
    }
    node = node->NextSiblingElement(strTag);
  }

  return bResult;
}

bool XMLUtils::GetPath(const TiXmlNode* pRootNode, const char* strTag, std::string& strStringValue)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement) return false;

  const char* encoded = pElement->Attribute("urlencoded");
  const TiXmlNode* pNode = pElement->FirstChild();
  if (pNode != NULL)
  {
    strStringValue = pNode->Value();
    if (encoded && strcasecmp(encoded,"yes") == 0)
      strStringValue = CURL::Decode(strStringValue);
    return true;
  }
  strStringValue.clear();
  return false;
}

bool XMLUtils::GetDate(const TiXmlNode* pRootNode, const char* strTag, CDateTime& date)
{
  std::string strDate;
  if (GetString(pRootNode, strTag, strDate) && !strDate.empty())
  {
    date.SetFromDBDate(strDate);
    return true;
  }

  return false;
}

bool XMLUtils::GetDateTime(const TiXmlNode* pRootNode, const char* strTag, CDateTime& dateTime)
{
  std::string strDateTime;
  if (GetString(pRootNode, strTag, strDateTime) && !strDateTime.empty())
  {
    dateTime.SetFromDBDateTime(strDateTime);
    return true;
  }

  return false;
}

std::string XMLUtils::GetAttribute(const TiXmlElement *element, const char *tag)
{
  if (element)
  {
    const char *attribute = element->Attribute(tag);
    if (attribute)
      return attribute;
  }
  return "";
}

void XMLUtils::SetAdditiveString(TiXmlNode* pRootNode, const char *strTag, const std::string& strSeparator, const std::string& strValue)
{
  std::vector<std::string> list = StringUtils::Split(strValue, strSeparator);
  for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i)
    SetString(pRootNode, strTag, *i);
}

void XMLUtils::SetStringArray(TiXmlNode* pRootNode, const char *strTag, const std::vector<std::string>& arrayValue)
{
  for (unsigned int i = 0; i < arrayValue.size(); i++)
    SetString(pRootNode, strTag, arrayValue.at(i));
}

void XMLUtils::SetString(TiXmlNode* pRootNode, const char *strTag, const std::string& strValue)
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
  std::string strValue = StringUtils::Format("%i", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetLong(TiXmlNode* pRootNode, const char *strTag, long value)
{
  std::string strValue = StringUtils::Format("%ld", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetFloat(TiXmlNode* pRootNode, const char *strTag, float value)
{
  std::string strValue = StringUtils::Format("%f", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetBoolean(TiXmlNode* pRootNode, const char *strTag, bool value)
{
  SetString(pRootNode, strTag, value ? "true" : "false");
}

void XMLUtils::SetHex(TiXmlNode* pRootNode, const char *strTag, uint32_t value)
{
  std::string strValue = StringUtils::Format("%x", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetPath(TiXmlNode* pRootNode, const char *strTag, const std::string& strValue)
{
  TiXmlElement newElement(strTag);
  newElement.SetAttribute("pathversion", path_version);
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    TiXmlText value(strValue);
    pNewNode->InsertEndChild(value);
  }
}

void XMLUtils::SetDate(TiXmlNode* pRootNode, const char *strTag, const CDateTime& date)
{
  SetString(pRootNode, strTag, date.IsValid() ? date.GetAsDBDate() : "");
}

void XMLUtils::SetDateTime(TiXmlNode* pRootNode, const char *strTag, const CDateTime& dateTime)
{
  SetString(pRootNode, strTag, dateTime.IsValid() ? dateTime.GetAsDBDateTime() : "");
}
