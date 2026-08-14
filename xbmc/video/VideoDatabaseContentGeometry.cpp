/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"
#include "dbwrappers/dataset.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/geometry/ContentGeometryRecord.h"
#include "video/geometry/GeometryTransforms.h"

#include <cinttypes>
#include <memory>
#include <utility>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

//! \brief The outcome column holds the enumerator's value. Anything else is a row from a future
//! version, treated as a measurement so its rectangle is still read.
ContentGeometryOutcome OutcomeFromColumn(int value)
{
  return value == static_cast<int>(ContentGeometryOutcome::Failed)
             ? ContentGeometryOutcome::Failed
             : ContentGeometryOutcome::Measured;
}

ContentGeometryRecord RecordFromDataset(dbiplus::Dataset& ds)
{
  ContentGeometryRecord geometry;

  const int codedWidth{ds.fv("codedWidth").get_asInt()};
  const int codedHeight{ds.fv("codedHeight").get_asInt()};
  geometry.coded = CRectInt{0, 0, codedWidth, codedHeight};

  geometry.rect = OriginSizeRect(ds.fv("rectX").get_asInt(), ds.fv("rectY").get_asInt(),
                                 ds.fv("rectWidth").get_asInt(), ds.fv("rectHeight").get_asInt());
  geometry.envelope =
      OriginSizeRect(ds.fv("envelopeX").get_asInt(), ds.fv("envelopeY").get_asInt(),
                     ds.fv("envelopeWidth").get_asInt(), ds.fv("envelopeHeight").get_asInt());

  geometry.displayAspect = ds.fv("displayAspect").get_asFloat();
  geometry.varies = ds.fv("varies").get_asBool();
  geometry.hasReading = ds.fv("hasReading").get_asBool();
  geometry.confidence = ds.fv("confidence").get_asFloat();
  geometry.outcome = OutcomeFromColumn(ds.fv("outcome").get_asInt());
  geometry.algorithmVersion = ds.fv("algorithmVersion").get_asInt();
  geometry.identity.size = ds.fv("fileSize").get_asInt64();
  geometry.identity.time = ds.fv("fileMTime").get_asInt64();
  geometry.computed.SetFromDBDateTime(ds.fv("dateComputed").get_asString());
  geometry.sections = DecodeGeometrySections(ds.fv("sections").get_asString());

  return geometry;
}

//! \brief What is already stored for one file, from a row carrying those four columns. Read by
//! the sweep and by the check taken before a file is played, so the two agree on "already done".
ContentGeometryAttempt AttemptFromDataset(dbiplus::Dataset& ds)
{
  ContentGeometryAttempt attempt;
  attempt.exists = true;
  attempt.algorithmVersion = ds.fv("algorithmVersion").get_asInt();
  attempt.identity.size = ds.fv("fileSize").get_asInt64();
  attempt.identity.time = ds.fv("fileMTime").get_asInt64();
  attempt.outcome = OutcomeFromColumn(ds.fv("outcome").get_asInt());
  return attempt;
}

//! \brief Named rather than starred; AttemptFromDataset() reads them by name, so the order is
//! nothing.
constexpr const char* ATTEMPT_COLUMNS{"algorithmVersion, fileSize, fileMTime, outcome"};

//! \brief What RecordFromDataset() reads, and what SetContentGeometry() writes. Named rather
//! than starred, so a column added later does not silently join the per-item read a listing
//! does, and stated once so the read and the write cannot disagree.
constexpr const char* RECORD_COLUMNS{
    "codedWidth, codedHeight, rectX, rectY, rectWidth, rectHeight, envelopeX, envelopeY, "
    "envelopeWidth, envelopeHeight, displayAspect, varies, hasReading, confidence, outcome, "
    "algorithmVersion, fileSize, fileMTime, dateComputed, sections"};

} // unnamed namespace

bool CVideoDatabase::SetContentGeometry(int idFile, const ContentGeometryRecord& geometry)
{
  bool begun{false};
  try
  {
    if (idFile < 0 || nullptr == m_pDB || nullptr == m_pDS)
      return false;

    // A failed attempt is a row with no rectangle in it, which is the whole point of storing
    // it, so only a record claiming to be a measurement has to carry one.
    if (geometry.outcome == ContentGeometryOutcome::Measured && !geometry.IsValid())
    {
      CLog::LogF(LOGERROR, "refusing content geometry for file {} with no coded frame", idFile);
      return false;
    }

    // An envelope that was never set would store as an empty rectangle and read back as a
    // measurement claiming no picture at all, so it falls back to the rectangle rather than
    // to nothing.
    const CRectInt envelope{geometry.envelope.IsEmpty() ? geometry.rect : geometry.envelope};

    // The column list comes from the constant the read uses, so adding a column is one edit
    // rather than three that have to be kept in step by hand.
    const std::string sql{PrepareSQL(
        "REPLACE INTO contentgeometry (idFile, %s) "
        "VALUES (%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%f,%i,%i,%f,%i,%i,%" PRId64 ",%" PRId64
        ",'%s','%s')",
        RECORD_COLUMNS, idFile, geometry.coded.Width(), geometry.coded.Height(), geometry.rect.x1,
        geometry.rect.y1, geometry.rect.Width(), geometry.rect.Height(), envelope.x1, envelope.y1,
        envelope.Width(), envelope.Height(), static_cast<double>(geometry.displayAspect),
        geometry.varies ? 1 : 0, geometry.hasReading ? 1 : 0,
        static_cast<double>(geometry.confidence), static_cast<int>(geometry.outcome),
        geometry.algorithmVersion, geometry.identity.size, geometry.identity.time,
        geometry.computed.GetAsDBDateTime().c_str(),
        EncodeGeometrySections(geometry.sections).c_str())};

    // The row and its diagnostics are two writes describing one measurement, so they land
    // together or not at all - a details write failing after the row was committed would answer
    // false with the row stored. Nested only when nothing is already writing.
    begun = !InTransaction();
    if (begun)
      BeginTransaction();

    bool stored{ExecuteQuery(sql)};

    // Only when the caller has some to state: a record that has been read back carries none,
    // and a listing round-trip must not erase the diagnostics it never read.
    if (stored && geometry.details)
    {
      stored = geometry.details->empty()
                   ? ExecuteQuery(
                         PrepareSQL("DELETE FROM contentgeometrydetails WHERE idFile=%i", idFile))
                   : ExecuteQuery(PrepareSQL(
                         "REPLACE INTO contentgeometrydetails (idFile, details) VALUES (%i,'%s')",
                         idFile, geometry.details->c_str()));
    }

    if (stored && (!begun || CommitTransaction()))
      return true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed for file {}", idFile);
  }

  if (begun)
    RollbackTransaction();
  return false;
}

int CVideoDatabase::GetPlayedFileId(const CFileItem& item)
{
  return GetFileId(item);
}

ContentGeometryLookup CVideoDatabase::GetContentGeometry(int idFile, const FileIdentity& identity)
{
  ContentGeometryRecord stored;
  if (!GetContentGeometryUnverified(idFile, stored))
    return {};

  if (!stored.identity.Matches(identity))
  {
    CLog::LogF(LOGINFO,
               "discarding content geometry for file {}: measured from size {} mtime {}, "
               "file is now size {} mtime {}",
               idFile, stored.identity.size, stored.identity.time, identity.size, identity.time);
    return {};
  }

  ContentGeometryLookup lookup;
  lookup.record = std::move(stored);
  lookup.state = StateOf(lookup.record);
  return lookup;
}

bool CVideoDatabase::GetContentGeometryUnverified(int idFile, ContentGeometryRecord& geometry)
{
  try
  {
    if (idFile < 0 || nullptr == m_pDB)
      return false;

    // Its own dataset, as GetStreamDetails() does and for the same reason: this is called from
    // GetDetailsForMovie/Episode/MusicVideo, which run while a caller iterates the result set
    // held by m_pDS. Querying that dataset here frees the rows being walked.
    const std::unique_ptr<dbiplus::Dataset> ds{m_pDB->CreateDataset()};
    if (!ds)
      return false;

    ds->query(PrepareSQL("SELECT %s FROM contentgeometry WHERE idFile=%i", RECORD_COLUMNS, idFile));
    if (ds->num_rows() == 0)
    {
      ds->close();
      return false;
    }

    geometry = RecordFromDataset(*ds);
    ds->close();

    // A failed attempt reads as no record at all: it exists for the sweep, which asks
    // GetContentGeometryAttempt() instead.
    return geometry.outcome == ContentGeometryOutcome::Measured && geometry.IsValid();
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed for file {}", idFile);
  }
  return false;
}

ContentGeometryAttempt CVideoDatabase::GetContentGeometryAttempt(int idFile)
{
  ContentGeometryAttempt attempt;
  try
  {
    if (idFile < 0 || nullptr == m_pDB)
      return attempt;

    const std::unique_ptr<dbiplus::Dataset> ds{m_pDB->CreateDataset()};
    if (!ds)
      return attempt;

    ds->query(
        PrepareSQL("SELECT %s FROM contentgeometry WHERE idFile=%i", ATTEMPT_COLUMNS, idFile));
    if (ds->num_rows() == 0)
    {
      ds->close();
      return attempt;
    }

    attempt = AttemptFromDataset(*ds);
    ds->close();
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed for file {}", idFile);
  }
  return attempt;
}

std::vector<ContentGeometryCandidate> CVideoDatabase::GetContentGeometryCandidates()
{
  std::vector<ContentGeometryCandidate> candidates;
  try
  {
    if (nullptr == m_pDB)
      return candidates;

    const std::unique_ptr<dbiplus::Dataset> ds{m_pDB->CreateDataset()};
    if (!ds)
      return candidates;

    // Every file, with whatever is already stored for it: an identity mismatch needs the file
    // stat'ing, so the rows come back whole and the caller decides. Ordered so that a run
    // interrupted and started again covers the library in the same order.
    ds->query("SELECT files.idFile, path.strPath, files.strFileName, "
              "contentgeometry.idFile AS storedFile, contentgeometry.algorithmVersion, "
              "contentgeometry.fileSize, contentgeometry.fileMTime, contentgeometry.outcome "
              "FROM files JOIN path ON path.idPath=files.idPath "
              "LEFT JOIN contentgeometry ON contentgeometry.idFile=files.idFile "
              "ORDER BY files.idFile");

    candidates.reserve(ds->num_rows());
    while (!ds->eof())
    {
      ContentGeometryCandidate candidate;
      candidate.idFile = ds->fv("idFile").get_asInt();
      candidate.path = URIUtils::AddFileToFolder(ds->fv("strPath").get_asString(),
                                                 ds->fv("strFileName").get_asString());

      if (!ds->fv("storedFile").get_isNull())
        candidate.attempt = AttemptFromDataset(*ds);

      candidates.emplace_back(std::move(candidate));
      ds->next();
    }
    ds->close();
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed");
  }
  return candidates;
}

