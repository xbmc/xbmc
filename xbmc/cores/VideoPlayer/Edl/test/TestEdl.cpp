/*
 *  Copyright (C) 2022- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Edl.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"

#include <chrono>
#include <cmath>

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
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  EXPECT_EQ(edl.GetSceneMarkers().at(1),
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

TEST_F(TestEdl, TestParsingMplayerTimeBasedInterleavedCutsEDL)
{
  CEdl edl;

  // create a dummy "media" fileitem whose corresponding edl file is testdata/mplayertimebasedinterleavedcuts.edl
  // this is an edl file with commbreaks interleaved with cuts
  CFileItem mediaItem;
  mediaItem.SetPath(XBMC_REF_FILE_PATH(
      "xbmc/cores/VideoPlayer/Edl/test/testdata/mplayertimebasedinterleavedcuts.mkv"));
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  const bool found = edl.ReadEditDecisionLists(mediaItem, fps);
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
  bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  bool found = edl.ReadEditDecisionLists(mediaItem, 0);
  // fps was not supplied, kodi will not be able to process the file
  EXPECT_EQ(found, false);
  // parse the file again this time supplying 60 fps
  const float fps = 60;
  found = edl.ReadEditDecisionLists(mediaItem, fps);
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
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0);
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
  const bool found = edl.ReadEditDecisionLists(mediaItem, 0);
  EXPECT_EQ(found, true);
  // kodi should merge all commbreaks into two
  EXPECT_EQ(edl.GetEditList().size(), 2);
  // second edit of the original file + third one
  EXPECT_EQ(edl.GetEditList().at(0).end - edl.GetEditList().at(0).start, 32s - 10s);
  // 4th, 5th and 6th commbreaks joined
  EXPECT_EQ(edl.GetEditList().at(1).end - edl.GetEditList().at(1).start, 65100ms - 37s);
}
