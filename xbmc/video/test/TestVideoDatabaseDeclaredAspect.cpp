/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoSettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "video/test/VideoDatabaseTestBase.h"

#include <gtest/gtest.h>

class TestVideoDatabaseDeclaredAspect : public VideoDatabaseTestBase
{
protected:
  TestVideoDatabaseDeclaredAspect()
    : VideoDatabaseTestBase("TestDeclaredAspect", "/test/declaredaspect/")
  {
  }
};

TEST_F(TestVideoDatabaseDeclaredAspect, StoresAndReadsBackOnInsert)
{
  const int idFile{AddTestFile("insert.mkv")};

  CVideoSettings stored;
  stored.m_declaredAspect = 2.40f;
  stored.m_declaredOn = "2026-08-07 10:15:00";
  stored.m_detectedWhenDeclared = 2.35f;
  m_db.SetVideoSettings(idFile, stored);

  CVideoSettings read;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, read));
  EXPECT_FLOAT_EQ(2.40f, read.m_declaredAspect);
  EXPECT_EQ("2026-08-07 10:15:00", read.m_declaredOn);
  EXPECT_FLOAT_EQ(2.35f, read.m_detectedWhenDeclared);
}

/*!
 * The update path carries a hand-written column list of its own, so a declaration written
 * over an existing settings row exercises entirely different SQL from the insert above.
 */
TEST_F(TestVideoDatabaseDeclaredAspect, StoresAndReadsBackOnUpdate)
{
  const int idFile{AddTestFile("update.mkv")};

  m_db.SetVideoSettings(idFile, CVideoSettings{}); // creates the row

  CVideoSettings stored;
  stored.m_declaredAspect = 1.33f;
  stored.m_declaredOn = "2026-08-07 11:00:00";
  stored.m_detectedWhenDeclared = 1.78f;
  m_db.SetVideoSettings(idFile, stored);

  CVideoSettings read;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, read));
  EXPECT_FLOAT_EQ(1.33f, read.m_declaredAspect);
  EXPECT_EQ("2026-08-07 11:00:00", read.m_declaredOn);
  EXPECT_FLOAT_EQ(1.78f, read.m_detectedWhenDeclared);
}

//! "Auto" is the absence of a declaration rather than a value, so clearing must store nothing
//! a resolver could mistake for intent.
TEST_F(TestVideoDatabaseDeclaredAspect, ClearingADeclarationLeavesNoRatio)
{
  const int idFile{AddTestFile("cleared.mkv")};

  CVideoSettings declared;
  declared.m_declaredAspect = 2.76f;
  declared.m_declaredOn = "2026-08-07 12:00:00";
  declared.m_detectedWhenDeclared = 2.40f;
  m_db.SetVideoSettings(idFile, declared);

  m_db.SetVideoSettings(idFile, CVideoSettings{});

  CVideoSettings read;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, read));
  EXPECT_FLOAT_EQ(0.0f, read.m_declaredAspect);
  EXPECT_TRUE(read.m_declaredOn.empty());
  EXPECT_FLOAT_EQ(0.0f, read.m_detectedWhenDeclared);
}

//! A row written before the feature existed reads as undeclared rather than as 0:1.
TEST_F(TestVideoDatabaseDeclaredAspect, ARowWithoutTheColumnsReadsAsUndeclared)
{
  const int idFile{AddTestFile("legacy.mkv")};

  m_db.SetVideoSettings(idFile, CVideoSettings{});
  ASSERT_TRUE(m_db.ExecuteQuery(
      StringUtils::Format("UPDATE settings SET DeclaredAspect=NULL, DeclaredOn=NULL, "
                          "DetectedWhenDeclared=NULL WHERE idFile={}",
                          idFile)));

  CVideoSettings read;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, read));
  EXPECT_FLOAT_EQ(0.0f, read.m_declaredAspect);
  EXPECT_TRUE(read.m_declaredOn.empty());
}

//! A declaration is per-title intent, so it must not read as a change requiring the settings
//! to be re-applied when nothing about it moved.
TEST_F(TestVideoDatabaseDeclaredAspect, ProvenanceAloneIsNotASettingsChange)
{
  CVideoSettings a;
  CVideoSettings b;
  a.m_declaredAspect = 2.40f;
  b.m_declaredAspect = 2.40f;
  a.m_declaredOn = "2026-08-07 10:00:00";
  b.m_declaredOn = "2026-01-01 00:00:00";
  a.m_detectedWhenDeclared = 2.35f;
  b.m_detectedWhenDeclared = 0.0f;

  EXPECT_FALSE(a != b);

  b.m_declaredAspect = 1.85f;
  EXPECT_TRUE(a != b);
}
