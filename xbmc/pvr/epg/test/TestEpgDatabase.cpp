/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "dbwrappers/Database.h"
#include "filesystem/SpecialProtocol.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace PVR;

namespace
{

constexpr int EPG_ID = 1;
constexpr unsigned int BROADCAST_UID = 4242;
constexpr unsigned int PERSISTED_BROADCAST_UID = 4243;

// Past the unsigned 32-bit ceiling of 2106-02-07, and so past the signed one of 2038-01-19 as
// well: a value this size survives neither width, whichever way it is converted.
constexpr int64_t START = 7258114800; // 2199-12-31 23:00:00 UTC
constexpr int64_t END = 7258118400; // 2200-01-01 00:00:00 UTC

// the persisted tag gets its own slot, because (idEpg, iStartTime) is unique and a REPLACE on the
// seeded row would delete it
constexpr int64_t PERSISTED_START = START + 7200;
constexpr int64_t PERSISTED_END = END + 7200;

class TestEpgDatabase : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // the tag constructor shifts by this and the read path does not, so the round trip through
    // the database is an identity only at zero
    auto& correction{
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection};
    m_savedTimeCorrection = correction;
    correction = 0;

    DatabaseSettings settings;
    settings.type = "sqlite3";
    settings.host = CSpecialProtocol::TranslatePath("special://temp/");

    m_database = std::make_shared<CPVREpgDatabase>();
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
              m_database->Connect("TestEpgDatabase", settings, true));

    // the database file outlives a single test, and a test that persists a tag leaves it behind,
    // so start every one of them from the same single row
    ASSERT_TRUE(m_database->ExecuteQuery("DELETE FROM epgtags;"));

    // inserted directly, so that a read can be tested without depending on the write
    ASSERT_TRUE(m_database->ExecuteQuery(
        StringUtils::Format("REPLACE INTO epgtags (idEpg, iBroadcastUid, iStartTime, iEndTime, "
                            "sTitle) VALUES ({}, {}, {}, {}, 'Far Future Programme');",
                            EPG_ID, BROADCAST_UID, START, END)));
  }

  void TearDown() override
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection =
        m_savedTimeCorrection;
  }

  std::shared_ptr<CPVREpgDatabase> m_database;

private:
  int m_savedTimeCorrection{0};
};

// A count of seconds is the assertion, not a rendered date: CDateTime renders through gmtime_r on
// POSIX, which is bound to time_t. The rendering is worth printing when one fails, though, since
// it names the year.
std::string AsText(const CDateTime& value)
{
  return value.IsValid() ? value.GetAsDBDateTime() : "invalid";
}

} // unnamed namespace

/*!
 The column holds a 64-bit count, so a far future time reads back as stored.
 */
TEST_F(TestEpgDatabase, ATagKeepsAFarFutureTime)
{
  const std::shared_ptr<CPVREpgInfoTag> tag{
      m_database->GetEpgTagByUniqueBroadcastID(EPG_ID, BROADCAST_UID)};

  ASSERT_NE(nullptr, tag);
  EXPECT_EQ(END, tag->EndAsUTC().GetAsSecondsSinceEpoch()) << AsText(tag->EndAsUTC());
  EXPECT_EQ(START, tag->StartAsUTC().GetAsSecondsSinceEpoch()) << AsText(tag->StartAsUTC());
}

/*!
 The write is the other half: a tag persisted with a far future time reads back as itself.
 */
TEST_F(TestEpgDatabase, APersistedTagKeepsAFarFutureTime)
{
  CPVREpgInfoTag written{nullptr, EPG_ID, CDateTime::FromSecondsSinceEpoch(PERSISTED_START),
                         CDateTime::FromSecondsSinceEpoch(PERSISTED_END), false};
  written.SetUniqueBroadcastID(PERSISTED_BROADCAST_UID);

  ASSERT_TRUE(m_database->QueuePersistQuery(written));
  ASSERT_TRUE(m_database->CommitInsertQueries());

  const std::shared_ptr<CPVREpgInfoTag> read{
      m_database->GetEpgTagByUniqueBroadcastID(EPG_ID, PERSISTED_BROADCAST_UID)};

  ASSERT_NE(nullptr, read);
  EXPECT_EQ(PERSISTED_END, read->EndAsUTC().GetAsSecondsSinceEpoch()) << AsText(read->EndAsUTC());
  EXPECT_EQ(PERSISTED_START, read->StartAsUTC().GetAsSecondsSinceEpoch())
      << AsText(read->StartAsUTC());
}

/*!
 A far future time is written into a query at the width the column holds, so it selects the row it
 names rather than one an overflow landed on.
 */
TEST_F(TestEpgDatabase, ATagIsFoundByAFarFutureStartTime)
{
  const std::shared_ptr<CPVREpgInfoTag> tag{
      m_database->GetEpgTagByStartTime(EPG_ID, CDateTime::FromSecondsSinceEpoch(START))};

  ASSERT_NE(nullptr, tag);
  EXPECT_EQ(END, tag->EndAsUTC().GetAsSecondsSinceEpoch()) << AsText(tag->EndAsUTC());
}

/*!
 The guide window is sized from the last end time.
 */
TEST_F(TestEpgDatabase, TheLastEndTimeIsNotWrapped)
{
  const CDateTime last{m_database->GetLastEndTime(EPG_ID)};

  EXPECT_EQ(END, last.GetAsSecondsSinceEpoch()) << AsText(last);
}

TEST_F(TestEpgDatabase, TheFirstAndLastDatesAreNotWrapped)
{
  const auto [first, last] = m_database->GetFirstAndLastEPGDate();

  EXPECT_EQ(START, first.GetAsSecondsSinceEpoch()) << AsText(first);
  EXPECT_EQ(END, last.GetAsSecondsSinceEpoch()) << AsText(last);
}

TEST_F(TestEpgDatabase, TheMaxEndTimeIsNotWrapped)
{
  const CDateTime maxEnd{
      m_database->GetMaxEndTime(EPG_ID, CDateTime::FromSecondsSinceEpoch(END + 1))};

  EXPECT_EQ(END, maxEnd.GetAsSecondsSinceEpoch()) << AsText(maxEnd);
}

TEST_F(TestEpgDatabase, TheMinStartTimeIsNotWrapped)
{
  const CDateTime minStart{
      m_database->GetMinStartTime(EPG_ID, CDateTime::FromSecondsSinceEpoch(START - 1))};

  EXPECT_EQ(START, minStart.GetAsSecondsSinceEpoch()) << AsText(minStart);
}
