/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "interfaces/json-rpc/PlayerOperations.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/IMessageTarget.h"
#include "test/TestUtils.h"
#include "utils/Variant.h"

#include <memory>
#include <string>
#include <tuple>

#include <gtest/gtest.h>

using namespace KODI::MESSAGING;

class TestPlayerOpenDefault : public testing::TestWithParam<std::tuple<std::string, std::string>>,
                              public IMessageTarget
{
protected:
  void SetUp() override
  {
    m_previousMessenger = CServiceBroker::GetAppMessenger();
    m_messenger = std::make_shared<CApplicationMessenger>();
    m_messenger->RegisterReceiver(this);
    CServiceBroker::RegisterAppMessenger(m_messenger);
    m_file = XBMC_CREATETEMPFILE(std::get<0>(GetParam()));
    ASSERT_NE(nullptr, m_file);
  }

  void TearDown() override
  {
    m_messenger->ProcessMessages();
    CServiceBroker::RegisterAppMessenger(m_previousMessenger);
    EXPECT_TRUE(XBMC_DELETETEMPFILE(m_file));
  }

  int GetMessageMask() override { return TMSG_MASK_PLAYLISTPLAYER; }

  void OnApplicationMessage(ThreadMessage* message) override
  {
    ASSERT_EQ(TMSG_MEDIA_PLAY, message->dwMessage);
    m_items.reset(static_cast<CFileItemList*>(message->lpVoid));
    m_player = message->strParam;
  }

  std::shared_ptr<CApplicationMessenger> m_previousMessenger;
  std::shared_ptr<CApplicationMessenger> m_messenger;
  XFILE::CFile* m_file{nullptr};
  std::unique_ptr<CFileItemList> m_items;
  std::string m_player;
};

TEST_P(TestPlayerOpenDefault, PostsDefaultPlayerForFile)
{
  CVariant parameters(CVariant::VariantTypeObject);
  const std::string path = CXBMCTestUtils::Instance().TempFilePath(m_file);
  parameters["item"]["file"] = path;
  const std::string& option = std::get<1>(GetParam());
  if (option == "null")
    parameters["options"]["playername"] = CVariant();
  else if (option == "default")
    parameters["options"]["playername"] = option;

  CVariant result;
  EXPECT_EQ(JSONRPC::ACK,
            JSONRPC::CPlayerOperations::Open("Player.Open", nullptr, nullptr, parameters, result));
  m_messenger->ProcessMessages();

  ASSERT_NE(nullptr, m_items);
  ASSERT_EQ(1, m_items->Size());
  EXPECT_EQ(path, (*m_items)[0]->GetPath());
  EXPECT_TRUE(m_player.empty()) << m_player;
}

INSTANTIATE_TEST_SUITE_P(FileOptions,
                         TestPlayerOpenDefault,
                         testing::Combine(testing::Values(".wav", ".mp4"),
                                          testing::Values("omitted", "null", "default")));
