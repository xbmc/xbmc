/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FilterPreset.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <chrono>
#include <iomanip>
#include <sstream>

using namespace KODI::SEMANTIC;
using namespace XFILE;

namespace
{
std::string GetCurrentTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}
} // namespace

// FilterPreset implementation

std::string FilterPreset::Serialize() const
{
  CVariant data(CVariant::VariantTypeObject);

  data["name"] = name;
  data["description"] = description;
  data["createdAt"] = createdAt;
  data["modifiedAt"] = modifiedAt;

  // Serialize filters
  std::string filterData = filters.Serialize();
  CVariant filterVariant;
  if (CJSONVariantParser::Parse(filterData, filterVariant))
  {
    data["filters"] = filterVariant;
  }

  std::string serialized;
  CJSONVariantWriter::Write(data, serialized, true);
  return serialized;
}

bool FilterPreset::Deserialize(const std::string& data)
{
  if (data.empty())
    return false;

  CVariant variant;
  if (!CJSONVariantParser::Parse(data, variant))
  {
    CLog::Log(LOGERROR, "FilterPreset: Failed to parse JSON data");
    return false;
  }

  try
  {
    if (variant.isMember("name"))
      name = variant["name"].asString();

    if (variant.isMember("description"))
      description = variant["description"].asString();

    if (variant.isMember("createdAt"))
      createdAt = variant["createdAt"].asString();

    if (variant.isMember("modifiedAt"))
      modifiedAt = variant["modifiedAt"].asString();

    // Deserialize filters
    if (variant.isMember("filters"))
    {
      std::string filterData;
      CJSONVariantWriter::Write(variant["filters"], filterData, true);
      if (!filters.Deserialize(filterData))
      {
        CLog::Log(LOGERROR, "FilterPreset: Failed to deserialize filters");
        return false;
      }
    }

    return true;
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "FilterPreset: Exception during deserialization: {}", ex.what());
    return false;
  }
}

// CFilterPresetManager implementation

CFilterPresetManager::CFilterPresetManager()
{
  m_storageFile = GetStorageFilePath();
}

CFilterPresetManager::~CFilterPresetManager() = default;

std::string CFilterPresetManager::GetStorageFilePath() const
{
  // Store in user profile directory
  const std::shared_ptr<CProfileManager> profileManager =
      CServiceBroker::GetSettingsComponent()->GetProfileManager();

  std::string profileDir = profileManager->GetUserDataFolder();
  std::string semanticDir = URIUtils::AddFileToFolder(profileDir, "semantic");

  // Ensure directory exists
  if (!CDirectory::Exists(semanticDir))
  {
    CDirectory::Create(semanticDir);
  }

  return URIUtils::AddFileToFolder(semanticDir, "filter_presets.json");
}

bool CFilterPresetManager::Load()
{
  m_presets.clear();

  if (!CFile::Exists(m_storageFile))
  {
    CLog::Log(LOGINFO, "CFilterPresetManager: No preset file found, initializing defaults");
    InitializeDefaults();
    return true;
  }

  CFile file;
  if (!file.Open(m_storageFile))
  {
    CLog::Log(LOGERROR, "CFilterPresetManager: Failed to open preset file: {}", m_storageFile);
    return false;
  }

  std::string content;
  std::vector<uint8_t> buffer;
  buffer.resize(static_cast<size_t>(file.GetLength()));

  if (file.Read(buffer.data(), buffer.size()) != static_cast<ssize_t>(buffer.size()))
  {
    CLog::Log(LOGERROR, "CFilterPresetManager: Failed to read preset file");
    return false;
  }

  content.assign(reinterpret_cast<const char*>(buffer.data()), buffer.size());
  file.Close();

  CVariant data;
  if (!CJSONVariantParser::Parse(content, data))
  {
    CLog::Log(LOGERROR, "CFilterPresetManager: Failed to parse preset JSON");
    return false;
  }

  if (!data.isMember("presets") || !data["presets"].isArray())
  {
    CLog::Log(LOGWARNING, "CFilterPresetManager: Invalid preset file format");
    return false;
  }

  // Load each preset
  for (unsigned int i = 0; i < data["presets"].size(); ++i)
  {
    std::string presetData;
    CJSONVariantWriter::Write(data["presets"][i], presetData, true);

    FilterPreset preset;
    if (preset.Deserialize(presetData))
    {
      m_presets[preset.name] = preset;
    }
    else
    {
      CLog::Log(LOGWARNING, "CFilterPresetManager: Failed to load preset at index {}", i);
    }
  }

  CLog::Log(LOGINFO, "CFilterPresetManager: Loaded {} presets", m_presets.size());
  return true;
}

bool CFilterPresetManager::Save()
{
  CVariant data(CVariant::VariantTypeObject);
  CVariant presetsArray(CVariant::VariantTypeArray);

  // Serialize each preset
  for (const auto& [name, preset] : m_presets)
  {
    std::string presetData = preset.Serialize();
    CVariant presetVariant;
    if (CJSONVariantParser::Parse(presetData, presetVariant))
    {
      presetsArray.push_back(presetVariant);
    }
  }

  data["presets"] = presetsArray;
  data["version"] = 1;

  std::string content;
  CJSONVariantWriter::Write(data, content, true);

  CFile file;
  if (!file.OpenForWrite(m_storageFile, true))
  {
    CLog::Log(LOGERROR, "CFilterPresetManager: Failed to open preset file for writing: {}",
              m_storageFile);
    return false;
  }

  ssize_t written = file.Write(content.c_str(), content.length());
  file.Close();

  if (written != static_cast<ssize_t>(content.length()))
  {
    CLog::Log(LOGERROR, "CFilterPresetManager: Failed to write preset file");
    return false;
  }

  CLog::Log(LOGINFO, "CFilterPresetManager: Saved {} presets", m_presets.size());
  return true;
}

bool CFilterPresetManager::SavePreset(const std::string& name,
                                       const std::string& description,
                                       const CSearchFilters& filters)
{
  if (name.empty())
  {
    CLog::Log(LOGERROR, "CFilterPresetManager: Cannot save preset with empty name");
    return false;
  }

  FilterPreset preset;
  preset.name = name;
  preset.description = description;
  preset.filters = filters;

  std::string timestamp = GetCurrentTimestamp();

  // Check if updating existing preset
  if (HasPreset(name))
  {
    preset.createdAt = m_presets[name].createdAt;
    preset.modifiedAt = timestamp;
    CLog::Log(LOGINFO, "CFilterPresetManager: Updating existing preset '{}'", name);
  }
  else
  {
    preset.createdAt = timestamp;
    preset.modifiedAt = timestamp;
    CLog::Log(LOGINFO, "CFilterPresetManager: Creating new preset '{}'", name);
  }

  m_presets[name] = preset;
  return Save();
}

bool CFilterPresetManager::DeletePreset(const std::string& name)
{
  auto it = m_presets.find(name);
  if (it == m_presets.end())
  {
    CLog::Log(LOGWARNING, "CFilterPresetManager: Preset '{}' not found", name);
    return false;
  }

  m_presets.erase(it);
  CLog::Log(LOGINFO, "CFilterPresetManager: Deleted preset '{}'", name);
  return Save();
}

const FilterPreset* CFilterPresetManager::GetPreset(const std::string& name) const
{
  auto it = m_presets.find(name);
  if (it != m_presets.end())
  {
    return &it->second;
  }
  return nullptr;
}

std::vector<std::string> CFilterPresetManager::GetPresetNames() const
{
  std::vector<std::string> names;
  names.reserve(m_presets.size());

  for (const auto& [name, preset] : m_presets)
  {
    names.push_back(name);
  }

  return names;
}

std::vector<FilterPreset> CFilterPresetManager::GetAllPresets() const
{
  std::vector<FilterPreset> presets;
  presets.reserve(m_presets.size());

  for (const auto& [name, preset] : m_presets)
  {
    presets.push_back(preset);
  }

  return presets;
}

bool CFilterPresetManager::HasPreset(const std::string& name) const
{
  return m_presets.find(name) != m_presets.end();
}

void CFilterPresetManager::Clear()
{
  m_presets.clear();
}

void CFilterPresetManager::InitializeDefaults()
{
  // Create some useful default presets
  CSearchFilters filters;

  // Recent Movies (2020+)
  filters.Clear();
  filters.SetMediaType(MediaTypeFilter::Movies);
  filters.SetMinYear(2020);
  SavePreset("Recent Movies", "Movies from 2020 onwards", filters);

  // Action Movies
  filters.Clear();
  filters.SetMediaType(MediaTypeFilter::Movies);
  filters.AddGenre("Action");
  SavePreset("Action Movies", "Action genre movies", filters);

  // Classic Movies (1980-1999)
  filters.Clear();
  filters.SetMediaType(MediaTypeFilter::Movies);
  filters.SetYearRange(1980, 1999);
  SavePreset("Classic Movies", "Movies from the 80s and 90s", filters);

  // Recent TV Shows
  filters.Clear();
  filters.SetMediaType(MediaTypeFilter::TVShows);
  filters.SetMinYear(2020);
  SavePreset("Recent TV Shows", "TV episodes from 2020 onwards", filters);

  // Short Content
  filters.Clear();
  filters.SetDuration(DurationFilter::Short);
  SavePreset("Short Content", "Content under 30 minutes", filters);

  CLog::Log(LOGINFO, "CFilterPresetManager: Initialized {} default presets", m_presets.size());
}

FilterPreset CFilterPresetManager::CreateDefaultPreset(const std::string& name,
                                                        const std::string& description,
                                                        const CSearchFilters& filters) const
{
  FilterPreset preset;
  preset.name = name;
  preset.description = description;
  preset.filters = filters;
  preset.createdAt = GetCurrentTimestamp();
  preset.modifiedAt = preset.createdAt;
  return preset;
}
