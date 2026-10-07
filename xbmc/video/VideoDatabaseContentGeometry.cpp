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

#include <cinttypes>
#include <memory>
#include <utility>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

ContentGeometryRecord RecordFromDataset(dbiplus::Dataset& ds)
{
  ContentGeometryRecord geometry;
  geometry.aspects = DecodeContentAspects(ds.fv("aspects").get_asString());
  geometry.algorithmVersion = ds.fv("algorithmVersion").get_asInt();
  geometry.identity.size = ds.fv("fileSize").get_asInt64();
  geometry.identity.time = ds.fv("fileMTime").get_asInt64();
  return geometry;
}

//! \brief What RecordFromDataset() reads and SetContentGeometry() writes.
constexpr const char* RECORD_COLUMNS{"aspects, algorithmVersion, fileSize, fileMTime"};

} // unnamed namespace

bool CVideoDatabase::SetContentGeometry(int idFile, const ContentGeometryRecord& geometry)
{
  try
  {
    if (idFile < 0 || nullptr == m_pDB)
      return false;

    // A record without an identity came from an NFO, which is trusted for the file as it
    // stands now.
    FileIdentity identity{geometry.identity};
    const std::unique_ptr<dbiplus::Dataset> ds{identity.IsKnown() ? nullptr
                                                                  : m_pDB->CreateDataset()};
    if (ds)
    {
      ds->query(PrepareSQL("SELECT strPath, strFileName FROM files JOIN path ON "
                           "path.idPath=files.idPath WHERE idFile=%i",
                           idFile));
      if (!ds->eof())
        identity = GetFileIdentity(URIUtils::AddFileToFolder(ds->fv("strPath").get_asString(),
                                                             ds->fv("strFileName").get_asString()));
      ds->close();
    }

    if (ExecuteQuery(PrepareSQL("REPLACE INTO contentgeometry (idFile, %s) "
                                "VALUES (%i,'%s',%i,%" PRId64 ",%" PRId64 ")",
                                RECORD_COLUMNS, idFile,
                                EncodeContentAspects(geometry.aspects).c_str(),
                                geometry.algorithmVersion, identity.size, identity.time)))
      return true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed for file {}", idFile);
  }

  return false;
}

ContentGeometryLookup CVideoDatabase::GetContentGeometry(int idFile, const FileIdentity& identity)
{
  std::optional<ContentGeometryRecord> stored{GetStoredContentGeometry(idFile)};
  if (!stored || !stored->HasReading())
    return {};

  if (!stored->identity.Matches(identity))
  {
    CLog::LogF(LOGINFO,
               "discarding content geometry for file {}: measured from size {} mtime {}, "
               "file is now size {} mtime {}",
               idFile, stored->identity.size, stored->identity.time, identity.size, identity.time);
    return {};
  }

  ContentGeometryLookup lookup;
  lookup.record = std::move(*stored);
  lookup.state = StateOf(lookup.record);
  return lookup;
}

std::optional<ContentGeometryRecord> CVideoDatabase::GetStoredContentGeometry(int idFile)
{
  try
  {
    if (idFile < 0 || nullptr == m_pDB)
      return std::nullopt;

    const std::unique_ptr<dbiplus::Dataset> ds{m_pDB->CreateDataset()};
    if (!ds)
      return std::nullopt;

    ds->query(PrepareSQL("SELECT %s FROM contentgeometry WHERE idFile=%i", RECORD_COLUMNS, idFile));
    std::optional<ContentGeometryRecord> stored;
    if (!ds->eof())
      stored = RecordFromDataset(*ds);
    ds->close();
    return stored;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed for file {}", idFile);
  }
  return std::nullopt;
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

    ds->query(
        "SELECT files.idFile, path.strPath, files.strFileName, "
        "contentgeometry.idFile AS storedFile, contentgeometry.aspects, "
        "contentgeometry.algorithmVersion, contentgeometry.fileSize, contentgeometry.fileMTime "
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
        candidate.stored = RecordFromDataset(*ds);

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

