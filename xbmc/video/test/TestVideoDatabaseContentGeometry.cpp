/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabaseDDL.h"
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

constexpr int CODED_WIDTH{1920};
constexpr int CODED_HEIGHT{1080};

//! A 2.39:1 picture in a 16:9 frame - 132 lines of black top and bottom.
constexpr int CONTENT_TOP{132};
constexpr int CONTENT_BOTTOM{948};

//! \brief A taller sequence the scan also caught, so the envelope is wider than the rectangle
//! and the two cannot be confused for one another in a round trip.
constexpr int ENVELOPE_TOP{70};
constexpr int ENVELOPE_BOTTOM{1010};

//! \brief The file as it is on disk, sized past 32 bits so truncation cannot round-trip.
const FileIdentity IDENTITY{8'000'000'000, 1'700'000'000};

ContentGeometryRecord MakeRecord(const FileIdentity& identity = IDENTITY)
{
  ContentGeometryRecord record;
  record.coded = CRectInt{0, 0, CODED_WIDTH, CODED_HEIGHT};
  record.rect = CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM};
  record.envelope = CRectInt{0, ENVELOPE_TOP, CODED_WIDTH, ENVELOPE_BOTTOM};

  // Set to values a default-constructed record does not hold, so that reading them back is
  // evidence they were stored rather than of the destination being untouched.
  record.displayAspect = 16.0f / 9.0f;
  record.varies = true;
  record.hasReading = true;
  record.confidence = 0.875f;
  record.identity = identity;
  record.computed = CDateTime(2026, 8, 6, 21, 30, 0);
  record.sections.push_back(CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM});
  record.sections.push_back(CRectInt{0, ENVELOPE_TOP, CODED_WIDTH, ENVELOPE_BOTTOM});

  // Carries both quote characters, to exercise the escaping of the blob column.
  record.details = R"({"detector":"contentbar","note":"O'Brien said \"bars\""})";

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

  //! \brief The stored diagnostics, which no lookup reads back.
  std::string StoredDetails(int idFile)
  {
    return m_db.GetSingleValue(
        StringUtils::Format("SELECT details FROM contentgeometrydetails WHERE idFile={}", idFile));
  }

  int CountDetailRows(int idFile)
  {
    return m_db.GetSingleValueInt(
        StringUtils::Format("SELECT count(*) FROM contentgeometrydetails WHERE idFile={}", idFile));
  }
};

TEST_F(TestVideoDatabaseContentGeometry, StoresAndReadsBack)
{
  const int idFile{AddTestFile("stores.mkv")};
  const ContentGeometryRecord stored{MakeRecord(IDENTITY)};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, stored));

  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, IDENTITY)};
  ASSERT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(stored.coded, lookup.record.coded);
  EXPECT_EQ(stored.rect, lookup.record.rect);
  EXPECT_EQ(stored.envelope, lookup.record.envelope);
  EXPECT_NE(lookup.record.rect, lookup.record.envelope) << "the two columns were confused";
  EXPECT_FLOAT_EQ(stored.displayAspect, lookup.record.displayAspect);
  EXPECT_TRUE(lookup.record.varies);
  EXPECT_EQ(stored.varies, lookup.record.varies);
  EXPECT_EQ(stored.hasReading, lookup.record.hasReading);
  EXPECT_FLOAT_EQ(stored.confidence, lookup.record.confidence);
  EXPECT_EQ(stored.algorithmVersion, lookup.record.algorithmVersion);
  EXPECT_EQ(stored.identity.size, lookup.record.identity.size);
  EXPECT_EQ(stored.identity.time, lookup.record.identity.time);
  EXPECT_EQ(stored.sections, lookup.record.sections);
  EXPECT_EQ(stored.computed.GetAsDBDateTime(), lookup.record.computed.GetAsDBDateTime());
}

/*!
 * The diagnostics are several kB of retained per-sample readings and no consumer of a
 * rectangle wants them, so a lookup does not fetch them - this row is read once per item a
 * listing fills. They are still stored, and still escape correctly.
 */
TEST_F(TestVideoDatabaseContentGeometry, ALookupDoesNotDragTheDiagnosticsWithIt)
{
  const int idFile{AddTestFile("diagnostics.mkv")};
  const ContentGeometryRecord stored{MakeRecord(IDENTITY)};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, stored));

  EXPECT_EQ(*stored.details, StoredDetails(idFile));

  EXPECT_FALSE(m_db.GetContentGeometry(idFile, IDENTITY).record.details.has_value());

  ContentGeometryRecord unverified;
  ASSERT_TRUE(m_db.GetContentGeometryUnverified(idFile, unverified));
  EXPECT_FALSE(unverified.details.has_value());
}

/*!
 * A listing fills a tag from the lookup above and a library update writes that tag back, so
 * the common write states no diagnostics at all. Erasing them on it would lose the scan's
 * account of the measurement to a row being touched for an unrelated reason.
 */
TEST_F(TestVideoDatabaseContentGeometry, StoringARecordThatStatesNoDiagnosticsKeepsThem)
{
  const int idFile{AddTestFile("roundtrip.mkv")};
  const ContentGeometryRecord stored{MakeRecord(IDENTITY)};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, stored));

  ContentGeometryRecord readBack;
  ASSERT_TRUE(m_db.GetContentGeometryUnverified(idFile, readBack));
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, readBack));

  EXPECT_EQ(*stored.details, StoredDetails(idFile));
}

//! A re-measurement that could not read the file states empty diagnostics, and what the
//! measurement it replaces recorded goes with it.
TEST_F(TestVideoDatabaseContentGeometry, StoringEmptyDiagnosticsClearsThem)
{
  const int idFile{AddTestFile("cleared.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord(IDENTITY)));
  ASSERT_FALSE(StoredDetails(idFile).empty());

  ContentGeometryRecord failure;
  failure.outcome = ContentGeometryOutcome::Failed;
  failure.identity = IDENTITY;
  failure.details = std::string{};
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, failure));

  EXPECT_TRUE(StoredDetails(idFile).empty());
}

//! The shapes a title contains are stored in their own right, so that resolving one costs no
//! parse of the diagnostics they were measured alongside.
TEST_F(TestVideoDatabaseContentGeometry, TheSectionsRoundTripWithoutTheDiagnostics)
{
  const int idFile{AddTestFile("sections.mkv")};

  ContentGeometryRecord record{MakeRecord(IDENTITY)};

  // A pillarboxed shape as well, so the round trip covers a rectangle whose origin is not zero
  // on either axis and cannot be reconstructed from the others.
  record.sections.push_back(CRectInt{240, 0, CODED_WIDTH - 240, CODED_HEIGHT});
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, record));

  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, IDENTITY)};
  EXPECT_EQ(record.sections, lookup.record.sections);
  ASSERT_EQ(3u, lookup.record.sections.size()) << "the order is the dominant shape first";
  EXPECT_EQ(CRectInt(0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM), lookup.record.sections[0]);
  EXPECT_EQ(CRectInt(240, 0, CODED_WIDTH - 240, CODED_HEIGHT), lookup.record.sections[2]);
}

/*!
 * The other half of a rule the scan's side already has a test for: an envelope that was never
 * set stores as an empty rectangle and reads back as a measurement claiming there is no picture
 * at all, so the write falls back to the rectangle. Implemented twice, and until now tested
 * once.
 */
TEST_F(TestVideoDatabaseContentGeometry, AnUnsetEnvelopeIsStoredAsTheRectangle)
{
  const int idFile{AddTestFile("noenvelope.mkv")};

  ContentGeometryRecord record{MakeRecord(IDENTITY)};
  record.envelope = CRectInt{};
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, record));

  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, IDENTITY)};
  ASSERT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(record.rect, lookup.record.envelope);
  EXPECT_FALSE(lookup.record.envelope.IsEmpty());
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
  EXPECT_EQ(record.rect, lookup.record.rect);
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
  updated.rect = CRectInt{0, 0, CODED_WIDTH, CODED_HEIGHT};
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, updated));

  EXPECT_EQ(1, CountRows(idFile));
  EXPECT_EQ(ContentGeometryState::MISSING, m_db.GetContentGeometry(idFile, first).state);

  const ContentGeometryLookup lookup{m_db.GetContentGeometry(idFile, second)};
  ASSERT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(updated.rect, lookup.record.rect);
}

TEST_F(TestVideoDatabaseContentGeometry, ARecordWithNoCodedFrameIsRefused)
{
  const int idFile{AddTestFile("nocodedframe.mkv")};

  ContentGeometryRecord record{MakeRecord({8'000'000'000, 1'700'000'000})};
  record.coded = CRectInt{};

  EXPECT_FALSE(m_db.SetContentGeometry(idFile, record));
  EXPECT_EQ(0, CountRows(idFile));
}

/*!
 * The exception to the rule above: a record saying the file could not be measured is stored
 * precisely because it has no rectangle. Refusing it would leave the sweep unable to remember
 * that it had already tried.
 */
TEST_F(TestVideoDatabaseContentGeometry, AFailureIsStoredDespiteHavingNoRectangle)
{
  const int idFile{AddTestFile("unreadable.mkv")};

  ContentGeometryRecord record;
  record.outcome = ContentGeometryOutcome::Failed;
  record.identity = IDENTITY;
  record.computed = CDateTime(2026, 8, 7, 14, 0, 0);

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, record));
  EXPECT_EQ(1, CountRows(idFile));

  const ContentGeometryAttempt attempt{m_db.GetContentGeometryAttempt(idFile)};
  EXPECT_TRUE(attempt.exists);
  EXPECT_EQ(ContentGeometryOutcome::Failed, attempt.outcome);
  EXPECT_EQ(IDENTITY.size, attempt.identity.size);
  EXPECT_EQ(IDENTITY.time, attempt.identity.time);
  EXPECT_FALSE(NeedsContentGeometry(attempt, IDENTITY));
}

//! Nothing that resolves a rectangle should have to know the difference between never having
//! tried and having tried and got nothing, so a failure reads back as no measurement at all.
TEST_F(TestVideoDatabaseContentGeometry, AFailureReadsAsMissingToEveryConsumer)
{
  const int idFile{AddTestFile("unreadable-lookup.mkv")};

  ContentGeometryRecord record;
  record.outcome = ContentGeometryOutcome::Failed;
  record.identity = IDENTITY;
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, record));

  EXPECT_EQ(ContentGeometryState::MISSING, m_db.GetContentGeometry(idFile, IDENTITY).state);

  ContentGeometryRecord unverified;
  EXPECT_FALSE(m_db.GetContentGeometryUnverified(idFile, unverified));
}

TEST_F(TestVideoDatabaseContentGeometry, AMeasurementRoundTripsItsOutcome)
{
  const int idFile{AddTestFile("outcome.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord(IDENTITY)));

  EXPECT_EQ(ContentGeometryOutcome::Measured,
            m_db.GetContentGeometry(idFile, IDENTITY).record.outcome);
  EXPECT_EQ(ContentGeometryOutcome::Measured, m_db.GetContentGeometryAttempt(idFile).outcome);
}

TEST_F(TestVideoDatabaseContentGeometry, AFileWithNoRowHasNoAttempt)
{
  const int idFile{AddTestFile("neverattempted.mkv")};

  const ContentGeometryAttempt attempt{m_db.GetContentGeometryAttempt(idFile)};
  EXPECT_FALSE(attempt.exists);
  EXPECT_TRUE(NeedsContentGeometry(attempt, {8'000'000'000, 1'700'000'000}));
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
  failure.outcome = ContentGeometryOutcome::Failed;
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
  EXPECT_TRUE(first.attempt.exists);
  EXPECT_EQ(ContentGeometryOutcome::Measured, first.attempt.outcome);
  EXPECT_EQ(CONTENT_GEOMETRY_ALGORITHM_VERSION, first.attempt.algorithmVersion);
  EXPECT_EQ(IDENTITY.size, first.attempt.identity.size);
  EXPECT_FALSE(NeedsContentGeometry(first.attempt, IDENTITY));

  EXPECT_FALSE(find(untouched).attempt.exists);
  EXPECT_TRUE(NeedsContentGeometry(find(untouched).attempt, IDENTITY));

  EXPECT_EQ(ContentGeometryOutcome::Failed, find(failed).attempt.outcome);
}

/*!
 * The row and its diagnostics are two writes describing one measurement, so they land together
 * or not at all. Written unguarded, a diagnostics write that failed after the row was committed
 * answered false with the row stored - and a caller reading that as "not stored" measures the
 * file all over again. The failure is forced by taking away the table the second write needs.
 */
TEST_F(TestVideoDatabaseContentGeometry, AFailedDiagnosticsWriteLeavesNoRowBehind)
{
  const int idFile{AddTestFile("halfwritten.mkv")};
  ASSERT_TRUE(m_db.ExecuteQuery("DROP TABLE contentgeometrydetails"));

  EXPECT_FALSE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));
  EXPECT_EQ(0, CountRows(idFile)) << "a call that answered false stored the row anyway";
}

/*!
 * A library update stores the geometry from inside its own transaction, so the guard the two
 * writes are wrapped in has to join that one rather than open a second - a second BEGIN is an
 * error, and committing an inner one would end the caller's early. Shown by rolling the
 * caller's back afterwards: the geometry goes with it.
 */
TEST_F(TestVideoDatabaseContentGeometry, StoringInsideACallersTransactionJoinsIt)
{
  const int idFile{AddTestFile("nested.mkv")};

  m_db.BeginTransaction();
  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));
  ASSERT_TRUE(m_db.InTransaction()) << "the caller's transaction was ended under it";
  m_db.RollbackTransaction();

  EXPECT_EQ(0, CountRows(idFile)) << "the write did not join the caller's transaction";
  EXPECT_EQ(0, CountDetailRows(idFile));
}

//! The delete_file trigger reaches the new tables.
TEST_F(TestVideoDatabaseContentGeometry, DeletingTheFileCascadesToTheGeometry)
{
  const int idFile{AddTestFile("cascade.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));
  ASSERT_EQ(1, CountRows(idFile));
  ASSERT_EQ(1, CountDetailRows(idFile));

  ASSERT_TRUE(m_db.DeleteFile(idFile));

  EXPECT_EQ(0, CountRows(idFile));
  EXPECT_EQ(0, CountDetailRows(idFile));
}

//! The trigger on its own, rather than whatever else DeleteFile() does on the way.
TEST_F(TestVideoDatabaseContentGeometry, TheTriggerAloneRemovesTheGeometry)
{
  const int idFile{AddTestFile("trigger.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));
  ASSERT_EQ(1, CountRows(idFile));

  ASSERT_TRUE(DeleteFilesRow(idFile));

  EXPECT_EQ(0, CountRows(idFile));
  EXPECT_EQ(0, CountDetailRows(idFile));
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

//! The schema version moves with the table.
TEST_F(TestVideoDatabaseContentGeometry, TheSchemaVersionCarriesTheTable)
{
  EXPECT_EQ(149, m_db.GetSingleValueInt("SELECT idVersion FROM version"));
}

//! The identity check stands on its own, without relying on the cascade.
TEST_F(TestVideoDatabaseContentGeometry, AnOrphanedRowNeverAttachesToAnotherFile)
{
  const int idFile{AddTestFile("orphan.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord({8'000'000'000, 1'700'000'000})));

  EXPECT_EQ(ContentGeometryState::MISSING,
            m_db.GetContentGeometry(idFile, {4'000'000'000, 1'800'000'000}).state);
}

//! The real 148 to 149 upgrade, driven through CDatabaseManager.
TEST(TestVideoDatabaseMigration, UpgradingFrom148AddsTheTableAndItsCascade)
{
  const std::string folder{CSpecialProtocol::TranslatePath("special://temp/")};

  DatabaseSettings settings;
  settings.type = "sqlite3";
  settings.host = folder;

  // A database as 148 left it. Connect() builds the *current* schema, so the tables 149
  // touches are restated here as 148 defined them rather than derived by subtraction from
  // the current ones - a column added to the 149 migration and not taken back out here would
  // otherwise abort the upgrade on a duplicate and make this test fail for the wrong reason.
  //
  // Started at 148 rather than earlier because 148 is upstream's streamdetails migration, and
  // replaying it over a table the current schema already built adds a duplicate column and
  // aborts. This test is about the upgrade this work introduces.
  {
    CVideoDatabase old;
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
              old.Connect("MyVideosMigration148", settings, true));
    ASSERT_TRUE(old.ExecuteQuery("DROP TABLE contentgeometry"));
    ASSERT_TRUE(old.ExecuteQuery("DROP TABLE contentgeometrydetails"));
    ASSERT_TRUE(old.ExecuteQuery("DROP TABLE settings"));
    ASSERT_TRUE(old.ExecuteQuery(
        "CREATE TABLE settings ( idFile integer, Deinterlace bool,"
        "ViewMode integer,ZoomAmount float, PixelRatio float, VerticalShift float, AudioStream "
        "integer, SubtitleStream integer,"
        "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
        "VolumeAmplification float, AudioDelay float, ResumeTime integer,"
        "Sharpness float, NoiseReduction float, NonLinStretch bool, PostProcess bool,"
        "ScalingMethod integer, DeinterlaceMode integer, StereoMode integer, StereoInvert bool, "
        "VideoStream integer,"
        "TonemapMethod integer, TonemapParam float, Orientation integer, CenterMixLevel integer)"));
    ASSERT_TRUE(old.ExecuteQuery("UPDATE version SET idVersion=148"));
    old.Close();
  }

  const auto advancedSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};
  const DatabaseSettings restore{advancedSettings->m_databaseVideo};

  advancedSettings->m_databaseVideo.type = "sqlite3";
  advancedSettings->m_databaseVideo.host = folder;
  advancedSettings->m_databaseVideo.name = "MyVideosMigration";

  CDatabaseManager& manager{CServiceBroker::GetDatabaseManager()};
  manager.Deinitialize();
  const bool initialized{manager.Initialize()};

  advancedSettings->m_databaseVideo = restore;
  manager.Deinitialize();

  ASSERT_TRUE(initialized) << "the database manager could not migrate the video database";

  CVideoDatabase migrated;
  ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
            migrated.Connect("MyVideosMigration149", settings, false));

  EXPECT_EQ(149, migrated.GetSingleValueInt("SELECT idVersion FROM version"));

  // The same upgrade carries the declaration columns, which live with the other per-file
  // overrides rather than in the cache.
  EXPECT_EQ(0, migrated.GetSingleValueInt(
                   "SELECT count(*) FROM settings WHERE DeclaredAspect IS NOT NULL"));

  const int idFile{migrated.AddFile("/test/migration/upgraded.mkv", "/test/migration/")};
  ASSERT_GE(idFile, 0);

  ContentGeometryRecord record{MakeRecord({8'000'000'000, 1'700'000'000})};
  ASSERT_TRUE(migrated.SetContentGeometry(idFile, record))
      << "the migration did not create a usable contentgeometry table";

  EXPECT_EQ(*record.details,
            migrated.GetSingleValue(StringUtils::Format(
                "SELECT details FROM contentgeometrydetails WHERE idFile={}", idFile)))
      << "the migration did not create a usable contentgeometrydetails table";

  ASSERT_TRUE(
      migrated.ExecuteQuery(StringUtils::Format("DELETE FROM files WHERE idFile={}", idFile)));

  EXPECT_EQ(0, migrated.GetSingleValueInt(StringUtils::Format(
                   "SELECT count(*) FROM contentgeometry WHERE idFile={}", idFile)))
      << "the trigger recreated after the migration does not cover contentgeometry";

  EXPECT_EQ(0, migrated.GetSingleValueInt(StringUtils::Format(
                   "SELECT count(*) FROM contentgeometrydetails WHERE idFile={}", idFile)))
      << "the trigger recreated after the migration does not cover contentgeometrydetails";

  migrated.Close();
}

//! The unverified read is for moving the row about, and never consults the file.
TEST_F(TestVideoDatabaseContentGeometry, TheUnverifiedReadIgnoresIdentityEntirely)
{
  const int idFile{AddTestFile("export.mkv")};

  ASSERT_TRUE(m_db.SetContentGeometry(idFile, MakeRecord(IDENTITY)));

  ContentGeometryRecord record;
  ASSERT_TRUE(m_db.GetContentGeometryUnverified(idFile, record));
  EXPECT_EQ(IDENTITY.size, record.identity.size);
  EXPECT_EQ(IDENTITY.time, record.identity.time);

  ContentGeometryRecord absent;
  EXPECT_FALSE(m_db.GetContentGeometryUnverified(idFile + 100000, absent));
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
  ASSERT_TRUE(db.ExecuteQuery("DELETE FROM contentgeometrydetails"));

  const int idFile{db.AddFile("/test/contentgeometry/mysql.mkv", "/test/contentgeometry/")};
  ASSERT_GE(idFile, 0);

  // Past the 2.1GB an INTEGER column would have stopped at.
  const FileIdentity identity{68'719'476'736, 1'700'000'000};
  ASSERT_TRUE(db.SetContentGeometry(idFile, MakeRecord(identity)));

  const ContentGeometryLookup lookup{db.GetContentGeometry(idFile, identity)};
  ASSERT_EQ(ContentGeometryState::VALID, lookup.state);
  EXPECT_EQ(identity.size, lookup.record.identity.size) << "the file size did not survive MySQL";
  EXPECT_EQ(CRectInt(0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM), lookup.record.rect);

  EXPECT_EQ(ContentGeometryState::MISSING,
            db.GetContentGeometry(idFile, {identity.size, identity.time + 1}).state);

  ASSERT_TRUE(db.ExecuteQuery(StringUtils::Format("DELETE FROM files WHERE idFile={}", idFile)));
  EXPECT_EQ(0, db.GetSingleValueInt(StringUtils::Format(
                   "SELECT count(*) FROM contentgeometry WHERE idFile={}", idFile)))
      << "the delete_file trigger does not cascade to contentgeometry on MySQL";
  EXPECT_EQ(0, db.GetSingleValueInt(StringUtils::Format(
                   "SELECT count(*) FROM contentgeometrydetails WHERE idFile={}", idFile)))
      << "the delete_file trigger does not cascade to contentgeometrydetails on MySQL";

  db.Close();
}
