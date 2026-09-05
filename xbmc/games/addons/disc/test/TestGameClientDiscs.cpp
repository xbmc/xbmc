/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "cores/RetroPlayer/playback/ReversiblePlayback.h"
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "cores/RetroPlayer/savestates/SavestateFlatBuffer.h"
#include "cores/RetroPlayer/streams/IStreamManager.h"
#include "cores/RetroPlayer/streams/memory/DeltaPairMemoryStream.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientCallbacks.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscXML.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <future>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace KODI::GAME;

namespace
{
// Explicit instantiation allows the fixture to access members without production test hooks.
template<typename Tag, typename Tag::Type member>
struct TestMemberAccess
{
  friend typename Tag::Type GetMember(Tag) { return member; }
};

template<typename Tag, typename Object>
decltype(auto) TestMember(Object& object)
{
  return object.*GetMember(Tag{});
}

struct ClientPlaying
{
  using Type = std::atomic_bool CGameClient::*;
  friend Type GetMember(ClientPlaying);
};
template struct TestMemberAccess<ClientPlaying, &CGameClient::m_bIsPlaying>;

struct ClientInitializeGameplay
{
  using Type = bool (CGameClient::*)(const std::string&, RETRO::IStreamManager&, IGameInputCallback*);
  friend Type GetMember(ClientInitializeGameplay);
};
template struct TestMemberAccess<ClientInitializeGameplay, &CGameClient::InitializeGameplay>;

struct ClientInput
{
  using Type = IGameInputCallback* CGameClient::*;
  friend Type GetMember(ClientInput);
};
template struct TestMemberAccess<ClientInput, &CGameClient::m_input>;

struct ClientFrameRun
{
  using Type = std::atomic_bool CGameClient::*;
  friend Type GetMember(ClientFrameRun);
};
template struct TestMemberAccess<ClientFrameRun, &CGameClient::m_hasFrameRun>;

struct ClientSerializeSize
{
  using Type = size_t CGameClient::*;
  friend Type GetMember(ClientSerializeSize);
};
template struct TestMemberAccess<ClientSerializeSize, &CGameClient::m_serializeSize>;

struct ClientFrameRate
{
  using Type = std::atomic<double> CGameClient::*;
  friend Type GetMember(ClientFrameRate);
};
template struct TestMemberAccess<ClientFrameRate, &CGameClient::m_framerate>;

struct ClientGamePath
{
  using Type = std::string CGameClient::*;
  friend Type GetMember(ClientGamePath);
};
template struct TestMemberAccess<ClientGamePath, &CGameClient::m_gamePath>;

struct PlaybackMemory
{
  using Type =
      std::unique_ptr<KODI::RETRO::CDeltaPairMemoryStream> KODI::RETRO::CReversiblePlayback::*;
  friend Type GetMember(PlaybackMemory);
};
template struct TestMemberAccess<PlaybackMemory, &KODI::RETRO::CReversiblePlayback::m_memoryStream>;

struct PlaybackDiscs
{
  using Type = KODI::RETRO::CDiscStateHistory KODI::RETRO::CReversiblePlayback::*;
  friend Type GetMember(PlaybackDiscs);
};
template struct TestMemberAccess<PlaybackDiscs,
                                 &KODI::RETRO::CReversiblePlayback::m_discStateHistory>;

struct PlaybackTotalFrames
{
  using Type = uint64_t KODI::RETRO::CReversiblePlayback::*;
  friend Type GetMember(PlaybackTotalFrames);
};
template struct TestMemberAccess<PlaybackTotalFrames,
                                 &KODI::RETRO::CReversiblePlayback::m_totalFrameCount>;

struct PlaybackRestoreFailed
{
  using Type = bool KODI::RETRO::CReversiblePlayback::*;
  friend Type GetMember(PlaybackRestoreFailed);
};
template struct TestMemberAccess<PlaybackRestoreFailed,
                                 &KODI::RETRO::CReversiblePlayback::m_restoreFailed>;

struct PlaybackStats
{
  using Type = void (KODI::RETRO::CReversiblePlayback::*)();
  friend Type GetMember(PlaybackStats);
};
template struct TestMemberAccess<PlaybackStats,
                                 &KODI::RETRO::CReversiblePlayback::UpdatePlaybackStats>;

struct PlaybackSaveTasks
{
  using Type = std::vector<std::future<void>> KODI::RETRO::CReversiblePlayback::*;
  friend Type GetMember(PlaybackSaveTasks);
};
template struct TestMemberAccess<PlaybackSaveTasks,
                                 &KODI::RETRO::CReversiblePlayback::m_savestateThreads>;

struct DiscCore
{
  std::vector<std::string> slots;
  unsigned int selected{0};
  bool ejected{false};
  unsigned int mutations{0};
  unsigned int addCalls{0};
  unsigned int removeCalls{0};
  unsigned int modelQueries{0};
  bool failReplace{false};
  bool failClose{false};
  uint8_t machineFrame{3};
  uint8_t failedFrame{0};
  bool failAllFrames{false};
  bool failCloseAfterDeserialize{false};
  bool changeDiscOnRun{false};
  std::string rewindFramePath;
  std::string replacementAfterDeserialize;
  unsigned int runFrameCalls{0};
  unsigned int inputPollCalls{0};
  bool pendingInput{false};
  bool polledInput{false};
  std::vector<bool> frameInputs;
  bool compactRemoval{false};
  std::string initialPath;
  unsigned int initialIndex{0};
  bool failInitialImage{false};
  unsigned int deserializeCalls{0};
  std::vector<std::string> deserializedSlots;
  std::string machinePath;
};

class TestStreams : public RETRO::IStreamManager
{
public:
  RETRO::StreamPtr CreateStream(RETRO::StreamType) override { return {}; }
  void CloseStream(RETRO::StreamPtr) override {}
  void SetVideoFps(float) override {}
  RETRO::HwProcedureAddress GetHwProcedureAddress(const char*) override { return nullptr; }
};

class TestInput : public IGameInputCallback
{
public:
  TestInput(DiscCore& core, CCriticalSection& clientAccess)
    : m_core(core), m_clientAccess(clientAccess)
  {
  }

  bool AcceptsInput() const override { return true; }

  void PollInput() override
  {
    // The real event scanner calls back into the client on another thread.
    EXPECT_TRUE(std::async(std::launch::async,
                           [this]
                           {
                             std::unique_lock lock(m_clientAccess, std::try_to_lock);
                             return lock.owns_lock();
                           })
                    .get());
    ++m_core.inputPollCalls;
    m_core.polledInput = std::exchange(m_core.pendingInput, false);
  }

private:
  DiscCore& m_core;
  CCriticalSection& m_clientAccess;
};

DiscCore& Core(const AddonInstance_Game* instance)
{
  return *static_cast<DiscCore*>(instance->toAddon->addonInstance);
}
} // namespace

namespace KODI::GAME
{
class TestGameClientDiscs : public testing::Test
{
protected:
  void SetUp() override
  {
    CXBMCTinyXML2 xml;
    const std::string addonXml = R"(<addon id="game.test.discs" name="Disc test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so">
        <supports_disc_control>true</supports_disc_control>
        <extensions>chd|m3u</extensions>
      </extension>
      <extension point="kodi.addon.metadata"><platform>all</platform></extension>
    </addon>)";
    ASSERT_TRUE(xml.Parse(addonXml));
    const auto info =
        ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
    ASSERT_NE(info, nullptr);
    m_client = std::make_unique<CGameClient>(info);
    auto* callbacks = m_client->GetInstanceInterface()->toAddon;
    callbacks->addonInstance = &m_core;
    callbacks->GetEjectState = [](const AddonInstance_Game* game) { return Core(game).ejected; };
    callbacks->SetEjectState = [](const AddonInstance_Game* game, bool ejected)
    {
      if (!ejected && Core(game).failClose)
        return GAME_ERROR_FAILED;
      Core(game).ejected = ejected;
      ++Core(game).mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->GetImageCount = [](const AddonInstance_Game* game)
    {
      ++Core(game).modelQueries;
      return static_cast<unsigned int>(Core(game).slots.size());
    };
    callbacks->GetImageIndex = [](const AddonInstance_Game* game) { return Core(game).selected; };
    callbacks->SetImageIndex = [](const AddonInstance_Game* game, unsigned int index)
    {
      Core(game).selected = index;
      ++Core(game).mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->AddImageIndex = [](const AddonInstance_Game* game)
    {
      Core(game).slots.emplace_back();
      ++Core(game).addCalls;
      ++Core(game).mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->ReplaceImageIndex =
        [](const AddonInstance_Game* game, unsigned int index, const char* path)
    {
      auto& core = Core(game);
      if (core.failReplace || index >= core.slots.size() || !core.ejected)
        return GAME_ERROR_FAILED;
      core.slots[index] = path ? path : "";
      ++core.mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->RemoveImageIndex = [](const AddonInstance_Game* game, unsigned int index)
    {
      ++Core(game).removeCalls;
      if (index >= Core(game).slots.size())
        return GAME_ERROR_FAILED;
      if (Core(game).compactRemoval)
        Core(game).slots.erase(Core(game).slots.begin() + index);
      else
        Core(game).slots[index].clear();
      ++Core(game).mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->SetInitialImage =
        [](const AddonInstance_Game* game, unsigned int index, const char* path)
    {
      Core(game).initialIndex = index;
      Core(game).initialPath = path ? path : "";
      return Core(game).failInitialImage ? GAME_ERROR_FAILED : GAME_ERROR_NO_ERROR;
    };
    callbacks->GetImagePath = [](const AddonInstance_Game* game, unsigned int index) -> char*
    {
      return index < Core(game).slots.size() && !Core(game).slots[index].empty()
                 ? strdup(Core(game).slots[index].c_str())
                 : nullptr;
    };
    callbacks->GetImageLabel = [](const AddonInstance_Game*, unsigned int) -> char*
    { return nullptr; };
    callbacks->FreeString = [](const AddonInstance_Game*, char* string) { free(string); };
    callbacks->Deserialize = [](const AddonInstance_Game* game, const uint8_t* data, size_t size)
    {
      auto& core = Core(game);
      ++core.deserializeCalls;
      core.deserializedSlots = core.slots;
      if (size < 2 || core.selected != data[0] || core.selected >= core.slots.size())
        return GAME_ERROR_FAILED;
      const std::string path(reinterpret_cast<const char*>(data + 1), size - 1);
      if (core.slots[core.selected] != path)
        return GAME_ERROR_FAILED;
      core.machinePath = path;
      return GAME_ERROR_NO_ERROR;
    };
  }

  void TearDown() override
  {
    m_playback.reset();
    m_playbackEnvironment.reset();
    if (m_client)
      TestMember<ClientPlaying>(*m_client) = false;
    for (auto* file : m_files)
    {
      const std::string path = XBMC_TEMPFILEPATH(file);
      XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(path));
      XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(path));
      EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
    }
  }

  std::string CreateFile(const std::string& extension, const std::string& contents = {})
  {
    auto* file = XBMC_CREATETEMPFILE(extension);
    EXPECT_NE(file, nullptr);
    if (!file)
      return {};
    m_files.push_back(file);
    if (!contents.empty())
      EXPECT_EQ(file->Write(contents.data(), contents.size()),
                static_cast<ssize_t>(contents.size()));
    file->Close();
    return XBMC_TEMPFILEPATH(file);
  }

  bool InitializeGameplay(const std::string& gamePath, bool validTiming = true)
  {
    m_playbackEnvironment = std::make_unique<RETRO::CPlaybackTestEnvironment>();
    auto* callbacks = m_client->GetInstanceInterface()->toAddon;
    callbacks->RequiresGameLoop = [](const AddonInstance_Game*) { return true; };
    callbacks->GetGameTiming = [](const AddonInstance_Game*, game_system_timing* timing)
    {
      timing->fps = 1.0;
      timing->sample_rate = 44100.0;
      return GAME_ERROR_NO_ERROR;
    };
    if (!validTiming)
      callbacks->GetGameTiming = [](const AddonInstance_Game*, game_system_timing*)
      { return GAME_ERROR_FAILED; };
    callbacks->GetRegion = [](const AddonInstance_Game*) { return GAME_REGION_NTSC; };
    callbacks->SerializeSize = [](const AddonInstance_Game*) -> size_t { return 1; };
    callbacks->UnloadGame = [](const AddonInstance_Game*) { return GAME_ERROR_NO_ERROR; };
    callbacks->LoadGame = [](const AddonInstance_Game*, const char*) { return GAME_ERROR_FAILED; };
    callbacks->GetMemory = [](const AddonInstance_Game*, GAME_MEMORY, uint8_t** data, size_t* size)
    {
      *data = nullptr;
      *size = 0;
      return GAME_ERROR_NO_ERROR;
    };
    TestStreams streams;
    return (m_client.get()->*GetMember(ClientInitializeGameplay{}))(gamePath, streams, nullptr);
  }

  void StartPlaying()
  {
    TestMember<ClientPlaying>(*m_client) = true;
    TestMember<ClientFrameRun>(*m_client) = true;
    m_client->Discs().RefreshDiscState();
  }

  RestoreResult Deserialize(const CGameClientDiscModel& target,
                            uint8_t selected,
                            const std::string& path)
  {
    std::vector<uint8_t> data{selected};
    data.insert(data.end(), path.begin(), path.end());
    return m_client->Deserialize(data.data(), data.size(), &target);
  }

  void CreatePlayback(const CGameClientDiscModel* firstFrameDiscs = nullptr,
                      const CGameClientDiscModel* lastFrameDiscs = nullptr)
  {
    TestMember<ClientSerializeSize>(*m_client) = 1;
    TestMember<ClientFrameRate>(*m_client) = 1.0;
    TestMember<ClientGamePath>(*m_client) = m_core.slots.front();
    auto* callbacks = m_client->GetInstanceInterface()->toAddon;
    callbacks->Deserialize = [](const AddonInstance_Game* game, const uint8_t* data, size_t)
    {
      auto& core = Core(game);
      ++core.deserializeCalls;
      core.machineFrame = 99;
      if (core.failAllFrames || data[0] == core.failedFrame)
        return GAME_ERROR_FAILED;
      if (data[0] == 2 && !core.rewindFramePath.empty() &&
          (core.ejected || core.selected >= core.slots.size() ||
           core.slots[core.selected] != core.rewindFramePath))
        return GAME_ERROR_FAILED;
      core.machineFrame = data[0];
      if (!core.replacementAfterDeserialize.empty())
      {
        core.slots[0] = core.replacementAfterDeserialize;
        core.failReplace = true;
      }
      core.failClose = core.failCloseAfterDeserialize;
      if (core.failClose)
        core.ejected = true;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->Serialize = [](const AddonInstance_Game* game, uint8_t* data, size_t)
    {
      data[0] = Core(game).machineFrame;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->RunFrame = [](const AddonInstance_Game* game)
    {
      auto& core = Core(game);
      ++core.runFrameCalls;
      core.frameInputs.push_back(core.polledInput);
      ++core.machineFrame;
      if (core.changeDiscOnRun)
      {
        core.selected = 1;
        core.ejected = true;
      }
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->AudioAvailable = [](const AddonInstance_Game*)
    { return GAME_ERROR_NOT_IMPLEMENTED; };
    callbacks->AchievementStateSize = [](const AddonInstance_Game*) -> size_t { return 1; };
    callbacks->SerializeAchievements = [](const AddonInstance_Game* game, uint8_t* data, size_t)
    {
      data[0] = Core(game).machineFrame;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->DeserializeAchievements = [](const AddonInstance_Game*, const uint8_t*, size_t)
    { return GAME_ERROR_NO_ERROR; };
    m_playbackEnvironment = std::make_unique<RETRO::CPlaybackTestEnvironment>();
    m_playback = std::make_unique<RETRO::CReversiblePlayback>(
        m_client.get(), m_playbackEnvironment->Renderer(), m_playbackEnvironment->Messenger(), 1.0,
        1);
    TestMember<PlaybackMemory>(*m_playback) = std::make_unique<RETRO::CDeltaPairMemoryStream>();
    TestMember<PlaybackMemory>(*m_playback)->Init(1, 10);
    const auto discID = TestMember<PlaybackDiscs>(*m_playback).Intern(m_client->Discs().GetDiscs());
    for (uint8_t frame = 1; frame <= 3; ++frame)
    {
      *TestMember<PlaybackMemory>(*m_playback)->BeginFrame() = frame;
      const auto* discs = frame == 1 ? firstFrameDiscs : frame == 3 ? lastFrameDiscs : nullptr;
      const auto frameDiscID =
          discs ? TestMember<PlaybackDiscs>(*m_playback).Intern(*discs) : discID;
      TestMember<PlaybackMemory>(*m_playback)->SubmitFrame(frameDiscID, frame);
    }
    TestMember<PlaybackTotalFrames>(*m_playback) = 3;
    (m_playback.get()->*GetMember(PlaybackStats{}))();
    m_playback->SetSpeed(2.0);
  }

  bool RestoreFailureLatched() const { return TestMember<PlaybackRestoreFailed>(*m_playback); }

  std::string WriteSavestate(const CGameClientDiscModel* discs)
  {
    RETRO::CSavestateFlatBuffer save;
    *save.GetMemoryBuffer(1) = 1;
    save.SetTimestampFrames(1);
    if (discs)
      save.SetDiscState(discs->GetState());
    save.Finalize();
    const std::string path = CreateFile(".sav");
    EXPECT_TRUE(RETRO::CSavestateDatabase().AddSavestate(path, m_client->GetGamePath(), save));
    return path;
  }

  std::string CaptureSavestate()
  {
    const auto path = CreateFile(".sav");
    EXPECT_EQ(m_playback->CreateSavestate(false, path), path);
    for (auto& task : TestMember<PlaybackSaveTasks>(*m_playback))
      task.wait();
    return path;
  }

  std::vector<uint8_t> ReadFile(const std::string& path)
  {
    std::vector<uint8_t> data;
    XFILE::CFile file;
    EXPECT_GT(file.LoadFile(path, data), 0);
    return data;
  }

  struct DiscFileSnapshot
  {
    std::string path;
    std::vector<uint8_t> contents;
    std::filesystem::file_time_type modified;
  };

  std::vector<DiscFileSnapshot> SnapshotDiscFiles()
  {
    m_client->Discs().SaveDiscState();
    std::vector<DiscFileSnapshot> files;
    for (const auto& path : {CGameClientDiscXML::GetXMLPath(m_client->GetGamePath()),
                             CGameClientDiscM3U::GetM3UPath(m_client->GetGamePath())})
    {
      const auto local = CSpecialProtocol::TranslatePath(path);
      // Backdate the files so even an identical rewrite is detected without timing sleeps.
      std::filesystem::last_write_time(
          local, std::filesystem::file_time_type::clock::now() - std::chrono::hours(24));
      files.push_back({local, ReadFile(path), std::filesystem::last_write_time(local)});
    }
    return files;
  }

  void ExpectDiscFilesUnchanged(const std::vector<DiscFileSnapshot>& files)
  {
    for (const auto& file : files)
    {
      EXPECT_EQ(ReadFile(file.path), file.contents);
      EXPECT_TRUE(std::filesystem::last_write_time(file.path) == file.modified);
    }
  }

  template<typename Edit>
  void ExpectRewindCursorSaveRoundtrip(Edit edit)
  {
    m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
    m_core.rewindFramePath = m_core.slots[0];
    StartPlaying();
    CreatePlayback();
    m_playback->RewindEvent();
    m_playback->SetSpeed(0.0);
    ASSERT_EQ(m_core.machineFrame, 3);
    ASSERT_EQ(m_playback->GetTimeMs(), 2000U);
    const auto historical = m_client->Discs().GetDiscs();
    auto edited = historical;
    edit(edited, CreateFile(".chd"));
    m_client->Discs().SetDiscModel(edited);
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    ASSERT_TRUE(edited == historical);
    const auto expected = historical.GetState();

    const auto path = CaptureSavestate();

    EXPECT_EQ(m_core.machineFrame, 3);
    EXPECT_TRUE(m_client->Discs().GetDiscs() == edited);
    const auto discID = TestMember<PlaybackMemory>(*m_playback)->GetDiscStateID();
    const auto* cursorDiscs = TestMember<PlaybackDiscs>(*m_playback).Get(discID);
    ASSERT_NE(cursorDiscs, nullptr);
    EXPECT_TRUE(*cursorDiscs == historical);
    RETRO::CSavestateFlatBuffer saved;
    ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(path, saved));
    ASSERT_TRUE(saved.PrepareMemoryData(1));
    EXPECT_EQ(saved.GetMemoryData()[0], 2);
    EXPECT_EQ(saved.TimestampFrames(), 2U);
    ASSERT_TRUE(saved.GetDiscState().has_value());
    EXPECT_EQ(*saved.GetDiscState(), expected);

    m_core.machineFrame = 99;
    m_client->Discs().SetDiscModel(historical);
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    ASSERT_TRUE(m_playback->LoadSavestate(path));
    EXPECT_EQ(m_core.machineFrame, 2);
    EXPECT_EQ(TestMember<PlaybackTotalFrames>(*m_playback), 2U);
    const auto restored = m_client->Discs().GetDiscs();
    EXPECT_EQ(restored.GetState(), expected);
    EXPECT_EQ(m_core.ejected, expected.trayEjected);
    if (expected.selectedSlot < 0)
      EXPECT_GE(m_core.selected, m_core.slots.size());
    else
      EXPECT_EQ(m_core.selected, static_cast<unsigned int>(expected.selectedSlot));
    for (size_t i = 0; i < edited.Size(); ++i)
    {
      EXPECT_EQ(restored.GetPathByIndex(i), edited.GetPathByIndex(i));
      ASSERT_LT(i, m_core.slots.size());
      EXPECT_EQ(m_core.slots[i], edited.GetPathByIndex(i));
    }
    EXPECT_FALSE(RestoreFailureLatched());
  }

  template<typename Edit>
  void ExpectRewindDiscEditSaveRejected(Edit edit)
  {
    m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
    StartPlaying();
    CreatePlayback();
    m_playback->RewindEvent();
    m_playback->SetSpeed(0.0);
    const auto historical = m_client->Discs().GetDiscs();
    auto edited = historical;
    edit(edited, CreateFile(".chd"));
    ASSERT_FALSE(edited == historical);
    m_client->Discs().SetDiscModel(edited);
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    const auto slots = m_core.slots;
    const auto selected = m_core.selected;
    const auto ejected = m_core.ejected;
    const auto mutations = m_core.mutations;
    const auto calls = m_core.deserializeCalls;

    const auto path = CaptureSavestate();

    XFILE::CFile file;
    ASSERT_TRUE(file.Open(path));
    EXPECT_EQ(file.GetLength(), 0);
    file.Close();
    EXPECT_EQ(m_core.machineFrame, 3);
    EXPECT_EQ(m_core.slots, slots);
    EXPECT_EQ(m_core.selected, selected);
    EXPECT_EQ(m_core.ejected, ejected);
    EXPECT_EQ(m_core.mutations, mutations);
    EXPECT_EQ(m_core.deserializeCalls, calls);
    EXPECT_TRUE(m_client->Discs().GetDiscs() == edited);
    EXPECT_EQ(m_client->Discs().GetDiscs().GetKnownDiscPaths(), edited.GetKnownDiscPaths());
    EXPECT_EQ(m_playback->GetTimeMs(), 2000U);
    EXPECT_EQ(m_playback->GetSpeed(), 0.0);
    const auto& memory = TestMember<PlaybackMemory>(*m_playback);
    EXPECT_EQ(*memory->CurrentFrame(), 2);
    EXPECT_EQ(memory->GetFrameCounter(), 2U);
    const auto* cursorDiscs = TestMember<PlaybackDiscs>(*m_playback).Get(memory->GetDiscStateID());
    ASSERT_NE(cursorDiscs, nullptr);
    EXPECT_TRUE(*cursorDiscs == historical);
    EXPECT_FALSE(RestoreFailureLatched());

    m_playback->SetSpeed(1.0);
    EXPECT_EQ(m_playback->GetSpeed(), 1.0);
    m_playback->FrameEvent();
    EXPECT_EQ(m_core.machineFrame, 3);
    RETRO::CSavestateFlatBuffer saved;
    ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(CaptureSavestate(), saved));
    ASSERT_TRUE(saved.PrepareMemoryData(1));
    EXPECT_EQ(saved.GetMemoryData()[0], 3);
    EXPECT_EQ(saved.TimestampFrames(), 3U);
    ASSERT_TRUE(saved.GetDiscState().has_value());
    EXPECT_EQ(*saved.GetDiscState(), edited.GetState());
    EXPECT_FALSE(RestoreFailureLatched());
  }

  void ExpectRewindPreviewResume(bool changeDiscOnRun)
  {
    m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
    StartPlaying();
    CreatePlayback();
    m_input = std::make_unique<TestInput>(m_core, *m_client->LockForSnapshot().mutex());
    TestMember<ClientInput>(*m_client) = m_input.get();
    auto previewDiscs = m_client->Discs().GetDiscs();
    m_core.changeDiscOnRun = changeDiscOnRun;
    if (changeDiscOnRun)
    {
      ASSERT_TRUE(previewDiscs.SetSelectedDiscByIndex(1));
      previewDiscs.SetEjected(true);
    }

    const auto persisted = SnapshotDiscFiles();
    m_playback->RewindEvent();

    const auto& memory = TestMember<PlaybackMemory>(*m_playback);
    ASSERT_EQ(*memory->CurrentFrame(), 2);
    ASSERT_EQ(memory->GetFrameCounter(), 2U);
    ASSERT_EQ(memory->FutureFramesAvailable(), 1U);
    ASSERT_EQ(TestMember<PlaybackTotalFrames>(*m_playback), 2U);
    ASSERT_EQ(m_core.machineFrame, 3);
    ASSERT_EQ(m_core.runFrameCalls, 1U);
    ASSERT_EQ(m_core.inputPollCalls, 1U);
    ASSERT_EQ(m_core.frameInputs, std::vector<bool>{false});
    ASSERT_EQ(m_core.selected, changeDiscOnRun ? 1U : 0U);
    ASSERT_EQ(m_core.ejected, changeDiscOnRun);

    m_core.pendingInput = true;
    m_playback->SetSpeed(1.0);
    m_playback->FrameEvent();

    EXPECT_EQ(m_core.machineFrame, 3);
    EXPECT_EQ(m_core.runFrameCalls, 1U);
    EXPECT_EQ(m_core.inputPollCalls, 1U);
    EXPECT_TRUE(m_core.pendingInput);
    EXPECT_EQ(m_core.frameInputs, std::vector<bool>{false});
    EXPECT_EQ(*memory->CurrentFrame(), 3);
    EXPECT_EQ(memory->GetFrameCounter(), 3U);
    EXPECT_EQ(TestMember<PlaybackTotalFrames>(*m_playback), 3U);
    EXPECT_EQ(memory->PastFramesAvailable(), 2U);
    EXPECT_EQ(memory->FutureFramesAvailable(), 0U);
    EXPECT_TRUE(m_client->Discs().GetDiscs() == previewDiscs);
    const auto* committedDiscs = TestMember<PlaybackDiscs>(*m_playback).Get(memory->GetDiscStateID());
    ASSERT_NE(committedDiscs, nullptr);
    EXPECT_TRUE(*committedDiscs == previewDiscs);
    RETRO::CSavestateFlatBuffer saved;
    ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(CaptureSavestate(), saved));
    ASSERT_TRUE(saved.PrepareMemoryData(1));
    EXPECT_EQ(saved.GetMemoryData()[0], 3);
    EXPECT_EQ(saved.TimestampFrames(), 3U);
    ASSERT_TRUE(saved.GetDiscState().has_value());
    EXPECT_EQ(*saved.GetDiscState(), previewDiscs.GetState());
    ASSERT_EQ(saved.GetAchievementSize(), 1U);
    EXPECT_EQ(saved.GetAchievementData()[0], 3);

    m_playback->FrameEvent();

    EXPECT_EQ(m_core.machineFrame, 4);
    EXPECT_EQ(m_core.runFrameCalls, 2U);
    EXPECT_EQ(m_core.inputPollCalls, 2U);
    EXPECT_EQ(m_core.frameInputs, (std::vector<bool>{false, true}));
    EXPECT_EQ(*memory->CurrentFrame(), 4);
    EXPECT_EQ(memory->GetFrameCounter(), 4U);
    EXPECT_EQ(TestMember<PlaybackTotalFrames>(*m_playback), 4U);
    EXPECT_EQ(memory->FutureFramesAvailable(), 0U);
    ExpectDiscFilesUnchanged(persisted);
    EXPECT_FALSE(RestoreFailureLatched());
  }

  void ExpectUnsupportedSavestateRejected(bool selectUnsupported)
  {
    const auto disc1 = CreateFile(".chd");
    const auto disc2 = CreateFile(".unsupported");
    m_core.slots = {disc1};
    StartPlaying();
    CreatePlayback();
    auto current = m_client->Discs().GetDiscs();
    current.RememberDiscPath(disc2);
    m_client->Discs().SetDiscModel(current);
    CGameClientDiscModel historical;
    historical.AddDisc(disc1);
    historical.AddDisc(disc2);
    ASSERT_TRUE(historical.SetSelectedDiscByIndex(selectUnsupported ? 1 : 0));
    CGameClientDiscModel resolved;
    ASSERT_TRUE(current.ResolveState(historical.GetState(), resolved));
    ASSERT_EQ(resolved.GetPathByIndex(1), disc2);
    const auto path = WriteSavestate(&historical);
    const auto mutations = m_core.mutations;
    const auto calls = m_core.deserializeCalls;

    EXPECT_FALSE(m_playback->LoadSavestate(path));

    EXPECT_EQ(m_core.deserializeCalls, calls);
    EXPECT_EQ(m_core.mutations, mutations);
    EXPECT_EQ(m_core.machineFrame, 3);
    EXPECT_EQ(m_core.slots, (std::vector<std::string>{disc1}));
    EXPECT_EQ(m_core.selected, 0U);
    EXPECT_FALSE(m_core.ejected);
    EXPECT_TRUE(m_client->Discs().GetDiscs() == current);
    EXPECT_EQ(m_client->Discs().GetDiscs().GetKnownDiscPaths(), current.GetKnownDiscPaths());
    EXPECT_EQ(m_playback->GetSpeed(), 2.0);
    EXPECT_FALSE(RestoreFailureLatched());
  }

  void ExpectFailedDiscAddition(bool compactRemoval, bool reuseTail)
  {
    CGameClientDiscModel current;
    current.AddDisc("/roms/disc1.chd");
    current.SetEjected(true);
    m_core.slots = {"/roms/disc1.chd"};
    if (reuseTail)
      m_core.slots.emplace_back("/roms/existing-tail.chd");
    m_core.compactRemoval = compactRemoval;
    m_core.ejected = true;
    m_core.failReplace = true;
    m_client->Discs().SetDiscModel(current);
    auto expectedSlots = m_core.slots;
    if (!reuseTail && !compactRemoval)
      expectedSlots.emplace_back();

    EXPECT_FALSE(m_client->Discs().AddDisc("/roms/new-disc.chd"));

    EXPECT_EQ(m_core.slots, expectedSlots);
    EXPECT_EQ(m_core.addCalls, reuseTail ? 0U : 1U);
    EXPECT_EQ(m_core.removeCalls, reuseTail ? 0U : 1U);
    EXPECT_EQ(m_core.selected, 0U);
    EXPECT_TRUE(m_core.ejected);
    EXPECT_TRUE(m_client->Discs().GetDiscs() == current);
    EXPECT_EQ(m_client->Discs().GetDiscs().GetKnownDiscPaths(), current.GetKnownDiscPaths());
  }

  void ExpectSavestateCanBeCreated()
  {
    const auto path = CreateFile(".sav");
    EXPECT_EQ(m_playback->CreateSavestate(false, path), path);
    m_playback->Deinitialize();
    RETRO::CSavestateFlatBuffer save;
    ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(path, save));
    ASSERT_TRUE(save.PrepareMemoryData(1));
    EXPECT_EQ(save.GetMemoryData()[0], 3);
  }

  std::unique_ptr<RETRO::CPlaybackTestEnvironment> m_playbackEnvironment;
  std::unique_ptr<RETRO::CReversiblePlayback> m_playback;
  DiscCore m_core;
  std::unique_ptr<TestInput> m_input;
  std::unique_ptr<CGameClient> m_client;
  std::vector<XFILE::CFile*> m_files;
};
} // namespace KODI::GAME

TEST_F(TestGameClientDiscs, OlderDiscModelKeepsLaterMediaIdentity)
{
  const std::string path = CreateFile(".chd");
  ASSERT_FALSE(path.empty());
  CGameClientDiscModel later;
  later.AddDisc(path);
  const auto state = later.GetState();
  m_client->Discs().SetDiscModel(later);
  m_client->Discs().SetDiscModel({});

  const auto current = m_client->Discs().GetDiscs();
  EXPECT_TRUE(current.Empty());
  CGameClientDiscModel restored;
  ASSERT_TRUE(current.ResolveState(state, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), path);
}

TEST_F(TestGameClientDiscs, UnchangedDiscModelCanLearnMediaIdentity)
{
  const std::string path = CreateFile(".chd");
  ASSERT_FALSE(path.empty());
  CGameClientDiscModel history;
  history.AddDisc(path);
  const auto state = history.GetState();
  ASSERT_TRUE(history.EraseDiscByIndex(0));
  m_client->Discs().SetDiscModel(history);

  CGameClientDiscModel restored;
  ASSERT_TRUE(m_client->Discs().GetDiscs().ResolveState(state, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), path);
}

TEST_F(TestGameClientDiscs, EmptyPersistedStateLearnsOriginalPlaylistWithoutActivatingIt)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", disc + "\n");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(playlist, {}));
  CGameClientDiscModel historical;
  historical.AddDisc(disc);

  m_client->Discs().Initialize(playlist);

  EXPECT_TRUE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_TRUE(current.Empty());
  EXPECT_TRUE(current.IsSelectedNoDisc());
  CGameClientDiscModel restored;
  ASSERT_TRUE(current.ResolveState(historical.GetState(), restored));
  EXPECT_EQ(restored.GetPathByIndex(0), disc);
}

TEST_F(TestGameClientDiscs, StartupPruningPreservesRemovedMediaFromXML)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel previous;
  previous.AddDisc(disc);
  const auto state = previous.GetState();
  ASSERT_TRUE(previous.MarkRemovedByIndex(0));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(playlist, previous));

  m_client->Discs().Initialize(playlist);

  EXPECT_TRUE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_TRUE(current.Empty());
  EXPECT_TRUE(current.IsSelectedNoDisc());
  CGameClientDiscModel restored;
  ASSERT_TRUE(current.ResolveState(state, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), disc);
}

TEST_F(TestGameClientDiscs, MissingHistoricalMediaDoesNotInvalidateCurrentPlaylist)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", disc + "\n");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel persisted;
  persisted.AddDisc(disc);
  persisted.RememberDiscPath("/missing/historical-disc.chd");
  persisted.SetEjected(true);
  ASSERT_TRUE(CGameClientDiscXML::Save(playlist, persisted));

  m_client->Discs().Initialize(playlist);

  EXPECT_TRUE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current, persisted);
  CGameClientDiscModel missing;
  missing.AddDisc("/missing/historical-disc.chd");
  CGameClientDiscModel resolved;
  EXPECT_FALSE(current.ResolveState(missing.GetState(), resolved));
}

TEST_F(TestGameClientDiscs, NewSessionDiscardsPreviousMediaCatalog)
{
  const std::string previousDisc = CreateFile(".chd");
  const std::string nextDisc = CreateFile(".chd");
  ASSERT_FALSE(previousDisc.empty());
  ASSERT_FALSE(nextDisc.empty());
  m_client->Discs().Initialize(previousDisc);
  const auto previousState = m_client->Discs().GetDiscs().GetState();
  m_client->Discs().Deinitialize();
  EXPECT_TRUE(m_client->Discs().GetDiscs().GetKnownDiscPaths().empty());

  m_client->Discs().Initialize(nextDisc);

  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current.GetKnownDiscPaths(), std::vector<std::string>{nextDisc});
  CGameClientDiscModel resolved;
  EXPECT_FALSE(current.ResolveState(previousState, resolved));
}

TEST_F(TestGameClientDiscs, InvalidPersistedSelectionFallsBackToSource)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", disc + "\n");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel previous;
  previous.AddDisc(disc);
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(playlist, previous));
  CXBMCTinyXML2 document;
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(playlist);
  ASSERT_TRUE(document.LoadFile(xmlPath));
  document.RootElement()->FirstChildElement("selected")->SetAttribute("index", 5);
  ASSERT_TRUE(document.SaveFile(xmlPath));

  m_client->Discs().Initialize(playlist);

  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current.GetPathByIndex(0), disc);
  EXPECT_EQ(current.GetSelectedDiscIndex(), 0U);
}

TEST_F(TestGameClientDiscs, MalformedPersistedSlotCannotShiftSelectionOntoAnotherDisc)
{
  const std::string first = CreateFile(".chd");
  const std::string second = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", first + "\n" + second + "\n");
  ASSERT_FALSE(first.empty());
  ASSERT_FALSE(second.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel previous;
  previous.AddDisc(first);
  previous.AddDisc(second);
  ASSERT_TRUE(CGameClientDiscXML::Save(playlist, previous));
  CXBMCTinyXML2 document;
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(playlist);
  ASSERT_TRUE(document.LoadFile(xmlPath));
  document.RootElement()->FirstChildElement("slots")->FirstChildElement("slot")->DeleteAttribute(
      "path");
  ASSERT_TRUE(document.SaveFile(xmlPath));

  m_client->Discs().Initialize(playlist);

  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current.Size(), 2U);
  EXPECT_EQ(current.GetSelectedDiscPath(), first);
}

TEST_F(TestGameClientDiscs, DeserializePreparesMediaAfterEmptyPlaylist)
{
  m_core.ejected = true;
  StartPlaying();
  CGameClientDiscModel target;
  target.AddDisc("/roms/disc1.chd");
  target.AddDisc("/roms/disc2.chd");
  ASSERT_TRUE(target.SetSelectedDiscByIndex(1));

  EXPECT_EQ(Deserialize(target, 1, "/roms/disc2.chd"), RestoreResult::Restored);

  EXPECT_EQ(m_core.machinePath, "/roms/disc2.chd");
  EXPECT_EQ(m_core.selected, 1U);
  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestGameClientDiscs, DeserializePreparesReorderedMedia)
{
  m_core.slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
  StartPlaying();
  CGameClientDiscModel target;
  target.AddDisc("/roms/disc1.chd");
  target.AddDisc("/roms/disc2.chd");
  ASSERT_TRUE(target.SetSelectedDiscByIndex(1));

  EXPECT_EQ(Deserialize(target, 1, "/roms/disc2.chd"), RestoreResult::Restored);

  EXPECT_EQ(m_core.machinePath, "/roms/disc2.chd");
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc2.chd"}));
  EXPECT_EQ(m_core.selected, 1U);
}

TEST_F(TestGameClientDiscs, RejectedMediaPreparationSkipsDeserializeAndRestoresPreviousState)
{
  m_core.slots = {"/roms/disc1.chd"};
  StartPlaying();
  const CGameClientDiscModel previous = m_client->Discs().GetDiscs();
  m_core.failReplace = true;
  CGameClientDiscModel target;
  target.AddDisc("/roms/disc2.chd");
  target.SetEjected(true);

  EXPECT_EQ(Deserialize(target, 0, "/roms/disc2.chd"), RestoreResult::Rejected);

  EXPECT_EQ(m_core.deserializeCalls, 0U);
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
}

TEST_F(TestGameClientDiscs, FailedDeserializeRestoresPreviousMediaAndTray)
{
  m_core.slots = {"/roms/disc1.chd"};
  m_core.ejected = true;
  StartPlaying();
  const CGameClientDiscModel previous = m_client->Discs().GetDiscs();
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    ++Core(game).deserializeCalls;
    Core(game).deserializedSlots = Core(game).slots;
    return GAME_ERROR_FAILED;
  };
  CGameClientDiscModel target;
  target.AddDisc("/roms/disc2.chd");

  EXPECT_EQ(Deserialize(target, 0, "/roms/disc2.chd"), RestoreResult::StateUncertain);

  EXPECT_EQ(m_core.deserializedSlots, (std::vector<std::string>{"/roms/disc2.chd"}));
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
}

TEST_F(TestGameClientDiscs, DeserializeReconcilesMediaChangedByCore)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  StartPlaying();
  CGameClientDiscModel target = m_client->Discs().GetDiscs();
  target.SetEjected(true);
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    Core(game).slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
    Core(game).selected = 1;
    Core(game).ejected = false;
    return GAME_ERROR_NO_ERROR;
  };

  EXPECT_EQ(Deserialize(target, 0, "/roms/disc1.chd"), RestoreResult::Restored);

  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc2.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == target);
}

TEST_F(TestGameClientDiscs, DeserializeReconcilesMediaWithoutPathMetadata)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  StartPlaying();
  CGameClientDiscModel target = m_client->Discs().GetDiscs();
  target.SetEjected(true);
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    Core(game).slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
    Core(game).selected = 1;
    Core(game).ejected = false;
    return GAME_ERROR_NO_ERROR;
  };

  EXPECT_EQ(Deserialize(target, 0, "/roms/disc1.chd"), RestoreResult::Restored);

  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc2.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == target);
}

TEST_F(TestGameClientDiscs, FailedDeserializeRestoresUnchangedModelWithoutPathMetadata)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  StartPlaying();
  const CGameClientDiscModel target = m_client->Discs().GetDiscs();
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    Core(game).slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
    return GAME_ERROR_FAILED;
  };

  EXPECT_EQ(Deserialize(target, 0, "/roms/disc1.chd"), RestoreResult::StateUncertain);

  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc2.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == target);
}

TEST_F(TestGameClientDiscs, DeserializeRetriesClosedTrayWithoutLosingSavedOpenTray)
{
  m_core.slots = {"/roms/disc1.chd"};
  m_core.ejected = true;
  StartPlaying();
  const CGameClientDiscModel target = m_client->Discs().GetDiscs();
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    ++Core(game).deserializeCalls;
    return Core(game).ejected ? GAME_ERROR_FAILED : GAME_ERROR_NO_ERROR;
  };

  EXPECT_EQ(Deserialize(target, 0, "/roms/disc1.chd"), RestoreResult::Restored);

  EXPECT_EQ(m_core.deserializeCalls, 2U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == target);
}

TEST_F(TestGameClientDiscs, LegacyDeserializeClosesTrayBeforeLoading)
{
  m_core.slots = {"/roms/disc1.chd"};
  m_core.ejected = true;
  StartPlaying();
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  { return Core(game).ejected ? GAME_ERROR_FAILED : GAME_ERROR_NO_ERROR; };
  const uint8_t data = 0;

  EXPECT_EQ(m_client->Deserialize(&data, sizeof(data)), RestoreResult::Restored);

  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestGameClientDiscs, RestoreHistoricalTopologyClearsRemovedAndExtraSlots)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd", "One");
  historical.AddRemovedSlot();
  historical.AddDisc("/roms/disc2.chd", "Two");
  ASSERT_TRUE(historical.SetSelectedDiscByIndex(2));
  m_core.slots = {"/roms/disc2.chd", "/roms/reused.chd", "/roms/disc1.chd", "/roms/extra.chd"};
  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "", "/roms/disc2.chd", ""}));
  EXPECT_EQ(m_core.selected, 2U);
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
  m_core.mutations = 0;
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_EQ(m_core.mutations, 0U);
  m_client->Discs().RefreshDiscState();
  EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
}

TEST_F(TestGameClientDiscs, SourceRetryReplacesPersistedInitialHint)
{
  std::unique_ptr<XFILE::CFile> firstDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> secondDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(firstDisc, nullptr);
  ASSERT_NE(secondDisc, nullptr);
  ASSERT_NE(playlist, nullptr);
  firstDisc->Close();
  secondDisc->Close();
  const std::string firstPath = XBMC_TEMPFILEPATH(firstDisc.get());
  const std::string secondPath = XBMC_TEMPFILEPATH(secondDisc.get());
  const std::string source = firstPath + "\n" + secondPath + "\n";
  ASSERT_EQ(playlist->Write(source.data(), source.size()), static_cast<ssize_t>(source.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel persisted;
  persisted.AddDisc(firstPath);
  persisted.AddDisc(secondPath);
  ASSERT_TRUE(persisted.SetSelectedDiscByIndex(1));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, persisted));

  m_client->Discs().Initialize(gamePath);
  EXPECT_EQ(m_core.initialIndex, 1U);
  EXPECT_EQ(m_core.initialPath, secondPath);

  m_client->Discs().Initialize(gamePath, false);
  EXPECT_EQ(m_core.initialIndex, 0U);
  EXPECT_EQ(m_core.initialPath, firstPath);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(secondDisc.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(firstDisc.release()));
}

TEST_F(TestGameClientDiscs, SourceRetryReplacesPersistedHintAfterCallbackFailure)
{
  std::unique_ptr<XFILE::CFile> firstDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> secondDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(firstDisc, nullptr);
  ASSERT_NE(secondDisc, nullptr);
  ASSERT_NE(playlist, nullptr);
  firstDisc->Close();
  secondDisc->Close();
  const std::string firstPath = XBMC_TEMPFILEPATH(firstDisc.get());
  const std::string secondPath = XBMC_TEMPFILEPATH(secondDisc.get());
  const std::string source = firstPath + "\n" + secondPath + "\n";
  ASSERT_EQ(playlist->Write(source.data(), source.size()), static_cast<ssize_t>(source.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel persisted;
  persisted.AddDisc(firstPath);
  persisted.AddDisc(secondPath);
  ASSERT_TRUE(persisted.SetSelectedDiscByIndex(1));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, persisted));

  m_core.failInitialImage = true;
  m_client->Discs().Initialize(gamePath);
  EXPECT_EQ(m_core.initialIndex, 1U);
  EXPECT_EQ(m_core.initialPath, secondPath);

  m_client->Discs().Initialize(gamePath, false);
  EXPECT_EQ(m_core.initialIndex, 0U);
  EXPECT_EQ(m_core.initialPath, firstPath);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(secondDisc.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(firstDisc.release()));
}

TEST_F(TestGameClientDiscs, RestoreSelectionAndTrayIndependently)
{
  for (const bool ejected : {false, true})
  {
    for (const bool noDisc : {false, true})
    {
      CGameClientDiscModel historical;
      historical.AddDisc("/roms/disc1.chd");
      historical.AddDisc("/roms/disc2.chd");
      ASSERT_TRUE(historical.SetSelectedDiscByIndex(1));
      if (noDisc)
        historical.SetSelectedNoDisc();
      historical.SetEjected(ejected);
      m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
      m_core.selected = 0;
      m_core.ejected = !ejected;
      m_client->Discs().SetDiscModel(historical);
      ASSERT_TRUE(m_client->Discs().RestoreDiscList());
      EXPECT_EQ(m_core.selected, noDisc ? 2U : 1U);
      EXPECT_EQ(m_core.ejected, ejected);
    }
  }
}

TEST_F(TestGameClientDiscs, NoDiscAcceptsCoreSentinelBeyondImageCount)
{
  m_client->GetInstanceInterface()->toAddon->GetImageIndex = [](const AddonInstance_Game* game)
  {
    return Core(game).selected < Core(game).slots.size() ? Core(game).selected
                                                         : std::numeric_limits<unsigned int>::max();
  };
  for (const bool ejected : {false, true})
  {
    CGameClientDiscModel historical;
    historical.AddDisc("/roms/disc1.chd", "One");
    historical.AddDisc("/roms/disc2.chd", "Two");
    historical.SetSelectedNoDisc();
    historical.SetEjected(ejected);
    m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
    m_core.selected = 0;
    m_core.ejected = !ejected;
    m_client->Discs().SetDiscModel(historical);
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    EXPECT_EQ(m_core.selected, 2U);
    EXPECT_EQ(m_core.ejected, ejected);
    m_core.mutations = 0;
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    EXPECT_EQ(m_core.mutations, 0U);
    m_client->Discs().RefreshDiscState();
    EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
  }
}

TEST_F(TestGameClientDiscs, PrepareDeserializePreservesHistoricalTrayModel)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd");
  historical.SetEjected(true);
  m_client->Discs().SetDiscModel(historical);
  m_core.ejected = true;
  ASSERT_TRUE(m_client->Discs().PrepareForDeserialize());
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
  m_core.mutations = 0;
  ASSERT_TRUE(m_client->Discs().PrepareForDeserialize());
  EXPECT_EQ(m_core.mutations, 0U);
}

TEST_F(TestGameClientDiscs, RestoreEmptyPlaylistAndTrailingRemovedSlot)
{
  m_core.slots = {"/roms/disc1.chd"};
  CGameClientDiscModel empty;
  m_client->Discs().SetDiscModel(empty);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_TRUE(m_core.slots[0].empty());
  EXPECT_EQ(m_core.selected, m_core.slots.size());

  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd");
  historical.AddRemovedSlot();
  historical.AddRemovedSlot();
  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_EQ(m_core.slots.size(), 3U);
  EXPECT_TRUE(m_core.slots[2].empty());
}

TEST_F(TestGameClientDiscs, RefusedMediaRestoreReportsFailure)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc2.chd");
  m_core.slots = {"/roms/disc1.chd"};
  m_core.failReplace = true;
  m_client->Discs().SetDiscModel(historical);
  EXPECT_FALSE(m_client->Discs().RestoreDiscList());
}

TEST_F(TestGameClientDiscs, PresentEmptyPersistedStateIsAuthoritative)
{
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(playlist, nullptr);
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscXML xml;
  CGameClientDiscModel empty;
  ASSERT_TRUE(xml.Save(gamePath, empty));

  m_client->Discs().Initialize(gamePath);
  EXPECT_TRUE(m_client->Discs().HasPersistedState());
  EXPECT_TRUE(m_client->Discs().GetDiscs().Empty());

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
}

TEST_F(TestGameClientDiscs, InvalidPersistedStateFallsBackToWholeSourcePlaylist)
{
  std::unique_ptr<XFILE::CFile> disc(XBMC_CREATETEMPFILE(".chd"));
  ASSERT_NE(disc, nullptr);
  disc->Close();
  const std::string discPath = XBMC_TEMPFILEPATH(disc.get());

  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(playlist, nullptr);
  ASSERT_EQ(playlist->Write(discPath.data(), discPath.size()),
            static_cast<ssize_t>(discPath.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel stale;
  stale.AddDisc("/missing/disc.chd");
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, stale));

  m_client->Discs().Initialize(gamePath);
  const CGameClientDiscModel source = m_client->Discs().GetDiscs();
  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  ASSERT_EQ(source.Size(), 1U);
  EXPECT_EQ(source.GetPathByIndex(0), discPath);
  EXPECT_EQ(source.GetSelectedDiscIndex(), 0U);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(disc.release()));
}

TEST_F(TestGameClientDiscs, UnsupportedPersistedMediaFallsBackToWholeSourcePlaylist)
{
  std::unique_ptr<XFILE::CFile> sourceDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> unsupportedDisc(XBMC_CREATETEMPFILE(".iso"));
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(sourceDisc, nullptr);
  ASSERT_NE(unsupportedDisc, nullptr);
  ASSERT_NE(playlist, nullptr);
  sourceDisc->Close();
  unsupportedDisc->Close();
  const std::string sourcePath = XBMC_TEMPFILEPATH(sourceDisc.get());
  ASSERT_EQ(playlist->Write(sourcePath.data(), sourcePath.size()),
            static_cast<ssize_t>(sourcePath.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel incompatible;
  incompatible.AddDisc(XBMC_TEMPFILEPATH(unsupportedDisc.get()));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, incompatible));

  m_client->Discs().Initialize(gamePath);
  const CGameClientDiscModel source = m_client->Discs().GetDiscs();
  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  ASSERT_EQ(source.Size(), 1U);
  EXPECT_EQ(source.GetPathByIndex(0), sourcePath);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(unsupportedDisc.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(sourceDisc.release()));
}

TEST_F(TestGameClientDiscs, SourceOnlyInitializationIgnoresValidPersistedState)
{
  std::unique_ptr<XFILE::CFile> sourceDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> persistedDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(sourceDisc, nullptr);
  ASSERT_NE(persistedDisc, nullptr);
  ASSERT_NE(playlist, nullptr);
  sourceDisc->Close();
  persistedDisc->Close();
  const std::string sourcePath = XBMC_TEMPFILEPATH(sourceDisc.get());
  ASSERT_EQ(playlist->Write(sourcePath.data(), sourcePath.size()),
            static_cast<ssize_t>(sourcePath.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel persisted;
  persisted.AddDisc(XBMC_TEMPFILEPATH(persistedDisc.get()));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, persisted));

  m_client->Discs().Initialize(gamePath, false);
  const CGameClientDiscModel source = m_client->Discs().GetDiscs();
  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  ASSERT_EQ(source.Size(), 1U);
  EXPECT_EQ(source.GetPathByIndex(0), sourcePath);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(persistedDisc.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(sourceDisc.release()));
}

TEST_F(TestGameClientDiscs, PersistedEmptyAndShortModelsRestoreAcrossCoreRemovalBehaviors)
{
  for (const bool compactRemoval : {false, true})
  {
    std::unique_ptr<XFILE::CFile> disc(XBMC_CREATETEMPFILE(".chd"));
    std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
    ASSERT_NE(disc, nullptr);
    ASSERT_NE(playlist, nullptr);
    disc->Close();
    playlist->Close();
    const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

    for (const bool empty : {false, true})
    {
      CGameClientDiscModel persisted;
      if (!empty)
        persisted.AddDisc(XBMC_TEMPFILEPATH(disc.get()));
      CGameClientDiscXML xml;
      ASSERT_TRUE(xml.Save(gamePath, persisted));

      m_core.compactRemoval = compactRemoval;
      m_core.slots = {"/core/one.chd", "/core/two.chd", "/core/three.chd"};
      m_core.selected = 0;
      m_client->Discs().Initialize(gamePath);
      ASSERT_TRUE(m_client->Discs().HasPersistedState());
      ASSERT_TRUE(m_client->Discs().RestoreDiscList());
      m_client->Discs().RefreshDiscState();

      const CGameClientDiscModel restored = m_client->Discs().GetDiscs();
      EXPECT_EQ(restored.Size(), empty ? 0U : 1U);
      EXPECT_EQ(m_core.slots.size(), compactRemoval ? (empty ? 0U : 1U) : 3U);
      if (!compactRemoval)
      {
        for (size_t i = empty ? 0U : 1U; i < m_core.slots.size(); ++i)
          EXPECT_TRUE(m_core.slots[i].empty());
      }
    }

    XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
    XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
    EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
    EXPECT_TRUE(XBMC_DELETETEMPFILE(disc.release()));
  }
}

TEST_F(TestGameClientDiscs, AddAfterHistoricalRestoreReusesClearedPhysicalTail)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd");
  historical.SetEjected(true);
  m_core.slots = {"/roms/disc1.chd", "/roms/future.chd"};
  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  ASSERT_TRUE(m_client->Discs().AddDisc("/roms/disc2.chd"));
  const auto model = m_client->Discs().GetDiscs();
  EXPECT_EQ(model.Size(), 2U);
  EXPECT_EQ(model.GetPathByIndex(1), "/roms/disc2.chd");
  EXPECT_EQ(m_core.slots.size(), 2U);
}

TEST_F(TestGameClientDiscs, ReusedRemovedSlotKeepsIdentityWithoutCoreMetadata)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd");
  historical.AddRemovedSlot();
  historical.SetEjected(true);
  m_core.slots = {"/roms/disc1.chd", ""};
  m_core.ejected = true;
  m_client->Discs().SetDiscModel(historical);
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };
  ASSERT_TRUE(m_client->Discs().AddDisc("/roms/disc2.chd"));
  const auto model = m_client->Discs().GetDiscs();
  EXPECT_EQ(model.Size(), 2U);
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
  EXPECT_EQ(model.GetPathByIndex(1), "/roms/disc2.chd");
}

TEST_F(TestGameClientDiscs, RepeatedRestoreWithoutCorePathMetadataDoesNotMutateMedia)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd", "One");
  historical.AddDisc("/roms/disc2.chd", "Two");
  ASSERT_TRUE(historical.SetSelectedDiscByIndex(1));
  m_core.slots = {"/roms/stale1.chd", "/roms/stale2.chd"};
  m_core.selected = 1;
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };

  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_GT(m_core.mutations, 0U);

  m_core.mutations = 0;
  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_EQ(m_core.mutations, 0U);
}

TEST_F(TestGameClientDiscs, UnknownCorePathsCannotBypassHistoricalRemoval)
{
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };
  for (const bool removedSlot : {false, true})
  {
    CGameClientDiscModel historical;
    if (removedSlot)
      historical.AddRemovedSlot();
    m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
    m_client->Discs().SetDiscModel(historical);
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    EXPECT_TRUE(m_core.slots[0].empty());
    EXPECT_TRUE(m_core.slots[1].empty());
    EXPECT_EQ(m_core.selected, 2U);
    m_client->Discs().RefreshDiscState();
    EXPECT_EQ(m_client->Discs().GetDiscs().Size(), historical.Size());
  }
}

TEST_F(TestGameClientDiscs, RejectedRewindResumesPreviousSpeedAfterRollback)
{
  m_core.slots = {CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  m_core.failedFrame = 1;

  m_playback->SeekTimeMs(1000);

  EXPECT_EQ(m_core.machineFrame, 3);
  EXPECT_GE(m_core.deserializeCalls, 2U);
  EXPECT_EQ(m_playback->GetTimeMs(), 3000U);
  EXPECT_EQ(m_playback->GetSpeed(), 2.0);
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, RejectedAdvanceResumesPreviousSpeedAfterRollback)
{
  m_core.slots = {CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  m_playback->SeekTimeMs(1000);
  ASSERT_EQ(m_core.machineFrame, 1);
  m_core.failedFrame = 3;

  m_playback->SeekTimeMs(3000);

  EXPECT_EQ(m_core.machineFrame, 1);
  EXPECT_EQ(m_playback->GetTimeMs(), 1000U);
  EXPECT_EQ(m_playback->GetSpeed(), 2.0);
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, RejectedSavestateMediaDoesNotLatchOrBlockSaving)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  const auto previous = m_client->Discs().GetDiscs();
  CreatePlayback();
  CGameClientDiscModel target;
  target.AddDisc(m_core.slots[1]);
  target.AddDisc(m_core.slots[0]);
  const auto path = WriteSavestate(&target);
  m_core.failReplace = true;

  EXPECT_FALSE(m_playback->LoadSavestate(path));

  EXPECT_EQ(m_core.deserializeCalls, 0U);
  EXPECT_EQ(m_core.machineFrame, 3);
  EXPECT_EQ(m_core.slots[0], previous.GetPathByIndex(0));
  EXPECT_EQ(m_core.slots[1], previous.GetPathByIndex(1));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
  EXPECT_FALSE(RestoreFailureLatched());
  EXPECT_EQ(m_playback->GetSpeed(), 2.0);
  ExpectSavestateCanBeCreated();
}

TEST_F(TestGameClientDiscs, SavestateDeserializeFailureLeavesStateUncertainDespiteMediaRollback)
{
  m_core.slots = {CreateFile(".chd")};
  StartPlaying();
  const auto previous = m_client->Discs().GetDiscs();
  CreatePlayback();
  m_core.failAllFrames = true;

  EXPECT_FALSE(m_playback->LoadSavestate(WriteSavestate(&previous)));

  EXPECT_GT(m_core.deserializeCalls, 0U);
  EXPECT_EQ(m_core.machineFrame, 99);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
  EXPECT_TRUE(RestoreFailureLatched());
  m_playback->SetSpeed(1.0);
  EXPECT_EQ(m_playback->GetSpeed(), 0.0);
  EXPECT_TRUE(m_playback->CreateSavestate(false).empty());
}

TEST_F(TestGameClientDiscs, SavestateDiscRollbackFailureLeavesStateUncertainBeforeDeserialize)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  CGameClientDiscModel target;
  target.AddDisc(m_core.slots[1]);
  target.AddDisc(m_core.slots[0]);
  const auto path = WriteSavestate(&target);
  m_core.failReplace = true;
  m_core.failClose = true;

  EXPECT_FALSE(m_playback->LoadSavestate(path));

  EXPECT_EQ(m_core.deserializeCalls, 0U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(RestoreFailureLatched());
  m_playback->SetSpeed(1.0);
  EXPECT_EQ(m_playback->GetSpeed(), 0.0);
}

TEST_F(TestGameClientDiscs, SeekMachineRollbackFailureLeavesStateUncertain)
{
  m_core.slots = {CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  m_core.failAllFrames = true;

  m_playback->SeekTimeMs(1000);

  EXPECT_GE(m_core.deserializeCalls, 2U);
  EXPECT_EQ(m_core.machineFrame, 99);
  EXPECT_TRUE(RestoreFailureLatched());
  m_playback->SetSpeed(1.0);
  EXPECT_EQ(m_playback->GetSpeed(), 0.0);
}

TEST_F(TestGameClientDiscs, RestoredSeekAndSavestateKeepPlaybackUsable)
{
  m_core.slots = {CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  const auto discs = m_client->Discs().GetDiscs();

  m_playback->SeekTimeMs(1000);
  EXPECT_EQ(m_core.machineFrame, 1);
  EXPECT_EQ(m_playback->GetTimeMs(), 1000U);
  EXPECT_EQ(m_playback->GetSpeed(), 2.0);
  m_playback->SeekTimeMs(3000);
  EXPECT_EQ(m_core.machineFrame, 3);
  EXPECT_EQ(m_playback->GetTimeMs(), 3000U);
  EXPECT_EQ(m_playback->GetSpeed(), 2.0);

  EXPECT_TRUE(m_playback->LoadSavestate(WriteSavestate(&discs)));
  EXPECT_EQ(m_core.machineFrame, 1);
  EXPECT_EQ(m_playback->GetSpeed(), 2.0);
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, LegacySavestateDeserializeFailureLeavesStateUncertain)
{
  m_core.slots = {CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  m_core.failAllFrames = true;

  EXPECT_FALSE(m_playback->LoadSavestate(WriteSavestate(nullptr)));

  EXPECT_EQ(m_core.deserializeCalls, 1U);
  EXPECT_EQ(m_core.machineFrame, 99);
  EXPECT_TRUE(RestoreFailureLatched());
  m_playback->SetSpeed(1.0);
  EXPECT_EQ(m_playback->GetSpeed(), 0.0);
}

TEST_F(TestGameClientDiscs, LegacySavestateMediaReconciliationFailureLeavesStateUncertain)
{
  m_core.slots = {CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  m_client->Discs().RefreshDiscState();
  const auto previous = m_client->Discs().GetDiscs();
  const auto xmlPath = CGameClientDiscXML::GetXMLPath(m_client->GetGamePath());
  const auto m3uPath = CGameClientDiscM3U::GetM3UPath(m_client->GetGamePath());
  const auto xml = ReadFile(xmlPath);
  const auto m3u = ReadFile(m3uPath);
  m_core.failCloseAfterDeserialize = true;
  m_core.replacementAfterDeserialize = CreateFile(".chd");

  EXPECT_FALSE(m_playback->LoadSavestate(WriteSavestate(nullptr)));

  EXPECT_EQ(m_core.deserializeCalls, 1U);
  EXPECT_EQ(m_core.machineFrame, 1);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_EQ(m_core.slots[0], m_core.replacementAfterDeserialize);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
  EXPECT_EQ(ReadFile(xmlPath), xml);
  EXPECT_EQ(ReadFile(m3uPath), m3u);
  EXPECT_TRUE(RestoreFailureLatched());
  m_playback->SetSpeed(1.0);
  EXPECT_EQ(m_playback->GetSpeed(), 0.0);
}

TEST_F(TestGameClientDiscs, OnlyRestoredSavestateClearsAnExistingRestoreLatch)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  const auto previous = m_client->Discs().GetDiscs();
  const auto validPath = WriteSavestate(&previous);
  m_core.failAllFrames = true;
  ASSERT_FALSE(m_playback->LoadSavestate(validPath));
  ASSERT_TRUE(RestoreFailureLatched());
  const auto calls = m_core.deserializeCalls;
  m_core.failAllFrames = false;
  m_core.failReplace = true;
  CGameClientDiscModel target;
  target.AddDisc(m_core.slots[1]);
  target.AddDisc(m_core.slots[0]);

  EXPECT_FALSE(m_playback->LoadSavestate(WriteSavestate(&target)));

  EXPECT_EQ(m_core.deserializeCalls, calls);
  EXPECT_TRUE(RestoreFailureLatched());
  m_playback->SetSpeed(1.0);
  EXPECT_EQ(m_playback->GetSpeed(), 0.0);

  m_core.failReplace = false;
  EXPECT_TRUE(m_playback->LoadSavestate(validPath));
  EXPECT_FALSE(RestoreFailureLatched());
  m_playback->SetSpeed(1.0);
  EXPECT_EQ(m_playback->GetSpeed(), 1.0);
}

TEST_F(TestGameClientDiscs, RewindPreviewSavestateUsesHistoricalMachineDiscAndTimestamp)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  const auto historical = m_client->Discs().GetDiscs();
  m_core.changeDiscOnRun = true;
  m_playback->SetSpeed(-4.0);

  m_playback->RewindEvent();

  ASSERT_EQ(m_core.runFrameCalls, 1U);
  ASSERT_EQ(m_core.machineFrame, 3);
  ASSERT_EQ(m_core.selected, 1U);
  ASSERT_TRUE(m_core.ejected);
  ASSERT_EQ(m_playback->GetTimeMs(), 2000U);
  ASSERT_TRUE(m_client->Discs().GetDiscs() == historical);
  const auto path = CaptureSavestate();
  m_playback->SetSpeed(0.0);
  const auto pausedPath = CaptureSavestate();
  EXPECT_EQ(m_core.machineFrame, 3);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->GetFrameCounter(), 2U);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->FutureFramesAvailable(), 1U);

  for (const auto& capturedPath : {path, pausedPath})
  {
    RETRO::CSavestateFlatBuffer saved;
    ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(capturedPath, saved));
    ASSERT_TRUE(saved.PrepareMemoryData(1));
    EXPECT_EQ(saved.GetMemoryData()[0], 2);
    EXPECT_EQ(saved.TimestampFrames(), 2U);
    ASSERT_TRUE(saved.GetDiscState().has_value());
    EXPECT_EQ(*saved.GetDiscState(), historical.GetState());
    EXPECT_EQ(saved.GetAchievementSize(), 0U);
  }

  ASSERT_TRUE(m_playback->LoadSavestate(path));
  EXPECT_EQ(m_core.machineFrame, 2);
  EXPECT_EQ(TestMember<PlaybackTotalFrames>(*m_playback), 2U);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestGameClientDiscs, ForwardSavestateCapturesLiveMachineDiscAndTimestamp)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  TestMember<PlaybackMemory>(*m_playback).reset();
  const auto persisted = SnapshotDiscFiles();
  auto expected = m_client->Discs().GetDiscs().GetState();
  expected.selectedSlot = 1;
  expected.trayEjected = true;
  m_core.changeDiscOnRun = true;

  m_playback->FrameEvent();

  ASSERT_EQ(m_core.machineFrame, 4);
  ASSERT_EQ(m_core.selected, 1U);
  ASSERT_TRUE(m_core.ejected);
  ASSERT_EQ(m_client->Discs().GetDiscs().GetState().selectedSlot, 0);
  const auto path = CaptureSavestate();
  RETRO::CSavestateFlatBuffer saved;
  ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(path, saved));
  ASSERT_TRUE(saved.PrepareMemoryData(1));
  EXPECT_EQ(saved.GetMemoryData()[0], 4);
  EXPECT_EQ(saved.TimestampFrames(), 4U);
  ASSERT_EQ(saved.GetAchievementSize(), 1U);
  EXPECT_EQ(saved.GetAchievementData()[0], 4);
  ASSERT_TRUE(saved.GetDiscState().has_value());
  EXPECT_EQ(*saved.GetDiscState(), expected);
  ExpectDiscFilesUnchanged(persisted);
}

TEST_F(TestGameClientDiscs, SavestateAfterSeekCapturesLiveDiscEdits)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  m_playback->RewindEvent();
  m_playback->SeekTimeMs(1000);
  ASSERT_EQ(m_core.machineFrame, 1);
  auto edited = m_client->Discs().GetDiscs();
  edited.SetSelectedNoDisc();
  edited.SetEjected(true);
  m_client->Discs().SetDiscModel(edited);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());

  const auto path = CaptureSavestate();

  RETRO::CSavestateFlatBuffer saved;
  ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(path, saved));
  ASSERT_TRUE(saved.PrepareMemoryData(1));
  EXPECT_EQ(saved.GetMemoryData()[0], 1);
  EXPECT_EQ(saved.TimestampFrames(), 1U);
  ASSERT_TRUE(saved.GetDiscState().has_value());
  EXPECT_EQ(*saved.GetDiscState(), edited.GetState());
}

TEST_F(TestGameClientDiscs, RejectedRewindMediaRestoresOnlyTheTimelineCursor)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  const auto previous = m_client->Discs().GetDiscs();
  CGameClientDiscModel target;
  target.AddDisc(m_core.slots[1]);
  target.AddDisc(m_core.slots[0]);
  CreatePlayback(&target);
  const auto discID = TestMember<PlaybackMemory>(*m_playback)->GetDiscStateID();
  m_core.failReplace = true;
  m_core.failAllFrames = true;

  m_playback->SeekTimeMs(1000);

  EXPECT_EQ(m_core.deserializeCalls, 0U);
  EXPECT_EQ(m_core.machineFrame, 3);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->GetFrameCounter(), 3U);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->GetDiscStateID(), discID);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->PastFramesAvailable(), 2U);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->FutureFramesAvailable(), 0U);
  EXPECT_EQ(m_playback->GetTimeMs(), 3000U);
  EXPECT_EQ(m_playback->GetSpeed(), 2.0);
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, RejectedAdvanceMediaRestoresOnlyTheTimelineCursor)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  const auto previous = m_client->Discs().GetDiscs();
  CGameClientDiscModel target;
  target.AddDisc(m_core.slots[1]);
  target.AddDisc(m_core.slots[0]);
  CreatePlayback(nullptr, &target);
  m_playback->SeekTimeMs(1000);
  ASSERT_EQ(m_core.machineFrame, 1);
  const auto discID = TestMember<PlaybackMemory>(*m_playback)->GetDiscStateID();
  m_core.deserializeCalls = 0;
  m_core.failReplace = true;
  m_core.failAllFrames = true;

  m_playback->SeekTimeMs(3000);

  EXPECT_EQ(m_core.deserializeCalls, 0U);
  EXPECT_EQ(m_core.machineFrame, 1);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->GetFrameCounter(), 1U);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->GetDiscStateID(), discID);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->PastFramesAvailable(), 0U);
  EXPECT_EQ(TestMember<PlaybackMemory>(*m_playback)->FutureFramesAvailable(), 2U);
  EXPECT_EQ(m_playback->GetTimeMs(), 1000U);
  EXPECT_EQ(m_playback->GetSpeed(), 2.0);
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, RejectedSeekAfterRewindPreviewKeepsHistoricalCapture)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  CGameClientDiscModel target;
  target.AddDisc(m_core.slots[1]);
  target.AddDisc(m_core.slots[0]);
  CreatePlayback(&target);
  m_playback->RewindEvent();
  ASSERT_EQ(m_core.machineFrame, 3);
  ASSERT_EQ(m_playback->GetTimeMs(), 2000U);
  m_core.deserializeCalls = 0;
  m_core.failReplace = true;
  m_core.failAllFrames = true;

  m_playback->SeekTimeMs(1000);
  const auto path = CaptureSavestate();

  EXPECT_EQ(m_core.deserializeCalls, 0U);
  EXPECT_EQ(m_core.machineFrame, 3);
  EXPECT_EQ(m_playback->GetTimeMs(), 2000U);
  EXPECT_FALSE(RestoreFailureLatched());
  RETRO::CSavestateFlatBuffer saved;
  ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(path, saved));
  ASSERT_TRUE(saved.PrepareMemoryData(1));
  EXPECT_EQ(saved.GetMemoryData()[0], 2);
  EXPECT_EQ(saved.TimestampFrames(), 2U);
}

TEST_F(TestGameClientDiscs, RewindPreviewSaveWithoutHistoricalFrameIsRejected)
{
  m_core.slots = {CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  m_playback->RewindEvent();
  ASSERT_EQ(m_core.machineFrame, 3);
  TestMember<PlaybackMemory>(*m_playback).reset();

  const auto path = CaptureSavestate();

  RETRO::CSavestateFlatBuffer saved;
  EXPECT_FALSE(RETRO::CSavestateDatabase().GetSavestate(path, saved));
  EXPECT_EQ(m_core.machineFrame, 3);
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, RewindSavestateWithoutDiscEditsRoundtrips)
{
  ExpectRewindCursorSaveRoundtrip([](CGameClientDiscModel&, const std::string&) {});
}

TEST_F(TestGameClientDiscs, RewindSavestateRejectsSelectedDiscEdit)
{
  ExpectRewindDiscEditSaveRejected([](CGameClientDiscModel& model, const std::string&)
                                   { ASSERT_TRUE(model.SetSelectedDiscByIndex(1)); });
}

TEST_F(TestGameClientDiscs, RewindSavestateRejectsNoDiscEdit)
{
  ExpectRewindDiscEditSaveRejected([](CGameClientDiscModel& model, const std::string&)
                                   { model.SetSelectedNoDisc(); });
}

TEST_F(TestGameClientDiscs, RewindSavestateRejectsTrayEdit)
{
  ExpectRewindDiscEditSaveRejected([](CGameClientDiscModel& model, const std::string&)
                                   { model.SetEjected(true); });
}

TEST_F(TestGameClientDiscs, RewindSavestateRejectsRemovedSlot)
{
  ExpectRewindDiscEditSaveRejected([](CGameClientDiscModel& model, const std::string&)
                                   { ASSERT_TRUE(model.RemoveDiscByIndex(0)); });
}

TEST_F(TestGameClientDiscs, RewindSavestateRejectsAddedDisc)
{
  ExpectRewindDiscEditSaveRejected([](CGameClientDiscModel& model, const std::string& extraDisc)
                                   { model.AddDisc(extraDisc, "Added disc"); });
}

TEST_F(TestGameClientDiscs, RewindSavestateRejectsReorderedPlaylist)
{
  ExpectRewindDiscEditSaveRejected(
      [](CGameClientDiscModel& model, const std::string&)
      {
        auto discs = model.GetDiscs();
        std::reverse(discs.begin(), discs.end());
        model.SetDiscs(discs);
        ASSERT_TRUE(model.SetSelectedDiscByIndex(1));
      });
}

TEST_F(TestGameClientDiscs, RewindSavestateRejectsEmptyPlaylist)
{
  ExpectRewindDiscEditSaveRejected(
      [](CGameClientDiscModel& model, const std::string&)
      {
        while (!model.Empty())
          ASSERT_TRUE(model.EraseDiscByIndex(0));
        model.SetEjected(true);
      });
}

TEST_F(TestGameClientDiscs, RewindSavestateAllowsKnownMediaOnlyChanges)
{
  ExpectRewindCursorSaveRoundtrip(
      [](CGameClientDiscModel& model, const std::string& extraDisc)
      {
        const auto active = model;
        model.RememberDiscPath(extraDisc);
        ASSERT_TRUE(model == active);
      });
}

TEST_F(TestGameClientDiscs, RewindSavestateWithMismatchedMediaFailsCoreValidation)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  m_core.rewindFramePath = m_core.slots[0];
  StartPlaying();
  CreatePlayback();
  m_playback->RewindEvent();
  const auto historical = m_client->Discs().GetDiscs();
  const auto validPath = CaptureSavestate();
  RETRO::CSavestateFlatBuffer saved;
  ASSERT_TRUE(RETRO::CSavestateDatabase().GetSavestate(validPath, saved));
  ASSERT_TRUE(saved.PrepareMemoryData(1));
  ASSERT_EQ(saved.GetMemoryData()[0], 2);
  RETRO::CSavestateFlatBuffer mixed;
  *mixed.GetMemoryBuffer(1) = saved.GetMemoryData()[0];
  mixed.SetTimestampFrames(saved.TimestampFrames());
  auto edited = historical;
  ASSERT_TRUE(edited.SetSelectedDiscByIndex(1));
  // A permissive core would hide a save that pairs cursor memory with later media edits.
  mixed.SetDiscState(edited.GetState());
  mixed.Finalize();
  const auto mixedPath = CreateFile(".sav");
  ASSERT_TRUE(RETRO::CSavestateDatabase().AddSavestate(mixedPath, m_client->GetGamePath(), mixed));
  const auto calls = m_core.deserializeCalls;

  EXPECT_FALSE(m_playback->LoadSavestate(mixedPath));

  EXPECT_GT(m_core.deserializeCalls, calls);
  EXPECT_TRUE(RestoreFailureLatched());
  ASSERT_TRUE(m_playback->LoadSavestate(validPath));
  EXPECT_EQ(m_core.machineFrame, 2);
  EXPECT_EQ(TestMember<PlaybackTotalFrames>(*m_playback), 2U);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, ForwardResumeCommitsRewindPreviewWithoutRunningAgain)
{
  ExpectRewindPreviewResume(false);
}

TEST_F(TestGameClientDiscs, ForwardResumeCommitsRewindPreviewDiscAndTrayChanges)
{
  ExpectRewindPreviewResume(true);
}

TEST_F(TestGameClientDiscs, GameplayStartupPersistsKnownMediaBeforeAnyDiscMutation)
{
  const auto disc1 = CreateFile(".chd");
  const auto disc2 = CreateFile(".chd");
  const auto playlist = CreateFile(".m3u", disc1 + "\n" + disc2 + "\n");
  m_core.slots = {disc1, disc2};
  m_client->Discs().Initialize(playlist);
  auto historical = m_client->Discs().GetDiscs();
  ASSERT_TRUE(historical.SetSelectedDiscByIndex(1));
  const auto state = historical.GetState();
  ASSERT_TRUE(m_client->GetGamePath().empty());
  ASSERT_FALSE(XFILE::CFile::Exists(CGameClientDiscXML::GetXMLPath(playlist)));

  ASSERT_TRUE(InitializeGameplay(playlist));

  ASSERT_TRUE(XFILE::CFile::Exists(CGameClientDiscXML::GetXMLPath(playlist)));
  CXBMCTinyXML2 xml;
  ASSERT_TRUE(xml.LoadFile(CGameClientDiscXML::GetXMLPath(playlist)));
  ASSERT_NE(xml.RootElement()->FirstChildElement("knownmedia"), nullptr);
  CGameClientDiscModel persisted;
  ASSERT_TRUE(CGameClientDiscXML().Load(playlist, persisted));
  EXPECT_EQ(persisted.GetKnownDiscPaths(), (std::vector<std::string>{disc1, disc2}));
  XFILE::CFile source;
  ASSERT_TRUE(source.OpenForWrite(playlist, true));
  const auto shortened = disc1 + "\n";
  ASSERT_EQ(source.Write(shortened.data(), shortened.size()), static_cast<ssize_t>(shortened.size()));
  source.Close();
  ASSERT_TRUE(XFILE::CFile::Exists(disc2));
  CGameClientDiscModel sourceOnly;
  ASSERT_TRUE(CGameClientDiscM3U().Load(playlist, sourceOnly));
  CGameClientDiscModel resolved;
  EXPECT_FALSE(sourceOnly.ResolveState(state, resolved));
  ASSERT_TRUE(persisted.ResolveState(state, resolved));
  EXPECT_EQ(resolved.GetPathByIndex(1), disc2);
  EXPECT_EQ(resolved.GetSelectedDiscIndex(), 1U);
}

TEST_F(TestGameClientDiscs, GameplayStartupKeepsEmptyPlaylistAndSourceMediaHistory)
{
  const auto disc1 = CreateFile(".chd");
  const auto disc2 = CreateFile(".chd");
  const auto playlist = CreateFile(".m3u", disc1 + "\n" + disc2 + "\n");
  CGameClientDiscModel empty;
  empty.SetEjected(true);
  ASSERT_TRUE(CGameClientDiscXML().Save(playlist, empty));
  m_core.slots = {disc1, disc2};
  m_client->Discs().Initialize(playlist);
  ASSERT_TRUE(m_client->Discs().HasPersistedState());

  ASSERT_TRUE(InitializeGameplay(playlist));

  CGameClientDiscModel persisted;
  ASSERT_TRUE(CGameClientDiscXML().Load(playlist, persisted));
  EXPECT_TRUE(persisted.Empty());
  EXPECT_TRUE(persisted.IsSelectedNoDisc());
  EXPECT_TRUE(persisted.IsEjected());
  EXPECT_EQ(persisted.GetKnownDiscPaths(), (std::vector<std::string>{disc1, disc2}));
}

TEST_F(TestGameClientDiscs, FailedGameplayStartupDoesNotCreateDiscState)
{
  const auto disc = CreateFile(".chd");
  const auto playlist = CreateFile(".m3u", disc + "\n");
  m_core.slots = {disc};
  m_client->Discs().Initialize(playlist);

  EXPECT_FALSE(InitializeGameplay(playlist, false));

  EXPECT_TRUE(m_client->GetGamePath().empty());
  EXPECT_FALSE(XFILE::CFile::Exists(CGameClientDiscXML::GetXMLPath(playlist)));
  EXPECT_FALSE(XFILE::CFile::Exists(CGameClientDiscM3U::GetM3UPath(playlist)));
}

TEST_F(TestGameClientDiscs, FailedGameplayStartupDoesNotReplaceDiscState)
{
  const auto disc = CreateFile(".chd");
  const auto playlist = CreateFile(".m3u", disc + "\n");
  m_core.slots = {disc};
  CGameClientDiscModel persisted;
  persisted.AddDisc(disc);
  ASSERT_TRUE(CGameClientDiscXML().Save(playlist, persisted));
  ASSERT_TRUE(CGameClientDiscM3U().Save(playlist, persisted));
  const auto xml = ReadFile(CGameClientDiscXML::GetXMLPath(playlist));
  const auto m3u = ReadFile(CGameClientDiscM3U::GetM3UPath(playlist));
  m_client->Discs().Initialize(playlist);

  EXPECT_FALSE(InitializeGameplay(playlist, false));

  EXPECT_TRUE(m_client->GetGamePath().empty());
  EXPECT_EQ(ReadFile(CGameClientDiscXML::GetXMLPath(playlist)), xml);
  EXPECT_EQ(ReadFile(CGameClientDiscM3U::GetM3UPath(playlist)), m3u);
}

TEST_F(TestGameClientDiscs, ForwardFramesCaptureLiveDiscHistoryWithoutPersisting)
{
  m_core.slots = {CreateFile(".chd"), CreateFile(".chd")};
  StartPlaying();
  CreatePlayback();
  const auto persisted = SnapshotDiscFiles();
  m_core.changeDiscOnRun = true;
  auto expected = m_client->Discs().GetDiscs().GetState();
  expected.selectedSlot = 1;
  expected.trayEjected = true;
  const auto queries = m_core.modelQueries;

  m_playback->FrameEvent();

  const auto& memory = TestMember<PlaybackMemory>(*m_playback);
  EXPECT_EQ(*memory->CurrentFrame(), 4);
  EXPECT_EQ(memory->GetFrameCounter(), 4U);
  const auto* discs = TestMember<PlaybackDiscs>(*m_playback).Get(memory->GetDiscStateID());
  ASSERT_NE(discs, nullptr);
  EXPECT_EQ(discs->GetState(), expected);
  EXPECT_EQ(m_core.modelQueries, queries + 1);
  m_playback->FrameEvent();
  EXPECT_EQ(m_core.modelQueries, queries + 2);
  ExpectDiscFilesUnchanged(persisted);

  m_playback->SeekTimeMs(4000);
  EXPECT_EQ(m_core.machineFrame, 4);
  EXPECT_EQ(m_core.selected, 1U);
  EXPECT_TRUE(m_core.ejected);
  m_playback->SeekTimeMs(3000);
  EXPECT_EQ(m_core.machineFrame, 3);
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_FALSE(m_core.ejected);
  m_playback->SeekTimeMs(4000);
  EXPECT_EQ(m_core.machineFrame, 4);
  EXPECT_EQ(m_core.selected, 1U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, SavestateRejectsKnownMediaUnsupportedByActiveCore)
{
  ExpectUnsupportedSavestateRejected(true);
}

TEST_F(TestGameClientDiscs, SavestateRejectsUnsupportedUnselectedMedia)
{
  ExpectUnsupportedSavestateRejected(false);
}

TEST_F(TestGameClientDiscs, SavestateLoadsSupportedHistoryAndSkipsRemovedSlots)
{
  const auto disc1 = CreateFile(".chd");
  const auto disc2 = CreateFile(".chd");
  const auto unsupported = CreateFile(".unsupported");
  m_core.slots = {disc1};
  StartPlaying();
  CreatePlayback();
  auto current = m_client->Discs().GetDiscs();
  current.RememberDiscPath(disc2);
  current.RememberDiscPath(unsupported);
  m_client->Discs().SetDiscModel(current);
  CGameClientDiscModel historical;
  historical.AddDisc(unsupported);
  historical.AddDisc(disc2);
  ASSERT_TRUE(historical.RemoveDiscByIndex(0));
  ASSERT_TRUE(historical.SetSelectedDiscByIndex(1));

  ASSERT_TRUE(m_playback->LoadSavestate(WriteSavestate(&historical)));

  EXPECT_EQ(m_core.deserializeCalls, 1U);
  EXPECT_EQ(m_core.machineFrame, 1);
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"", disc2}));
  EXPECT_EQ(m_core.selected, 1U);
  EXPECT_FALSE(m_core.ejected);
  EXPECT_EQ(m_client->Discs().GetDiscs().GetState(), historical.GetState());
  EXPECT_FALSE(RestoreFailureLatched());
}

TEST_F(TestGameClientDiscs, FailedExcessSlotReuseDoesNotClearExistingTail)
{
  ExpectFailedDiscAddition(false, true);
}

TEST_F(TestGameClientDiscs, FailedExcessSlotReuseDoesNotCompactExistingTail)
{
  ExpectFailedDiscAddition(true, true);
}

TEST_F(TestGameClientDiscs, FailedNewSlotReplacementClearsCreatedSlot)
{
  ExpectFailedDiscAddition(false, false);
}

TEST_F(TestGameClientDiscs, FailedNewSlotReplacementRemovesCreatedSlot)
{
  ExpectFailedDiscAddition(true, false);
}
