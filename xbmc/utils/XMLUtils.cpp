/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XMLUtils.h"

#include "StringUtils.h"
#include "URL.h"
#include "XBDateTime.h"

bool XMLUtils::GetHex(const TiXmlNode* pRootNode, const char* strTag, uint32_t& hexValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  return sscanf(pNode->FirstChild()->Value(), "%x", &hexValue) == 1;
}

bool XMLUtils::GetHex(const tinyxml2::XMLNode* rootNode, const char* tag, uint32_t& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  return sscanf(node->FirstChild()->Value(), "%x", &value) == 1;
}

bool XMLUtils::GetUInt(const TiXmlNode* pRootNode, const char* strTag, uint32_t& uintValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  uintValue = atol(pNode->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetUInt(const tinyxml2::XMLNode* rootNode, const char* tag, uint32_t& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  value = atol(node->FirstChild()->Value());
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

bool XMLUtils::GetUInt(const tinyxml2::XMLNode* rootNode,
                       const char* tag,
                       uint32_t& value,
                       const uint32_t min,
                       const uint32_t max)
{
  if (GetUInt(rootNode, tag, value))
  {
    if (value < min)
      value = min;
    if (value > max)
      value = max;
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

bool XMLUtils::GetLong(const tinyxml2::XMLNode* rootNode, const char* tag, long& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  value = atol(node->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  iIntValue = atoi(pNode->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetInt(const tinyxml2::XMLNode* rootNode, const char* tag, int& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  value = atoi(node->FirstChild()->Value());
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

bool XMLUtils::GetInt(
    const tinyxml2::XMLNode* rootNode, const char* tag, int& value, const int min, const int max)
{
  if (GetInt(rootNode, tag, value))
  {
    if (value < min)
      value = min;
    if (value > max)
      value = max;
    return true;
  }
  return false;
}

bool XMLUtils::GetDouble(const TiXmlNode* root, const char* tag, double& value)
{
  const TiXmlNode* node = root->FirstChild(tag);
  if (!node || !node->FirstChild()) return false;
  value = atof(node->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetDouble(const tinyxml2::XMLNode* rootNode, const char* tag, double& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

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

bool XMLUtils::GetFloat(const tinyxml2::XMLNode* rootNode, const char* tag, float& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  value = static_cast<float>(atof(node->FirstChild()->Value()));
  return true;
}

bool XMLUtils::GetFloat(const TiXmlNode* pRootNode,
                        const char* tagName,
                        float& fValue,
                        const float fMin,
                        const float fMax)
{
  if (GetFloat(pRootNode, tagName, fValue))
  { // check range
    if (fValue < fMin)
      fValue = fMin;
    if (fValue > fMax)
      fValue = fMax;
    return true;
  }
  return false;
}

bool XMLUtils::GetFloat(const tinyxml2::XMLNode* rootNode,
                        const char* tag,
                        float& value,
                        const float min,
                        const float max)
{
  if (GetFloat(rootNode, tag, value))
  { // check range
    if (value < min)
      value = min;
    if (value > max)
      value = max;
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

bool XMLUtils::GetBoolean(const tinyxml2::XMLNode* rootNode, const char* tag, bool& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  std::string enabled = node->FirstChild()->Value();
  StringUtils::ToLower(enabled);
  if (enabled == "off" || enabled == "no" || enabled == "disabled" || enabled == "false" ||
      enabled == "0")
  {
    value = false;
  }
  else
  {
    value = true;
    if (enabled != "on" && enabled != "yes" && enabled != "enabled" && enabled != "true")
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
    if (encoded && StringUtils::CompareNoCase(encoded, "yes") == 0)
      strStringValue = CURL::Decode(strStringValue);
    return true;
  }
  strStringValue.clear();
  return true;
}

bool XMLUtils::GetString(const tinyxml2::XMLNode* rootNode, const char* tag, std::string& value)
{
  auto* element = rootNode->FirstChildElement(tag);
  if (!element)
    return false;

  auto* encoded = element->Attribute("urlencoded");
  auto* node = element->FirstChild();
  if (node)
  {
    value = node->Value();
    if (encoded && StringUtils::CompareNoCase(encoded, "yes") == 0)
      value = CURL::Decode(value);
    return true;
  }
  value.clear();
  return true;
}

std::string XMLUtils::GetString(const TiXmlNode* pRootNode, const char* strTag)
{
  std::string temp;
  GetString(pRootNode, strTag, temp);
  return temp;
}

std::string XMLUtils::GetString(const tinyxml2::XMLNode* rootNode, const char* tag)
{
  std::string temp;
  GetString(rootNode, tag, temp);
  return temp;
}

bool XMLUtils::HasChild(const TiXmlNode* pRootNode, const char* strTag)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement) return false;
  const TiXmlNode* pNode = pElement->FirstChild();
  return (pNode != NULL);
}

bool XMLUtils::HasChild(const tinyxml2::XMLNode* rootNode, const char* tag)
{
  const auto* element = rootNode->FirstChildElement(tag);
  if (!element)
    return false;

  const auto* node = element->FirstChild();
  return (node != nullptr);
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
      if (strStringValue.empty() || (clear && StringUtils::CompareNoCase(clear, "true") == 0))
        strStringValue = strTemp;
      else
        strStringValue += strSeparator+strTemp;
    }
    node = node->NextSiblingElement(strTag);
  }

  return bResult;
}

bool XMLUtils::GetAdditiveString(const tinyxml2::XMLNode* rootNode,
                                 const char* tag,
                                 const std::string& separator,
                                 std::string& value,
                                 bool clear)
{
  std::string temp;
  auto* node = rootNode->FirstChildElement(tag);
  bool result = false;
  if (node && node->FirstChild() && clear)
    value.clear();

  while (node)
  {
    if (node->FirstChild())
    {
      result = true;
      temp = node->FirstChild()->Value();
      auto* clear = node->Attribute("clear");
      if (value.empty() || (clear && StringUtils::CompareNoCase(clear, "true") == 0))
        value = temp;
      else
        value += separator + temp;
    }
    node = node->NextSiblingElement(tag);
  }

  return result;
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
      if (clearAttr && StringUtils::CompareNoCase(clearAttr, "true") == 0)
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

bool XMLUtils::GetStringArray(const tinyxml2::XMLNode* rootNode,
                              const char* tag,
                              std::vector<std::string>& value,
                              bool clear /* = false */,
                              const std::string& separator /* = "" */)
{
  std::string temp;
  auto* node = rootNode->FirstChildElement(tag);
  bool result = false;
  if (node && node->FirstChild() && clear)
    value.clear();

  while (node)
  {
    if (node->FirstChild())
    {
      result = true;
      temp = node->FirstChild()->Value();

      auto clearAttr = node->Attribute("clear");
      if (clearAttr && StringUtils::CompareNoCase(clearAttr, "true") == 0)
        value.clear();

      if (temp.empty())
        continue;

      if (separator.empty())
      {
        value.push_back(temp);
      }
      else
      {
        std::vector<std::string> tempArray = StringUtils::Split(temp, separator);
        value.insert(value.end(), tempArray.begin(), tempArray.end());
      }
    }
    node = node->NextSiblingElement(tag);
  }

  return result;
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
    if (encoded && StringUtils::CompareNoCase(encoded, "yes") == 0)
      strStringValue = CURL::Decode(strStringValue);
    return true;
  }
  strStringValue.clear();
  return false;
}

bool XMLUtils::GetPath(const tinyxml2::XMLNode* rootNode, const char* tag, std::string& value)
{
  auto* element = rootNode->FirstChildElement(tag);
  if (!element)
    return false;

  auto encoded = element->Attribute("urlencoded");
  auto* node = element->FirstChild();
  if (node)
  {
    value = node->Value();
    if (encoded && StringUtils::CompareNoCase(encoded, "yes") == 0)
      value = CURL::Decode(value);

    return true;
  }
  value.clear();
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

bool XMLUtils::GetDate(const tinyxml2::XMLNode* rootNode, const char* tag, CDateTime& date)
{
  std::string strDate;
  if (GetString(rootNode, tag, strDate) && !strDate.empty())
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

bool XMLUtils::GetDateTime(const tinyxml2::XMLNode* rootNode, const char* tag, CDateTime& dateTime)
{
  std::string strDateTime;
  if (GetString(rootNode, tag, strDateTime) && !strDateTime.empty())
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

std::string XMLUtils::GetAttribute(const tinyxml2::XMLElement* element, const char* tag)
{
  if (element)
  {
    auto attribute = element->Attribute(tag);
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

void XMLUtils::SetAdditiveString(tinyxml2::XMLNode* rootNode,
                                 const char* tag,
                                 const std::string& separator,
                                 const std::string& value)
{
  std::vector<std::string> list = StringUtils::Split(value, separator);
  for (auto i = list.begin(); i != list.end(); ++i)
    SetString(rootNode, tag, *i);
}

void XMLUtils::SetStringArray(TiXmlNode* pRootNode, const char *strTag, const std::vector<std::string>& arrayValue)
{
  for (unsigned int i = 0; i < arrayValue.size(); i++)
    SetString(pRootNode, strTag, arrayValue.at(i));
}

void XMLUtils::SetStringArray(tinyxml2::XMLNode* rootNode,
                              const char* tag,
                              const std::vector<std::string>& value)
{
  for (unsigned int i = 0; i < value.size(); i++)
    SetString(rootNode, tag, value.at(i));
}

TiXmlNode* XMLUtils::SetString(TiXmlNode* pRootNode, const char *strTag, const std::string& strValue)
{
  TiXmlElement newElement(strTag);
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    TiXmlText value(strValue);
    pNewNode->InsertEndChild(value);
  }
  return pNewNode;
}

tinyxml2::XMLNode* XMLUtils::SetString(tinyxml2::XMLNode* rootNode,
                                       const char* tag,
                                       const std::string& value)
{
  auto* element = rootNode->GetDocument()->NewElement(tag);
  auto* node = rootNode->InsertEndChild(element);
  if (node)
  {
    element->SetText(value.c_str());
  }
  return node;
}

TiXmlNode* XMLUtils::SetInt(TiXmlNode* pRootNode, const char *strTag, int value)
{
  std::string strValue = std::to_string(value);
  return SetString(pRootNode, strTag, strValue);
}

tinyxml2::XMLNode* XMLUtils::SetInt(tinyxml2::XMLNode* rootNode, const char* tag, int value)
{
  std::string strValue = std::to_string(value);
  return SetString(rootNode, tag, strValue);
}

void XMLUtils::SetLong(TiXmlNode* pRootNode, const char *strTag, long value)
{
  std::string strValue = std::to_string(value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetLong(tinyxml2::XMLNode* rootNode, const char* tag, long value)
{
  std::string strValue = std::to_string(value);
  SetString(rootNode, tag, strValue);
}

TiXmlNode* XMLUtils::SetFloat(TiXmlNode* pRootNode, const char *strTag, float value)
{
  std::string strValue = StringUtils::Format("{:f}", value);
  return SetString(pRootNode, strTag, strValue);
}

tinyxml2::XMLNode* XMLUtils::SetFloat(tinyxml2::XMLNode* rootNode, const char* tag, float value)
{
  std::string strValue = StringUtils::Format("{:f}", value);
  return SetString(rootNode, tag, strValue);
}

TiXmlNode* XMLUtils::SetDouble(TiXmlNode* pRootNode, const char* strTag, double value)
{
  std::string strValue = StringUtils::Format("{:f}", value);
  return SetString(pRootNode, strTag, strValue);
}

tinyxml2::XMLNode* XMLUtils::SetDouble(tinyxml2::XMLNode* rootNode, const char* tag, double value)
{
  std::string strValue = StringUtils::Format("{:f}", value);
  return SetString(rootNode, tag, strValue);
}

void XMLUtils::SetBoolean(TiXmlNode* pRootNode, const char *strTag, bool value)
{
  SetString(pRootNode, strTag, value ? "true" : "false");
}

void XMLUtils::SetBoolean(tinyxml2::XMLNode* rootNode, const char* tag, bool value)
{
  SetString(rootNode, tag, value ? "true" : "false");
}

void XMLUtils::SetHex(TiXmlNode* pRootNode, const char *strTag, uint32_t value)
{
  std::string strValue = StringUtils::Format("{:x}", value);
  SetString(pRootNode, strTag, strValue);
}

void XMLUtils::SetHex(tinyxml2::XMLNode* rootNode, const char* tag, uint32_t value)
{
  std::string strValue = StringUtils::Format("{:x}", value);
  SetString(rootNode, tag, strValue);
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

void XMLUtils::SetPath(tinyxml2::XMLNode* rootNode, const char* tag, const std::string& value)
{
  auto element = rootNode->GetDocument()->NewElement(tag);
  element->SetAttribute("pathversion", path_version);
  auto* node = rootNode->InsertEndChild(element);
  if (node)
  {
    element->SetText(value.c_str());
  }
}

void XMLUtils::SetDate(TiXmlNode* pRootNode, const char *strTag, const CDateTime& date)
{
  SetString(pRootNode, strTag, date.IsValid() ? date.GetAsDBDate() : "");
}

void XMLUtils::SetDate(tinyxml2::XMLNode* rootNode, const char* tag, const CDateTime& date)
{
  SetString(rootNode, tag, date.IsValid() ? date.GetAsDBDate() : "");
}

void XMLUtils::SetDateTime(TiXmlNode* pRootNode, const char *strTag, const CDateTime& dateTime)
{
  SetString(pRootNode, strTag, dateTime.IsValid() ? dateTime.GetAsDBDateTime() : "");
}

void XMLUtils::SetDateTime(tinyxml2::XMLNode* rootNode, const char* tag, const CDateTime& dateTime)
{
  SetString(rootNode, tag, dateTime.IsValid() ? dateTime.GetAsDBDateTime() : "");
}
