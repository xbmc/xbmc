/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheats/GameClientCheats.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "jobs/JobManager.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
constexpr auto DATABASE = "resource.games.cheats.libretro";

template<typename Tag, typename Tag::Type member>
struct MemberAccess
{
  friend typename Tag::Type GetMember(Tag) { return member; }
};
struct Subsystems
{
  using Type = GameClientSubsystems CGameClient::*;
  friend Type GetMember(Subsystems);
};
template struct MemberAccess<Subsystems, &CGameClient::m_subsystems>;
struct Playing
{
  using Type = std::atomic_bool CGameClient::*;
  friend Type GetMember(Playing);
};
template struct MemberAccess<Playing, &CGameClient::m_bIsPlaying>;
struct ClientMutex
{
  using Type = CCriticalSection CGameClient::*;
  friend Type GetMember(ClientMutex);
};
template struct MemberAccess<ClientMutex, &CGameClient::m_critSection>;
struct Events
{
  using Type = CEventSource<ADDON::AddonEvent> ADDON::CAddonMgr::*;
  friend Type GetMember(Events);
};
template struct MemberAccess<Events, &ADDON::CAddonMgr::m_events>;

class TestGUI : public CGUIComponent
{
public:
  TestGUI() : CGUIComponent(false)
  {
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
    CServiceBroker::RegisterGUI(this);
  }
  ~TestGUI() override { m_pWindowManager.reset(); }
};

class TestCheats : public CGameClientCheats
{
public:
  using CGameClientCheats::CGameClientCheats;
  using CGameClientCheats::DatabaseState;
  using CGameClientCheats::OnAddonEvent;
  using CGameClientCheats::Source;

  DatabaseState database{DatabaseState::MISSING};
  bool installSucceeds{true};
  bool enableSucceeds{true};
  int installs{0};
  int enables{0};
  int lookups{0};
  mutable int availabilityChecks{0};
  std::vector<Source> sources;
  std::deque<std::function<void()>> jobs;
  std::function<void()> onInstall;
  std::function<void()> onLookup;
  std::function<void()> onSubmit;

  void RunJobs()
  {
    while (!jobs.empty())
    {
      auto job = std::move(jobs.front());
      jobs.pop_front();
      job();
    }
  }

protected:
  DatabaseState GetDatabaseState() const override { return database; }
  bool IsDatabaseInstallable() const override
  {
    ++availabilityChecks;
    return true;
  }
  bool InstallDatabase() override
  {
    ++installs;
    if (onInstall)
      onInstall();
    if (installSucceeds)
      database = DatabaseState::AVAILABLE;
    return installSucceeds;
  }
  bool EnableDatabase() override
  {
    ++enables;
    if (enableSucceeds)
      database = DatabaseState::AVAILABLE;
    return enableSucceeds;
  }
  std::vector<Source> GetSources() const override
  {
    return database == DatabaseState::AVAILABLE ? sources : std::vector<Source>{};
  }
  bool IsResourceAddon(const std::string& id) const override
  {
    return id == DATABASE || id == "resource.games.other";
  }
  CCheatPack ReadPack(const std::string& path, const std::string&) override
  {
    ++lookups;
    if (const auto callback = onLookup)
      callback();
    return CCheatPack::Load(path);
  }
  void Submit(std::function<void()> job) override
  {
    if (onSubmit)
      onSubmit();
    jobs.emplace_back(std::move(job));
  }
};
} // namespace

class TestGameClientCheats : public testing::Test
{
protected:
  void SetUp() override
  {
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    CXBMCTinyXML2 xml;
    ASSERT_TRUE(
        xml.Parse(std::string(R"(<addon id="game.test.cheats" name="Cheats test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so"><extensions>rom</extensions></extension>
      <extension point="kodi.addon.metadata"><platform>all</platform></extension>
    </addon>)")));
    auto info = ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
    ASSERT_NE(info, nullptr);
    client = std::make_shared<CGameClient>(info);
    auto* callbacks = client->GetInstanceInterface()->toAddon;
    callbacks->addonInstance = this;
    callbacks->CheatReset = [](const AddonInstance_Game* game)
    {
      auto& test = *static_cast<TestGameClientCheats*>(game->toAddon->addonInstance);
      ++test.resets;
      test.applied.clear();
      return test.supportsCheats ? GAME_ERROR_NO_ERROR : GAME_ERROR_NOT_IMPLEMENTED;
    };
    callbacks->SetCheat = [](const AddonInstance_Game* game, unsigned int, bool, const char* code)
    {
      auto& test = *static_cast<TestGameClientCheats*>(game->toAddon->addonInstance);
      test.applied.emplace_back(code);
      return GAME_ERROR_NO_ERROR;
    };
    client.get()->*GetMember(Playing{}) = true;
    auto& subsystems = client.get()->*GetMember(Subsystems{});
    subsystems.Cheats =
        std::make_unique<TestCheats>(*client, *client->GetInstanceInterface(), ClientAccess());
    cheats = static_cast<TestCheats*>(subsystems.Cheats.get());
    file = XBMC_CREATETEMPFILE(".cht");
    ASSERT_NE(file, nullptr);
    file->Close();
    WritePack("cheats = 2\ncheat0_desc = Lives\ncheat0_code = AAA\n"
              "cheat1_desc = Health\ncheat1_code = BBB\n");
    cheats->sources = {{DATABASE, XBMC_TEMPFILEPATH(file), info}};
  }

  void TearDown() override
  {
    if (client)
    {
      cheats->Clear();
      client.get()->*GetMember(Playing{}) = false;
      client.reset();
    }
    if (file)
      XBMC_DELETETEMPFILE(file);
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::UnregisterJobManager();
  }

  void WritePack(const std::string& contents)
  {
    XFILE::CFile output;
    ASSERT_TRUE(output.OpenForWrite(XBMC_TEMPFILEPATH(file), true));
    ASSERT_EQ(output.Write(contents.data(), contents.size()), contents.size());
  }

  using Result = CGameClientCheats::InstallResult;
  using State = TestCheats::DatabaseState;
  TestGUI gui;
  CCriticalSection& ClientAccess() { return client.get()->*GetMember(ClientMutex{}); }
  std::shared_ptr<CGameClient> client;
  TestCheats* cheats{nullptr};
  XFILE::CFile* file{nullptr};
  bool supportsCheats{true};
  int resets{0};
  std::vector<std::string> applied;
};

TEST_F(TestGameClientCheats, SuccessfulInstallationFindsCheats)
{
  cheats->Load("game.rom");
  auto install = cheats->GetInstallTask();
  ASSERT_TRUE(install);
  EXPECT_EQ(install(), Result::CHEATS_FOUND);
  EXPECT_EQ(cheats->installs, 1);
  EXPECT_FALSE(cheats->CanInstallCheats());
  EXPECT_EQ(cheats->GetCheats().size(), 2U);
}

TEST_F(TestGameClientCheats, SuccessfulInstallationCompletesEmptyLookup)
{
  WritePack("cheats = 1\ncheat0_desc = Unusable\n");
  cheats->Load("game.rom");
  EXPECT_EQ(cheats->GetInstallTask()(), Result::NO_CHEATS);
  EXPECT_EQ(cheats->lookups, 1);
  EXPECT_FALSE(cheats->HasCheats());
}

TEST_F(TestGameClientCheats, FailedOrCancelledInstallationAllowsRetry)
{
  cheats->installSucceeds = false;
  cheats->Load("game.rom");
  EXPECT_EQ(cheats->GetInstallTask()(), Result::FAILED);
  EXPECT_EQ(cheats->lookups, 0);
  EXPECT_TRUE(cheats->CanInstallCheats());
  auto retry = cheats->GetInstallTask();
  ASSERT_TRUE(retry);
  cheats->installSucceeds = true;
  EXPECT_EQ(retry(), Result::CHEATS_FOUND);
}

TEST_F(TestGameClientCheats, FailedEnableDoesNotReportAnEmptyPack)
{
  cheats->database = State::DISABLED;
  cheats->enableSucceeds = false;
  cheats->Load("game.rom");
  EXPECT_EQ(cheats->GetInstallTask()(), Result::FAILED);
  EXPECT_EQ(cheats->enables, 1);
  EXPECT_EQ(cheats->installs, 0);
  EXPECT_EQ(cheats->lookups, 0);
  EXPECT_TRUE(cheats->GetInstallTask());
}

TEST_F(TestGameClientCheats, DatabaseBecomingAvailableBeforeJobRunsIsSuccess)
{
  cheats->Load("game.rom");
  auto install = cheats->GetInstallTask();
  cheats->database = State::AVAILABLE;
  EXPECT_EQ(install(), Result::CHEATS_FOUND);
  EXPECT_EQ(cheats->installs, 0);
  EXPECT_EQ(cheats->enables, 0);
}

TEST_F(TestGameClientCheats, ExternalInstallOrEnableReloadsDuringGameplay)
{
  for (bool enable : {false, true})
  {
    SCOPED_TRACE(enable);
    cheats->database = enable ? State::DISABLED : State::MISSING;
    cheats->Load("game.rom");
    EXPECT_FALSE(cheats->HasCheats());
    cheats->database = State::AVAILABLE;
    if (enable)
      cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
    else
      cheats->OnAddonEvent(ADDON::AddonEvents::ReInstalled(DATABASE));
    ASSERT_EQ(cheats->jobs.size(), 1U);
    EXPECT_FALSE(cheats->HasCheats());
    cheats->RunJobs();
    EXPECT_TRUE(cheats->HasCheats());
    EXPECT_TRUE(cheats->CanOfferCheats());
  }
}

TEST_F(TestGameClientCheats, DuplicateEventsAndInstallPreserveUserChoices)
{
  cheats->Load("game.rom");
  cheats->onInstall = [this]
  {
    cheats->OnAddonEvent(ADDON::AddonEvents::ReInstalled(DATABASE));
    cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  };
  auto install = cheats->GetInstallTask();
  EXPECT_FALSE(cheats->GetInstallTask());
  EXPECT_EQ(install(), Result::CHEATS_FOUND);
  cheats->SetEnabled(1, true, cheats->GetCheats()[1]);
  const int previousResets = resets;
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  cheats->OnAddonEvent(ADDON::AddonEvents::ReInstalled(DATABASE));
  cheats->RunJobs();
  EXPECT_EQ(cheats->lookups, 1);
  EXPECT_EQ(resets, previousResets);
  EXPECT_TRUE(cheats->GetCheats()[1].enabled);
}

TEST_F(TestGameClientCheats, UpdatedSourcePreservesUnchangedCheatsAndRemovesMissingCodes)
{
  cheats->database = State::AVAILABLE;
  cheats->Load("game.rom");
  cheats->SetEnabled(0, true, cheats->GetCheats()[0]);
  cheats->SetEnabled(1, true, cheats->GetCheats()[1]);
  WritePack("cheats = 2\ncheat0_desc = Health\ncheat0_code = BBB\n"
            "cheat1_desc = New cheat\ncheat1_code = CCC\n");
  cheats->sources[0].revision.reset();
  cheats->OnAddonEvent(ADDON::AddonEvents::ReInstalled(DATABASE));
  cheats->RunJobs();
  ASSERT_EQ(cheats->GetCheats().size(), 2U);
  EXPECT_TRUE(cheats->GetCheats()[0].enabled);
  EXPECT_FALSE(cheats->GetCheats()[1].enabled);
  EXPECT_EQ(applied, (std::vector<std::string>{"BBB"}));
}

TEST_F(TestGameClientCheats, DisableOrRemovalDropsAppliedCheats)
{
  for (bool remove : {false, true})
  {
    SCOPED_TRACE(remove);
    cheats->database = State::AVAILABLE;
    cheats->Load("game.rom");
    cheats->SetEnabled(0, true, cheats->GetCheats()[0]);
    cheats->database = remove ? State::MISSING : State::DISABLED;
    if (remove)
      cheats->OnAddonEvent(ADDON::AddonEvents::UnInstalled(DATABASE));
    else
      cheats->OnAddonEvent(ADDON::AddonEvents::Disabled(DATABASE));
    cheats->RunJobs();
    EXPECT_FALSE(cheats->HasCheats());
    EXPECT_TRUE(applied.empty());
  }
}

TEST_F(TestGameClientCheats, UnrelatedEventsDoNotQueuePackLookups)
{
  cheats->Load("game.rom");
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled("plugin.video.test"));
  cheats->OnAddonEvent(ADDON::AddonEvents::MetadataChanged(DATABASE));
  cheats->OnAddonEvent(ADDON::AddonEvents::AutoUpdateStateChanged(DATABASE));
  cheats->RunJobs();
  EXPECT_EQ(cheats->lookups, 0);
}

TEST_F(TestGameClientCheats, UnsupportedClientsNeverSearchPacks)
{
  supportsCheats = false;
  cheats->database = State::AVAILABLE;
  cheats->Load("game.rom");
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  cheats->RunJobs();
  EXPECT_EQ(cheats->lookups, 0);
  EXPECT_FALSE(cheats->CanOfferCheats());
  EXPECT_FALSE(cheats->GetInstallTask());
}

TEST_F(TestGameClientCheats, PendingReloadAndInstallationCannotReachReplacementGame)
{
  cheats->Load("old.rom");
  auto install = cheats->GetInstallTask();
  cheats->Clear();
  cheats->Load("replacement.rom");
  EXPECT_EQ(install(), Result::FAILED);
  EXPECT_EQ(cheats->installs, 0);
  cheats->database = State::AVAILABLE;
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  ASSERT_FALSE(cheats->jobs.empty());
  cheats->Clear();
  cheats->database = State::MISSING;
  cheats->Load("another.rom");
  cheats->RunJobs();
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_EQ(cheats->lookups, 0);
}

TEST_F(TestGameClientCheats, ClosingDuringLookupDiscardsTheResult)
{
  cheats->Load("game.rom");
  cheats->database = State::AVAILABLE;
  cheats->onLookup = [this] { cheats->Clear(); };
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  cheats->RunJobs();
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_TRUE(applied.empty());
}

TEST_F(TestGameClientCheats, PendingJobsDoNotKeepTheClientAlive)
{
  cheats->Load("game.rom");
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  auto jobs = std::move(cheats->jobs);
  auto install = cheats->GetInstallTask();
  std::weak_ptr<CGameClient> lifetime = client;
  cheats->Clear();
  client.get()->*GetMember(Playing{}) = false;
  client.reset();
  EXPECT_TRUE(lifetime.expired());
  for (auto& job : jobs)
    job();
  EXPECT_EQ(install(), Result::FAILED);
}

TEST_F(TestGameClientCheats, CallbackDoesNotWaitForTheClientLock)
{
  cheats->Load("game.rom");
  std::unique_lock clientLock(ClientAccess());
  auto callback = std::async(std::launch::async, [this]
                             { cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE)); });
  callback.get();
  cheats->Clear();
  clientLock.unlock();
  cheats->RunJobs();
  EXPECT_FALSE(cheats->HasCheats());
}

TEST_F(TestGameClientCheats, SuccessfulEnableLoadsCheats)
{
  cheats->database = State::DISABLED;
  cheats->Load("game.rom");
  EXPECT_EQ(cheats->GetInstallTask()(), Result::CHEATS_FOUND);
  EXPECT_EQ(cheats->enables, 1);
  EXPECT_EQ(cheats->installs, 0);
}

TEST_F(TestGameClientCheats, AlreadyAvailableDatabaseDoesNotReloadAnUnchangedPack)
{
  cheats->database = State::AVAILABLE;
  cheats->Load("game.rom");
  cheats->SetEnabled(0, true, cheats->GetCheats()[0]);
  const int previousResets = resets;
  EXPECT_EQ(cheats->GetInstallTask()(), Result::CHEATS_FOUND);
  cheats->RunJobs();
  EXPECT_EQ(cheats->installs, 0);
  EXPECT_EQ(cheats->enables, 0);
  EXPECT_EQ(cheats->lookups, 1);
  EXPECT_EQ(resets, previousResets);
  EXPECT_TRUE(cheats->GetCheats()[0].enabled);
}

TEST_F(TestGameClientCheats, ReplacementDuringInstallationDiscardsTheOldLookup)
{
  cheats->Load("old.rom");
  cheats->onInstall = [this]
  {
    cheats->Clear();
    cheats->Load("replacement.rom");
  };
  EXPECT_EQ(cheats->GetInstallTask()(), Result::FAILED);
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_EQ(cheats->lookups, 0);
}

TEST_F(TestGameClientCheats, ReplacementDuringLookupDiscardsTheOldResult)
{
  cheats->Load("old.rom");
  cheats->database = State::AVAILABLE;
  cheats->onLookup = [this]
  {
    cheats->onLookup = {};
    cheats->Clear();
    cheats->database = State::MISSING;
    cheats->Load("replacement.rom");
  };
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  cheats->RunJobs();
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_TRUE(applied.empty());
}

TEST_F(TestGameClientCheats, DifferentSourceDoesNotInheritEnabledChoices)
{
  cheats->database = State::AVAILABLE;
  cheats->Load("game.rom");
  cheats->SetEnabled(0, true, cheats->GetCheats()[0]);
  cheats->sources[0].id = "resource.games.other";
  cheats->OnAddonEvent(ADDON::AddonEvents::Disabled(DATABASE));
  cheats->RunJobs();
  ASSERT_EQ(cheats->GetCheats().size(), 2U);
  EXPECT_FALSE(cheats->GetCheats()[0].enabled);
  EXPECT_TRUE(applied.empty());
}

TEST_F(TestGameClientCheats, RepositoryEventsInvalidateAvailabilityWithoutLoadingPacks)
{
  cheats->Load("game.rom");
  EXPECT_TRUE(cheats->CanInstallCheats());
  EXPECT_TRUE(cheats->CanInstallCheats());
  EXPECT_EQ(cheats->availabilityChecks, 1);
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled("repository.test"));
  EXPECT_TRUE(cheats->CanInstallCheats());
  EXPECT_EQ(cheats->availabilityChecks, 2);
  cheats->RunJobs();
  EXPECT_EQ(cheats->lookups, 0);
}

TEST_F(TestGameClientCheats, ExternalReloadQueuesDialogRefreshOnGuiThread)
{
  cheats->Load("game.rom");
  const int messages[] = {GUI_MSG_UPDATE, 0};
  gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages);
  cheats->database = State::AVAILABLE;
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  EXPECT_EQ(gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages), 0);
  cheats->RunJobs();
  EXPECT_EQ(gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages), 1);
}

TEST_F(TestGameClientCheats, ClosingUnsubscribesWhileCallbackIsStillScheduling)
{
  cheats->Load("game.rom");
  std::promise<void> callbackEntered;
  std::promise<void> finishCallback;
  auto finish = finishCallback.get_future();
  cheats->onSubmit = [&]
  {
    callbackEntered.set_value();
    finish.wait();
  };
  auto& events = CServiceBroker::GetAddonMgr().*GetMember(Events{});
  events.Publish(ADDON::AddonEvents::Enabled("resource.games.other"));
  callbackEntered.get_future().wait();

  std::promise<void> clientLocked;
  auto close = std::async(std::launch::async,
                          [&]
                          {
                            std::unique_lock clientLock(ClientAccess());
                            clientLocked.set_value();
                            cheats->Clear();
                          });
  clientLocked.get_future().wait();
  finishCallback.set_value();
  close.get();
  cheats->RunJobs();
  EXPECT_FALSE(cheats->HasCheats());
}

TEST_F(TestGameClientCheats, DuplicateCheatsKeepTheirIndividualEnabledStates)
{
  WritePack("cheats = 2\ncheat0_desc = Lives\ncheat0_code = AAA\n"
            "cheat1_desc = Lives\ncheat1_code = AAA\n");
  cheats->database = State::AVAILABLE;
  cheats->Load("game.rom");
  cheats->SetEnabled(1, true, cheats->GetCheats()[1]);
  cheats->sources[0].revision.reset();
  cheats->OnAddonEvent(ADDON::AddonEvents::ReInstalled(DATABASE));
  cheats->RunJobs();
  ASSERT_EQ(cheats->GetCheats().size(), 2U);
  EXPECT_FALSE(cheats->GetCheats()[0].enabled);
  EXPECT_TRUE(cheats->GetCheats()[1].enabled);
}

TEST_F(TestGameClientCheats, RemovedResourceIsRecognizedFromPreviousSources)
{
  cheats->database = State::AVAILABLE;
  cheats->sources[0].id = "resource.games.temporary";
  cheats->Load("game.rom");
  cheats->SetEnabled(0, true, cheats->GetCheats()[0]);
  cheats->sources.clear();
  cheats->OnAddonEvent(ADDON::AddonEvents::UnInstalled("resource.games.temporary"));
  cheats->RunJobs();
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_TRUE(applied.empty());
}

TEST_F(TestGameClientCheats, GuiVisibilityDoesNotRunQueuedPackLookups)
{
  cheats->Load("game.rom");
  cheats->database = State::AVAILABLE;
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  for (int i = 0; i < 10; ++i)
    cheats->CanOfferCheats();
  EXPECT_EQ(cheats->lookups, 0);
  cheats->RunJobs();
  EXPECT_TRUE(cheats->CanOfferCheats());
  EXPECT_EQ(cheats->lookups, 1);
}

TEST_F(TestGameClientCheats, InstallationSendsOneRefreshIncludingWhenItFails)
{
  for (bool success : {false, true})
  {
    SCOPED_TRACE(success);
    cheats->installSucceeds = success;
    cheats->Load("game.rom");
    const int messages[] = {GUI_MSG_UPDATE, 0};
    gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages);
    cheats->GetInstallTask()();
    cheats->RunJobs();
    EXPECT_EQ(gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages), 1);
  }
}

TEST_F(TestGameClientCheats, RemovedDatabaseDuringLookupDoesNotReportAnEmptyPack)
{
  WritePack("cheats = 0\n");
  cheats->Load("game.rom");
  cheats->onLookup = [this]
  {
    cheats->database = State::MISSING;
    cheats->OnAddonEvent(ADDON::AddonEvents::UnInstalled(DATABASE));
  };
  EXPECT_EQ(cheats->GetInstallTask()(), Result::FAILED);
  cheats->RunJobs();
  EXPECT_TRUE(cheats->CanInstallCheats());
}

TEST_F(TestGameClientCheats, StaleDialogCannotEnableADifferentCheatAfterReload)
{
  cheats->database = State::AVAILABLE;
  cheats->Load("game.rom");
  const auto displayed = cheats->GetCheats();
  WritePack("cheats = 2\ncheat0_desc = Health\ncheat0_code = BBB\n"
            "cheat1_desc = New cheat\ncheat1_code = CCC\n");
  cheats->sources[0].revision.reset();
  cheats->OnAddonEvent(ADDON::AddonEvents::ReInstalled(DATABASE));
  cheats->RunJobs();
  EXPECT_FALSE(cheats->SetEnabled(0, true, displayed[0]));
  EXPECT_FALSE(cheats->GetCheats()[0].enabled);
  EXPECT_TRUE(applied.empty());
}
