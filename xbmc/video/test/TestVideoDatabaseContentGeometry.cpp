/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "filesystem/File.h"
#include "test/TestUtils.h"
#include "utils/Artwork.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabaseDDL.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/geometry/ContentGeometryRecord.h"
#include "video/test/VideoDatabaseTestBase.h"

#include <algorithm>
#include <cstdlib>
#include <ranges>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

//! \brief The file as it is on disk, sized past 32 bits so truncation cannot round-trip.
const FileIdentity IDENTITY{8'000'000'000, 1'700'000'000};

ContentGeometryRecord MakeRecord(const FileIdentity& identity = IDENTITY)
{
  ContentGeometryRecord record;
  record.aspects = {2.35f, 1.78f};
  record.identity = identity;
  return record;
}

} // unnamed namespace

class TestVideoDatabaseContentGeometry : public VideoDatabaseTestBase
{
protected:
  TestVideoDatabaseContentGeometry()
    : VideoDatabaseTestBase("TestContentGeometry", "/test/contentgeometry/")
  {
  }

  bool DeleteFilesRow(int idFile)
  {
    return m_db.ExecuteQuery(StringUtils::Format("DELETE FROM files WHERE idFile={}", idFile));
  }

  int CountRows(int idFile)
  {
    return m_db.GetSingleValueInt(
        StringUtils::Format("SELECT count(*) FROM contentgeometry WHERE idFile={}", idFile));
  }
};

TEST_F(TestVideoDatabaseContentGeometry, StoresAndReadsBack)
{
  const int idFile{AddTestFile("stores.mkv")};
  const ContentGeometryRecord stored{MakeRecord(IDENTITY)};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, stored));

  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, IDENTITY)};
  ASSERT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(stored.aspects, lookup.record.aspects) << "the order is the dominant ratio first";
  EXPECT_TRUE(lookup.record.Varies());
  EXPECT_EQ(stored.algorithmVersion, lookup.record.algorithmVersion);
  EXPECT_EQ(stored.identity.size, lookup.record.identity.size);
  EXPECT_EQ(stored.identity.time, lookup.record.identity.time);
}

//! A file size past MySQL's signed 32-bit INTEGER.
TEST_F(TestVideoDatabaseContentGeometry, StoresAFileSizeBeyondThirtyTwoBits)
{
  const int idFile{AddTestFile("large.mkv")};
  const FileIdentity identity{68'719'476'736, 1'700'000'000}; // 64GB

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord(identity)));

  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, identity)};
  ASSERT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(identity.size, lookup.record.identity.size);
}

//! Same file, different content.
TEST_F(TestVideoDatabaseContentGeometry, AChangedFileReadsAsMissing)
{
  const int idFile{AddTestFile("recropped.mkv")};
  const FileIdentity before{8'000'000'000, 1'700'000'000};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord(before)));

  for (const FileIdentity& after :
       {FileIdentity{7'000'000'000, 1'700'000'000}, FileIdentity{8'000'000'000, 1'700'009'999}})
  {
    const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, after)};
    EXPECT_EQ(ContentGeometryState::MISSING, lookup.state);
    EXPECT_FALSE(lookup.HasRecord());
  }
}

//! A caller that could not stat the file gets no rectangle.
TEST_F(TestVideoDatabaseContentGeometry, AnUnknownIdentityReadsAsMissing)
{
  const int idFile{AddTestFile("unstattable.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));

  EXPECT_EQ(ContentGeometryState::MISSING, m_db.GetContentGeometry(idFile, {}).state);
}

TEST_F(TestVideoDatabaseContentGeometry, AFileWithNoRowReadsAsMissing)
{
  const int idFile{AddTestFile("nevermeasured.mkv")};

  EXPECT_EQ(ContentGeometryState::MISSING,
            m_db.GetContentGeometry(idFile, {8'000'000'000, 1'700'000'000}).state);
}

//! A superseded detector keeps serving.
TEST_F(TestVideoDatabaseContentGeometry, AnOlderAlgorithmVersionIsStaleNotMissing)
{
  const int idFile{AddTestFile("oldalgorithm.mkv")};

  ContentGeometryRecord record{MakeRecord(IDENTITY)};
  record.algorithmVersion = CONTENT_GEOMETRY_ALGORITHM_VERSION - 1;
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, record));

  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, IDENTITY)};
  EXPECT_EQ(ContentGeometryState::STALE, lookup.state);
  ASSERT_TRUE(lookup.HasRecord());
  EXPECT_EQ(record.aspects, lookup.record.aspects);
}

//! Identity is checked before version.
TEST_F(TestVideoDatabaseContentGeometry, AStaleRowFromAChangedFileIsStillMissing)
{
  const int idFile{AddTestFile("stalechanged.mkv")};

  ContentGeometryRecord record{MakeRecord({8'000'000'000, 1'700'000'000})};
  record.algorithmVersion = CONTENT_GEOMETRY_ALGORITHM_VERSION - 1;
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, record));

  EXPECT_EQ(ContentGeometryState::MISSING,
            m_db.GetContentGeometry(idFile, {9'000'000'000, 1'700'000'000}).state);
}

TEST_F(TestVideoDatabaseContentGeometry, StoringAgainReplacesRatherThanAccumulates)
{
  const int idFile{AddTestFile("remeasured.mkv")};
  const FileIdentity first{8'000'000'000, 1'700'000'000};
  const FileIdentity second{8'000'000'001, 1'700'000'001};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord(first)));

  ContentGeometryRecord updated{MakeRecord(second)};
  updated.aspects = {1.78f};
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, updated));

  EXPECT_EQ(1, CountRows(idFile));
  EXPECT_EQ(ContentGeometryState::MISSING, m_db.GetContentGeometry(idFile, first).state);

  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, second)};
  ASSERT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(updated.aspects, lookup.record.aspects);
}

/*!
 * A record saying measuring found nothing is stored precisely because it has no ratio. Without
 * it the sweep could not remember that it had already tried.
 */
TEST_F(TestVideoDatabaseContentGeometry, AnAttemptThatFoundNothingIsStored)
{
  const int idFile{AddTestFile("unreadable.mkv")};

  ContentGeometryRecord record;
  record.identity = IDENTITY;

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, record));
  EXPECT_EQ(1, CountRows(idFile));

  const std::optional<ContentGeometryRecord> stored{m_db.GetStoredContentGeometry(idFile)};
  ASSERT_TRUE(stored);
  EXPECT_FALSE(stored->HasReading());
  EXPECT_EQ(IDENTITY.size, stored->identity.size);
  EXPECT_EQ(IDENTITY.time, stored->identity.time);
  EXPECT_FALSE(NeedsContentGeometry(stored, IDENTITY));
}

//! Nothing that resolves a rectangle should have to know the difference between never having
//! tried and having tried and got nothing, so a failure reads back as no measurement at all.
TEST_F(TestVideoDatabaseContentGeometry, AFailureReadsAsMissingToEveryConsumer)
{
  const int idFile{AddTestFile("unreadable-lookup.mkv")};

  ContentGeometryRecord record;
  record.identity = IDENTITY;
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, record));

  EXPECT_EQ(ContentGeometryState::MISSING, m_db.GetContentGeometry(idFile, IDENTITY).state);
}

//! An NFO is trusted for the file as it stands when it is imported.
TEST_F(TestVideoDatabaseContentGeometry, ARecordWithNoIdentityTakesTheFilesOwn)
{
  XFILE::CFile* const file{XBMC_CREATETEMPFILE(".mkv")};
  ASSERT_NE(nullptr, file);
  const std::string path{XBMC_TEMPFILEPATH(file)};
  const int idFile{m_db.AddFile(path, URIUtils::GetDirectory(path))};
  ASSERT_GE(idFile, 0);

  ContentGeometryRecord imported;
  imported.aspects = {2.35f};
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, imported));

  const FileIdentity identity{GetFileIdentity(path)};
  ASSERT_TRUE(identity.IsKnown());
  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, identity)};
  EXPECT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(imported.aspects, lookup.record.aspects);

  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST_F(TestVideoDatabaseContentGeometry, AFileWithNoRowHasNothingStored)
{
  const int idFile{AddTestFile("neverattempted.mkv")};

  const std::optional<ContentGeometryRecord> stored{m_db.GetStoredContentGeometry(idFile)};
  EXPECT_FALSE(stored);
  EXPECT_TRUE(NeedsContentGeometry(stored, {8'000'000'000, 1'700'000'000}));
}

/*!
 * The sweep's work list. Every file comes back whether or not it has been measured, because
 * the criterion that retires a row - the file having changed underneath it - can only be
 * answered by stat'ing the file, which is not something SQL can do.
 */
TEST_F(TestVideoDatabaseContentGeometry, TheCandidateListCarriesEveryFileAndWhatItHolds)
{
  const int measured{AddTestFile("candidate-measured.mkv")};
  const int untouched{AddTestFile("candidate-untouched.mkv")};
  const int failed{AddTestFile("candidate-failed.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(measured, MakeRecord(IDENTITY)));

  ContentGeometryRecord failure;
  failure.identity = IDENTITY;
  ASSERT_TRUE(m_db.SetContentGeometry(failed, failure));

  const std::vector<ContentGeometryCandidate> candidates{m_db.GetContentGeometryCandidates()};
  ASSERT_EQ(3u, candidates.size());

  const auto find = [&candidates](int idFile)
  {
    const auto it = std::ranges::find(candidates, idFile, &ContentGeometryCandidate::idFile);
    EXPECT_NE(candidates.end(), it);
    return *it;
  };

  const ContentGeometryCandidate first{find(measured)};
  EXPECT_EQ("/test/contentgeometry/candidate-measured.mkv", first.path);
  ASSERT_TRUE(first.stored);
  EXPECT_TRUE(first.stored->HasReading());
  EXPECT_EQ(CONTENT_GEOMETRY_ALGORITHM_VERSION, first.stored->algorithmVersion);
  EXPECT_EQ(IDENTITY.size, first.stored->identity.size);
  EXPECT_FALSE(NeedsContentGeometry(first.stored, IDENTITY));

  EXPECT_FALSE(find(untouched).stored);
  EXPECT_TRUE(NeedsContentGeometry(find(untouched).stored, IDENTITY));

  ASSERT_TRUE(find(failed).stored);
  EXPECT_FALSE(find(failed).stored->HasReading());
}

/*!
 * A library update stores the geometry from inside its own transaction, and the write must
 * belong to it. Shown by rolling the caller's back afterwards: the geometry goes with it.
 */
TEST_F(TestVideoDatabaseContentGeometry, StoringInsideACallersTransactionJoinsIt)
{
  const int idFile{AddTestFile("nested.mkv")};

  m_db.BeginTransaction();
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));
  ASSERT_TRUE(m_db.InTransaction()) << "the caller's transaction was ended under it";
  m_db.RollbackTransaction();

  EXPECT_EQ(0, CountRows(idFile)) << "the write did not join the caller's transaction";
}

//! The delete_file trigger reaches the new tables.
TEST_F(TestVideoDatabaseContentGeometry, DeletingTheFileCascadesToTheGeometry)
{
  const int idFile{AddTestFile("cascade.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));
  ASSERT_EQ(1, CountRows(idFile));

  ASSERT_TRUE(m_db.DeleteFile(idFile));

  EXPECT_EQ(0, CountRows(idFile));
}

//! The trigger on its own, rather than whatever else DeleteFile() does on the way.
TEST_F(TestVideoDatabaseContentGeometry, TheTriggerAloneRemovesTheGeometry)
{
  const int idFile{AddTestFile("trigger.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));
  ASSERT_EQ(1, CountRows(idFile));

  ASSERT_TRUE(DeleteFilesRow(idFile));

  EXPECT_EQ(0, CountRows(idFile));
}

//! The cascade returns by itself after the drop-and-recreate cycle an upgrade performs.
TEST_F(TestVideoDatabaseContentGeometry, TheCascadeSurvivesTheAnalyticsCycleAnUpgradePerforms)
{
  const int idFile{AddTestFile("analyticscycle.mkv")};

  m_db.DropAnalytics();

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));
  ASSERT_TRUE(DeleteFilesRow(idFile));
  ASSERT_EQ(1, CountRows(idFile)) << "analytics were not actually dropped";

  KODI::DATABASE::CVideoDatabaseDDL::CreateAnalytics(m_db);

  const int idSecond{AddTestFile("analyticscycle2.mkv")};
  ASSERT_TRUE(m_db.SetContentGeometry(idSecond, MakeRecord({8'000'000'000, 1'700'000'000})));
  ASSERT_TRUE(DeleteFilesRow(idSecond));

  EXPECT_EQ(0, CountRows(idSecond)) << "the recreated delete_file trigger does not cover "
                                       "contentgeometry";
}

//! The real 150 to 152 upgrade, driven through CDatabaseManager.
TEST(TestVideoDatabaseMigration, UpgradingFrom150AddsTheTableAndItsCascade)
{
  const DatabaseSettings settings{TestDatabaseSettings()};

  // Started at 150 rather than earlier because 150 is upstream's set sort title migration, and
  // replaying it over a table the current schema already built adds a duplicate column and
  // aborts. This test is about the 152 upgrade.
  {
    CVideoDatabase old;
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
              old.Connect("MyVideosMigration150", settings, true));
    RestateVideo150(old);
    ASSERT_TRUE(old.ExecuteQuery("UPDATE version SET idVersion=150"));
    old.Close();
  }

  ASSERT_TRUE(UpgradeThroughManager(&CAdvancedSettings::m_databaseVideo, "MyVideosMigration"))
      << "the database manager could not migrate the video database";

  CVideoDatabase migrated;
  ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
            migrated.Connect("MyVideosMigration152", settings, false));

  EXPECT_EQ(152, migrated.GetSingleValueInt("SELECT idVersion FROM version"));

  // The same upgrade carries the declaration columns, which live with the other per-file
  // overrides rather than in the cache.
  EXPECT_EQ(0, migrated.GetSingleValueInt(
                   "SELECT count(*) FROM settings WHERE DeclaredAspect IS NOT NULL"));

  const int idFile{migrated.AddFile("/test/migration/upgraded.mkv", "/test/migration/")};
  ASSERT_GE(idFile, 0);

  ContentGeometryRecord record{MakeRecord({8'000'000'000, 1'700'000'000})};
  ASSERT_TRUE(migrated.SetContentGeometry(idFile, record))
      << "the migration did not create a usable contentgeometry table";

  ASSERT_TRUE(
      migrated.ExecuteQuery(StringUtils::Format("DELETE FROM files WHERE idFile={}", idFile)));

  EXPECT_EQ(0, migrated.GetSingleValueInt(StringUtils::Format(
                   "SELECT count(*) FROM contentgeometry WHERE idFile={}", idFile)))
      << "the trigger recreated after the migration does not cover contentgeometry";

  migrated.Close();
}

//! The stored read is for moving the row about, and never consults the file.
TEST_F(TestVideoDatabaseContentGeometry, TheStoredReadIgnoresIdentityEntirely)
{
  const int idFile{AddTestFile("export.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord(IDENTITY)));

  const std::optional<ContentGeometryRecord> stored{m_db.GetStoredContentGeometry(idFile)};
  ASSERT_TRUE(stored);
  EXPECT_EQ(IDENTITY.size, stored->identity.size);
  EXPECT_EQ(IDENTITY.time, stored->identity.time);

  EXPECT_FALSE(m_db.GetStoredContentGeometry(idFile + 100000));
}

TEST_F(TestVideoDatabaseContentGeometry, AVersionStoresTheMeasurementItWasAddedWith)
{
  CVideoInfoTag movie;
  movie.m_strTitle = "Movie";
  movie.m_strFileNameAndPath = "/test/contentgeometry/movie.mkv";
  movie.m_strPath = "/test/contentgeometry/";
  const int idMovie{m_db.SetDetailsForMovie(movie, KODI::ART::Artwork{})};
  ASSERT_GT(idMovie, 0);

  CFileItem version{"/test/contentgeometry/movie extended.mkv", false};
  version.GetVideoInfoTag()->m_contentGeometry = MakeRecord();
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie,
                                 m_db.AddOrValidateVideoVersionType("Extended"),
                                 VideoAssetType::VERSION, version));

  const std::optional<ContentGeometryRecord> stored{
      m_db.GetStoredContentGeometry(version.GetVideoInfoTag()->m_iFileId)};
  ASSERT_TRUE(stored);
  EXPECT_EQ(MakeRecord().aspects, stored->aspects);
}

/*!
 * The same storage and cascade against a real MySQL server, which the SQLite tests cannot
 * cover. Skipped unless pointed at one:
 *
 *   KODI_TEST_MYSQL_HOST=127.0.0.1 KODI_TEST_MYSQL_USER=kodi KODI_TEST_MYSQL_PASS=kodi
 *
 * The named database is created if the user may create it, and is left behind afterwards.
 */
TEST(TestVideoDatabaseContentGeometryMySQL, StoresAndCascadesOnMySQL)
{
  const char* const host{std::getenv("KODI_TEST_MYSQL_HOST")};
  const char* const user{std::getenv("KODI_TEST_MYSQL_USER")};
  const char* const pass{std::getenv("KODI_TEST_MYSQL_PASS")};

  if (!host || !user || !pass)
  {
    GTEST_SKIP() << "set KODI_TEST_MYSQL_HOST, KODI_TEST_MYSQL_USER and KODI_TEST_MYSQL_PASS "
                    "to run the MySQL half of these tests";
  }

  DatabaseSettings settings;
  settings.type = "mysql";
  settings.host = host;
  settings.user = user;
  settings.pass = pass;
  if (const char* const port{std::getenv("KODI_TEST_MYSQL_PORT")})
    settings.port = port;

  CVideoDatabase db;
  ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
            db.Connect("kodi_test_contentgeometry", settings, true))
      << "could not create the test database on " << host;

  ASSERT_TRUE(db.ExecuteQuery("DELETE FROM contentgeometry"));

  const int idFile{db.AddFile("/test/contentgeometry/mysql.mkv", "/test/contentgeometry/")};
  ASSERT_GE(idFile, 0);

  // Past the 2.1GB an INTEGER column would have stopped at.
  const FileIdentity identity{68'719'476'736, 1'700'000'000};
  ASSERT_TRUE(db.SetContentGeometry(idFile, MakeRecord(identity)));

  const ContentGeometryLookup lookup{db.GetContentGeometry(idFile, identity)};
  ASSERT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(identity.size, lookup.record.identity.size) << "the file size did not survive MySQL";
  EXPECT_EQ(MakeRecord().aspects, lookup.record.aspects);

  EXPECT_EQ(ContentGeometryState::MISSING,
            db.GetContentGeometry(idFile, {identity.size, identity.time + 1}).state);

  ASSERT_TRUE(db.ExecuteQuery(StringUtils::Format("DELETE FROM files WHERE idFile={}", idFile)));
  EXPECT_EQ(0, db.GetSingleValueInt(StringUtils::Format(
                   "SELECT count(*) FROM contentgeometry WHERE idFile={}", idFile)))
      << "the delete_file trigger does not cascade to contentgeometry on MySQL";

  db.Close();
}
