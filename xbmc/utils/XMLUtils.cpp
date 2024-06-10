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

#include <tinyxml2.h>

bool XMLUtils::GetHex(const tinyxml2::XMLNode* rootNode, const char* tag, uint32_t& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  return sscanf(node->FirstChild()->Value(), "%x", &value) == 1;
}

bool XMLUtils::GetUInt(const tinyxml2::XMLNode* rootNode, const char* tag, uint32_t& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  value = atol(node->FirstChild()->Value());
  return true;
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

bool XMLUtils::GetLong(const tinyxml2::XMLNode* rootNode, const char* tag, long& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  value = atol(node->FirstChild()->Value());
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

bool XMLUtils::GetDouble(const tinyxml2::XMLNode* rootNode, const char* tag, double& value)
{
  auto* node = rootNode->FirstChildElement(tag);
  if (!node || !node->FirstChild())
    return false;

  value = atof(node->FirstChild()->Value());
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

std::string XMLUtils::GetString(const tinyxml2::XMLNode* rootNode, const char* tag)
{
  std::string temp;
  GetString(rootNode, tag, temp);
  return temp;
}

bool XMLUtils::HasChild(const tinyxml2::XMLNode* rootNode, const char* tag)
{
  const auto* element = rootNode->FirstChildElement(tag);
  if (!element)
    return false;

  const auto* node = element->FirstChild();
  return (node != nullptr);
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

void XMLUtils::SetAdditiveString(tinyxml2::XMLNode* rootNode,
                                 const char* tag,
                                 const std::string& separator,
                                 const std::string& value)
{
  std::vector<std::string> list = StringUtils::Split(value, separator);
  for (auto i = list.begin(); i != list.end(); ++i)
    SetString(rootNode, tag, *i);
}

void XMLUtils::SetStringArray(tinyxml2::XMLNode* rootNode,
                              const char* tag,
                              const std::vector<std::string>& value)
{
  for (unsigned int i = 0; i < value.size(); i++)
    SetString(rootNode, tag, value.at(i));
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

tinyxml2::XMLNode* XMLUtils::SetInt(tinyxml2::XMLNode* rootNode, const char* tag, int value)
{
  std::string strValue = std::to_string(value);
  return SetString(rootNode, tag, strValue);
}

void XMLUtils::SetLong(tinyxml2::XMLNode* rootNode, const char* tag, long value)
{
  std::string strValue = std::to_string(value);
  SetString(rootNode, tag, strValue);
}

tinyxml2::XMLNode* XMLUtils::SetFloat(tinyxml2::XMLNode* rootNode, const char* tag, float value)
{
  std::string strValue = StringUtils::Format("{:f}", value);
  return SetString(rootNode, tag, strValue);
}

tinyxml2::XMLNode* XMLUtils::SetDouble(tinyxml2::XMLNode* rootNode, const char* tag, double value)
{
  std::string strValue = StringUtils::Format("{:f}", value);
  return SetString(rootNode, tag, strValue);
}

void XMLUtils::SetBoolean(tinyxml2::XMLNode* rootNode, const char* tag, bool value)
{
  SetString(rootNode, tag, value ? "true" : "false");
}

void XMLUtils::SetHex(tinyxml2::XMLNode* rootNode, const char* tag, uint32_t value)
{
  std::string strValue = StringUtils::Format("{:x}", value);
  SetString(rootNode, tag, strValue);
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

void XMLUtils::SetDate(tinyxml2::XMLNode* rootNode, const char* tag, const CDateTime& date)
{
  SetString(rootNode, tag, date.IsValid() ? date.GetAsDBDate() : "");
}

void XMLUtils::SetDateTime(tinyxml2::XMLNode* rootNode, const char* tag, const CDateTime& dateTime)
{
  SetString(rootNode, tag, dateTime.IsValid() ? dateTime.GetAsDBDateTime() : "");
}
