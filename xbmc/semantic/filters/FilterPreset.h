/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SearchFilters.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Named filter preset for quick access to common filter combinations
 *
 * A preset stores a named combination of search filters that can be
 * quickly applied. Useful for frequently used filter combinations.
 *
 * Example presets:
 * - "Recent Action Movies" (Movies + Action + 2020-2024)
 * - "Classic Comedies" (Movies + Comedy + 1980-1999)
 * - "Short Episodes" (TV Shows + Duration: Short)
 */
struct FilterPreset
{
  std::string name;                  //!< Display name for the preset
  std::string description;           //!< Optional description
  CSearchFilters filters;            //!< The filter configuration
  std::string createdAt;             //!< Creation timestamp
  std::string modifiedAt;            //!< Last modified timestamp

  /*!
   * @brief Serialize preset to JSON string
   */
  std::string Serialize() const;

  /*!
   * @brief Deserialize preset from JSON string
   */
  bool Deserialize(const std::string& data);
};

/*!
 * @brief Manager for filter presets with persistence
 *
 * This class manages a collection of named filter presets, providing:
 * - Preset creation, modification, and deletion
 * - Loading and saving from profile storage
 * - Built-in default presets
 * - Preset listing and retrieval
 *
 * Presets are stored in the user's profile directory in JSON format,
 * allowing persistence across sessions.
 *
 * Example usage:
 * \code
 * CFilterPresetManager manager;
 * manager.Load();
 *
 * // Create a new preset
 * CSearchFilters filters;
 * filters.SetMediaType(MediaTypeFilter::Movies);
 * filters.AddGenre("Action");
 * manager.SavePreset("Action Movies", "Action movie filter", filters);
 *
 * // Apply a preset
 * if (auto preset = manager.GetPreset("Action Movies"))
 * {
 *   searchDialog->ApplyFilters(preset->filters);
 * }
 * \endcode
 */
class CFilterPresetManager
{
public:
  CFilterPresetManager();
  ~CFilterPresetManager();

  /*!
   * @brief Load presets from storage
   * @return true if loaded successfully (or file doesn't exist yet)
   */
  bool Load();

  /*!
   * @brief Save presets to storage
   * @return true if saved successfully
   */
  bool Save();

  /*!
   * @brief Save or update a preset
   * @param name Preset name (unique identifier)
   * @param description Optional description
   * @param filters Filter configuration to save
   * @return true if saved successfully
   */
  bool SavePreset(const std::string& name,
                  const std::string& description,
                  const CSearchFilters& filters);

  /*!
   * @brief Delete a preset
   * @param name Preset name to delete
   * @return true if deleted successfully
   */
  bool DeletePreset(const std::string& name);

  /*!
   * @brief Get a preset by name
   * @param name Preset name
   * @return Pointer to preset if found, nullptr otherwise
   */
  const FilterPreset* GetPreset(const std::string& name) const;

  /*!
   * @brief Get all preset names
   * @return Vector of preset names
   */
  std::vector<std::string> GetPresetNames() const;

  /*!
   * @brief Get all presets
   * @return Vector of all presets
   */
  std::vector<FilterPreset> GetAllPresets() const;

  /*!
   * @brief Check if a preset exists
   * @param name Preset name
   * @return true if preset exists
   */
  bool HasPreset(const std::string& name) const;

  /*!
   * @brief Get the number of saved presets
   * @return Preset count
   */
  size_t GetPresetCount() const { return m_presets.size(); }

  /*!
   * @brief Clear all presets
   */
  void Clear();

  /*!
   * @brief Initialize with default presets if none exist
   */
  void InitializeDefaults();

private:
  std::map<std::string, FilterPreset> m_presets;
  std::string m_storageFile;

  /*!
   * @brief Get the storage file path
   */
  std::string GetStorageFilePath() const;

  /*!
   * @brief Create a default preset
   */
  FilterPreset CreateDefaultPreset(const std::string& name,
                                    const std::string& description,
                                    const CSearchFilters& filters) const;
};

} // namespace SEMANTIC
} // namespace KODI
