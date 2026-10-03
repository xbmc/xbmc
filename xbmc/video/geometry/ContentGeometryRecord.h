/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class CArchive;
class TiXmlElement;
class TiXmlNode;

namespace KODI::VIDEO::GEOMETRY
{

//! \brief Bumped when the detector changes its results. Older records read as STALE and stay
//! usable.
inline constexpr int CONTENT_GEOMETRY_ALGORITHM_VERSION{1};

//! \brief Which file a stored measurement was taken from.
struct FileIdentity
{
  int64_t size{-1}; //!< bytes; negative when unknown
  int64_t time{-1}; //!< modification time, seconds since the epoch; negative when unknown

  bool IsKnown() const { return size >= 0 && time >= 0; }

  //! \brief An unknown identity matches nothing, not even another unknown one.
  bool Matches(const FileIdentity& other) const
  {
    return IsKnown() && other.IsKnown() && size == other.size && time == other.time;
  }
};

//! \return an unknown identity if the file cannot be stat'd, or reports neither a time nor a size
FileIdentity GetFileIdentity(const std::string& path);

//! \brief One file's measured content geometry, as stored.
struct ContentGeometryRecord
{
  //! \brief The display ratios the title contains, dominant first, to two decimals. Empty when
  //! measuring found nothing usable.
  std::vector<float> aspects;

  int algorithmVersion{CONTENT_GEOMETRY_ALGORITHM_VERSION};
  FileIdentity identity;

  bool HasReading() const { return !aspects.empty(); }
  bool Varies() const { return aspects.size() > 1; }
};

//! \brief \p aspect rounded to the two decimals a ratio is stored and written at.
float StoredAspect(float aspect);

//! \brief The ratios as "2.35;1.78".
std::string EncodeContentAspects(const std::vector<float>& aspects);

//! \brief Read EncodeContentAspects() back, skipping anything that is not a ratio.
std::vector<float> DecodeContentAspects(const std::string& packed);

//! \brief Read or write the record on \p ar.
void Archive(CArchive& ar, ContentGeometryRecord& record);

//! \brief Write the ratios under \p movie as a <contentgeometry> element, nothing when the
//! record has no reading.
void SaveContentGeometryXML(TiXmlNode& movie, const ContentGeometryRecord& record);

//! \brief Read SaveContentGeometryXML() back. Nothing when \p movie names no ratio.
std::optional<ContentGeometryRecord> LoadContentGeometryXML(const TiXmlElement& movie);

//! \brief What a lookup found.
enum class ContentGeometryState
{
  MISSING, //!< nothing stored, or what is stored describes a different file
  STALE, //!< stored by a superseded detector, and still usable
  VALID,
};

struct ContentGeometryLookup
{
  ContentGeometryState state{ContentGeometryState::MISSING};

  //! Meaningful only when state is not MISSING.
  ContentGeometryRecord record;

  bool HasRecord() const { return state != ContentGeometryState::MISSING; }
};

//! \brief STALE or VALID for a record in hand - MISSING is a lookup's answer, not a record's.
ContentGeometryState StateOf(const ContentGeometryRecord& record);

//! \brief The widest ratio the record holds, which the masking opens to. Zero with no reading.
float WidestAspect(const ContentGeometryRecord& record);

//! \brief Whether the file still needs measuring: nothing is stored, or what is stored is
//! superseded or describes a different file. A record that found nothing counts as done until
//! the file changes.
bool NeedsContentGeometry(const std::optional<ContentGeometryRecord>& stored,
                          const FileIdentity& identity);

} // namespace KODI::VIDEO::GEOMETRY
