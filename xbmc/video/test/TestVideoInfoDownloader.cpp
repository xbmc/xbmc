/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/Scraper.h"
#include "events/EventLog.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/IMessageTarget.h"
#include "messaging/ThreadMessage.h"
#include "video/VideoInfoDownloader.h"

#include <cstdint>
#include <memory>

#include <gtest/gtest.h>

using namespace KODI::MESSAGING;

namespace
{
//! Counts the OK dialogs asked for, through a messenger of its own. Without a window system,
//! asking doesn't wait for the dialog, and the request waits in the messenger to be counted.
class CDialogsAsked : public IMessageTarget
{
public:
  CDialogsAsked() : m_previous{CServiceBroker::GetAppMessenger()}
  {
    m_messenger->RegisterReceiver(this);
    CServiceBroker::RegisterAppMessenger(m_messenger);
  }

  ~CDialogsAsked() override
  {
    if (m_previous)
      CServiceBroker::RegisterAppMessenger(m_previous);
    else
      CServiceBroker::UnregisterAppMessenger();
  }

  int GetMessageMask() override { return TMSG_MASK_WINDOWMANAGER; }

  void OnApplicationMessage(ThreadMessage* msg) override
  {
    if (msg->dwMessage == static_cast<uint32_t>(TMSG_GUI_DIALOG_OK))
      ++m_count;
  }

  int Count()
  {
    m_messenger->ProcessMessages();
    return m_count;
  }

private:
  std::shared_ptr<CApplicationMessenger> m_previous;
  std::shared_ptr<CApplicationMessenger> m_messenger{std::make_shared<CApplicationMessenger>()};
  int m_count{0};
};
} // unnamed namespace

TEST(TestVideoInfoDownloader, AnErrorInABackgroundScanIsLoggedNotShown)
{
  // With one, asking for a dialog would wait for a GUI thread to show it
  ASSERT_EQ(nullptr, CServiceBroker::GetWinSystem());
  CEventLog* eventLog{CServiceBroker::GetEventLog()};
  ASSERT_NE(nullptr, eventLog);
  const size_t errors{eventLog->Get(EventLevel::Error).size()};
  CDialogsAsked dialogs;
  const ADDON::CScraperError error{"Site down", "Try again later"};

  {
    CLoggedScraperErrors loggedScraperErrors;
    CVideoInfoDownloader::ShowErrorDialog(error);
    CVideoInfoDownloader::ShowErrorDialog(error);
    CVideoInfoDownloader::ShowErrorDialog({"Site down", "Your key has expired"});

    EXPECT_EQ(0, dialogs.Count());
    // Once for each title and message
    const Events added{eventLog->Get(EventLevel::Error)};
    ASSERT_EQ(errors + 2, added.size());
    EXPECT_EQ("Site down", added[errors]->GetLabel());
    EXPECT_EQ("Try again later", added[errors]->GetDescription());
    EXPECT_EQ("Your key has expired", added[errors + 1]->GetDescription());
  }

  // Once the scan is over, an error is shown again
  CVideoInfoDownloader::ShowErrorDialog(error);
  EXPECT_EQ(1, dialogs.Count());
  EXPECT_EQ(errors + 2, eventLog->Get(EventLevel::Error).size());
}
