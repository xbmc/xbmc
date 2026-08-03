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

CaptureResult MakeResult()
{
  CaptureResult result;
  result.width = 4;
  result.height = 2;
  result.stride = 16;
  result.bitDepth = 8;
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
