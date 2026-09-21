/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "ServiceManager.h"
#include "URL.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "addons/RepositoryUpdater.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "application/Application.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheats/GameClientCheats.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "jobs/JobManager.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <algorithm>
#include <deque>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
constexpr auto DATABASE = "resource.games.cheats.libretro";

void ExpectLocalPathEqual(const std::string& actual, const std::filesystem::path& expected)
{
  EXPECT_EQ(std::filesystem::path(actual), expected);
}

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

struct RepositoryUpdater
{
  using Type = std::unique_ptr<ADDON::CRepositoryUpdater> CServiceManager::*;
  friend Type GetMember(RepositoryUpdater);
};
template struct MemberAccess<RepositoryUpdater, &CServiceManager::m_repositoryUpdater>;
struct RepositoryEvents
{
  using Type =
      CEventSource<ADDON::CRepositoryUpdater::RepositoryUpdated> ADDON::CRepositoryUpdater::*;
  friend Type GetMember(RepositoryEvents);
};
template struct MemberAccess<RepositoryEvents, &ADDON::CRepositoryUpdater::m_events>;

using RepositoryUpdate = ADDON::CRepositoryUpdater::RepositoryUpdated;
struct RepositorySubscriptions
{
  using Type = std::vector<std::shared_ptr<detail::ISubscription<RepositoryUpdate>>>
      CEventStream<RepositoryUpdate>::*;
  friend Type GetMember(RepositorySubscriptions);
};
template struct MemberAccess<RepositorySubscriptions,
                             &CEventStream<RepositoryUpdate>::m_subscriptions>;
using RepositorySubscription = detail::CSubscription<RepositoryUpdate, CGameClientCheats>;
struct RepositoryHandler
{
  using Type = std::function<void(const RepositoryUpdate&)> RepositorySubscription::*;
  friend Type GetMember(RepositoryHandler);
};
template struct MemberAccess<RepositoryHandler, &RepositorySubscription::m_eventHandler>;

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
  bool installable{true};
  bool installSucceeds{true};
  bool enableSucceeds{true};
  int installs{0};
  int enables{0};
  int lookups{0};
  int discoveries{0};
  bool realDiscovery{false};
  std::string selectionDirectory;
  mutable int availabilityChecks{0};
  mutable int databaseChecks{0};
  std::vector<Source> sources;
  std::deque<std::function<void()>> jobs;
  std::function<void()> onInstall;
  std::function<void()> onLookup;
  std::function<void()> onDiscovery;
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
  DatabaseState GetDatabaseState() const override
  {
    ++databaseChecks;
    return database;
  }
  bool IsDatabaseInstallable() const override
  {
    ++availabilityChecks;
    return installable;
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
  std::vector<PackCandidate> FindCandidates(const Source& source,
                                            const std::string& fileName) override
  {
    ++discoveries;
    if (realDiscovery)
    {
      auto candidates = CGameClientCheats::FindCandidates(source, fileName);
      if (const auto callback = onDiscovery)
        callback();
      return candidates;
    }
    return {{source.id + source.path, source.id, source.id, source.path}};
  }
  std::string GetSelectionPath(const std::string& gamePath) const override
  {
    return URIUtils::AddFileToFolder(
        selectionDirectory, URIUtils::GetFileName(CGameClientCheats::GetSelectionPath(gamePath)));
  }
  CCheatPack ReadPack(const std::string& path) override
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
    g_application.m_ServiceManager.get()->*GetMember(RepositoryUpdater{}) =
        std::make_unique<ADDON::CRepositoryUpdater>(CServiceBroker::GetAddonMgr());
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
    directory = XBMC_TEMPFILEPATH(file) + "_packs";
    std::filesystem::create_directories(directory);
    cheats->selectionDirectory = directory + "/choices";
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
    std::filesystem::remove_all(directory);
    (g_application.m_ServiceManager.get()->*GetMember(RepositoryUpdater{})).reset();
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::UnregisterJobManager();
  }

  auto& RepositoryEventSource()
  {
    return CServiceBroker::GetRepositoryUpdater().*GetMember(RepositoryEvents{});
  }

  void UpdateRepository()
  {
    std::promise<void> dispatched;
    auto& events = RepositoryEventSource();
    events.Subscribe(&dispatched, [&dispatched](const auto&) { dispatched.set_value(); });
    events.Publish(ADDON::CRepositoryUpdater::RepositoryUpdated{});
    dispatched.get_future().wait();
    events.Unsubscribe(&dispatched);
  }

  void WritePack(const std::string& contents)
  {
    XFILE::CFile output;
    ASSERT_TRUE(output.OpenForWrite(XBMC_TEMPFILEPATH(file), true));
    ASSERT_EQ(output.Write(contents.data(), contents.size()), contents.size());
  }

  void AddPack(const std::string& source,
               const std::string& system,
               const std::string& filename = "game.cht")
  {
    const auto folder = std::filesystem::path(directory) / source / system;
    std::filesystem::create_directories(folder);
    ASSERT_TRUE(XFILE::CFile::Copy(XBMC_TEMPFILEPATH(file), (folder / filename).string()));
  }

  void Discover(std::vector<TestCheats::Source> sources)
  {
    cheats->realDiscovery = true;
    cheats->database = State::AVAILABLE;
    cheats->sources = std::move(sources);
    cheats->Load("game.a26");
  }

  std::shared_ptr<const ADDON::CAddonInfo> ResourceInfo(const std::string& id,
                                                        const std::string& name)
  {
    ADDON::CAddonInfoBuilderFromDB builder;
    builder.SetId(id);
    builder.SetName(name);
    return builder.get();
  }

  void Changed()
  {
    for (auto& source : cheats->sources)
      source.revision = source.revision ? nullptr : client->AddonInfo();
    cheats->OnAddonEvent(ADDON::AddonEvents::ReInstalled(DATABASE));
    cheats->RunJobs();
  }

  bool Select(size_t index)
  {
    const auto state = cheats->GetPacks();
    auto task = cheats->GetSelectionTask(state);
    return task && index < state.candidates.size() && task(state.candidates[index].id);
  }

  std::string directory;
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
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_TRUE(cheats->GetCheats().empty());
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
  EXPECT_FALSE(cheats->SupportsCheats());
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_FALSE(cheats->GetPacks().HasMatch());
  EXPECT_FALSE(cheats->GetInstallTask());
}

TEST_F(TestGameClientCheats, InstallableDatabaseDoesNotCountAsAMatchingPack)
{
  cheats->Load("game.rom");
  EXPECT_TRUE(cheats->SupportsCheats());
  EXPECT_TRUE(cheats->CanInstallCheats());
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_FALSE(cheats->GetPacks().HasMatch());
  EXPECT_EQ(cheats->lookups, 0);
  EXPECT_EQ(cheats->installs, 0);
}

TEST_F(TestGameClientCheats, SupportedClientWithNoMatchStillExposesCheatCapability)
{
  Discover({{DATABASE, directory, {}}});
  const auto packs = cheats->GetPacks();
  EXPECT_TRUE(cheats->SupportsCheats());
  EXPECT_FALSE(cheats->CanInstallCheats());
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_FALSE(packs.HasMatch());
  EXPECT_FALSE(packs.NeedsSelection());
  EXPECT_TRUE(packs.selected.empty());
  cheats->Clear();
  EXPECT_FALSE(cheats->SupportsCheats());
  EXPECT_TRUE(cheats->GetPacks().fileName.empty());
}

TEST_F(TestGameClientCheats, ExpectedFilenameUsesTheGamePathAndSurvivesAnEmptyLookup)
{
  cheats->realDiscovery = true;
  cheats->database = State::AVAILABLE;
  cheats->sources = {{DATABASE, directory, client->AddonInfo()}};
  const std::string gamePath = "/Users/garrett/Library/Application Support/Kodi/userdata/"
                               "addon_data/plugin.program.iagl/game_cache/Nintendo Game Boy/"
                               "Frogger (USA).gb";
  cheats->Load(gamePath);
  EXPECT_EQ(cheats->GetPacks().fileName, "Frogger (USA).cht");
  EXPECT_FALSE(cheats->GetPacks().HasMatch());
  AddPack("", "Game Boy", "Frogger (USA).cht");
  Changed();
  EXPECT_EQ(cheats->GetPacks().fileName, "Frogger (USA).cht");
  EXPECT_TRUE(cheats->GetPacks().HasMatch());
  cheats->Load("/games/another.gb");
  EXPECT_EQ(cheats->GetPacks().fileName, "another.cht");
  EXPECT_FALSE(cheats->GetPacks().HasMatch());
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

TEST_F(TestGameClientCheats, IgnoredEventDuringDiscoveryDoesNotDiscardThePack)
{
  AddPack("addon", "System");
  cheats->onDiscovery = [this]
  { cheats->OnAddonEvent(ADDON::AddonEvents::MetadataChanged("plugin.video.test")); };
  Discover({{DATABASE, directory + "/addon", {}}});
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_EQ(cheats->GetCheats().size(), 2U);
  EXPECT_TRUE(cheats->jobs.empty());
  EXPECT_EQ(cheats->discoveries, 1);
}

TEST_F(TestGameClientCheats, UnrelatedStateEventsDuringDiscoveryDoNotDiscardOrReloadThePack)
{
  AddPack("addon", "System");
  for (const bool enabled : {true, false})
  {
    SCOPED_TRACE(enabled);
    const int discoveries = cheats->discoveries;
    cheats->onDiscovery = [this, enabled]
    {
      if (enabled)
        cheats->OnAddonEvent(ADDON::AddonEvents::Enabled("plugin.video.test"));
      else
        cheats->OnAddonEvent(ADDON::AddonEvents::Disabled("plugin.video.test"));
    };
    Discover({{DATABASE, directory + "/addon", {}}});
    EXPECT_TRUE(cheats->HasCheats());
    EXPECT_EQ(cheats->GetCheats().size(), 2U);
    cheats->RunJobs();
    EXPECT_EQ(cheats->discoveries, discoveries + 1);
  }
}

TEST_F(TestGameClientCheats, RemovedSourceDuringInitialDiscoveryRejectsStaleCandidates)
{
  AddPack("old", "System");
  AddPack("new", "System");
  cheats->onDiscovery = [this]
  {
    cheats->onDiscovery = {};
    cheats->sources = {{DATABASE, directory + "/new", client->AddonInfo()}};
    cheats->OnAddonEvent(ADDON::AddonEvents::UnInstalled("resource.games.temporary"));
  };
  Discover({{"resource.games.temporary", directory + "/old", {}}});
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_TRUE(cheats->GetPacks().candidates.empty());
  cheats->RunJobs();
  ASSERT_EQ(cheats->GetPacks().candidates.size(), 1U);
  ExpectLocalPathEqual(cheats->GetPacks().candidates.front().path,
                       std::filesystem::path(directory) / "new" / "System" / "game.cht");
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_EQ(cheats->discoveries, 2);
}

TEST_F(TestGameClientCheats, NewResourceDuringDiscoveryRejectsIncompleteCandidates)
{
  AddPack("first", "System");
  AddPack("second", "System");
  cheats->onDiscovery = [this]
  {
    cheats->onDiscovery = {};
    cheats->sources.push_back({"resource.games.other", directory + "/second", client->AddonInfo()});
    cheats->OnAddonEvent(ADDON::AddonEvents::Enabled("resource.games.other"));
  };
  Discover({{DATABASE, directory + "/first", {}}});
  EXPECT_FALSE(cheats->HasCheats());
  cheats->RunJobs();
  EXPECT_EQ(cheats->GetPacks().candidates.size(), 2U);
  EXPECT_TRUE(cheats->GetPacks().NeedsSelection());
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_EQ(cheats->discoveries, 3);
}

TEST_F(TestGameClientCheats, RepositoryUpdateInvalidatesUnavailableDatabaseAndRefreshesDialog)
{
  cheats->installable = false;
  cheats->Load("game.rom");
  EXPECT_FALSE(cheats->CanInstallCheats());
  EXPECT_FALSE(cheats->CanInstallCheats());
  EXPECT_EQ(cheats->availabilityChecks, 1);
  const int messages[] = {GUI_MSG_UPDATE, 0};
  gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages);

  cheats->installable = true;
  UpdateRepository();
  EXPECT_TRUE(cheats->CanInstallCheats());
  EXPECT_TRUE(cheats->CanInstallCheats());
  EXPECT_EQ(cheats->availabilityChecks, 2);
  EXPECT_TRUE(cheats->jobs.empty());
  EXPECT_EQ(gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages), 1);
}

TEST_F(TestGameClientCheats, RepositoryUpdateDoesNotDiscardOrReloadDiscovery)
{
  AddPack("addon", "System");
  cheats->onDiscovery = [this] { UpdateRepository(); };
  Discover({{DATABASE, directory + "/addon", {}}});
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_FALSE(cheats->CanInstallCheats());
  const int checks = cheats->databaseChecks;
  EXPECT_FALSE(cheats->CanInstallCheats());
  EXPECT_EQ(cheats->databaseChecks, checks);
  UpdateRepository();
  EXPECT_FALSE(cheats->CanInstallCheats());
  EXPECT_EQ(cheats->databaseChecks, checks + 1);
  EXPECT_TRUE(cheats->jobs.empty());
  cheats->RunJobs();
  EXPECT_EQ(cheats->discoveries, 1);
}

TEST_F(TestGameClientCheats, RepositoryCallbackDoesNotWaitForTheClientLock)
{
  cheats->Load("game.rom");
  std::unique_lock clientLock(ClientAccess());
  UpdateRepository();
  cheats->Clear();
  EXPECT_FALSE(cheats->HasCheats());
}

TEST_F(TestGameClientCheats, ClosingUnsubscribesWhileRepositoryCallbackIsRunning)
{
  cheats->Load("game.rom");
  auto& events = RepositoryEventSource();
  auto& subscriptions = events.*GetMember(RepositorySubscriptions{});
  ASSERT_EQ(subscriptions.size(), 1U);
  auto subscription = std::dynamic_pointer_cast<RepositorySubscription>(subscriptions.front());
  ASSERT_NE(subscription, nullptr);
  auto& handler = subscription.get()->*GetMember(RepositoryHandler{});
  std::promise<void> callbackEntered;
  std::promise<void> finishCallback;
  auto finish = finishCallback.get_future();
  handler = [&, callback = std::move(handler)](const auto& event)
  {
    callbackEntered.set_value();
    finish.wait();
    callback(event);
  };
  events.Publish(RepositoryUpdate{});
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

  EXPECT_TRUE(cheats->CanInstallCheats());
  const int checks = cheats->availabilityChecks;
  const int messages[] = {GUI_MSG_UPDATE, 0};
  gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages);
  UpdateRepository();
  EXPECT_TRUE(cheats->CanInstallCheats());
  EXPECT_EQ(cheats->availabilityChecks, checks);
  EXPECT_EQ(gui.GetWindowManager().RemoveThreadMessageByMessageIds(messages), 0);
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

TEST_F(TestGameClientCheats, GuiInfoQueriesDoNotRunQueuedPackLookups)
{
  cheats->Load("game.rom");
  cheats->database = State::AVAILABLE;
  cheats->OnAddonEvent(ADDON::AddonEvents::Enabled(DATABASE));
  for (int i = 0; i < 10; ++i)
  {
    EXPECT_TRUE(cheats->SupportsCheats());
    EXPECT_FALSE(cheats->HasCheats());
  }
  EXPECT_EQ(cheats->lookups, 0);
  EXPECT_EQ(cheats->availabilityChecks, 0);
  cheats->RunJobs();
  EXPECT_TRUE(cheats->SupportsCheats());
  EXPECT_TRUE(cheats->HasCheats());
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

TEST_F(TestGameClientCheats, SystemLabelsKeepRawDirectoryAndArchiveIdentities)
{
  constexpr auto system = "Nintendo - Game Boy";
  AddPack("addon", system);
  const auto info = ResourceInfo(DATABASE, "Libretro Cheats");
  Discover({{DATABASE, directory + "/addon", info}});
  const auto folders = cheats->GetPacks();
  ASSERT_EQ(folders.candidates.size(), 1U);
  EXPECT_EQ(folders.candidates[0].name, system);
  EXPECT_EQ(folders.candidates[0].source, "Libretro Cheats");
  EXPECT_EQ(folders.candidates[0].id,
            std::string(DATABASE) + "/Nintendo%20-%20Game%20Boy%2f/game.cht");

  std::filesystem::remove_all(directory + "/addon/" + system);
  const auto archive = XBMC_REF_FILE_PATH("xbmc/games/addons/cheats/test/game.cht.zip");
  ASSERT_TRUE(XFILE::CFile::Copy(archive, directory + "/addon/" + system + ".zip"));
  Discover({{DATABASE, directory + "/addon", info}});
  const auto archives = cheats->GetPacks();
  ASSERT_EQ(archives.candidates.size(), 1U);
  EXPECT_EQ(archives.candidates[0].name, system);
  EXPECT_EQ(archives.candidates[0].source, "Libretro Cheats");
  EXPECT_EQ(archives.candidates[0].id,
            std::string(DATABASE) + "/Nintendo%20-%20Game%20Boy.zip/game.cht");
  EXPECT_NE(archives.candidates[0].id, folders.candidates[0].id);
}

TEST_F(TestGameClientCheats, MissingAddonNameFallsBackToRedactedIdentifier)
{
  AddPack("addon", "System");
  for (const std::string id : {DATABASE, "https://user:secret@example.com/cheats"})
  {
    for (const bool hasInfo : {false, true})
    {
      SCOPED_TRACE(id);
      SCOPED_TRACE(hasInfo);
      Discover({{id, directory + "/addon", hasInfo ? ResourceInfo(id, "") : nullptr}});
      const auto packs = cheats->GetPacks();
      ASSERT_EQ(packs.candidates.size(), 1U);
      EXPECT_EQ(packs.candidates[0].name, "System");
      EXPECT_EQ(packs.candidates[0].source, CURL::GetRedacted(id));
      EXPECT_NE(packs.candidates[0].source.find("cheats"), std::string::npos);
      EXPECT_EQ(packs.candidates[0].source.find("secret"), std::string::npos);
    }
  }
}

TEST_F(TestGameClientCheats, CustomSystemLabelsUseConfiguredRoot)
{
  AddPack("custom", "Nintendo - Game Boy");
  const auto archive = XBMC_REF_FILE_PATH("xbmc/games/addons/cheats/test/game.cht.zip");
  ASSERT_TRUE(XFILE::CFile::Copy(archive, directory + "/custom/Nintendo - Game Boy Advance.zip"));
  const std::string root = directory + "/custom";
  Discover({{"", root, client->AddonInfo()}});
  const auto packs = cheats->GetPacks();
  ASSERT_EQ(packs.candidates.size(), 2U);
  EXPECT_EQ(packs.candidates[0].name, "Nintendo - Game Boy Advance");
  EXPECT_EQ(packs.candidates[1].name, "Nintendo - Game Boy");
  for (const auto& candidate : packs.candidates)
    ExpectLocalPathEqual(candidate.source, std::filesystem::path(directory) / "custom");
  EXPECT_EQ(packs.candidates[0].id,
            CURL::Encode(root) + "/Nintendo%20-%20Game%20Boy%20Advance.zip/game.cht");
  EXPECT_EQ(packs.candidates[1].id, CURL::Encode(root) + "/Nintendo%20-%20Game%20Boy%2f/game.cht");
}

TEST_F(TestGameClientCheats, CustomVfsRootRedactsCredentials)
{
  CURL root = URIUtils::CreateArchivePath(
      "zip", CURL(XBMC_REF_FILE_PATH("xbmc/games/addons/cheats/test/game.cht.zip")));
  root.SetUserName("user");
  root.SetPassword("secret");
  Discover({{"", root.Get(), {}}});
  const auto packs = cheats->GetPacks();
  ASSERT_EQ(packs.candidates.size(), 1U);
  EXPECT_EQ(packs.candidates[0].source, root.GetRedacted());
  EXPECT_EQ(packs.candidates[0].source.find("secret"), std::string::npos);
  EXPECT_EQ(packs.candidates[0].id, CURL::Encode(root.Get()) + "//game.cht");
}

TEST_F(TestGameClientCheats, DuplicateVisibleRowsDisambiguateOnlyCollisionsAndKeepSelection)
{
  constexpr auto other = "resource.games.other";
  AddPack("first", "System A");
  AddPack("first", "System B");
  AddPack("second", "System A");
  Discover({{DATABASE, directory + "/first", ResourceInfo(DATABASE, "Some Cheats")},
            {other, directory + "/second", ResourceInfo(other, "Some Cheats")}});
  const auto packs = cheats->GetPacks();
  ASSERT_EQ(packs.candidates.size(), 3U);
  EXPECT_EQ(packs.candidates[0].source, "Some Cheats (resource.games.cheats.libretro)");
  EXPECT_EQ(packs.candidates[1].source, "Some Cheats");
  EXPECT_EQ(packs.candidates[2].source, "Some Cheats (resource.games.other)");
  EXPECT_EQ(packs.candidates[0].name, packs.candidates[2].name);
  ASSERT_TRUE(Select(2));
  const auto selected = cheats->GetPacks().selected;

  cheats->sources[1].revision = ResourceInfo(other, "Other Cheats");
  cheats->OnAddonEvent(ADDON::AddonEvents::ReInstalled(other));
  cheats->RunJobs();
  const auto renamed = cheats->GetPacks();
  ASSERT_EQ(renamed.candidates.size(), 3U);
  EXPECT_EQ(renamed.candidates[0].source, "Some Cheats");
  EXPECT_EQ(renamed.candidates[2].source, "Other Cheats");
  for (size_t i = 0; i < packs.candidates.size(); ++i)
    EXPECT_EQ(renamed.candidates[i].id, packs.candidates[i].id);
  EXPECT_EQ(renamed.selected, selected);
  cheats->Load("game.a26");
  EXPECT_EQ(cheats->GetPacks().selected, selected);
}

TEST_F(TestGameClientCheats, SameSourceDirectoryAndArchiveRowsRemainDistinct)
{
  AddPack("custom", "System");
  const auto archive = XBMC_REF_FILE_PATH("xbmc/games/addons/cheats/test/game.cht.zip");
  ASSERT_TRUE(XFILE::CFile::Copy(archive, directory + "/custom/System.zip"));
  const std::string root = directory + "/custom";
  Discover({{"", root, {}}});
  const auto packs = cheats->GetPacks();
  ASSERT_EQ(packs.candidates.size(), 2U);
  EXPECT_EQ(packs.candidates[0].name, "System");
  EXPECT_EQ(packs.candidates[1].name, "System");
  const auto sourceLabel = (std::filesystem::path(directory) / "custom").string();
  EXPECT_EQ(packs.candidates[0].source, sourceLabel + " (1)");
  EXPECT_EQ(packs.candidates[1].source, sourceLabel + " (2)");
  EXPECT_NE(packs.candidates[0].id, packs.candidates[1].id);
}

TEST_F(TestGameClientCheats, MultipleInstalledPacksWithOnlyOneMatchingFilenameLoadAutomatically)
{
  AddPack("first", "System A");
  AddPack("second", "System B", "different.cht");
  Discover(
      {{DATABASE, directory + "/first", {}}, {"resource.games.other", directory + "/second", {}}});
  EXPECT_TRUE(cheats->SupportsCheats());
  EXPECT_EQ(cheats->GetPacks().candidates.size(), 1U);
  EXPECT_TRUE(cheats->GetPacks().HasMatch());
  EXPECT_FALSE(cheats->GetSelectionTask(cheats->GetPacks()));
  EXPECT_TRUE(cheats->HasCheats());
}

TEST_F(TestGameClientCheats, MatchingSystemFoldersRequireAnExplicitChoice)
{
  AddPack("first", "System A");
  AddPack("first", "System B");
  Discover({{DATABASE, directory + "/first", {}}});
  const auto packs = cheats->GetPacks();
  ASSERT_EQ(packs.candidates.size(), 2U);
  EXPECT_TRUE(cheats->SupportsCheats());
  EXPECT_NE(packs.candidates[0].id, packs.candidates[1].id);
  EXPECT_NE(packs.candidates[0].name, packs.candidates[1].name);
  EXPECT_TRUE(packs.NeedsSelection());
  EXPECT_TRUE(packs.HasMatch());
  EXPECT_TRUE(packs.selected.empty());
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_TRUE(cheats->GetCheats().empty());
  EXPECT_EQ(cheats->GetInstallTask()(), Result::CHOOSE_PACK);
  EXPECT_TRUE(Select(1));
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_FALSE(cheats->GetPacks().NeedsSelection());
  EXPECT_TRUE(cheats->GetPacks().HasMatch());
  EXPECT_EQ(cheats->GetPacks().candidates.size(), 2U);
  EXPECT_EQ(cheats->GetPacks().selected, packs.candidates[1].id);
}

TEST_F(TestGameClientCheats, MatchesAcrossAddonsRequireSelectionRegardlessOfOrder)
{
  AddPack("first", "System");
  AddPack("second", "System");
  Discover(
      {{DATABASE, directory + "/first", {}}, {"resource.games.other", directory + "/second", {}}});
  ASSERT_EQ(cheats->GetPacks().candidates.size(), 2U);
  EXPECT_TRUE(cheats->GetPacks().NeedsSelection());
  EXPECT_TRUE(Select(1));
  const auto selected = cheats->GetPacks().selected;
  std::reverse(cheats->sources.begin(), cheats->sources.end());
  Changed();
  EXPECT_EQ(cheats->GetPacks().selected, selected);
}

TEST_F(TestGameClientCheats, CustomFolderAndDirectFileKeepPrecedence)
{
  AddPack("custom", "System A");
  AddPack("custom", "System B");
  AddPack("addon", "System C");
  Discover({{"", directory + "/custom", {}}, {DATABASE, directory + "/addon", {}}});
  ASSERT_EQ(cheats->GetPacks().candidates.size(), 2U);
  for (const auto& candidate : cheats->GetPacks().candidates)
    EXPECT_NE(candidate.path.find("custom"), std::string::npos);

  AddPack("custom", "");
  Changed();
  ASSERT_EQ(cheats->GetPacks().candidates.size(), 1U);
  EXPECT_TRUE(cheats->HasCheats());
  ExpectLocalPathEqual(cheats->GetPacks().candidates[0].path,
                       std::filesystem::path(directory) / "custom" / "game.cht");
  EXPECT_EQ(cheats->GetPacks().candidates[0].name, "custom");
  ExpectLocalPathEqual(cheats->GetPacks().candidates[0].source,
                       std::filesystem::path(directory) / "custom");

  cheats->sources.erase(cheats->sources.begin());
  Changed();
  ASSERT_EQ(cheats->GetPacks().candidates.size(), 1U);
  EXPECT_NE(cheats->GetPacks().candidates[0].path.find("addon"), std::string::npos);
}

TEST_F(TestGameClientCheats, CustomFolderWithoutMatchFallsBackToAddons)
{
  AddPack("custom", "System A", "different.cht");
  AddPack("addon", "System B");
  Discover({{"", directory + "/custom", {}}, {DATABASE, directory + "/addon", {}}});
  ASSERT_EQ(cheats->GetPacks().candidates.size(), 1U);
  EXPECT_TRUE(cheats->HasCheats());
}

TEST_F(TestGameClientCheats, ChoiceSurvivesReopeningAndANewEmulatorInstance)
{
  AddPack("first", "System A");
  AddPack("first", "System B");
  Discover({{DATABASE, directory + "/first", {}}});
  ASSERT_TRUE(Select(1));
  const auto selected = cheats->GetPacks().selected;
  cheats->Clear();
  cheats->Load("game.a26");
  EXPECT_EQ(cheats->GetPacks().selected, selected);

  cheats->Clear();
  CXBMCTinyXML2 xml;
  ASSERT_TRUE(xml.Parse(
      std::string(R"(<addon id="game.test.another" name="Another emulator" version="1.0.0">
    <extension point="kodi.gameclient" library="test.so"><extensions>rom</extensions></extension>
    <extension point="kodi.addon.metadata"><platform>all</platform></extension>
  </addon>)")));
  auto info = ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
  auto other = std::make_shared<CGameClient>(info);
  *other->GetInstanceInterface()->toAddon = *client->GetInstanceInterface()->toAddon;
  other.get()->*GetMember(Playing{}) = true;
  auto& subsystems = other.get()->*GetMember(Subsystems{});
  auto replacement = std::make_unique<TestCheats>(*other, *other->GetInstanceInterface(),
                                                  other.get()->*GetMember(ClientMutex{}));
  replacement->realDiscovery = true;
  replacement->database = State::AVAILABLE;
  replacement->sources = cheats->sources;
  replacement->selectionDirectory = cheats->selectionDirectory;
  auto* otherCheats = replacement.get();
  subsystems.Cheats = std::move(replacement);
  otherCheats->Load("game.a26");
  EXPECT_EQ(otherCheats->GetPacks().selected, selected);
  otherCheats->Load("elsewhere/game.a26");
  EXPECT_TRUE(otherCheats->GetPacks().NeedsSelection());
  otherCheats->Clear();
  other.get()->*GetMember(Playing{}) = false;
}

TEST_F(TestGameClientCheats, CancelledChooserDoesNotChangeSelectionOrEnabledCheats)
{
  AddPack("first", "System A");
  AddPack("first", "System B");
  Discover({{DATABASE, directory + "/first", {}}});
  {
    auto cancelled = cheats->GetSelectionTask(cheats->GetPacks());
    ASSERT_TRUE(cancelled);
  }
  EXPECT_TRUE(cheats->GetPacks().NeedsSelection());
  EXPECT_TRUE(applied.empty());
  ASSERT_TRUE(Select(0));
  ASSERT_TRUE(cheats->SetEnabled(0, true, cheats->GetCheats()[0]));
  const auto selected = cheats->GetPacks().selected;
  const int previousResets = resets;
  {
    auto cancelled = cheats->GetSelectionTask(cheats->GetPacks());
    ASSERT_TRUE(cancelled);
  }
  EXPECT_EQ(cheats->GetPacks().selected, selected);
  EXPECT_TRUE(cheats->GetCheats()[0].enabled);
  EXPECT_EQ(resets, previousResets);
  EXPECT_EQ(applied, (std::vector<std::string>{"AAA"}));
}

TEST_F(TestGameClientCheats, RemovedSelectionRequiresChoiceThenLoadsSoleRemainingMatch)
{
  for (const auto* system : {"A", "B", "C"})
    AddPack("first", system);
  Discover({{DATABASE, directory + "/first", {}}});
  ASSERT_TRUE(Select(0));
  ASSERT_TRUE(cheats->SetEnabled(0, true, cheats->GetCheats()[0]));
  std::filesystem::remove_all(directory + "/first/A");
  Changed();
  EXPECT_TRUE(cheats->GetPacks().NeedsSelection());
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_TRUE(cheats->GetCheats().empty());
  EXPECT_TRUE(applied.empty());

  std::filesystem::remove_all(directory + "/first/B");
  Changed();
  EXPECT_EQ(cheats->GetPacks().candidates.size(), 1U);
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_FALSE(cheats->GetCheats()[0].enabled);
  EXPECT_FALSE(cheats->GetSelectionTask(cheats->GetPacks()));

  std::filesystem::remove_all(directory + "/first/C");
  Changed();
  EXPECT_TRUE(cheats->GetPacks().candidates.empty());
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_EQ(cheats->GetInstallTask()(), Result::NO_CHEATS);
}

TEST_F(TestGameClientCheats, SwitchingPacksResetsAppliedCheatsWithoutTransferringStates)
{
  AddPack("first", "System A");
  AddPack("first", "System B");
  Discover({{DATABASE, directory + "/first", {}}});
  ASSERT_TRUE(Select(0));
  ASSERT_TRUE(cheats->SetEnabled(0, true, cheats->GetCheats()[0]));
  const auto previous = cheats->GetPacks();
  ASSERT_TRUE(Select(1));
  EXPECT_TRUE(applied.empty());
  EXPECT_FALSE(cheats->GetCheats()[0].enabled);
  EXPECT_FALSE(cheats->SetEnabled(0, true, previous.cheats[0], previous.generation));
  ASSERT_TRUE(cheats->SetEnabled(1, true, cheats->GetCheats()[1]));
  Changed();
  EXPECT_TRUE(cheats->GetCheats()[1].enabled);
  EXPECT_EQ(applied, (std::vector<std::string>{"BBB"}));
}

TEST_F(TestGameClientCheats, SelectionRevalidatesRemovedCandidatesBeforeCommitting)
{
  AddPack("first", "System A");
  AddPack("first", "System B");
  Discover({{DATABASE, directory + "/first", {}}});
  const auto packs = cheats->GetPacks();
  auto select = cheats->GetSelectionTask(packs);
  ASSERT_TRUE(select);
  ASSERT_EQ(packs.candidates.size(), 2U);
  std::filesystem::remove_all(directory + "/first/System B");
  EXPECT_FALSE(select(packs.candidates[1].id));
  EXPECT_EQ(cheats->GetPacks().selected, packs.candidates[0].id);
  EXPECT_EQ(cheats->GetPacks().candidates.size(), 1U);
  cheats->Load("game.a26");
  EXPECT_EQ(cheats->GetPacks().selected, packs.candidates[0].id);
}

TEST_F(TestGameClientCheats, StaleChooserCannotSelectForAReplacementSession)
{
  AddPack("first", "System A");
  AddPack("first", "System B");
  Discover({{DATABASE, directory + "/first", {}}});
  const auto packs = cheats->GetPacks();
  auto select = cheats->GetSelectionTask(packs);
  ASSERT_TRUE(select);
  ASSERT_EQ(packs.candidates.size(), 2U);
  cheats->Load("game.a26");
  EXPECT_FALSE(cheats->GetSelectionTask(packs));
  EXPECT_FALSE(select(packs.candidates[0].id));
  EXPECT_TRUE(cheats->GetPacks().NeedsSelection());
}

TEST_F(TestGameClientCheats, ClosingDuringSelectionDiscardsThePackAndPreference)
{
  AddPack("first", "System A");
  AddPack("first", "System B");
  Discover({{DATABASE, directory + "/first", {}}});
  cheats->onLookup = [this] { cheats->Clear(); };
  EXPECT_FALSE(Select(0));
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_TRUE(applied.empty());
  cheats->onLookup = {};
  cheats->Load("game.a26");
  EXPECT_TRUE(cheats->GetPacks().NeedsSelection());
}

TEST_F(TestGameClientCheats, AddonChangeDuringSelectionDiscardsTheStaleResult)
{
  AddPack("first", "System A");
  AddPack("first", "System B");
  Discover({{DATABASE, directory + "/first", {}}});
  cheats->onLookup = [this]
  {
    cheats->sources.clear();
    cheats->OnAddonEvent(ADDON::AddonEvents::UnInstalled(DATABASE));
  };
  EXPECT_FALSE(Select(0));
  EXPECT_TRUE(applied.empty());
  cheats->RunJobs();
  EXPECT_TRUE(cheats->GetPacks().candidates.empty());
}

TEST_F(TestGameClientCheats, TwoSystemArchivesInOneAddonAreDistinctPacks)
{
  const std::filesystem::path archive =
      XBMC_REF_FILE_PATH("xbmc/games/addons/cheats/test/game.cht.zip");
  std::error_code ec;
  ASSERT_TRUE(std::filesystem::exists(archive, ec)) << archive << ": " << ec.message();
  const auto addon = std::filesystem::path(directory) / "addon";
  std::filesystem::create_directories(addon, ec);
  ASSERT_FALSE(ec) << addon << ": " << ec.message();
  for (const auto* name : {"System A.zip", "System B.zip"})
  {
    const auto destination = addon / name;
    ASSERT_TRUE(std::filesystem::copy_file(archive, destination,
                                           std::filesystem::copy_options::overwrite_existing, ec))
        << archive << " -> " << destination << ": " << ec.message();
  }
  Discover({{DATABASE, addon.string(), {}}});
  const auto packs = cheats->GetPacks();
  ASSERT_EQ(packs.candidates.size(), 2U);
  EXPECT_TRUE(packs.NeedsSelection());
  EXPECT_EQ(packs.candidates[0].name, "System A");
  EXPECT_EQ(packs.candidates[1].name, "System B");
  EXPECT_NE(packs.candidates[0].id, packs.candidates[1].id);
  EXPECT_TRUE(Select(1));
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_EQ(cheats->GetPacks().selected, packs.candidates[1].id);
}

TEST_F(TestGameClientCheats, UnusableCustomFileKeepsFallbackToAddon)
{
  AddPack("addon", "System");
  WritePack("cheats = 1\ncheat0_desc = Memory patch\ncheat0_code = \"\"\n");
  AddPack("custom", "");
  Discover({{"", directory + "/custom", {}}, {DATABASE, directory + "/addon", {}}});
  ASSERT_EQ(cheats->GetPacks().candidates.size(), 1U);
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_NE(cheats->GetPacks().candidates[0].path.find("addon"), std::string::npos);
}

TEST_F(TestGameClientCheats, MatchingPackDoesNotRequireUsableCheatEntries)
{
  for (const std::string contents : {"cheats = 0\n", "cheats = 1\ncheat0_desc = Unusable\n"})
  {
    SCOPED_TRACE(contents);
    WritePack(contents);
    AddPack("addon", "System");
    Discover({{DATABASE, directory + "/addon", {}}});
    const auto packs = cheats->GetPacks();
    ASSERT_EQ(packs.candidates.size(), 1U);
    EXPECT_TRUE(cheats->SupportsCheats());
    EXPECT_TRUE(packs.HasMatch());
    EXPECT_EQ(packs.selected, packs.candidates.front().id);
    EXPECT_FALSE(packs.NeedsSelection());
    EXPECT_TRUE(packs.cheats.empty());
    EXPECT_TRUE(cheats->HasCheats());
  }
}

TEST_F(TestGameClientCheats, EmptyMatchingArchivesRemainAmbiguousUntilSelected)
{
  WritePack("cheats = 0\n");
  AddPack("addon", "A");
  AddPack("addon", "B");
  Discover({{DATABASE, directory + "/addon", {}}});
  EXPECT_TRUE(cheats->GetPacks().NeedsSelection());
  EXPECT_TRUE(cheats->GetPacks().HasMatch());
  EXPECT_EQ(cheats->GetInstallTask()(), Result::CHOOSE_PACK);
  ASSERT_TRUE(Select(0));
  EXPECT_FALSE(cheats->GetPacks().NeedsSelection());
  EXPECT_TRUE(cheats->GetPacks().HasMatch());
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_TRUE(cheats->GetCheats().empty());
  EXPECT_EQ(cheats->GetPacks().candidates.size(), 2U);
  EXPECT_EQ(cheats->GetInstallTask()(), Result::NO_CHEATS);
}

TEST_F(TestGameClientCheats, ReplacementDuringAmbiguousDiscoveryDiscardsCandidates)
{
  AddPack("addon", "A");
  AddPack("addon", "B");
  cheats->onDiscovery = [this]
  {
    cheats->onDiscovery = {};
    cheats->database = State::MISSING;
    cheats->Load("replacement.a26");
  };
  Discover({{DATABASE, directory + "/addon", {}}});
  EXPECT_TRUE(cheats->GetPacks().candidates.empty());
  EXPECT_FALSE(cheats->HasCheats());
  EXPECT_EQ(cheats->lookups, 0);
}

TEST_F(TestGameClientCheats, RememberedCustomSelectionRemainsValidWhenItsCheatsBecomeEmpty)
{
  AddPack("custom", "A");
  AddPack("custom", "B");
  AddPack("addon", "System");
  Discover({{"", directory + "/custom", {}}, {DATABASE, directory + "/addon", {}}});
  ASSERT_TRUE(Select(0));
  const auto selected = cheats->GetPacks().selected;
  WritePack("cheats = 0\n");
  AddPack("custom", "A");
  std::filesystem::remove_all(directory + "/custom/B");
  Changed();
  EXPECT_EQ(cheats->GetPacks().selected, selected);
  EXPECT_EQ(cheats->GetPacks().candidates.size(), 1U);
  EXPECT_TRUE(cheats->GetPacks().HasMatch());
  EXPECT_TRUE(cheats->HasCheats());
  EXPECT_TRUE(cheats->GetCheats().empty());
}
