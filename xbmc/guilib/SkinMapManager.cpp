/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinMapManager.h"

#include "utils/log.h"

#include <algorithm>

#include <tinyxml.h>

using namespace KODI::GUILIB;

void CSkinMapManager::Clear()
{
  m_maps.clear();
  m_refs.clear();
}

namespace
{
CSkinMapManager::SkinMap LoadEntries(const TiXmlElement* mapElement)
{
  CSkinMapManager::SkinMap entries;
  const TiXmlElement* entry = mapElement->FirstChildElement("entry");
  while (entry)
  {
    const char* key = entry->Attribute("key");
    const TiXmlNode* textNode = entry->FirstChild();
    if (key && *key && textNode && textNode->Type() == TiXmlNode::TINYXML_TEXT)
    {
      auto [it, inserted] = entries.try_emplace(key, textNode->ValueStr());
      if (!inserted)
        CLog::LogF(LOGWARNING, "Skin map entry: duplicate key '{}' ignored", key);
    }
    entry = entry->NextSiblingElement("entry");
  }
  return entries;
}
} // unnamed namespace

void CSkinMapManager::LoadMaps(const TiXmlElement* node)
{
  if (!node)
    return;

  const TiXmlElement* mapElement = node->FirstChildElement("map");
  while (mapElement)
  {
    const char* name = mapElement->Attribute("name");
    if (name && *name)
    {
      const bool hadOldDefinition =
          m_maps.find(name) != m_maps.end() || m_refs.find(name) != m_refs.end();
      if (hadOldDefinition)
      {
        CLog::LogF(LOGWARNING, "Skin map '{}' is defined more than once, last definition wins",
                   name);
        m_maps.erase(name);
        m_refs.erase(name);
      }

      const char* ref = mapElement->Attribute("ref");
      const bool hasRef = ref && *ref;
      if (hasRef)
      {
        m_refs.insert_or_assign(name, ref);
        CLog::LogF(LOGDEBUG, "Skin map '{}' references map '{}'", name, ref);
      }

      SkinMap entries = LoadEntries(mapElement);
      if (!entries.empty())
      {
        CLog::LogF(LOGDEBUG, "Skin map '{}' loaded {} entries", name, entries.size());
        m_maps.insert_or_assign(name, std::move(entries));
      }
      else if (!hasRef)
      {
        CLog::LogF(LOGWARNING, "Skin map '{}' has no valid entries, skipping", name);
      }
    }
    mapElement = mapElement->NextSiblingElement("map");
  }

  // Validate refs — all referenced maps must be defined
  for (const auto& [refName, target] : m_refs)
  {
    if (m_maps.find(target) == m_maps.end() && m_refs.find(target) == m_refs.end())
      CLog::LogF(LOGWARNING, "Skin map '{}' references unknown map '{}'", refName, target);
  }
}

std::string CSkinMapManager::Lookup(std::string_view mapName,
                                    std::string_view key,
                                    std::vector<std::string> visited) const
{
  if (std::ranges::find(visited, mapName) != visited.end())
  {
    CLog::LogF(LOGWARNING, "Skin map '{}' has a circular ref chain, aborting lookup", mapName);
    return std::string{key};
  }
  visited.emplace_back(mapName);

  // Look for the key in this map's own entries first
  const auto mapIt = m_maps.find(mapName);
  if (mapIt != m_maps.end())
  {
    const auto entryIt = mapIt->second.find(key);
    if (entryIt != mapIt->second.end())
      return entryIt->second;
  }

  // Not found -- follow ref if one exists
  const auto refIt = m_refs.find(mapName);
  if (refIt != m_refs.end())
    return Lookup(refIt->second, key, visited);

  // No ref and key not found -- return raw value unchanged.
  return std::string{key};
}
