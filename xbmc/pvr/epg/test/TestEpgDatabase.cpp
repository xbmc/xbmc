/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XBDateTime.h"
#include "dbwrappers/Database.h"
#include "filesystem/SpecialProtocol.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace PVR;

namespace
{

constexpr int EPG_ID = 1;
constexpr unsigned int BROADCAST_UID = 4242;

// 2099-12-31 23:59:00 UTC: past the signed 32-bit ceiling of 2038-01-19, within the
// unsigned one of 2106-02-07 that the insert side uses.
constexpr int64_t FAR_FUTURE_END = 4102444740;
constexpr int64_t START = 4102441140; // an hour earlier

class TestEpgDatabase : public ::testing::Test
{
protected:
  void SetUp() override
  {
    DatabaseSettings settings;
    settings.type = "sqlite3";
    settings.host = CSpecialProtocol::TranslatePath("special://temp/");

    m_database = std::make_shared<CPVREpgDatabase>();
    ASSERT_EQ(m_database->Connect("TestEpgDatabase", settings, true),
              CDatabase::ConnectionState::STATE_CONNECTED);

    // inserted directly so that only the read path is under test
    ASSERT_TRUE(m_database->ExecuteQuery(
        StringUtils::Format("REPLACE INTO epgtags (idEpg, iBroadcastUid, iStartTime, iEndTime, "
                            "sTitle) VALUES ({}, {}, {}, {}, 'Far Future Programme');",
                            EPG_ID, BROADCAST_UID, START, FAR_FUTURE_END)));
  }

  std::shared_ptr<CPVREpgDatabase> m_database;
};

CDateTime AsDateTime(int64_t seconds)
{
  return CDateTime(static_cast<time_t>(seconds));
}

// gtest prints a CDateTime as raw bytes; the rendered date names the year in a failure
std::string AsText(const CDateTime& value)
{
  return value.IsValid() ? value.GetAsDBDateTime() : "invalid";
}

std::string Expected(int64_t seconds)
{
  return AsText(AsDateTime(seconds));
}

} // unnamed namespace

/*!
 The column holds an unsigned 32-bit count, so a time past 2038-01-19 reads back as stored.
 */
TEST_F(TestEpgDatabase, ATagKeepsAnEndTimePast2038)
{
  const std::shared_ptr<CPVREpgInfoTag> tag{
      m_database->GetEpgTagByUniqueBroadcastID(EPG_ID, BROADCAST_UID)};

  ASSERT_NE(nullptr, tag);
  EXPECT_EQ(Expected(FAR_FUTURE_END), AsText(tag->EndAsUTC()));
  EXPECT_EQ(Expected(START), AsText(tag->StartAsUTC()));
}

/*!
 The guide window is sized from the last end time.
 */
TEST_F(TestEpgDatabase, TheLastEndTimeIsNotWrapped)
{
  EXPECT_EQ(Expected(FAR_FUTURE_END), AsText(m_database->GetLastEndTime(EPG_ID)));
}

TEST_F(TestEpgDatabase, TheFirstAndLastDatesAreNotWrapped)
{
  const auto [first, last] = m_database->GetFirstAndLastEPGDate();

  EXPECT_EQ(Expected(START), AsText(first));
  EXPECT_EQ(Expected(FAR_FUTURE_END), AsText(last));
}

TEST_F(TestEpgDatabase, TheMaxEndTimeIsNotWrapped)
{
  EXPECT_EQ(Expected(FAR_FUTURE_END),
            AsText(m_database->GetMaxEndTime(EPG_ID, AsDateTime(FAR_FUTURE_END + 1))));
}

TEST_F(TestEpgDatabase, TheMinStartTimeIsNotWrapped)
{
  EXPECT_EQ(Expected(START), AsText(m_database->GetMinStartTime(EPG_ID, AsDateTime(START - 1))));
}
