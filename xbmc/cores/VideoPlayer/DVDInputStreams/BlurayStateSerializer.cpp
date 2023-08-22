/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayStateSerializer.h"

#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
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
  CXBMCTinyXML2 xmlDoc;

  auto* eRoot = xmlDoc.NewElement("libbluraystate");
  if (eRoot == nullptr)
    return false;
  eRoot->SetAttribute("version", BLURAYSTATESERIALIZER_VERSION);

  auto* xmlElement = xmlDoc.NewElement("playlistId");
  if (xmlElement == nullptr)
    return false;

  auto* xmlElementValue = xmlDoc.NewText(std::to_string(state.playlistId).c_str());
  if (xmlElementValue == nullptr)
    return false;

  xmlElement->InsertEndChild(xmlElementValue);
  eRoot->InsertEndChild(xmlElement);
  xmlDoc.InsertEndChild(eRoot);

  tinyxml2::XMLPrinter printer;
  xmlDoc.Accept(&printer);
  xmlstate = printer.CStr();
  return true;
}

bool CBlurayStateSerializer::XMLToBlurayState(BlurayState& state, const std::string& xmlstate)
{
  CXBMCTinyXML2 xmlDoc;

  if (!xmlDoc.Parse(xmlstate))
    return false;

  tinyxml2::XMLHandle hRoot(xmlDoc.RootElement());
  if (!hRoot.ToElement() ||
      !StringUtils::EqualsNoCase(hRoot.ToElement()->Value(), "libbluraystate"))
  {
    CLog::LogF(LOGERROR, "Failed to deserialize bluray state - failed to detect root element.");
    return false;
  }

  auto version = hRoot.ToElement()->Attribute("version");
  if (!version ||
      !StringUtils::EqualsNoCase(version, std::to_string(BLURAYSTATESERIALIZER_VERSION)))
  {
    CLog::LogF(LOGERROR, "Failed to deserialize bluray state - incompatible serializer version.");
    return false;
  }

  const auto* childElement = hRoot.ToElement()->FirstChildElement();
  while (childElement != nullptr)
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
