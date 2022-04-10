/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDStateSerializer.h"

#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <charconv>
#include <cstring>
#include <sstream>

namespace
{
// Serializer version - used to avoid processing deprecated/legacy schemas
constexpr int DVDSTATESERIALIZER_VERSION = 2;
}

bool CDVDStateSerializer::DVDStateToXML(std::string& xmlstate, const DVDState& state)
{
  CXBMCTinyXML xmlDoc{"navstate"};

  TiXmlElement eRoot{"navstate"};
  eRoot.SetAttribute("version", DVDSTATESERIALIZER_VERSION);

  AddXMLElement(eRoot, "title", std::to_string(state.title));
  AddXMLElement(eRoot, "pgn", std::to_string(state.pgn));
  AddXMLElement(eRoot, "pgcn", std::to_string(state.pgcn));
  AddXMLElement(eRoot, "current_angle", std::to_string(state.current_angle));
  AddXMLElement(eRoot, "audio_num", std::to_string(state.audio_num));
  AddXMLElement(eRoot, "subp_num", std::to_string(state.subp_num));
  AddXMLElement(eRoot, "sub_enabled", state.sub_enabled ? "true" : "false");
  xmlDoc.InsertEndChild(eRoot);

  std::stringstream stream;
  stream << xmlDoc;
  xmlstate = stream.str();
  return true;
}

bool CDVDStateSerializer::XMLToDVDState(DVDState& state, const std::string& xmlstate)
{
  CXBMCTinyXML xmlDoc;

  xmlDoc.Parse(xmlstate);
  if (xmlDoc.Error())
    return false;

  TiXmlHandle hRoot(xmlDoc.RootElement());
  if (!hRoot.Element() || !StringUtils::EqualsNoCase(hRoot.Element()->Value(), "navstate"))
  {
    CLog::LogF(LOGERROR, "Failed to deserialize dvd state - failed to detect root element.");
    return false;
  }

  auto version = hRoot.Element()->Attribute("version");
  if (!version || !StringUtils::EqualsNoCase(version, std::to_string(DVDSTATESERIALIZER_VERSION)))
  {
    CLog::LogF(LOGERROR, "Failed to deserialize dvd state - incompatible serializer version.");
    return false;
  }

  const TiXmlElement* childElement = hRoot.Element()->FirstChildElement();
  while (childElement)
  {
    const std::string property = childElement->Value();
    if (property == "title")
    {
      std::from_chars(childElement->GetText(),
                      childElement->GetText() + std::strlen(childElement->GetText()), state.title);
    }
    else if (property == "pgn")
    {
      std::from_chars(childElement->GetText(),
                      childElement->GetText() + std::strlen(childElement->GetText()), state.pgn);
    }
    else if (property == "pgcn")
    {
      std::from_chars(childElement->GetText(),
                      childElement->GetText() + std::strlen(childElement->GetText()), state.pgcn);
    }
    else if (property == "current_angle")
    {
      std::from_chars(childElement->GetText(),
                      childElement->GetText() + std::strlen(childElement->GetText()),
                      state.current_angle);
    }
    else if (property == "subp_num")
    {
      std::from_chars(childElement->GetText(),
                      childElement->GetText() + std::strlen(childElement->GetText()),
                      state.subp_num);
    }
    else if (property == "audio_num")
    {
      std::from_chars(childElement->GetText(),
                      childElement->GetText() + std::strlen(childElement->GetText()),
                      state.audio_num);
    }
    else if (property == "sub_enabled")
    {
      state.sub_enabled = StringUtils::EqualsNoCase(childElement->GetText(), "true");
    }
    else
    {
      CLog::LogF(LOGWARNING, "Unmapped dvd state property {}, ignored.", childElement->Value());
    }
    childElement = childElement->NextSiblingElement();
  }
  return true;
}

void CDVDStateSerializer::AddXMLElement(TiXmlElement& root,
                                        const std::string& name,
                                        const std::string& value)
{
  TiXmlElement xmlElement{name};
  TiXmlText xmlElementValue = value;
  xmlElement.InsertEndChild(xmlElementValue);
  root.InsertEndChild(xmlElement);
}
