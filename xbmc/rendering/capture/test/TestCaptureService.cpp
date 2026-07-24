/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "jobs/JobManager.h"
#include "rendering/capture/CaptureHandle.h"
#include "rendering/capture/CaptureService.h"
#include "test/MtTestUtils.h"

#include <atomic>
#include <chrono>
#include <memory>

#include <gtest/gtest.h>

using namespace KODI::RENDERING::CAPTURE;
using namespace std::chrono_literals;

namespace
{

CaptureResult MakeResult(CaptureContent content = CaptureContent::COMPOSITE)
{
  CaptureResult result;
  result.width = 4;
  result.height = 2;
  result.stride = 16;
  result.bitDepth = 8;
  result.content = content;
  result.pixels = std::shared_ptr<uint8_t[]>(new uint8_t[32]());
  return result;
}

class TestCaptureService : public ::testing::Test
{
protected:
  TestCaptureService() { CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>()); }

  ~TestCaptureService() override
  {
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
    CServiceBroker::UnregisterJobManager();
  }

  CCaptureService m_service;
};

} // namespace

TEST_F(TestCaptureService, OneShotCompleteWait)
{
  auto handle = m_service.Submit({});
  EXPECT_TRUE(m_service.LatchFrame());

  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(handle->Wait(1000ms));
  EXPECT_EQ(handle->GetResult().width, 4u);

  // a finished one-shot leaves the active list
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::COMPOSITE).empty());
}

TEST_F(TestCaptureService, HandleDestructionCancels)
{
  m_service.Submit({});

  // the discarded handle cancelled the request before any latch
  EXPECT_FALSE(m_service.LatchFrame());
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::COMPOSITE).empty());
}

TEST_F(TestCaptureService, ContentFilter)
{
  CaptureSpec spec;
  spec.content = CaptureContent::VIDEO;
  auto handle = m_service.Submit(spec);

  EXPECT_TRUE(m_service.LatchFrame());
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::COMPOSITE).empty());
  EXPECT_EQ(m_service.TakeActive(CaptureContent::VIDEO).size(), 1u);
}

TEST_F(TestCaptureService, FailWakesWaiter)
{
  auto handle = m_service.Submit({});
  m_service.LatchFrame();

  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Fail(active[0]);
  EXPECT_FALSE(handle->Wait(1000ms));
}

TEST_F(TestCaptureService, CallbackRunsOnWorker)
{
  std::atomic<bool> ran{false};
  auto handle =
      m_service.Submit({}, [&ran](const CaptureResult& result) { ran = result.width == 4; });

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(ConditionPoll::poll(10000, [&ran] { return ran.load(); }));
}

TEST_F(TestCaptureService, ContinuousStaysActive)
{
  CaptureSpec spec;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto handle = m_service.Submit(spec);

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Complete(active[0], MakeResult());

  // a continuous request stays latched for the next frame
  EXPECT_EQ(m_service.TakeActive(CaptureContent::COMPOSITE).size(), 1u);
}

TEST_F(TestCaptureService, DetachedRequestCompletes)
{
  std::atomic<bool> ran{false};
  auto handle = m_service.Submit({}, [&ran](const CaptureResult&) { ran = true; });
  handle->Detach();

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(ConditionPoll::poll(10000, [&ran] { return ran.load(); }));
}

TEST_F(TestCaptureService, CancelledRequestPrunedAtLatch)
{
  auto keep = m_service.Submit({});
  {
    auto dropped = m_service.Submit({});
  }

  EXPECT_TRUE(m_service.LatchFrame());
  EXPECT_EQ(m_service.TakeActive(CaptureContent::COMPOSITE).size(), 1u);
}

TEST_F(TestCaptureService, ContinuousWaitNextSeesEachComplete)
{
  CaptureSpec spec;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto handle = m_service.Submit(spec);

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(handle->WaitNext(1000ms));

  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(handle->WaitNext(1000ms));

  // no third delivery: WaitNext times out
  EXPECT_FALSE(handle->WaitNext(10ms));
}

TEST_F(TestCaptureService, ContinuousFailIsTerminal)
{
  CaptureSpec spec;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto handle = m_service.Submit(spec);

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  // whether a tap can serve is fixed by the configuration, so a failure ends
  // the stream rather than retrying it frame after frame
  m_service.Fail(active[0]);
  EXPECT_FALSE(handle->WaitNext(10ms));
  EXPECT_EQ(handle->GetState(), CaptureState::FAILED);
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::COMPOSITE).empty());
}

TEST_F(TestCaptureService, OneShotFailIsTerminal)
{
  auto handle = m_service.Submit({});

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Fail(active[0]);
  EXPECT_FALSE(handle->WaitNext(1000ms));
  EXPECT_EQ(handle->GetState(), CaptureState::FAILED);
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::COMPOSITE).empty());
}

TEST_F(TestCaptureService, BothServedByBothTapsSameFrame)
{
  CaptureSpec spec;
  spec.content = CaptureContent::BOTH;
  int videoFiles = 0;
  int compositeFiles = 0;
  auto handle = m_service.Submit(spec, [&](const CaptureResult& r) {
    if (r.content == CaptureContent::VIDEO)
      videoFiles++;
    else if (r.content == CaptureContent::COMPOSITE)
      compositeFiles++;
  });

  m_service.LatchFrame();
  // a BOTH request is handed to both taps in the one frame
  ASSERT_EQ(m_service.TakeActive(CaptureContent::VIDEO).size(), 1u);
  auto both = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(both.size(), 1u);

  // video-only first: does not finish the request
  m_service.Complete(both[0], MakeResult(CaptureContent::VIDEO));
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::COMPOSITE).size() == 1u);

  // composite half finishes it
  m_service.Complete(both[0], MakeResult(CaptureContent::COMPOSITE));
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::COMPOSITE).empty());

  EXPECT_TRUE(ConditionPoll::poll(10000, [&] { return videoFiles == 1 && compositeFiles == 1; }));
}

TEST_F(TestCaptureService, BothWithoutCallbackRefused)
{
  CaptureSpec spec;
  spec.content = CaptureContent::BOTH;
  auto handle = m_service.Submit(spec); // no callback to consume the two captures

  // refused up front: BOTH is callback-only, a synchronous consumer would get
  // an ambiguous single half
  EXPECT_EQ(handle->GetState(), CaptureState::FAILED);
  EXPECT_FALSE(handle->Wait(10ms));
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::VIDEO).empty());
}

TEST_F(TestCaptureService, MostRecentRequestWins)
{
  // pure LIFO: the newest request is the serviced consumer whatever its
  // cadence, so a continuous submitted after a one-shot buries it (accepted).
  auto oneshot = m_service.Submit({});
  CaptureSpec cont;
  cont.cadence = CaptureCadence::CONTINUOUS;
  auto continuous = m_service.Submit(cont); // most recent, sits on top

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);
  EXPECT_EQ(active[0]->spec.cadence, CaptureCadence::CONTINUOUS);
}

TEST_F(TestCaptureService, WaitNextFalseOnCancel)
{
  CaptureSpec spec;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto handle = m_service.Submit(spec);

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Cancel(active[0]);
  EXPECT_FALSE(handle->WaitNext(1000ms));
  EXPECT_EQ(handle->GetState(), CaptureState::CANCELLED);
}

TEST_F(TestCaptureService, CopyResultLatestWins)
{
  CaptureSpec spec;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto handle = m_service.Submit(spec);

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Complete(active[0], MakeResult());
  CaptureResult second = MakeResult();
  second.width = 8;
  m_service.Complete(active[0], std::move(second));

  // two deliveries between waits collapse to a single wake with the latest result
  EXPECT_TRUE(handle->WaitNext(1000ms));
  EXPECT_EQ(handle->CopyResult().width, 8u);
  EXPECT_FALSE(handle->WaitNext(10ms));
}

TEST_F(TestCaptureService, OneShotWaitNextSeesDelivery)
{
  auto handle = m_service.Submit({});

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);

  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(handle->WaitNext(1000ms));
  EXPECT_EQ(handle->CopyResult().width, 4u);

  // a finished one-shot delivers exactly once
  EXPECT_FALSE(handle->WaitNext(10ms));
  EXPECT_EQ(handle->GetState(), CaptureState::DONE);
}

TEST_F(TestCaptureService, OneShotPreemptsContinuous)
{
  CaptureSpec spec;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto continuous = m_service.Submit(spec);

  m_service.LatchFrame();
  ASSERT_EQ(m_service.TakeActive(CaptureContent::COMPOSITE).size(), 1u);

  auto oneshot = m_service.Submit({});
  m_service.LatchFrame();

  // the one-shot pushed onto the stack top; only it is serviced this loop
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);
  EXPECT_EQ(active[0]->spec.cadence, CaptureCadence::ONESHOT);
  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(oneshot->Wait(1000ms));

  // the continuous request resumes as stack top on the next loop
  m_service.LatchFrame();
  active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);
  EXPECT_EQ(active[0]->spec.cadence, CaptureCadence::CONTINUOUS);
  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(continuous->WaitNext(1000ms));
}

TEST_F(TestCaptureService, StackedOneShotsServeNewestFirst)
{
  auto lower = m_service.Submit({});
  auto upper = m_service.Submit({});

  // newest first: the top one-shot serves, then the one under it becomes the
  // top on the next loop and serves (each forces a frame while it is chosen)
  EXPECT_TRUE(m_service.LatchFrame());
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);
  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(upper->Wait(1000ms));

  EXPECT_TRUE(m_service.LatchFrame());
  active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);
  m_service.Complete(active[0], MakeResult());
  EXPECT_TRUE(lower->Wait(1000ms));

  EXPECT_FALSE(m_service.LatchFrame());
}

TEST_F(TestCaptureService, LoneContinuousDoesNotForceFrames)
{
  CaptureSpec spec;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto handle = m_service.Submit(spec);

  // arrival forces exactly one frame; steady state never does (unchanged
  // frames are still skipped during a continuous capture)
  EXPECT_TRUE(m_service.LatchFrame());
  EXPECT_FALSE(m_service.LatchFrame());
}

TEST_F(TestCaptureService, StarvedRequestTimesOutClean)
{
  CaptureSpec spec;
  spec.content = CaptureContent::VIDEO;
  auto handle = m_service.Submit(spec);

  m_service.LatchFrame();

  // no tap ever serves it (no video layer drawn): waits time out, nothing
  // crashes, and handle destruction reclaims the request
  EXPECT_FALSE(handle->Wait(10ms));
  EXPECT_FALSE(handle->WaitNext(10ms));
  handle.reset();

  EXPECT_FALSE(m_service.LatchFrame());
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::VIDEO).empty());
}

TEST_F(TestCaptureService, UnservedOneShotFailsAtFrameEnd)
{
  CaptureSpec spec;
  spec.content = CaptureContent::VIDEO;
  auto handle = m_service.Submit(spec);

  // latched and granted a forced render, but no tap serves it (no video drawn)
  EXPECT_TRUE(m_service.LatchFrame());
  EXPECT_NE(handle->GetState(), CaptureState::FAILED);

  // the frame was drawn and every tap ran without serving it, so it cannot be
  // served in this configuration
  m_service.FrameComplete();
  EXPECT_EQ(handle->GetState(), CaptureState::FAILED);
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::VIDEO).empty());
  EXPECT_FALSE(m_service.LatchFrame()); // and it no longer forces frames
}

TEST_F(TestCaptureService, UnservedContinuousFailsAtFrameEnd)
{
  CaptureSpec spec;
  spec.content = CaptureContent::VIDEO;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto handle = m_service.Submit(spec);

  // a CONTINUOUS request gets the same judgement as a one-shot: without it the
  // request would be re-latched every frame forever, starving everything below
  m_service.LatchFrame();
  m_service.FrameComplete();

  EXPECT_EQ(handle->GetState(), CaptureState::FAILED);
  EXPECT_TRUE(m_service.TakeActive(CaptureContent::VIDEO).empty());
}

TEST_F(TestCaptureService, ServedContinuousSurvivesFrameEnd)
{
  CaptureSpec spec;
  spec.cadence = CaptureCadence::CONTINUOUS;
  auto handle = m_service.Submit(spec);

  m_service.LatchFrame();
  auto active = m_service.TakeActive(CaptureContent::COMPOSITE);
  ASSERT_EQ(active.size(), 1u);
  m_service.Complete(active[0], MakeResult());
  m_service.FrameComplete();

  EXPECT_NE(handle->GetState(), CaptureState::FAILED);
  EXPECT_TRUE(handle->WaitNext(1000ms));
}
