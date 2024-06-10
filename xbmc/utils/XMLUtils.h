/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

class CDateTime;

namespace tinyxml2
{
class XMLElement;
class XMLNode;
}

class XMLUtils
{
public:
  static bool HasChild(const tinyxml2::XMLNode* rootNode, const char* tag);

  static bool GetHex(const tinyxml2::XMLNode* rootNode, const char* tag, uint32_t& value);
  static bool GetUInt(const tinyxml2::XMLNode* rootNode, const char* tag, uint32_t& value);
  static bool GetLong(const tinyxml2::XMLNode* rootNode, const char* tag, long& value);
  static bool GetFloat(const tinyxml2::XMLNode* rootNode, const char* tag, float& value);
  static bool GetDouble(const tinyxml2::XMLNode* rootNode, const char* tag, double& value);
  static bool GetInt(const tinyxml2::XMLNode* rootNode, const char* tag, int& value);
  static bool GetBoolean(const tinyxml2::XMLNode* rootNode, const char* tag, bool& value);

  /*! \brief Get a string value from the xml tag
   If the specified tag isn't found strStringvalue is not modified and will contain whatever
   value it had before the method call.

   \param[in]     pRootNode the xml node that contains the tag
   \param[in]     strTag  the xml tag to read from
   \param[in,out] strStringValue  where to store the read string
   \return true on success, false if the tag isn't found
   */
  static bool GetString(const tinyxml2::XMLNode* rootNode, const char* tag, std::string& value);

  /*! \brief Get a string value from the xml tag

   \param[in]  pRootNode the xml node that contains the tag
   \param[in]  strTag the tag to read from

   \return the value in the specified tag or an empty string if the tag isn't found
   */
  static std::string GetString(const tinyxml2::XMLNode* rootNode, const char* tag);
  /*! \brief Get multiple tags, concatenating the values together.
   Transforms
     <tag>value1</tag>
     <tag clear="true">value2</tag>
     ...
     <tag>valuen</tag>
   into value2<sep>...<sep>valuen, appending it to the value string. Note that <value1> is overwritten by the clear="true" tag.

   \param rootNode    the parent containing the <tag>'s.
   \param tag         the <tag> in question.
   \param separator   the separator to use when concatenating values.
   \param value [out] the resulting string. Remains untouched if no <tag> is available, else is appended (or cleared based on the clear parameter).
   \param clear       if true, clears the string prior to adding tags, if tags are available. Defaults to false.
   */
  static bool GetAdditiveString(const tinyxml2::XMLNode* rootNode,
                                const char* tag,
                                const std::string& separator,
                                std::string& value,
                                bool clear = false);
  static bool GetStringArray(const tinyxml2::XMLNode* rootNode,
                             const char* tag,
                             std::vector<std::string>& value,
                             bool clear = false,
                             const std::string& separator = "");
  static bool GetPath(const tinyxml2::XMLNode* rootNode, const char* tag, std::string& value);
  static bool GetFloat(const tinyxml2::XMLNode* rootNode,
                       const char* tag,
                       float& value,
                       const float min,
                       const float max);
  static bool GetUInt(const tinyxml2::XMLNode* rootNode,
                      const char* tag,
                      uint32_t& value,
                      const uint32_t min,
                      const uint32_t max);
  static bool GetInt(
      const tinyxml2::XMLNode* rootNode, const char* tag, int& value, const int min, const int max);
  static bool GetDate(const tinyxml2::XMLNode* rootNode, const char* tag, CDateTime& date);
  static bool GetDateTime(const tinyxml2::XMLNode* rootNode, const char* tag, CDateTime& dateTime);
  /*! \brief Fetch a std::string copy of an attribute, if it exists.  Cannot distinguish between empty and non-existent attributes.
   \param element the element to query.
   \param tag the name of the attribute.
   \return the attribute, if it exists, else an empty string
   */
  static std::string GetAttribute(const tinyxml2::XMLElement* element, const char* tag);

  static tinyxml2::XMLNode* SetString(tinyxml2::XMLNode* rootNode,
                                      const char* tag,
                                      const std::string& value);
  static void SetAdditiveString(tinyxml2::XMLNode* rootNode,
                                const char* tag,
                                const std::string& separator,
                                const std::string& value);
  static void SetStringArray(tinyxml2::XMLNode* rootNode,
                             const char* tag,
                             const std::vector<std::string>& value);
  static tinyxml2::XMLNode* SetInt(tinyxml2::XMLNode* rootNode, const char* tag, int value);
  static tinyxml2::XMLNode* SetFloat(tinyxml2::XMLNode* rootNode, const char* tag, float value);
  static tinyxml2::XMLNode* SetDouble(tinyxml2::XMLNode* rootNode, const char* tag, double value);
  static void SetBoolean(tinyxml2::XMLNode* rootNode, const char* tag, bool value);
  static void SetHex(tinyxml2::XMLNode* rootNode, const char* tag, uint32_t value);
  static void SetPath(tinyxml2::XMLNode* rootNode, const char* tag, const std::string& value);
  static void SetLong(tinyxml2::XMLNode* rootNode, const char* tag, long value);
  static void SetDate(tinyxml2::XMLNode* rootNode, const char* tag, const CDateTime& date);
  static void SetDateTime(tinyxml2::XMLNode* rootNode, const char* tag, const CDateTime& dateTime);

  static const int path_version = 1;
};

