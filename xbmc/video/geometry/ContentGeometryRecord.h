/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "utils/Geometry.h"
#include "video/geometry/ContentGeometryCombiner.h"
#include "video/geometry/FrameSampling.h"

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

//! \brief Whether a stored row is a measurement or a note that measuring failed.
enum class ContentGeometryOutcome
{
  Measured, //!< opened and sampled; whether anything survived is hasReading
  Failed, //!< would not open, carried no video, or decoded nothing; no coded frame either
};

//! \brief Diagnostics behind one stored measurement. Written once and never read back.
struct ContentGeometryDetails
{
  std::string detector; //!< which detector produced the reading
  CombinerParams combining;
  SamplingParams sampling;
  std::vector<GeometrySample> samples;
  std::vector<GeometryCluster> clusters;
  unsigned int usable{0};
  unsigned int discarded{0};

  unsigned int unreadable{0}; //!< points that produced no reading
};

std::string EncodeContentGeometryDetails(const ContentGeometryDetails& details);

//! \brief Read EncodeContentGeometryDetails() back.
ContentGeometryDetails DecodeContentGeometryDetails(const std::string& json);

//! \brief Pack the shapes a title contains as "x,y,width,height", semicolon separated.
std::string EncodeGeometrySections(const std::vector<CRectInt>& sections);

//! \brief Read EncodeGeometrySections() back. A malformed value costs the shapes after the
//! point it stopped parsing.
std::vector<CRectInt> DecodeGeometrySections(const std::string& packed);

//! \brief One file's measured content geometry, as stored.
struct ContentGeometryRecord
{
  CRectInt coded; //!< the frame the rectangle was measured in
  CRectInt rect; //!< the content rectangle, in coded space

  //! \brief Outer extent of every measured geometry, in coded space. Equals rect unless the
  //! title varies.
  CRectInt envelope;

  //! \brief Every shape the title contains, dominant first, in coded space. Published rather
  //! than resolved from, and legitimately empty for an older or NFO-imported record.
  std::vector<CRectInt> sections;

  //! \brief The ratio the stream was displayed at when measured; zero for none declared.
  float displayAspect{0.0f};

  bool varies{false}; //!< the title's geometry changes partway through

  //! \brief A reading was obtained, which the rectangle alone does not say - it is the coded
  //! frame either way.
  bool hasReading{false};

  float confidence{0.0f}; //!< the dominant cluster's share of the surviving weight

  //! \brief A Failed record is invisible to everything but the sweep.
  ContentGeometryOutcome outcome{ContentGeometryOutcome::Measured};

  int algorithmVersion{CONTENT_GEOMETRY_ALGORITHM_VERSION};
  FileIdentity identity;
  CDateTime computed;

  //! \brief EncodeContentGeometryDetails() output. Disengaged means it was not read and storing
  //! leaves what is held alone; an engaged empty string is a scan that produced none.
  std::optional<std::string> details;

  //! \brief False for every Failed record, which carries no rectangle.
  bool IsValid() const { return coded.Width() > 0 && coded.Height() > 0; }
};

//! \brief Read or write the record on \p ar. The shapes travel with it, the diagnostics do not.
void Archive(CArchive& ar, ContentGeometryRecord& record);

//! \brief Write the record under \p movie as a <contentgeometry> element. The identity, version
//! and shapes travel with it; the diagnostics do not.
void SaveContentGeometryXML(TiXmlNode& movie, const ContentGeometryRecord& record);

//! \brief Read SaveContentGeometryXML() back. Nothing when \p movie carries no element, or one
//! without a usable coded frame.
std::optional<ContentGeometryRecord> LoadContentGeometryXML(const TiXmlElement& movie);

//! \brief Add the \p found shapes to the sections and the envelope, leaving the dominant ratio
//! alone. Nothing when the watch taught the record nothing new.
std::optional<ContentGeometryRecord> MergeDiscoveredGeometry(const ContentGeometryRecord& record,
                                                             const std::vector<CRectInt>& found,
                                                             const CombinerParams& params = {});

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

//! \brief The widest display-space ratio the record saw, from its own frame rather than a
//! playing stream. What the masking opens to. Zero when the record carries no reading.
float WidestAspect(const ContentGeometryRecord& record);

//! \brief What is stored for one file, without reading back the whole record.
struct ContentGeometryAttempt
{
  bool exists{false};
  int algorithmVersion{0};
  FileIdentity identity;
  ContentGeometryOutcome outcome{ContentGeometryOutcome::Measured};
};

//! \brief Whether the file still needs measuring: the attempt is missing, superseded, or
//! describes a different file. A failed attempt counts as done until the file changes.
bool NeedsContentGeometry(const ContentGeometryAttempt& attempt, const FileIdentity& identity);

} // namespace KODI::VIDEO::GEOMETRY
