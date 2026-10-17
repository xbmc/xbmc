/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/WindowIDs.h"
#include "guilib/test/TestGUIStubs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/IMessageTarget.h"
#include "messaging/ThreadMessage.h"
#include "threads/Event.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::MESSAGING;
using namespace std::chrono_literals;

namespace
{
//! \brief Records the messages the window manager would receive on the GUI thread
class CRecordingTarget : public IMessageTarget
{
public:
  int GetMessageMask() override { return TMSG_MASK_WINDOWMANAGER; }
  void OnApplicationMessage(ThreadMessage* msg) override { m_messages.push_back(*msg); }

  std::vector<ThreadMessage> m_messages;
};

//! \brief A messenger private to the test, so its receiver is not left behind in the shared one
class CScopedMessenger
{
public:
  CScopedMessenger()
    : m_previous(CServiceBroker::GetAppMessenger()),
      m_messenger(std::make_shared<CApplicationMessenger>())
  {
    CServiceBroker::RegisterAppMessenger(m_messenger);
  }

  ~CScopedMessenger()
  {
    if (m_previous)
      CServiceBroker::RegisterAppMessenger(m_previous);
    else
      CServiceBroker::UnregisterAppMessenger();
  }

  CApplicationMessenger& Get() { return *m_messenger; }

private:
  std::shared_ptr<CApplicationMessenger> m_previous;
  std::shared_ptr<CApplicationMessenger> m_messenger;
};

//! \brief A stub window system registered for the scope, restoring the previous one on every exit
class CScopedWinSystem
{
public:
  CScopedWinSystem() : m_previous(CServiceBroker::GetWinSystem())
  {
    CServiceBroker::RegisterWinSystem(&m_winSystem);
  }

  ~CScopedWinSystem()
  {
    if (m_previous)
      CServiceBroker::RegisterWinSystem(m_previous);
    else
      CServiceBroker::UnregisterWinSystem();
  }

private:
  CWinSystemBase* m_previous;
  KODI::GUILIB::TEST::CTestWinSystem m_winSystem;
};
} // unnamed namespace

TEST(TestGUIDialogExtendedProgressBar, GetHandleDoesNotWaitForTheGUIThread)
{
  // A synchronous open takes the graphics context lock on its way into SendMsg, so a window
  // system is needed for a regression to time out below instead of crashing.
  CScopedWinSystem winSystem;
  CRecordingTarget target;
  CScopedMessenger messenger;
  messenger.Get().RegisterReceiver(&target);

  CGUIDialogExtendedProgressBar dialog;
  CGUIDialogProgressBarHandle* handle{nullptr};
  CEvent returned;

  // Nothing processes messages while the caller runs, as when the GUI thread is blocked
  // joining the worker thread that asks for the handle.
  std::thread caller(
      [&dialog, &handle, &returned]()
      {
        handle = dialog.GetHandle("title");
        returned.Set();
      });

  const bool didReturn{returned.Wait(5000ms)};
  if (!didReturn)
    messenger.Get().Cleanup(); // lets a caller blocked in SendMsg return, so it can be joined
  caller.join();

  ASSERT_TRUE(didReturn);
  EXPECT_NE(nullptr, handle);

  // the open reaches the window manager once the GUI thread gets to its queue
  messenger.Get().ProcessMessages();
  ASSERT_EQ(1u, target.m_messages.size());
  EXPECT_EQ(static_cast<uint32_t>(TMSG_GUI_DIALOG_OPEN), target.m_messages.front().dwMessage);
  EXPECT_EQ(WINDOW_DIALOG_EXT_PROGRESS, target.m_messages.front().param1);
  EXPECT_EQ(nullptr, target.m_messages.front().lpVoid);
}
