/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayStateSerializer.h"

#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <charconv>
#include <cstring>
#include <sstream>

namespace
{
// Serializer version - used to avoid processing deprecated/legacy schemas
constexpr int BLURAYSTATESERIALIZER_VERSION = 1;
} // namespace

bool CBlurayStateSerializer::BlurayStateToXML(std::string& xmlstate, const BlurayState& state)
{
  CXBMCTinyXML xmlDoc{"libbluraystate"};

  TiXmlElement eRoot{"libbluraystate"};
  eRoot.SetAttribute("version", BLURAYSTATESERIALIZER_VERSION);

  TiXmlElement xmlElement{"playlistId"};
  TiXmlText xmlElementValue = std::to_string(state.playlistId);
  xmlElement.InsertEndChild(xmlElementValue);
  eRoot.InsertEndChild(xmlElement);
  xmlDoc.InsertEndChild(eRoot);

  std::stringstream stream;
  stream << xmlDoc;
  xmlstate = stream.str();
  return true;
}

bool CBlurayStateSerializer::XMLToBlurayState(BlurayState& state, const std::string& xmlstate)
{
  CXBMCTinyXML xmlDoc;

  xmlDoc.Parse(xmlstate);
  if (xmlDoc.Error())
    return false;

  TiXmlHandle hRoot(xmlDoc.RootElement());
  if (!hRoot.Element() || !StringUtils::EqualsNoCase(hRoot.Element()->Value(), "libbluraystate"))
  {
    CLog::LogF(LOGERROR, "Failed to deserialize bluray state - failed to detect root element.");
    return false;
  }

  auto version = hRoot.Element()->Attribute("version");
  if (!version ||
      !StringUtils::EqualsNoCase(version, std::to_string(BLURAYSTATESERIALIZER_VERSION)))
  {
    CLog::LogF(LOGERROR, "Failed to deserialize bluray state - incompatible serializer version.");
    return false;
  }

  const TiXmlElement* childElement = hRoot.Element()->FirstChildElement();
  while (childElement)
  {
    const std::string property = childElement->Value();
    if (property == "playlistId")
    {
      std::from_chars(childElement->GetText(),
                      childElement->GetText() + std::strlen(childElement->GetText()),
                      state.playlistId);
    }
    else
    {
      CLog::LogF(LOGWARNING, "Unmapped bluray state property {}, ignored.", childElement->Value());
    }
    childElement = childElement->NextSiblingElement();
  }
  return true;
}
