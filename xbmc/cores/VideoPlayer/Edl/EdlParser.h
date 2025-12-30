/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/EdlEdit.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

class CFileItem;

namespace EDL
{

struct EdlSourceLocation
{
  std::string
      source; //!< File path for file-based parsers, or identifier (e.g., "PVR", "plugin://")
  std::optional<int> index; //!< Optional index (line number, region index, property number, etc.)
};

/*!
 * @brief Result returned by EDL parsers
 *
 * Contains the raw parsed data from an EDL source. CEdl processes this
 * through AddEdit() which handles validation and applies settings like
 * autowait/autowind for commercial breaks.
 *
 * An empty result (both vectors empty) indicates the parser did not find
 * or could not parse its expected format.
 */
class CEdlParserResult
{
public:
  struct EditEntry
  {
    Edit edit;
    std::optional<EdlSourceLocation> source;
  };

  struct SceneMarkerEntry
  {
    std::chrono::milliseconds marker;
    std::optional<EdlSourceLocation> source;
  };

  const std::vector<EditEntry>& GetEdits() const { return m_edits; }
  const std::vector<SceneMarkerEntry>& GetSceneMarkers() const { return m_sceneMarkers; }

  void AddEdit(const Edit& edit, std::optional<EdlSourceLocation> source = std::nullopt)
  {
    m_edits.emplace_back(EditEntry{edit, source});
  }

  void AddSceneMarker(std::chrono::milliseconds marker,
                      std::optional<EdlSourceLocation> source = std::nullopt)
  {
    m_sceneMarkers.emplace_back(SceneMarkerEntry{marker, source});
  }

  bool IsEmpty() const { return m_edits.empty() && m_sceneMarkers.empty(); }

private:
  std::vector<EditEntry> m_edits;
  std::vector<SceneMarkerEntry> m_sceneMarkers;
};

/*!
 * @brief Pure interface for EDL parsers
 *
 * All EDL parsers must implement this interface. File-based parsers
 * should inherit from CEdlFileParserBase which provides common logic.
 */
class IEdlParser
{
public:
  virtual ~IEdlParser() = default;

  /*!
   * @brief Check if this parser can handle the given file item
   * @param item The file item to check
   * @return true if this parser can attempt to parse EDL data for this item
   */
  virtual bool CanParse(const CFileItem& item) const = 0;

  /*!
   * @brief Parse EDL data from a file item
   * @param item The file item (provides path for file-based, or PVR tags for PVR)
   * @param fps Frames per second (needed for frame-based formats, 0 if unavailable)
   * @return CEdlParserResult containing edits and scene markers, or empty if parsing failed
   */
  virtual CEdlParserResult Parse(const CFileItem& item, float fps) = 0;
};

/*!
 * @brief Base class for file-based EDL parsers
 *
 * CanParse only checks if the specific EDL file exists.
 * The local/LAN check is done ONCE in CEdl before iterating file parsers.
 *
 * Derived classes only need to implement GetEdlFilePath() and Parse().
 */
class CEdlFileParserBase : public IEdlParser
{
public:
  bool CanParse(const CFileItem& item) const override
  {
    return XFILE::CFile::Exists(GetEdlFilePath(item));
  }

protected:
  /*!
   * @brief Get the EDL file path for this parser type
   * @param item The file item
   * @return Path to the EDL file (e.g., media.edl, media.txt, media.Vprj)
   */
  virtual std::string GetEdlFilePath(const CFileItem& item) const = 0;
};

} // namespace EDL
