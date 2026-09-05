/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscXML.h"

#include "URL.h"
#include "filesystem/Directory.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <charconv>
#include <cstring>
#include <optional>

#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto XML_ROOT = "discstate";
constexpr auto XML_SLOTS = "slots";
constexpr auto XML_SLOT = "slot";
constexpr auto XML_SELECTED = "selected";
constexpr auto XML_TRAY = "tray";
constexpr auto XML_ATTR_TYPE = "type";
constexpr auto XML_ATTR_PATH = "path";
constexpr auto XML_ATTR_LABEL = "label";
constexpr auto XML_ATTR_INDEX = "index";
constexpr auto XML_ATTR_EJECTED = "ejected";

constexpr auto TYPE_DISC = "disc";
constexpr auto TYPE_REMOVED = "removed";
constexpr auto TYPE_NONE = "none";
} // namespace

std::string CGameClientDiscXML::GetXMLPath(const std::string& gamePath)
{
  return GetStateFilePath(gamePath, ".xml");
}

bool CGameClientDiscXML::Load(const std::string& gamePath, CGameClientDiscModel& model)
{
  model.Clear();

  if (gamePath.empty())
    return true;

  const std::string xmlPath = GetXMLPath(gamePath);

  if (!CFileUtils::Exists(xmlPath))
  {
    CLog::Log(LOGDEBUG, "Disc state XML {} does not exist, proceeding with empty disc model",
              CURL::GetRedacted(xmlPath));
    return true;
  }

  CLog::Log(LOGDEBUG, "Loading disc state XML {}", CURL::GetRedacted(xmlPath));

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(xmlPath))
  {
    CLog::Log(LOGWARNING, "Failed to load disc state XML {}: {} at line {}",
              CURL::GetRedacted(xmlPath), xmlDoc.ErrorStr(), xmlDoc.ErrorLineNum());
    return false;
  }

  const tinyxml2::XMLElement* rootElement = xmlDoc.RootElement();
  if (rootElement == nullptr || std::strcmp(rootElement->Name(), XML_ROOT) != 0)
  {
    CLog::Log(LOGWARNING, "Failed to parse disc state XML {}, missing root element '{}'",
              CURL::GetRedacted(xmlPath), XML_ROOT);
    return false;
  }

  const auto discs = ReadSlotsFromXML(rootElement);
  if (!discs)
  {
    CLog::Log(LOGWARNING, "Invalid slots in disc state XML {}", CURL::GetRedacted(xmlPath));
    return false;
  }
  model.SetDiscs(*discs);

  if (!ReadSelectedFromXML(rootElement, model))
  {
    CLog::Log(LOGWARNING, "Invalid selected disc in disc state XML {}", CURL::GetRedacted(xmlPath));
    model.Clear();
    return false;
  }
  ReadTrayFromXML(rootElement, model);

  return true;
}

bool CGameClientDiscXML::Save(const std::string& gamePath, const CGameClientDiscModel& model)
{
  if (gamePath.empty())
    return true;

  const std::string stateDirectory = GetDiscStateDirectory();
  if (!XFILE::CDirectory::Exists(stateDirectory) && !XFILE::CDirectory::Create(stateDirectory))
  {
    CLog::Log(LOGWARNING, "Failed to create disc state directory {}",
              CURL::GetRedacted(stateDirectory));
    return false;
  }

  const std::string xmlPath = GetXMLPath(gamePath);
  const std::string xmlDirectory = URIUtils::GetDirectory(xmlPath);

  CLog::Log(LOGDEBUG, "Saving disc state XML {}", CURL::GetRedacted(xmlPath));

  if (!XFILE::CDirectory::Exists(xmlDirectory) && !XFILE::CDirectory::Create(xmlDirectory))
  {
    CLog::Log(LOGWARNING, "Failed to create disc state subdirectory {}",
              CURL::GetRedacted(xmlDirectory));
    return false;
  }

  CXBMCTinyXML2 xmlDoc;

  tinyxml2::XMLElement* rootElement = xmlDoc.NewElement(XML_ROOT);
  xmlDoc.InsertEndChild(rootElement);

  WriteSlotsToXML(xmlDoc, rootElement, model);
  WriteSelectedToXML(xmlDoc, rootElement, model);
  WriteTrayToXML(xmlDoc, rootElement, model);

  if (!xmlDoc.SaveFile(xmlPath))
  {
    CLog::Log(LOGWARNING, "Failed to save disc state XML {}", CURL::GetRedacted(xmlPath));
    return false;
  }

  return true;
}

std::optional<std::vector<GameClientDiscEntry>> CGameClientDiscXML::ReadSlotsFromXML(
    const tinyxml2::XMLElement* rootElement)
{
  std::vector<GameClientDiscEntry> discs;

  const tinyxml2::XMLElement* slotsElement =
      rootElement != nullptr ? rootElement->FirstChildElement(XML_SLOTS) : nullptr;
  if (slotsElement == nullptr)
    return std::nullopt;
  const tinyxml2::XMLElement* slotElement = slotsElement->FirstChildElement(XML_SLOT);

  while (slotElement != nullptr)
  {
    const char* type = slotElement->Attribute(XML_ATTR_TYPE);
    const std::string label = slotElement->Attribute(XML_ATTR_LABEL) != nullptr
                                  ? slotElement->Attribute(XML_ATTR_LABEL)
                                  : "";

    if (type != nullptr && std::strcmp(type, TYPE_REMOVED) == 0)
    {
      discs.push_back({GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""});
    }
    else if (type != nullptr && std::strcmp(type, TYPE_DISC) == 0)
    {
      const char* path = slotElement->Attribute(XML_ATTR_PATH);
      if (path == nullptr || *path == '\0')
        return std::nullopt;
      discs.push_back({GameClientDiscEntry::DiscSlotType::Disc, path,
                       CGameClientDiscModel::DeriveBasename(path), label});
    }
    else
      return std::nullopt;

    slotElement = slotElement->NextSiblingElement(XML_SLOT);
  }

  return discs;
}

void CGameClientDiscXML::WriteSlotsToXML(CXBMCTinyXML2& xmlDoc,
                                         tinyxml2::XMLElement* rootElement,
                                         const CGameClientDiscModel& model)
{
  tinyxml2::XMLElement* slotsElement = xmlDoc.NewElement(XML_SLOTS);
  rootElement->InsertEndChild(slotsElement);

  for (const GameClientDiscEntry& disc : model.GetDiscs())
  {
    tinyxml2::XMLElement* slotElement = xmlDoc.NewElement(XML_SLOT);

    switch (disc.slotType)
    {
      case GameClientDiscEntry::DiscSlotType::Disc:
      {
        slotElement->SetAttribute(XML_ATTR_TYPE, TYPE_DISC);
        slotElement->SetAttribute(XML_ATTR_PATH, disc.path.c_str());

        if (!disc.cachedLabel.empty())
          slotElement->SetAttribute(XML_ATTR_LABEL, disc.cachedLabel.c_str());

        break;
      }
      case GameClientDiscEntry::DiscSlotType::RemovedSlot:
      {
        slotElement->SetAttribute(XML_ATTR_TYPE, TYPE_REMOVED);
        break;
      }
    }

    slotsElement->InsertEndChild(slotElement);
  }
}

void CGameClientDiscXML::ReadTrayFromXML(const tinyxml2::XMLElement* rootElement,
                                         CGameClientDiscModel& model)
{
  const tinyxml2::XMLElement* trayElement =
      rootElement != nullptr ? rootElement->FirstChildElement(XML_TRAY) : nullptr;

  const bool isEjected =
      trayElement != nullptr && trayElement->BoolAttribute(XML_ATTR_EJECTED, false);
  model.SetEjected(isEjected);
}

void CGameClientDiscXML::WriteTrayToXML(CXBMCTinyXML2& xmlDoc,
                                        tinyxml2::XMLElement* rootElement,
                                        const CGameClientDiscModel& model)
{
  tinyxml2::XMLElement* trayElement = xmlDoc.NewElement(XML_TRAY);
  rootElement->InsertEndChild(trayElement);
  trayElement->SetAttribute(XML_ATTR_EJECTED, model.IsEjected());
}

bool CGameClientDiscXML::ReadSelectedFromXML(const tinyxml2::XMLElement* rootElement,
                                             CGameClientDiscModel& model)
{
  const tinyxml2::XMLElement* selectedElement = rootElement->FirstChildElement(XML_SELECTED);
  if (selectedElement == nullptr)
    return false;

  const char* selectedType = selectedElement->Attribute(XML_ATTR_TYPE);
  if (selectedType == nullptr)
    return false;
  if (std::strcmp(selectedType, TYPE_NONE) == 0)
  {
    model.SetSelectedNoDisc();
    return true;
  }
  if (std::strcmp(selectedType, TYPE_DISC) != 0)
    return false;

  const char* index = selectedElement->Attribute(XML_ATTR_INDEX);
  if (index == nullptr)
    return false;

  size_t selectedIndex;
  const char* end = index + std::strlen(index);
  const auto result = std::from_chars(index, end, selectedIndex);
  return result.ec == std::errc{} && result.ptr == end &&
         model.SetSelectedDiscByIndex(selectedIndex);
}

void CGameClientDiscXML::WriteSelectedToXML(CXBMCTinyXML2& xmlDoc,
                                            tinyxml2::XMLElement* rootElement,
                                            const CGameClientDiscModel& model)
{
  tinyxml2::XMLElement* selectedElement = xmlDoc.NewElement(XML_SELECTED);
  rootElement->InsertEndChild(selectedElement);

  const std::optional<size_t> selectedIndex = model.GetSelectedDiscIndex();
  if (selectedIndex.has_value())
  {
    selectedElement->SetAttribute(XML_ATTR_TYPE, TYPE_DISC);
    selectedElement->SetAttribute(XML_ATTR_INDEX, static_cast<unsigned int>(*selectedIndex));
  }
  else
  {
    selectedElement->SetAttribute(XML_ATTR_TYPE, TYPE_NONE);
  }
}
