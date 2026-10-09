/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/AnnouncementManager.h"
#include "threads/Event.h"
#include "utils/Variant.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <gtest/gtest.h>

using namespace ANNOUNCEMENT;
using namespace std::chrono_literals;

namespace
{
constexpr auto TIMEOUT = 5s;

//! Stands in for an announcer that waits on another thread, as closing a window waits on the GUI
//! thread.
class CBlockingAnnouncer : public IAnnouncer
{
public:
  void Announce(AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override
  {
    m_entered.Set();
    m_released = m_release.Wait(TIMEOUT);
  }

  CEvent m_entered;
  CEvent m_release;
  std::atomic<bool> m_released{false};
};

class CRecordingAnnouncer : public IAnnouncer
{
public:
  void Announce(AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override
  {
    if (m_removed)
      m_calledAfterRemoval = true;
  }

  std::atomic<bool> m_removed{false};
  std::atomic<bool> m_calledAfterRemoval{false};
};

class CSelfRemovingAnnouncer : public IAnnouncer
{
public:
  explicit CSelfRemovingAnnouncer(CAnnouncementManager& manager) : m_manager(manager) {}

  void Announce(AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override
  {
    m_manager.RemoveAnnouncer(this);
    m_removed.Set();
  }

  CAnnouncementManager& m_manager;
  CEvent m_removed;
};

class CThrowingAnnouncer : public IAnnouncer
{
public:
  void Announce(AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override
  {
    m_called.Set();
    throw std::runtime_error("announcer failed");
  }

  CEvent m_called;
};
} // namespace

class TestAnnouncementManager : public ::testing::Test
{
protected:
  void SetUp() override { m_manager.Start(); }

  // Declared before the manager so they outlive its thread.
  CBlockingAnnouncer m_blocking;
  CRecordingAnnouncer m_recording;
  CAnnouncementManager m_manager;
};

TEST_F(TestAnnouncementManager, AnAnnouncerCanBeAddedWhileAnotherIsBeingCalled)
{
  m_manager.AddAnnouncer(&m_blocking);
  m_manager.Announce(Other, "Test");
  ASSERT_TRUE(m_blocking.m_entered.Wait(TIMEOUT));

  m_manager.AddAnnouncer(&m_recording);
  m_blocking.m_release.Set();
  m_manager.Deinitialize();

  EXPECT_TRUE(m_blocking.m_released);
}

TEST_F(TestAnnouncementManager, AnAnnouncerRemovedDuringADispatchIsNotCalledAfterwards)
{
  m_manager.AddAnnouncer(&m_blocking);
  m_manager.AddAnnouncer(&m_recording);
  m_manager.Announce(Other, "Test");
  ASSERT_TRUE(m_blocking.m_entered.Wait(TIMEOUT));

  m_manager.RemoveAnnouncer(&m_recording);
  m_recording.m_removed = true;
  m_blocking.m_release.Set();
  m_manager.Deinitialize();

  EXPECT_TRUE(m_blocking.m_released);
  EXPECT_FALSE(m_recording.m_calledAfterRemoval);
}

TEST_F(TestAnnouncementManager, RemovingAnAnnouncerWaitsForItsCallInProgress)
{
  m_manager.AddAnnouncer(&m_blocking);
  m_manager.Announce(Other, "Test");
  ASSERT_TRUE(m_blocking.m_entered.Wait(TIMEOUT));

  std::atomic<bool> removed{false};
  std::thread remover(
      [this, &removed]
      {
        m_manager.RemoveAnnouncer(&m_blocking);
        removed = true;
      });
  std::this_thread::sleep_for(200ms);
  EXPECT_FALSE(removed);

  m_blocking.m_release.Set();
  remover.join();
  EXPECT_TRUE(removed);
  EXPECT_TRUE(m_blocking.m_released);
}

TEST_F(TestAnnouncementManager, AnAnnouncerCanRemoveItselfWhileBeingCalled)
{
  CSelfRemovingAnnouncer announcer(m_manager);
  m_manager.AddAnnouncer(&announcer);
  m_manager.Announce(Other, "Test");

  EXPECT_TRUE(announcer.m_removed.Wait(TIMEOUT));
  m_manager.Deinitialize();
}

TEST(TestAnnouncementManagerFailure, RemovingAnAnnouncerWhoseCallThrewDoesNotWait)
{
  // Leaked if the removal never returns, so the thread left waiting in it touches nothing freed
  auto manager = std::make_unique<CAnnouncementManager>();
  auto announcer = std::make_unique<CThrowingAnnouncer>();
  manager->Start();
  manager->AddAnnouncer(announcer.get());
  manager->Announce(Other, "Test");
  ASSERT_TRUE(announcer->m_called.Wait(TIMEOUT));

  auto removed = std::make_shared<CEvent>();
  std::thread remover(
      [m = manager.get(), a = announcer.get(), removed]
      {
        m->RemoveAnnouncer(a);
        removed->Set();
      });
  if (!removed->Wait(TIMEOUT))
  {
    remover.detach();
    manager.release();
    announcer.release();
    FAIL() << "RemoveAnnouncer is still waiting on a call that threw";
  }
  remover.join();
}
