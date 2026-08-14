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

//! \brief Bumped when the detector changes in a way that alters its results. Records made by
//! an older version read as ContentGeometryState::STALE and remain usable.
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

//! \brief Whether a stored row is a measurement, or a note that measuring was tried and failed,
//! so the sweep does not reattempt the same unreadable file every run.
enum class ContentGeometryOutcome
{
  //! The file was opened and sampled. Whether anything survived is hasReading; either way the
  //! coded frame is known.
  Measured,

  //! The file would not open, carried no video stream, or decoded no picture. Not even its
  //! coded frame is known.
  Failed,
};

//! \brief Diagnostics behind one stored measurement, several kB of per-sample readings. Written
//! once by the scan that produced it and never read back.
struct ContentGeometryDetails
{
  std::string detector; //!< which detector produced the reading
  CombinerParams combining;
  SamplingParams sampling;
  std::vector<GeometrySample> samples;
  std::vector<GeometryCluster> clusters;
  unsigned int usable{0};
  unsigned int discarded{0};

  //! \brief Points that produced no reading, so the counts account for every point scheduled.
  unsigned int unreadable{0};
};

std::string EncodeContentGeometryDetails(const ContentGeometryDetails& details);

//! \brief Read EncodeContentGeometryDetails() back. Nothing at runtime calls it; it is what
//! pins the shape of what the encoder writes.
ContentGeometryDetails DecodeContentGeometryDetails(const std::string& json);

//! \brief Pack the shapes a title contains as "x,y,width,height" per shape, semicolon separated.
//! Not JSON: this is read for every visible row of a list refresh.
std::string EncodeGeometrySections(const std::vector<CRectInt>& sections);

//! \brief Read EncodeGeometrySections() back, tolerant of which separator appears where. A
//! malformed value costs the shapes after the point it stopped parsing and nothing else.
std::vector<CRectInt> DecodeGeometrySections(const std::string& packed);

//! \brief One file's measured content geometry, as stored.
struct ContentGeometryRecord
{
  CRectInt coded; //!< the frame the rectangle was measured in
  CRectInt rect; //!< the content rectangle, in coded space

  //! \brief Outer extent of every measured geometry, in coded space. Equals rect unless the
  //! title varies. See CombinedGeometry::envelope for why both are kept.
  CRectInt envelope;

  //! \brief Every shape the title is known to contain, dominant first, in coded space. Published
  //! rather than resolved from, and legitimately empty for an older or NFO-imported record.
  std::vector<CRectInt> sections;

  //! \brief The ratio the stream was displayed at when it was measured. Zero means the stream
  //! declared none, which is taken as square pixels.
  float displayAspect{0.0f};

  bool varies{false}; //!< the title's geometry changes partway through

  //! \brief A reading was obtained, which the rectangle alone does not say - it is the coded
  //! frame either way.
  bool hasReading{false};

  float confidence{0.0f}; //!< the dominant cluster's share of the surviving weight

  //! \brief A Failed record carries no rectangle, and is invisible to everything except the
  //! sweep that decides what still needs measuring.
  ContentGeometryOutcome outcome{ContentGeometryOutcome::Measured};

  int algorithmVersion{CONTENT_GEOMETRY_ALGORITHM_VERSION};
  FileIdentity identity;
  CDateTime computed;

  //! \brief EncodeContentGeometryDetails() output. Empty means it was not read and storing must
  //! leave what is held alone; an engaged empty string is a scan that produced no diagnostics.
  std::optional<std::string> details;

  //! \brief False for every Failed record, which carries no rectangle.
  bool IsValid() const { return coded.Width() > 0 && coded.Height() > 0; }
};

//! \brief Read or write the record on \p ar, whichever way the archive is open. The shapes
//! travel with the rectangle; the retained diagnostics stay behind.
void Archive(CArchive& ar, ContentGeometryRecord& record);

//! \brief Write the record under \p movie as a <contentgeometry> element. The identity, version
//! and shapes travel with the rectangle; the per-sample diagnostics do not.
void SaveContentGeometryXML(TiXmlNode& movie, const ContentGeometryRecord& record);

/*!
 * \brief Read SaveContentGeometryXML() back.
 *
 * \return the record, or nothing when \p movie carries no <contentgeometry> element or one
 *         without a usable coded frame
 */
std::optional<ContentGeometryRecord> LoadContentGeometryXML(const TiXmlElement& movie);

/*!
 * \brief Add shapes a playback saw to the sections and the envelope. The dominant ratio is left
 *        alone - the scan's uniform sampling is the better instrument for it.
 *
 * \param found shapes in coded space, from outside the opening and closing exclusions
 * \return the record to store, or nothing when the watch taught it nothing new
 */
std::optional<ContentGeometryRecord> MergeDiscoveredGeometry(const ContentGeometryRecord& record,
                                                             const std::vector<CRectInt>& found,
                                                             const CombinerParams& params = {});

//! \brief What a lookup found.
enum class ContentGeometryState
{
  //! Nothing stored, or what was stored describes a different file.
  MISSING,

  //! Stored by a superseded detector, and still usable.
  STALE,

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

/*!
 * \brief The display-space ratio of everything the record ever saw, built from the record's own
 *        frame and display ratio rather than from a playing stream.
 *
 * \return zero when the record carries no reading or no frame to express one against
 */
float EnvelopeAspect(const ContentGeometryRecord& record);

//! \brief What is already stored for one file, without reading back the whole record. All the
//! background sweep needs to decide whether a file is a candidate.
struct ContentGeometryAttempt
{
  bool exists{false};
  int algorithmVersion{0};
  FileIdentity identity;
  ContentGeometryOutcome outcome{ContentGeometryOutcome::Measured};
};

/*!
 * \brief Does this file still need measuring? True when the attempt is missing, superseded, or
 *        describes a different file. A failed attempt counts as done until the file changes.
 *
 * \param identity the file as it is now; an unknown one matches nothing, so callers must not
 *                 pass one they could not establish
 */
bool NeedsContentGeometry(const ContentGeometryAttempt& attempt, const FileIdentity& identity);

} // namespace KODI::VIDEO::GEOMETRY
