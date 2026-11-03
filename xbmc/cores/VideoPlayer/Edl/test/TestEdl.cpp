/*
 *  Copyright (C) 2022- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "Edl/EdlParsers/MultipleEpisodeEdlParser.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Edl.h"
#include "filesystem/Directory.h"
#include "platform/Filesystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace EDL;

class TestEdl : public ::testing::Test
{
protected:
  TestEdl() = default;
};

TEST_F(TestEdl, TestParsingMplayerTimeBasedEDL)
{
  CEdl edl;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/mplayertimebased.edl
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  // expect kodi to be able to parse the file correctly
  EXPECT_EQ(found, true);
  // the file has 5 interest points: 2 scenemarkers, 1 mute, 1 cut and 1 commbreak
  // edit list should contain any edit that is not a scene marker nor a cut, so 2.
  // scenemarkers should be 4 (two "raw" scene markers and 2 from the commbreak - start and end of commbreaks)
  EXPECT_EQ(edl.HasEdits(), true);
  EXPECT_EQ(edl.HasSceneMarker(), true);
  EXPECT_EQ(edl.GetEditList().size(), 2);
  EXPECT_EQ(edl.GetSceneMarkers().size(), 4);

  // cuts
  // file has only 1 cut starting at 5.3 secs and ending at 7.1 secs
  // so total cut time should be 1.8 seconds (1800 msec)
  EXPECT_EQ(edl.HasCuts(), true);
  EXPECT_EQ(edl.GetTotalCutTime(), 1800ms);
  EXPECT_EQ(edl.GetCutMarkers().size(), 1);
  EXPECT_EQ(edl.GetCutMarkers().at(0), 5300ms);
  // When removing or restoring cuts, EDL adds (or removes) 1 msec to jump over the start or end boundary of the edit
  EXPECT_EQ(edl.GetTimeWithoutCuts(edl.GetRawEditList().at(0).start),
            edl.GetRawEditList().at(0).start + 1ms);
  EXPECT_EQ(edl.GetTimeAfterRestoringCuts(edl.GetRawEditList().at(0).start),
            edl.GetRawEditList().at(0).start);
  EXPECT_EQ(edl.GetTimeAfterRestoringCuts(edl.GetRawEditList().at(0).start + 2ms),
            edl.GetRawEditList().at(0).end + 2ms);

  // the first edit (note editlist does not contain cuts) is a mute section starting at 15 seconds
  // of the real file this should correspond to 13.2 secs on the kodi VideoPlayer timeline (start - cuttime)
  // raw edit list contains cuts so mute should be index 1
  const auto mute = edl.GetEditList().at(0);
  const auto muteRaw = edl.GetRawEditList().at(1);
  EXPECT_EQ(mute.action, Action::MUTE);
  EXPECT_EQ(muteRaw.action, Action::MUTE);
  EXPECT_EQ(edl.GetTimeWithoutCuts(mute.start), mute.start - edl.GetTotalCutTime());
  EXPECT_EQ(edl.GetTimeAfterRestoringCuts(mute.start - edl.GetTotalCutTime()), mute.start);
  EXPECT_EQ(muteRaw.start - edl.GetTotalCutTime(), mute.start);
  EXPECT_NE(edl.InEdit(muteRaw.start), std::nullopt);
  EXPECT_EQ(edl.InEdit(mute.start), std::nullopt);

  // scene markers
  // one of the scenemarkers (the first) have start and end times defined, kodi should assume the marker at the END position (255.3 secs)
  EXPECT_EQ(edl.GetSceneMarkers().at(0),
            edl.GetTimeWithoutCuts(duration_cast<std::chrono::milliseconds>(255.3s)));
  // one of them only has start defined, at 720.1 secs
  EXPECT_EQ(edl.GetSceneMarkers().at(2),
            edl.GetTimeWithoutCuts(duration_cast<std::chrono::milliseconds>(720.1s)));

  // commbreaks
  // the second edit on the file is a commbreak
  const auto commbreak = edl.GetEditList().at(1);
  EXPECT_EQ(commbreak.action, Action::COMM_BREAK);
  // We should have a scenemarker at the commbreak start and another on commbreak end
  std::optional<std::chrono::milliseconds> time =
      edl.GetNextSceneMarker(Direction::FORWARD, commbreak.start - 1ms);
  // lets cycle to the next scenemarker if starting from 1 msec before the start (or end) of the commbreak
  EXPECT_NE(time, std::nullopt);
  EXPECT_EQ(edl.GetTimeWithoutCuts(time.value()), commbreak.start);
  time = edl.GetNextSceneMarker(Direction::FORWARD, commbreak.end - 1ms);
  EXPECT_NE(time, std::nullopt);
  EXPECT_EQ(edl.GetTimeWithoutCuts(time.value()), commbreak.end);
  // same if we cycle backwards
  time = edl.GetNextSceneMarker(Direction::BACKWARD, commbreak.start + 1ms);
  EXPECT_NE(time, std::nullopt);
  EXPECT_EQ(edl.GetTimeWithoutCuts(time.value()), commbreak.start);
  time = edl.GetNextSceneMarker(Direction::BACKWARD, commbreak.end + 1ms);
  EXPECT_NE(time, std::nullopt);
  EXPECT_EQ(edl.GetTimeWithoutCuts(time.value()), commbreak.end);
  // We should be in an edit if we are in the middle of a commbreak...
  // lets check and confirm the edits match (after restoring cuts)
  const std::chrono::milliseconds middleOfCommbreak =
      commbreak.start + (commbreak.end - commbreak.start) / 2;
  const auto hasEdit = edl.InEdit(edl.GetTimeWithoutCuts(middleOfCommbreak));
  EXPECT_NE(hasEdit, std::nullopt);
  const auto& edit = hasEdit.value();
  EXPECT_EQ(edit->action, Action::COMM_BREAK);
  EXPECT_EQ(edit->start, edl.GetTimeAfterRestoringCuts(commbreak.start));
  EXPECT_EQ(edit->end, edl.GetTimeAfterRestoringCuts(commbreak.end));
}

TEST_F(TestEdl, TestParsingEDLFromItemProperty)
{
  CEdl edl;

  CFileItem mediaItem;
  mediaItem.SetPath("https://example.com/stream.mpd");
  mediaItem.SetProperty(
      "edlpath",
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebasedmixed.edl"));
  EXPECT_EQ(edl.ReadEditDecisionLists(mediaItem, 0, 0ms), true);
  EXPECT_EQ(edl.HasCuts(), true);
  EXPECT_EQ(edl.GetSceneMarkers().size(), 4);
  EXPECT_EQ(edl.GetEditList().front().start, 15s - edl.GetTotalCutTime());
}

TEST_F(TestEdl, TestEDLPropertyTakesPriorityOverVideoReDo)
{
  CEdl edl;

  CFileItem mediaItem;
  mediaItem.SetPath(XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/videoredo.mkv"));
  mediaItem.SetProperty(
      "edlpath",
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebasedmixed.edl"));
  EXPECT_EQ(edl.ReadEditDecisionLists(mediaItem, 0, 0ms), true);
  ASSERT_FALSE(edl.GetEditList().empty());
  EXPECT_EQ(edl.GetEditList().front().start, 15s - edl.GetTotalCutTime());
}

TEST_F(TestEdl, TestParsingMplayerTimeBasedInterleavedCutsEDL)
{
  CEdl edl;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/mplayertimebasedinterleavedcuts.edl
  // this is an edl file with commbreaks interleaved with cuts
  CFileItem mediaItem;
  mediaItem.SetPath(XBMC_REF_FILE_PATH(
      "xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebasedinterleavedcuts.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  // expect kodi to be able to parse the file correctly
  EXPECT_EQ(found, true);
  EXPECT_EQ(edl.GetEditList().size(), 2);
  EXPECT_EQ(edl.HasCuts(), true);
  EXPECT_EQ(edl.HasEdits(), true);
  EXPECT_EQ(edl.GetCutMarkers().size(), 2);
  // lets check the total cut time matches the sum of the two cut durations defined in the file
  EXPECT_EQ(edl.GetTotalCutTime(), 2800ms); // (7.1s - 5.3s) + (19s - 18s)
  // the first edit is after the first cut, so lets check the start time was adjusted exactly by the cut duration
  EXPECT_EQ(edl.GetEditList().at(0).start, edl.GetRawEditList().at(1).start - (7.1s - 5.3s));
  EXPECT_EQ(edl.GetEditList().at(0).start,
            edl.GetTimeWithoutCuts(edl.GetRawEditList().at(1).start));
  EXPECT_EQ(edl.GetEditList().at(1).start,
            edl.GetRawEditList().at(3).start - edl.GetTotalCutTime());
  EXPECT_EQ(edl.GetEditList().at(1).start,
            edl.GetTimeWithoutCuts(edl.GetRawEditList().at(3).start));
}

TEST_F(TestEdl, TestParsingMplayerFrameBasedEDL)
{
  CEdl edl;
  // suppose we're playing a file with 60 fps
  const float fps = 60;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/mplayerframebased.edl
  // this is an edl file with frame based edit points
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayerframebased.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, fps, 0ms);
  // expect kodi to be able to parse the file correctly
  EXPECT_EQ(found, true);
  EXPECT_EQ(edl.HasEdits(), true);
  EXPECT_EQ(edl.HasCuts(), true);
  EXPECT_EQ(edl.HasSceneMarker(), true);
  EXPECT_EQ(edl.GetEditList().size(), 2);
  // check edit times are correctly calculated provided the fps
  EXPECT_EQ(edl.GetEditList().at(0).start,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::duration<double, std::ratio<1>>{360 / fps}) -
                edl.GetTotalCutTime());
  EXPECT_EQ(edl.GetSceneMarkers().at(0),
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::duration<double, std::ratio<1>>{6127 / fps}) -
                edl.GetTotalCutTime());
}

TEST_F(TestEdl, TestParsingMplayerTimeBasedMixedEDL)
{
  CEdl edl;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/mplayertimebasedmixed.edl
  // this file has edit points with seconds and others with seconds timestrings
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebasedmixed.mkv"));
  bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  // expect kodi to be able to parse the file correctly
  EXPECT_EQ(found, true);
  EXPECT_EQ(edl.HasEdits(), true);
  EXPECT_EQ(edl.HasCuts(), true);
  EXPECT_EQ(edl.HasSceneMarker(), true);
  EXPECT_EQ(edl.GetSceneMarkers().size(), 4);
  bool sceneFound = false;
  // check we have correctly parsed the scene with 12:00.1 start point
  for (const auto& scene : edl.GetSceneMarkers())
  {
    if (scene == (12 * 60s + 0.1s) - edl.GetTotalCutTime())
    {
      sceneFound = true;
      break;
    }
  }
  EXPECT_EQ(sceneFound, true);
  // check that the first ordered edit starts at 15 secs
  EXPECT_EQ(edl.GetEditList().front().start, 15s - edl.GetTotalCutTime());
}

TEST_F(TestEdl, TestParsingVideoRedoEDL)
{
  CEdl edl;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/videoredo.Vprj
  // this is an edl file in VideoReDo format
  CFileItem mediaItem;
  mediaItem.SetPath(XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/videoredo.mkv"));
  bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(found, true);
  EXPECT_EQ(edl.HasEdits(), true);
  // videoredo only supports cuts or scenemarkers, hence the editlist should be empty. Raw editlist should contain the cuts.
  EXPECT_EQ(edl.GetEditList().size(), 0);
  EXPECT_GT(edl.GetRawEditList().size(), 0);
  EXPECT_EQ(edl.HasCuts(), true);
  EXPECT_EQ(edl.HasSceneMarker(), true);
  EXPECT_EQ(edl.GetSceneMarkers().size(), 4);
  EXPECT_EQ(edl.GetCutMarkers().size(), 3);
  // in videoredo time processing is ms * 10000
  // first cut in the file is at 4235230000 - let's confirm this corresponds to second 423.523
  EXPECT_EQ(edl.GetCutMarkers().front(), 423.523s);
}

TEST_F(TestEdl, TestSnapStreamEDL)
{
  CEdl edl;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/snapstream.mkv.chapters.xml
  // this is an edl file in SnapStream BeyondTV format
  CFileItem mediaItem;
  mediaItem.SetPath(XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/snapstream.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(found, true);
  // this format only supports commbreak types
  EXPECT_EQ(edl.HasEdits(), true);
  EXPECT_EQ(edl.GetCutMarkers().empty(), true);
  EXPECT_EQ(edl.GetEditList().size(), 3);
  EXPECT_EQ(edl.GetSceneMarkers().size(), 3 * 2); // start and end of each commbreak
  // snapstream beyond tv uses ms * 10000
  // check if first commbreak (4235230000 - 5936600000) is 423.523 sec - 593.660 sec
  EXPECT_EQ(edl.GetEditList().front().start, 423.523s);
  EXPECT_EQ(edl.GetEditList().front().end, 593.660s);
}

TEST_F(TestEdl, TestComSkipVersion1EDL)
{
  CEdl edl;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/comskipversion1.txt
  // this is an edl file in ComSkip (version 1) format
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/comskipversion1.mkv"));
  bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  // fps was not supplied, kodi will not be able to process the file
  EXPECT_EQ(found, false);
  // parse the file again this time supplying 60 fps
  const float fps = 60;
  found = edl.ReadEditDecisionLists(mediaItem, fps, 0ms);
  EXPECT_EQ(found, true);
  EXPECT_EQ(edl.HasEdits(), true);
  EXPECT_EQ(edl.GetCutMarkers().empty(), true);
  EXPECT_EQ(edl.GetEditList().size(), 3);
  EXPECT_EQ(edl.GetSceneMarkers().size(), 3 * 2); // start and end of each commbreak
  EXPECT_EQ(edl.GetEditList().front().start,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::duration<double, std::ratio<1>>{12693 / fps}));
  EXPECT_EQ(edl.GetEditList().front().end,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::duration<double, std::ratio<1>>{17792 / fps}));
}

TEST_F(TestEdl, TestComSkipVersion2EDL)
{
  CEdl edl;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/comskipversion2.txt
  // this is an edl file in ComSkip (version 2) format where fps is obtained from the file
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/comskipversion2.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(found, true);
  // fps is obtained from the file as it always takes precedence (note we supplied 0 above),
  // the EDL file has the value of 2500 for fps. kodi converts this to 25 fps by dividing by a factor of 100
  const double fpsInEdlFile = 2500 / 100;
  // this format only supports commbreak types
  EXPECT_EQ(edl.HasEdits(), true);
  EXPECT_EQ(edl.GetCutMarkers().empty(), true);
  EXPECT_EQ(edl.GetEditList().size(), 3);
  EXPECT_EQ(edl.GetSceneMarkers().size(), 3 * 2); // start and end of each commbreak
  EXPECT_EQ(edl.GetEditList().front().start,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::duration<double, std::ratio<1>>{12693 / fpsInEdlFile}));
  EXPECT_EQ(edl.GetEditList().front().end,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::duration<double, std::ratio<1>>{17792 / fpsInEdlFile}));
}

TEST_F(TestEdl, TestRuntimeSetEDL)
{
  // this is a simple test for SetLastEditTime, SetLastEditActionType and corresponding getters
  CEdl edl;
  edl.SetLastEditTime(1000ms);
  edl.SetLastEditActionType(Action::COMM_BREAK);
  EXPECT_EQ(edl.GetLastEditTime(), 1000ms);
  EXPECT_EQ(edl.GetLastEditActionType(), Action::COMM_BREAK);
}

TEST_F(TestEdl, TestCommBreakAdvancedSettings)
{
  CEdl edl;
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  // the test goal is to test m_iEdlCommBreakAutowait and m_iEdlCommBreakAutowind
  // lets check first the behaviour with default values (both disabled)
  // Keep all EDL advanced settings to default
  advancedSettings->m_iEdlCommBreakAutowait = 0; // disabled (default)
  advancedSettings->m_iEdlCommBreakAutowind = 0; // disabled (default)
  advancedSettings->m_bEdlMergeShortCommBreaks = false; // disabled (default)
  advancedSettings->m_iEdlMinCommBreakLength = 3 * 30; // 3*30 secs (default)
  advancedSettings->m_iEdlMaxCommBreakLength = 8 * 30 + 10; // 8*30+10 secs (default value)
  advancedSettings->m_iEdlMaxStartGap = 5 * 60; // 5 minutes (default)
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowait, 0);
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowind, 0);
  EXPECT_EQ(advancedSettings->m_bEdlMergeShortCommBreaks, false);
  EXPECT_EQ(advancedSettings->m_iEdlMinCommBreakLength, 3 * 30);
  EXPECT_EQ(advancedSettings->m_iEdlMinCommBreakLength, 3 * 30);
  EXPECT_EQ(advancedSettings->m_iEdlMaxCommBreakLength, 8 * 30 + 10);
  EXPECT_EQ(advancedSettings->m_iEdlMaxStartGap, 5 * 60);

  // create a dummy "media" fileitem whose corresponding edl file is testdata/edlautowindautowait.txt
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/edlautowindautowait.mkv"));
  bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(found, true);
  // confirm the start and end times of all the commbreaks match
  EXPECT_EQ(edl.GetEditList().size(), 5);
  EXPECT_EQ(edl.GetEditList().at(0).start, 10s);
  EXPECT_EQ(edl.GetEditList().at(0).end, 22s);
  EXPECT_EQ(edl.GetEditList().at(1).start, 30s);
  EXPECT_EQ(edl.GetEditList().at(1).end, 32s);
  EXPECT_EQ(edl.GetEditList().at(2).start, 37s);
  EXPECT_EQ(edl.GetEditList().at(2).end, 50s);
  EXPECT_EQ(edl.GetEditList().at(3).start, 52s);
  EXPECT_EQ(edl.GetEditList().at(3).end, 60s);
  EXPECT_EQ(edl.GetEditList().at(4).start, 62s);
  EXPECT_EQ(edl.GetEditList().at(4).end, 65100ms);
  // now lets change autowait and autowind and check the edits are correctly adjusted
  edl.Clear();
  advancedSettings->m_iEdlCommBreakAutowait = 3; // secs
  advancedSettings->m_iEdlCommBreakAutowind = 3; // secs
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowait, 3);
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowind, 3);
  found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(edl.GetEditList().size(), 5);
  // the second edit has a duration smaller than the autowait
  // this moves the start time to the end of the edit
  EXPECT_EQ(edl.GetEditList().at(1).start, 32s);
  EXPECT_EQ(edl.GetEditList().at(1).end, edl.GetEditList().at(1).start);
  // the others should be adjusted + 3 secs at the start and -3 secs at the end
  // due to the provided values for autowait and autowind.
  EXPECT_EQ(edl.GetEditList().at(0).start, 10s + 3s);
  EXPECT_EQ(edl.GetEditList().at(0).end, 22s - 3s);
  EXPECT_EQ(edl.GetEditList().at(2).start, 37s + 3s);
  EXPECT_EQ(edl.GetEditList().at(2).end, 50s - 3s);
  EXPECT_EQ(edl.GetEditList().at(3).start, 52s + 3s);
  EXPECT_EQ(edl.GetEditList().at(3).end, 60s - 3s);
  // since we adjust the start to second 65 and the autowind is 3 seconds kodi should
  // shift the end time not by 3 seconds but by the "excess" time (in this case 0.1 sec)
  // this means start and end will be exactly the same. The commbreak would be removed if
  // mergeshortcommbreaks was active and advancedsetting m_iEdlMinCommBreakLength
  // was set to a reasonable threshold.
  EXPECT_EQ(edl.GetEditList().at(4).start, 62s + 3s);
  EXPECT_EQ(edl.GetEditList().at(4).end, 65.1s - 0.1s);
  EXPECT_EQ(edl.GetEditList().at(4).start, edl.GetEditList().at(4).end);
}

TEST_F(TestEdl, TestCommBreakAdvancedSettingsRemoveSmallCommbreaks)
{
  // this is a variation of TestCommBreakAdvancedSettings
  // should make sure the number of commbreaks in the file is now 3 instead of 5
  // since two of them have duration smaller than 1 sec
  CEdl edl;
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  // set EDL advanced settings specific for the test case
  advancedSettings->m_iEdlCommBreakAutowait = 3; // secs
  advancedSettings->m_iEdlCommBreakAutowind = 3; // secs
  advancedSettings->m_bEdlMergeShortCommBreaks = true;
  advancedSettings->m_iEdlMinCommBreakLength = 1; // sec
  advancedSettings->m_iEdlMaxCommBreakLength = 0; // deactivate
  advancedSettings->m_iEdlMaxStartGap = 0; // deactivate
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowait, 3);
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowind, 3);
  EXPECT_EQ(advancedSettings->m_bEdlMergeShortCommBreaks, true);
  EXPECT_EQ(advancedSettings->m_iEdlMinCommBreakLength, 1);
  EXPECT_EQ(advancedSettings->m_iEdlMaxCommBreakLength, 0);
  EXPECT_EQ(advancedSettings->m_iEdlMaxStartGap, 0);

  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/edlautowindautowait.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(found, true);
  EXPECT_EQ(edl.GetEditList().size(), 3);
}

TEST_F(TestEdl, TestMergeSmallCommbreaks)
{
  CEdl edl;
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  // set EDL advanced settings specific for the test case
  advancedSettings->m_bEdlMergeShortCommBreaks = true;
  EXPECT_EQ(advancedSettings->m_bEdlMergeShortCommBreaks, true);
  // keep any other EDL advanced settings to default
  advancedSettings->m_iEdlCommBreakAutowait = 0; // disabled (default)
  advancedSettings->m_iEdlCommBreakAutowind = 0; // disabled (default)
  advancedSettings->m_iEdlMinCommBreakLength = 3 * 30; // 3*30 secs (default)
  advancedSettings->m_iEdlMaxCommBreakLength = 8 * 30 + 10; // 8*30+10 secs (default value)
  advancedSettings->m_iEdlMaxStartGap = 5 * 60; // 5 minutes (default)
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowait, 0);
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowind, 0);
  EXPECT_EQ(advancedSettings->m_iEdlMinCommBreakLength, 3 * 30);
  EXPECT_EQ(advancedSettings->m_iEdlMaxCommBreakLength, 8 * 30 + 10);
  EXPECT_EQ(advancedSettings->m_iEdlMaxStartGap, 5 * 60);

  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/edlautowindautowait.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(found, true);
  // kodi should merge all commbreaks into a single one starting at the first point (0)
  // and ending at the last edit time
  EXPECT_EQ(edl.GetEditList().size(), 1);
  EXPECT_EQ(edl.GetEditList().at(0).start, 0ms);
  EXPECT_EQ(edl.GetEditList().at(0).end, 65100ms);
}

TEST_F(TestEdl, TestMergeSmallCommbreaksAdvanced)
{
  CEdl edl;
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  // set EDL advanced settings specific for the test case
  advancedSettings->m_bEdlMergeShortCommBreaks = true;
  advancedSettings->m_iEdlMaxCommBreakLength = 30; // 30 secs
  advancedSettings->m_iEdlMinCommBreakLength = 1; // 1 sec
  advancedSettings->m_iEdlMaxStartGap = 2; // 2 secs
  EXPECT_EQ(advancedSettings->m_bEdlMergeShortCommBreaks, true);
  EXPECT_EQ(advancedSettings->m_iEdlMaxCommBreakLength, 30);
  EXPECT_EQ(advancedSettings->m_iEdlMinCommBreakLength, 1);
  EXPECT_EQ(advancedSettings->m_iEdlMaxStartGap, 2);
  // keep any other EDL advanced settings to default
  advancedSettings->m_iEdlCommBreakAutowait = 0; // disabled (default)
  advancedSettings->m_iEdlCommBreakAutowind = 0; // disabled (default)
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowait, 0);
  EXPECT_EQ(advancedSettings->m_iEdlCommBreakAutowind, 0);

  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/edlautowindautowait.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(found, true);
  // kodi should merge all commbreaks into two
  EXPECT_EQ(edl.GetEditList().size(), 2);
  // second edit of the original file + third one
  EXPECT_EQ(edl.GetEditList().at(0).end - edl.GetEditList().at(0).start, 32s - 10s);
  // 4th, 5th and 6th commbreaks joined
  EXPECT_EQ(edl.GetEditList().at(1).end - edl.GetEditList().at(1).start, 65100ms - 37s);
}

TEST_F(TestEdl, TestMultipleEpisodeEdlProcess)
{
  // Helper: build an EpisodeFileMap entry with a bookmark at the given time (seconds)
  auto testEntry = [](int episode, double bookmarkTimeSec) -> EpisodeFileMapEntry
  {
    EpisodeInformation info;
    info.season = 1;
    info.episode = episode;
    info.bookmark.timeInSeconds = bookmarkTimeSec;
    info.bookmark.totalTimeInSeconds = 3600.0;
    info.bookmark.type = CBookmark::EPISODE;
    return {"video.mkv", info};
  };

  // Helper: build a CFileItem tagged as the given episode
  auto testItem = [](int episode) -> CFileItem
  {
    CFileItem item;
    item.SetPath("/path/to/video.mkv");
    CVideoInfoTag* tag = item.GetVideoInfoTag();
    tag->SetMediaType(KODI::MEDIA::TYPE::EPISODE);
    tag->m_iIdShow = 1;
    tag->m_iFileId = 1;
    tag->m_iSeason = 1;
    tag->m_iEpisode = episode;
    return item;
  };

  constexpr std::chrono::milliseconds fileDurationMs = 3600s; // 60 minutes

  // Scenario 1: 3-episode file, playing the middle episode (ep 2).
  // Ep1=0s, Ep2=1200s (20min), Ep3=2400s (40min). File=60min.
  // Expect CUT [0 - 20min] and CUT [40min - 60min].
  {
    EpisodeFileMap fileMap;
    fileMap.insert(testEntry(1, 0.0));
    fileMap.insert(testEntry(2, 1200.0));
    fileMap.insert(testEntry(3, 2400.0));

    const auto item = testItem(2);
    const auto result = CMultipleEpisodeEdlParser::Process(item, 0, fileDurationMs, fileMap);

    EXPECT_EQ(result.GetEdits().size(), 2);
    EXPECT_EQ(result.GetEdits().at(0).edit.action, Action::CUT);
    EXPECT_EQ(result.GetEdits().at(0).edit.start, 0ms);
    EXPECT_EQ(result.GetEdits().at(0).edit.end, 1200000ms);
    EXPECT_EQ(result.GetEdits().at(1).edit.action, Action::CUT);
    EXPECT_EQ(result.GetEdits().at(1).edit.start, 2400000ms);
    EXPECT_EQ(result.GetEdits().at(1).edit.end, std::chrono::milliseconds(fileDurationMs));
  }

  // Scenario 2: 2-episode file, playing the first episode.
  // Ep1=0s, Ep2=1800s (30min). File=60min.
  // start=0 → no leading CUT; end=30min → CUT [30min - 60min].
  {
    EpisodeFileMap fileMap;
    fileMap.insert(testEntry(1, 0.0));
    fileMap.insert(testEntry(2, 1800.0));

    const auto item = testItem(1);
    const auto result = CMultipleEpisodeEdlParser::Process(item, 0, fileDurationMs, fileMap);

    EXPECT_EQ(result.GetEdits().size(), 1);
    EXPECT_EQ(result.GetEdits().at(0).edit.start, 1800000ms);
    EXPECT_EQ(result.GetEdits().at(0).edit.end, std::chrono::milliseconds(fileDurationMs));
  }

  // Scenario 3: 2-episode file, playing the last episode.
  // Ep1=0s, Ep2=1500s (25min). File=60min.
  // start=25min → CUT [0 - 25min]; no next ep → end=length, no trailing CUT.
  {
    EpisodeFileMap fileMap;
    fileMap.insert(testEntry(1, 0.0));
    fileMap.insert(testEntry(2, 1500.0));

    const auto item = testItem(2);
    const auto result = CMultipleEpisodeEdlParser::Process(item, 0, fileDurationMs, fileMap);

    EXPECT_EQ(result.GetEdits().size(), 1);
    EXPECT_EQ(result.GetEdits().at(0).edit.start, 0ms);
    EXPECT_EQ(result.GetEdits().at(0).edit.end, 1500000ms);
  }

  // Scenario 4: Single episode, no bookmarks → empty result (no EDL needed).
  {
    EpisodeFileMap fileMap;
    EpisodeInformation info;
    info.season = 1;
    info.episode = 1;
    info.bookmark.timeInSeconds = 0.0;
    info.bookmark.totalTimeInSeconds = 0.0;
    fileMap.insert({"video.mkv", info});

    const auto item = testItem(1);
    const auto result = CMultipleEpisodeEdlParser::Process(item, 0, fileDurationMs, fileMap);

    EXPECT_EQ(result.IsEmpty(), true);
  }

  // Scenario 5: Episode not in file map → empty result.
  {
    EpisodeFileMap fileMap;
    fileMap.insert(testEntry(1, 0.0));
    fileMap.insert(testEntry(2, 1200.0));

    const auto item = testItem(5);
    const auto result = CMultipleEpisodeEdlParser::Process(item, 0, fileDurationMs, fileMap);

    EXPECT_EQ(result.IsEmpty(), true);
  }

  // Scenario 6: 4-episode file with unequal splits, playing episode 3.
  // Ep1=0s, Ep2=600s, Ep3=900s (15min), Ep4=2700s (45min). File=60min.
  // Expect CUT [0 - 15min] and CUT [45min - 60min]. Playable = 30min.
  {
    EpisodeFileMap fileMap;
    fileMap.insert(testEntry(1, 0.0));
    fileMap.insert(testEntry(2, 600.0));
    fileMap.insert(testEntry(3, 900.0));
    fileMap.insert(testEntry(4, 2700.0));

    const auto item = testItem(3);
    const auto result = CMultipleEpisodeEdlParser::Process(item, 0, fileDurationMs, fileMap);

    EXPECT_EQ(result.GetEdits().size(), 2);
    EXPECT_EQ(result.GetEdits().at(0).edit.start, 0ms);
    EXPECT_EQ(result.GetEdits().at(0).edit.end, 900000ms);
    EXPECT_EQ(result.GetEdits().at(1).edit.start, 2700000ms);
    EXPECT_EQ(result.GetEdits().at(1).edit.end, std::chrono::milliseconds(fileDurationMs));
    // playable duration = 60min - 15min - 15min = 30min
    const auto totalCut = (result.GetEdits().at(0).edit.end - result.GetEdits().at(0).edit.start) +
                          (result.GetEdits().at(1).edit.end - result.GetEdits().at(1).edit.start);
    EXPECT_EQ(std::chrono::milliseconds(fileDurationMs) - totalCut, 1800000ms);
  }

  // Scenario 7: Next episode's bookmark is 0 (unset) → end defaults to file length.
  // Ep1=0s, Ep2=1200s, Ep3 bookmark=0. File=60min. Playing Ep2.
  // Expect only CUT [0 - 20min].
  {
    EpisodeFileMap fileMap;
    fileMap.insert(testEntry(1, 0.0));
    fileMap.insert(testEntry(2, 1200.0));
    fileMap.insert(testEntry(3, 0.0));

    const auto item = testItem(2);
    const auto result = CMultipleEpisodeEdlParser::Process(item, 0, fileDurationMs, fileMap);

    EXPECT_EQ(result.GetEdits().size(), 1);
    EXPECT_EQ(result.GetEdits().at(0).edit.start, 0ms);
    EXPECT_EQ(result.GetEdits().at(0).edit.end, 1200000ms);
  }
}

class TestParseEditsForEpisode : public ::testing::Test
{
protected:
  // Build a CEdlParserResult with a start cut [0 - startEnd] and/or
  // an end cut [endStart - fileDuration].
  static EDL::CEdlParserResult MakeMultiEpResult(std::chrono::milliseconds startCutEnd,
                                                 std::chrono::milliseconds endCutStart,
                                                 std::chrono::milliseconds fileDuration)
  {
    EDL::CEdlParserResult r;
    EDL::Edit edit;
    edit.action = EDL::Action::CUT;
    if (startCutEnd > 0ms)
    {
      edit.start = 0ms;
      edit.end = startCutEnd;
      r.AddEdit(edit);
    }
    if (endCutStart > 0ms && endCutStart < fileDuration)
    {
      edit.start = endCutStart;
      edit.end = fileDuration;
      r.AddEdit(edit);
    }
    return r;
  }

  static void CallParseEditsForEpisode(CEdl& edl,
                                       const EDL::CEdlParserResult& multiEpisodeResult,
                                       std::chrono::milliseconds duration)
  {
    edl.ParseEditsForEpisode(multiEpisodeResult, duration);
  }

  static bool AddEdit(CEdl& edl, const EDL::Edit& edit) { return edl.AddEdit(edit); }
};

TEST_F(TestParseEditsForEpisode, EmptyMultiEpisodeResultIsNoOp)
{
  // ParseEditsForEpisode with an empty result should not modify the existing edits
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  const size_t editsBefore = edl.GetRawEditList().size();
  const size_t markersBefore = edl.GetSceneMarkers().size();

  EDL::CEdlParserResult empty;
  CallParseEditsForEpisode(edl, empty, 0ms);

  EXPECT_EQ(edl.GetRawEditList().size(), editsBefore);
  EXPECT_EQ(edl.GetSceneMarkers().size(), markersBefore);
}

TEST_F(TestParseEditsForEpisode, EditsOutsideEpisodeWindowAreRemoved)
{
  // The mplayertimebased.edl has a cut at 5.3s-7.1s and other edits.
  // Apply a multi-episode window starting at 10s - meaning the cut is before the window.
  // The cut [5.3s - 7.1s] should be removed.
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);

  // Sanity check: cut exists before ParseEditsForEpisode
  EXPECT_TRUE(edl.HasCuts() && edl.GetCutMarkers().size() == 1);

  // Episode window: 10s to 1000s (file duration 1000s)
  // Start cut [0 - 10s] will remove the 5.3s-7.1s cut which is inside [0-10s]
  const std::chrono::milliseconds fileDuration = 1000s;
  auto multiEpResult = MakeMultiEpResult(10s, 0ms, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  // There should still be one cut (the start cut)
  EXPECT_TRUE(edl.HasCuts() && edl.GetCutMarkers().size() == 1);
}

TEST_F(TestParseEditsForEpisode, EpisodeStartCutIsAddedToEditList)
{
  // After ParseEditsForEpisode with a start cut, that cut should be present in the raw edit list
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);

  const std::chrono::milliseconds fileDuration = 1000s;
  auto multiEpResult = MakeMultiEpResult(30s, 0ms, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  // The start cut [0 - 30s] should now appear at the start of the raw edit list
  EXPECT_TRUE(edl.HasCuts());
  if (edl.HasCuts())
  {
    const auto& edit{edl.GetRawEditList()[0]};
    EXPECT_TRUE(edit.action == EDL::Action::CUT && edit.start == 0ms && edit.end == 30s)
        << "Expected start cut [0 - 30s] at the start of the raw edit list";
  }
}

TEST_F(TestParseEditsForEpisode, EpisodeEndCutIsAddedToEditList)
{
  // After ParseEditsForEpisode with an end cut, that cut should be present in the raw edit list
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);

  const std::chrono::milliseconds fileDuration = 1000s;
  auto multiEpResult = MakeMultiEpResult(0ms, 500s, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  // The end cut [500s - 1000s] should now appear at the end of the raw edit list
  EXPECT_TRUE(edl.HasCuts());
  if (edl.HasCuts())
  {
    const auto& edit{edl.GetRawEditList().back()};
    EXPECT_TRUE(edit.action == EDL::Action::CUT && edit.start == 500s && edit.end == fileDuration)
        << "Expected end cut [500s - 1000s] at the end of the raw edit list";
  }
}

TEST_F(TestParseEditsForEpisode, TotalCutTimeIsRecalculatedAfterEpisodeWindow)
{
  // Verify m_totalCutTime is correct after episode window filtering removes cuts
  // mplayertimebased has cut [5.3s - 7.1s] = 1800ms
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_EQ(edl.GetTotalCutTime(), 1800ms);

  // Apply episode window that removes the cut (window starts at 10s)
  const std::chrono::milliseconds fileDuration = 1000s;
  auto multiEpResult = MakeMultiEpResult(10s, 0ms, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  // The original 1800ms cut is gone; only the episode start cut [0-10s] remains
  EXPECT_EQ(edl.GetTotalCutTime(), 10s);
}

TEST_F(TestParseEditsForEpisode, SceneMarkersOutsideEpisodeWindowAreRemoved)
{
  // Scene markers outside the episode window should be removed by ParseEditsForEpisode
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);

  // There should be 4 scene markers initially
  // Two explicitly set and two from the commbreak [420s-822s]
  EXPECT_TRUE(edl.HasSceneMarker() && edl.GetSceneMarkers().size() == 4)
      << "Expected 4 scene markers initially";

  // The file has scene markers. Apply an episode window [200s - 400s]
  // Scene markers outside this window should be stripped.
  const std::chrono::milliseconds fileDuration = 1000s;
  const std::chrono::milliseconds episodeStart = 200s;
  auto multiEpResult = MakeMultiEpResult(episodeStart, 400s, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  // There should be 1 scene marker remaining
  EXPECT_TRUE(edl.HasSceneMarker() && edl.GetSceneMarkers().size() == 1)
      << "Expected only 1 scene marker within the episode window";

  if (edl.HasSceneMarker())
  {
    const auto marker{edl.GetSceneMarkers()[0]};
    EXPECT_EQ(marker, 255300ms - episodeStart);
  }
}

TEST_F(TestParseEditsForEpisode, BothStartAndEndCutsApplied)
{
  // Apply both a start and end cut, verify both appear in edit list
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);

  const std::chrono::milliseconds fileDuration = 1000s;
  auto multiEpResult = MakeMultiEpResult(20s, 500s, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  EXPECT_TRUE(edl.HasCuts() && edl.GetRawEditList().size() >= 2)
      << "Expected at least 2 cuts - episode start and end";

  // The start cut [0 - 20s] should now appear at the start of the raw edit list
  if (edl.HasCuts() && edl.GetRawEditList().size() >= 2)
  {
    const auto& edit{edl.GetRawEditList()[0]};
    EXPECT_TRUE(edit.action == EDL::Action::CUT && edit.start == 0ms && edit.end == 20s)
        << "Expected start cut [0 - 20s] at the start of the raw edit list";
    // The end cut [500s - 1000s] should now appear at the end of the raw edit list
    const auto& editEnd{edl.GetRawEditList().back()};
    EXPECT_TRUE(editEnd.action == EDL::Action::CUT && editEnd.start == 500s &&
                editEnd.end == fileDuration)
        << "Expected end cut [500s - 1000s] at the end of the raw edit list";
  }
}

TEST_F(TestParseEditsForEpisode, EditsWithinEpisodeWindowAreRetained)
{
  // Edits that fall entirely within the episode window must be kept
  // mplayertimebased has: cut [5.3s-7.1s], mute [15s-16.7s], commbreak [420s-822s]
  // Apply window [4s - 900s] → cut, mute and commbreak within should survive
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);

  const size_t editsBeforeEpisodeWindow = edl.GetRawEditList().size();

  const std::chrono::milliseconds fileDuration = 1000s;
  auto multiEpResult = MakeMultiEpResult(4s, 900s, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  bool cutRetained = false;
  bool muteRetained = false;
  bool commbreakRetained = false;
  for (const auto& edit : edl.GetRawEditList())
  {
    if (edit.action == EDL::Action::CUT && edit.start == 5300ms && edit.end == 7100ms)
      cutRetained = true;

    if (edit.action == EDL::Action::MUTE && edit.start == 15000ms && edit.end == 16700ms)
      muteRetained = true;
    if (edit.action == EDL::Action::COMM_BREAK && edit.start == 420000ms && edit.end == 822000ms)
      commbreakRetained = true;
  }
  EXPECT_TRUE(cutRetained) << "Cut [5.3s-7.1s] is within the episode window and should be kept";
  EXPECT_TRUE(muteRetained) << "Mute [15s-16.7s] is within the episode window and should be kept";
  EXPECT_TRUE(commbreakRetained)
      << "CommBreak [420s-822s] is within the episode window and should be kept";

  // Plus episode start and end cuts were added, so total edits = original + 2
  EXPECT_EQ(edl.GetRawEditList().size(), editsBeforeEpisodeWindow + 2);
}

TEST_F(TestParseEditsForEpisode, EditStraddlingEpisodeStartIsClamped)
{
  // An edit that starts before the episode window but ends inside it should have
  // its start clamped to the episode start, not be removed entirely.
  // mplayertimebased has a cut at [5.3s - 7.1s].
  // Apply an episode window starting at 6s (i.e. start cut [0 - 6s]).
  // The cut straddles the episode start: starts at 5.3s (outside), ends at 7.1s (inside).
  // Expected: cut is clamped to [6s - 7.1s], not removed.
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);
  EXPECT_TRUE(edl.HasCuts());

  const std::chrono::milliseconds fileDuration = 1000s;
  const std::chrono::milliseconds episodeStart = 6s;
  auto multiEpResult = MakeMultiEpResult(episodeStart, 0ms, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  // The cut should have been clamped, not removed
  bool clampedCutFound = false;
  bool originalCutFound = false;
  for (const auto& edit : edl.GetRawEditList())
  {
    if (edit.action == EDL::Action::CUT && edit.start == episodeStart && edit.end == 7100ms)
      clampedCutFound = true;
    if (edit.action == EDL::Action::CUT && edit.start == 5300ms)
      originalCutFound = true;
  }
  EXPECT_TRUE(clampedCutFound) << "Expected cut clamped to [6s - 7.1s]";
  EXPECT_FALSE(originalCutFound) << "Original unclamped cut start (5.3s) should not exist";
}

TEST_F(TestParseEditsForEpisode, EditStraddlingBothBoundariesIsClamped)
{
  // An edit that starts before the episode window but ends after the episode window should have
  // its start and end clamped to the episode window.
  // mplayertimebased has a commbreak at [420s - 822s].
  // Apply an episode window starting at 500s and ending at 600s.
  // The commbreak straddles the episode window: starts at 420s (outside), ends at 822s (outside).
  // Expected: commbreak is clamped to [500s - 600s], not removed.
  CEdl edl;
  CFileItem mediaItem;
  mediaItem.SetPath(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebased.mkv"));
  edl.ReadEditDecisionLists(mediaItem, 0, 0ms);

  const std::chrono::milliseconds fileDuration = 1000s;
  auto multiEpResult = MakeMultiEpResult(500s, 600s, fileDuration);
  CallParseEditsForEpisode(edl, multiEpResult, fileDuration);

  bool clampedFound = false;
  for (const auto& edit : edl.GetRawEditList())
  {
    if (edit.action == EDL::Action::COMM_BREAK && edit.start == 500s && edit.end == 600s)
    {
      clampedFound = true;
      break;
    }
  }
  EXPECT_TRUE(clampedFound) << "Expected wide commbreak clamped to episode window [500s - 600s]";
}

namespace
{
struct DiscEdlTest
{
  //! The disc, from the test's folder
  std::string disc;
  //! The version (playlist) played, or none for the disc itself
  int playlist;
  //! The EDL files, from the test's folder
  std::vector<std::string> edlFiles;
  //! The one of them read, or none
  std::string read;
  //! The season and episode of the episode played, if it is one
  int season{-1};
  int episode{-1};
};

void PrintTo(const DiscEdlTest& test, std::ostream* os)
{
  *os << test.disc << " playlist " << test.playlist << " episode " << test.season << "x"
      << test.episode << " reads '" << test.read << "'";
}

//! The EDL of a disc, among files of a test's own folder. The nth of the files cuts at n seconds.
class TestDiscEdl : public ::testing::TestWithParam<DiscEdlTest>
{
protected:
  void SetUp() override
  {
    std::error_code ec;
    m_root = KODI::PLATFORM::FILESYSTEM::create_temp_directory(ec);
    ASSERT_FALSE(ec);
  }

  void TearDown() override { XFILE::CDirectory::RemoveRecursive(m_root); }

  std::string File(const std::string& name, const std::string& content = {}) const
  {
    const std::string path{URIUtils::AddFileToFolder(m_root, name)};
    std::string folder{URIUtils::GetDirectory(path)};
    // The folders holding it, from the outermost
    std::vector<std::string> folders;
    while (URIUtils::PathHasParent(folder, m_root) && !XFILE::CDirectory::Exists(folder))
    {
      folders.push_back(folder);
      folder = URIUtils::GetParentPath(folder);
    }
    for (auto it = folders.rbegin(); it != folders.rend(); ++it)
      EXPECT_TRUE(XFILE::CDirectory::Create(*it));
    std::ofstream of(path);
    of << content;
    return path;
  }

  std::string m_root;
};
} // namespace

TEST_P(TestDiscEdl, ReadsTheDiscsEdl)
{
  const std::string disc{File(GetParam().disc)};
  for (size_t i = 0; i < GetParam().edlFiles.size(); ++i)
    File(GetParam().edlFiles[i], std::to_string(i + 1) + " " + std::to_string(i + 2) + " 0\n");

  CFileItem item{
      GetParam().playlist > -1 ? URIUtils::GetBlurayPlaylistPath(disc, GetParam().playlist) : disc,
      false};
  item.GetVideoInfoTag()->m_iSeason = GetParam().season;
  item.GetVideoInfoTag()->m_iEpisode = GetParam().episode;
  CEdl edl;
  const bool found{edl.ReadEditDecisionLists(item, 0, 0ms)};

  if (GetParam().read.empty())
  {
    EXPECT_FALSE(found);
    return;
  }
  ASSERT_TRUE(found);
  const auto read{std::ranges::find(GetParam().edlFiles, GetParam().read)};
  ASSERT_NE(read, GetParam().edlFiles.end());
  ASSERT_EQ(1u, edl.GetCutMarkers().size());
  EXPECT_EQ(std::chrono::seconds(read - GetParam().edlFiles.begin() + 1),
            edl.GetCutMarkers().at(0));
}

// clang-format off
const DiscEdlTest discEdlTests[] = {
    // A version of a folder rip has its own beside BDMV, named as its art is, or the movie's, or as
    // up to v21, the disc's inside BDMV
    {"M3GAN/BDMV/index.bdmv", 800,
     {"M3GAN/index-00800.edl", "M3GAN/00800.edl", "M3GAN/M3GAN.edl", "M3GAN/BDMV/index.edl"},
     "M3GAN/index-00800.edl"},
    {"M3GAN/BDMV/index.bdmv", 800,
     {"M3GAN/00800.edl", "M3GAN/M3GAN.edl", "M3GAN/BDMV/index.edl"}, "M3GAN/00800.edl"},
    {"M3GAN/BDMV/index.bdmv", 800, {"M3GAN/M3GAN.edl", "M3GAN/BDMV/index.edl"}, "M3GAN/M3GAN.edl"},
    {"M3GAN/BDMV/index.bdmv", 800, {"M3GAN/BDMV/index.edl"}, "M3GAN/BDMV/index.edl"},
    // ..but not another version's, nor in the playlist's folder
    {"M3GAN/BDMV/index.bdmv", 800, {"M3GAN/index-00801.edl", "M3GAN/00801.edl"}, ""},
    {"M3GAN/BDMV/index.bdmv", 800, {"M3GAN/BDMV/PLAYLIST/00800.edl"}, ""},
    // The disc itself has the movie's, or as up to v21, the disc's inside BDMV
    {"M3GAN/BDMV/index.bdmv", -1, {"M3GAN/M3GAN.edl", "M3GAN/BDMV/index.edl"}, "M3GAN/M3GAN.edl"},
    {"M3GAN/BDMV/index.bdmv", -1, {"M3GAN/BDMV/index.edl"}, "M3GAN/BDMV/index.edl"},
    // A disc in a folder of its own within the movie's has them beside it, named after the movie
    {"M3GAN/disc 1/BDMV/index.bdmv", 800,
     {"M3GAN/disc 1/index-00800.edl", "M3GAN/disc 1/M3GAN.edl"}, "M3GAN/disc 1/index-00800.edl"},
    {"M3GAN/disc 1/BDMV/index.bdmv", 800, {"M3GAN/disc 1/M3GAN.edl"}, "M3GAN/disc 1/M3GAN.edl"},
    {"M3GAN/disc 1/BDMV/index.bdmv", 800, {"M3GAN/disc 1/disc 1.edl", "M3GAN/M3GAN.edl"}, ""},
    // A version of an ISO has its own named as its art is, or the ISO's, but not by its playlist
    // alone, as several ISOs can share a folder
    {"M3GAN/M3GAN.iso", 800, {"M3GAN/M3GAN-00800.edl", "M3GAN/M3GAN.edl"}, "M3GAN/M3GAN-00800.edl"},
    {"M3GAN/M3GAN.iso", 800, {"M3GAN/M3GAN.edl"}, "M3GAN/M3GAN.edl"},
    {"M3GAN/M3GAN.iso", 800, {"M3GAN/00800.edl", "M3GAN/M3GAN-00801.edl"}, ""},
    {"M3GAN/disc 1/M3GAN.iso", 800, {"M3GAN/disc 1/M3GAN-00800.edl"}, "M3GAN/disc 1/M3GAN-00800.edl"},
    // A dvd folder rip has the movie's beside VIDEO_TS, or as up to v21, the disc's inside it
    {"M3GAN/VIDEO_TS/VIDEO_TS.IFO", -1, {"M3GAN/M3GAN.edl", "M3GAN/VIDEO_TS/VIDEO_TS.edl"},
     "M3GAN/M3GAN.edl"},
    {"M3GAN/VIDEO_TS/VIDEO_TS.IFO", -1, {"M3GAN/VIDEO_TS/VIDEO_TS.edl"},
     "M3GAN/VIDEO_TS/VIDEO_TS.edl"},
    {"M3GAN/disc 1/VIDEO_TS/VIDEO_TS.IFO", -1, {"M3GAN/disc 1/M3GAN.edl"}, "M3GAN/disc 1/M3GAN.edl"},
    // Several ISOs can share a folder: each has only what is named after it
    {"Movies/A.iso", 800, {"Movies/A-00800.edl", "Movies/A.edl", "Movies/B.edl", "Movies/00800.edl"},
     "Movies/A-00800.edl"},
    {"Movies/A.iso", 800, {"Movies/B-00800.edl", "Movies/B.edl", "Movies/00800.edl"}, ""},
    {"Movies/A.iso", -1, {"Movies/A.edl", "Movies/B.edl"}, "Movies/A.edl"},

    // As up to v21, inside a disc in a folder of its own within the movie's
    {"M3GAN/disc 1/BDMV/index.bdmv", 800, {"M3GAN/disc 1/BDMV/index.edl"},
     "M3GAN/disc 1/BDMV/index.edl"},
    {"M3GAN/disc 1/VIDEO_TS/VIDEO_TS.IFO", -1, {"M3GAN/disc 1/VIDEO_TS/VIDEO_TS.edl"},
     "M3GAN/disc 1/VIDEO_TS/VIDEO_TS.edl"},
    // A video file's is named after it, as always
    {"Film/Film.mkv", -1, {"Film/Film.edl", "Film/movie.edl"}, "Film/Film.edl"},
    // An episode has its own named as its art is, before its playlist's and the disc's, but not
    // another episode's
    {"Show/S02/BDMV/index.bdmv", 1001,
     {"Show/S02/index-S02E01.edl", "Show/S02/S02E01.edl", "Show/S02/index-01001.edl",
      "Show/S02/S02.edl", "Show/S02/BDMV/index.edl"},
     "Show/S02/index-S02E01.edl", 2, 1},
    {"Show/S02/BDMV/index.bdmv", 1001,
     {"Show/S02/S02E01.edl", "Show/S02/index-01001.edl", "Show/S02/S02.edl"},
     "Show/S02/S02E01.edl", 2, 1},
    {"Show/S02/BDMV/index.bdmv", 1001,
     {"Show/S02/index-S02E02.edl", "Show/S02/S02E02.edl", "Show/S02/S02.edl"},
     "Show/S02/S02.edl", 2, 1},
    {"Show/S01/VIDEO_TS/VIDEO_TS.IFO", -1,
     {"Show/S01/VIDEO_TS-S01E03.edl", "Show/S01/S01E03.edl", "Show/S01/S01.edl"},
     "Show/S01/VIDEO_TS-S01E03.edl", 1, 3},
    {"Show/S01/VIDEO_TS/VIDEO_TS.IFO", -1, {"Show/S01/VIDEO_TS-S01E04.edl", "Show/S01/S01E04.edl"},
     "", 1, 3},
    // ..of an ISO or video file, named after the file, not by the episode alone
    {"Show/Show.S01E05E06.iso", 800,
     {"Show/Show.S01E05E06-S01E05.edl", "Show/Show.S01E05E06.edl"},
     "Show/Show.S01E05E06-S01E05.edl", 1, 5},
    {"Show/Show.S01E05E06.iso", 800, {"Show/S01E05.edl", "Show/Show.S01E05E06-S01E06.edl"}, "",
     1, 5},
    {"Show/Show.S01E07E08.mkv", -1,
     {"Show/Show.S01E07E08-S01E07.edl", "Show/Show.S01E07E08.edl"},
     "Show/Show.S01E07E08-S01E07.edl", 1, 7},
    {"Show/Show.S01E07E08.mkv", -1, {"Show/Show.S01E07E08-S01E08.edl"}, "", 1, 7},
    // A name with dots keeps them all
    {"The.Matrix.1999/BDMV/index.bdmv", 800, {"The.Matrix.1999/The.Matrix.1999.edl"},
     "The.Matrix.1999/The.Matrix.1999.edl"},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(TestEdl, TestDiscEdl, testing::ValuesIn(discEdlTests));

namespace
{
//! The EDL of a disc's playlist, played as an extra or a version of the movie, among files of a
//! test's own folder
class TestDiscExtraEdl : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // The library tells an extra stored as a disc's playlist from one that is a whole disc
    if (!CServiceBroker::GetDatabaseManager().CanOpen("MyVideos"))
    {
      ASSERT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());
    }
    std::error_code ec;
    m_root = KODI::PLATFORM::FILESYSTEM::create_temp_directory(ec);
    ASSERT_FALSE(ec);
    m_folder = URIUtils::AddFileToFolder(m_root, "M3GAN");
    ASSERT_TRUE(XFILE::CDirectory::Create(m_folder));
    ASSERT_TRUE(XFILE::CDirectory::Create(URIUtils::AddFileToFolder(m_folder, "BDMV")));
    File("BDMV/index.bdmv");
    File("M3GAN.iso");
  }

  void TearDown() override
  {
    CVideoDatabase db;
    if (!m_storedFiles.empty() && db.Open())
    {
      for (const int idFile : m_storedFiles)
        db.DeleteFile(idFile);
    }
    XFILE::CDirectory::RemoveRecursive(m_root);
  }

  //! A file of the movie's folder, an EDL cutting at the given second if it has one
  void File(const std::string& name, int cut = 0) const
  {
    std::ofstream of(URIUtils::AddFileToFolder(m_folder, name));
    if (cut > 0)
      of << cut << " " << cut + 1 << " 0\n";
  }

  //! The second the EDL read for a playlist of a disc cuts at, or 0 if none is read. An extra is
  //! stored in the library as the playlist, or as the whole disc.
  int CutOf(const std::string& disc, int playlist, VideoAssetType type, bool wholeDisc = false)
  {
    const std::string discPath{URIUtils::AddFileToFolder(m_folder, disc)};
    CFileItem item{URIUtils::GetBlurayPlaylistPath(discPath, playlist), false};
    CVideoInfoTag& tag{*item.GetVideoInfoTag()};
    tag.GetAssetInfo().SetType(type);
    if (type == VideoAssetType::EXTRA)
    {
      CVideoDatabase db;
      EXPECT_TRUE(db.Open());
      tag.m_iFileId = db.AddFile(wholeDisc ? discPath : item.GetDynPath());
      m_storedFiles.insert(tag.m_iFileId);
    }
    CEdl edl;
    if (!edl.ReadEditDecisionLists(item, 0, 0ms) || edl.GetCutMarkers().size() != 1)
      return 0;
    return static_cast<int>(
        std::chrono::duration_cast<std::chrono::seconds>(edl.GetCutMarkers().at(0)).count());
  }

  std::string m_root;
  std::string m_folder;
  //! The files of the extras stored in the library
  std::set<int> m_storedFiles;
};
} // namespace

// An extra of the movie on its disc, such as a featurette, which the library stores as one of the
// disc's playlists, has only its own EDL, named for its playlist, as the disc's is the movie's. A
// version of the movie has the disc's.
TEST_F(TestDiscExtraEdl, AFolderRipsExtraHasOnlyItsOwnEdl)
{
  File("M3GAN.edl", 1);
  File("BDMV/index.edl", 2);

  EXPECT_EQ(0, CutOf("BDMV/index.bdmv", 805, VideoAssetType::EXTRA));
  EXPECT_EQ(1, CutOf("BDMV/index.bdmv", 801, VideoAssetType::VERSION));

  File("00805.edl", 3);
  EXPECT_EQ(3, CutOf("BDMV/index.bdmv", 805, VideoAssetType::EXTRA));
  File("index-00805.edl", 4);
  EXPECT_EQ(4, CutOf("BDMV/index.bdmv", 805, VideoAssetType::EXTRA));
}

TEST_F(TestDiscExtraEdl, AnIsosExtraHasOnlyItsOwnEdl)
{
  File("M3GAN.edl", 1);

  EXPECT_EQ(0, CutOf("M3GAN.iso", 805, VideoAssetType::EXTRA));
  EXPECT_EQ(1, CutOf("M3GAN.iso", 801, VideoAssetType::VERSION));

  File("M3GAN-00805.edl", 2);
  EXPECT_EQ(2, CutOf("M3GAN.iso", 805, VideoAssetType::EXTRA));
}

// An extra that is a whole disc of its own, such as Bonus.iso in an extras folder, plays one of the
// disc's playlists, but has the disc's EDL, as it is the disc's only video
TEST_F(TestDiscExtraEdl, AnExtraThatIsAWholeDiscHasItsDiscsEdl)
{
  File("Bonus.iso");
  File("Bonus.edl", 1);
  EXPECT_EQ(1, CutOf("Bonus.iso", 1, VideoAssetType::EXTRA, true));
  File("Bonus-00001.edl", 2);
  EXPECT_EQ(2, CutOf("Bonus.iso", 1, VideoAssetType::EXTRA, true));

  // ..as does a folder rip, named after the folder holding BDMV, or as up to v21 inside BDMV
  File("BDMV/index.edl", 3);
  EXPECT_EQ(3, CutOf("BDMV/index.bdmv", 1, VideoAssetType::EXTRA, true));
  File("M3GAN.edl", 4);
  EXPECT_EQ(4, CutOf("BDMV/index.bdmv", 1, VideoAssetType::EXTRA, true));
}
