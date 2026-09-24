/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "storage/MediaManager.h"

#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)

#include "ServiceBroker.h"
#include "jobs/JobManager.h"
#include "messaging/ApplicationMessenger.h"
#include "storage/cdioSupport.h"
#include "storage/discs/IDiscDriveHandler.h"
#include "threads/Event.h"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include <gtest/gtest.h>

namespace
{
class FakeDiscDriveHandler : public IDiscDriveHandler
{
public:
  ~FakeDiscDriveHandler() override = default;

  DriveState GetDriveState(const std::string& devicePath) override
  {
    ++probes;
    if (const auto hook{onProbe}; hook)
      hook();
    return state;
  }
  TrayState GetTrayState(const std::string& devicePath) override { return TrayState::UNDEFINED; }
  bool EjectDriveTray(const std::string& devicePath) override { return true; }
  bool CloseDriveTray(const std::string& devicePath) override { return true; }
  bool ToggleDriveTray(const std::string& devicePath) override { return true; }

  DriveState state{DriveState::CLOSED_MEDIA_PRESENT};
  int probes{0};
  /*! Runs inside the probe, ie. while RefreshDriveStatus() holds no lock */
  std::function<void()> onProbe;
};
} // namespace

class TestMediaManager : public testing::Test
{
protected:
  using Clock = std::chrono::steady_clock;
  static constexpr const char* DEVICE_PATH = "?:\\kodi-test-nonexistent-optical-drive";
  static constexpr const char* SECOND_DEVICE_PATH = "!:\\kodi-test-second-optical-drive";

  void SetUp() override
  {
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    m_manager.m_bOpticalDrivePresent = true;
    m_manager.m_strFirstAvailDrive = DEVICE_PATH;
    m_manager.m_platformDiscDriveHander = m_drive;
  }

  void TearDown() override
  {
    ReleaseProbe();
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::UnregisterJobManager();
    m_manager.RemoveCdInfo(DEVICE_PATH);
    m_manager.RemoveCdInfo(SECOND_DEVICE_PATH);
  }

  uint64_t CdInfoGeneration() const { return m_manager.m_cdInfoGeneration; }
  uint64_t DiscInfoGeneration() const { return m_manager.m_discInfoGeneration; }

  std::shared_ptr<MEDIA_DETECT::CCdInfo> StoreCdInfo(std::unique_ptr<MEDIA_DETECT::CCdInfo> info,
                                                     uint64_t generation)
  {
    return m_manager.CacheCdInfo(DEVICE_PATH, std::move(info), generation);
  }

  void SeedCachedFailure(const std::string& devicePath = DEVICE_PATH,
                         Clock::time_point expires = Clock::time_point::max())
  {
    m_manager.m_cdInfoUnavailable[devicePath] = expires;
  }

  bool HasCachedFailure(const std::string& devicePath = DEVICE_PATH) const
  {
    return m_manager.m_cdInfoUnavailable.contains(devicePath);
  }
  bool HasCachedCdInfo() const { return m_manager.m_mapCdInfo.contains(DEVICE_PATH); }
  auto FailureExpiry() const { return m_manager.m_cdInfoUnavailable.at(DEVICE_PATH); }

  void StoreDiscInfo(const UTILS::DISCS::DiscInfo& info,
                     const std::string& label,
                     uint64_t generation,
                     const std::string& mediaPath = DEVICE_PATH)
  {
    CMediaManager::DiscInfoCacheEntry entry;
    entry.info = info;
    entry.label = label;
    m_manager.CacheDiscInfo(mediaPath, entry, generation);
  }

  void SeedDiscInfo(const std::string& label, Clock::time_point expires)
  {
    CMediaManager::DiscInfoCacheEntry entry;
    entry.label = label;
    entry.expires = expires;
    m_manager.m_mapDiscInfo[DEVICE_PATH] = entry;
  }

  bool HasCachedDiscInfo(const std::string& mediaPath = DEVICE_PATH) const
  {
    return m_manager.m_mapDiscInfo.contains(mediaPath);
  }
  auto ReadCachedDiscInfo() { return m_manager.GetCachedDiscInfo(DEVICE_PATH); }
  auto PollCachedDiscInfo() { return m_manager.GetCachedDiscInfo(DEVICE_PATH, true); }
  std::string CachedDiscLabel() const { return m_manager.m_mapDiscInfo.at(DEVICE_PATH).label; }
#ifdef HAVE_LIBBLURAY
  auto PlaylistStatus() const { return m_manager.m_hasBlurayPlaylist; }
#endif

  void SetFirstDrive(const std::string& devicePath) { m_manager.m_strFirstAvailDrive = devicePath; }

  /*! Replace the TOC reader, so no hardware is touched and every read is counted */
  void ReadTocWith(std::function<std::unique_ptr<MEDIA_DETECT::CCdInfo>()> reader)
  {
    m_manager.m_readToc = [this, reader = std::move(reader)](const std::string& devicePath)
    {
      ++m_tocReads;
      return reader();
    };
  }
  static std::unique_ptr<MEDIA_DETECT::CCdInfo> NoToc() { return nullptr; }
  static std::unique_ptr<MEDIA_DETECT::CCdInfo> SomeToc()
  {
    return std::make_unique<MEDIA_DETECT::CCdInfo>();
  }
  int m_tocReads{0};

  /*! The drive state cache is keyed by the device form of the path */
  std::string DeviceKey(const std::string& devicePath)
  {
    return m_manager.TranslateDevicePath(devicePath, true);
  }
  bool HasCachedDriveStatus(const std::string& devicePath)
  {
    return m_manager.m_driveStatusCache.contains(DeviceKey(devicePath));
  }
  auto DriveStatusExpiry(const std::string& devicePath)
  {
    return m_manager.m_driveStatusCache.at(DeviceKey(devicePath)).expires;
  }
  void ExpireDriveStatus(const std::string& devicePath)
  {
    m_manager.m_driveStatusCache.at(DeviceKey(devicePath)).expires =
        Clock::now() - std::chrono::seconds(1);
  }
  bool IsDriveStatusExpired(const std::string& devicePath)
  {
    return DriveStatusExpiry(devicePath) <= Clock::now();
  }
  bool IsRefreshing(const std::string& devicePath)
  {
    const std::string key{DeviceKey(devicePath)};
    std::unique_lock lock(m_manager.m_driveStatusSection);
    return m_manager.m_refreshingDrives.contains(key);
  }
  /*! Let the job queued for a cache miss ask the drive, so the cache can then be inspected */
  void WaitForRefresh()
  {
    const auto deadline{Clock::now() + std::chrono::seconds(10)};
    while (Clock::now() < deadline)
    {
      {
        std::unique_lock lock(m_manager.m_driveStatusSection);
        if (m_manager.m_refreshingDrives.empty())
          return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    FAIL() << "The refresh never finished";
  }
  /*! Let the job queued for a polling miss read the disc, so the cache can then be inspected */
  void WaitForFill()
  {
    const auto deadline{Clock::now() + std::chrono::seconds(10)};
    while (Clock::now() < deadline)
    {
      bool filling{false};
      {
        std::unique_lock lock(m_manager.m_muAutoSource);
        filling = !m_manager.m_cdInfoFilling.empty();
      }
      {
        std::unique_lock lock(m_manager.m_discInfoSection);
        filling = filling || !m_manager.m_discInfoFilling.empty();
      }
      if (!filling)
        return;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    FAIL() << "The read never finished";
  }
  /*! Run what a job posted to the application thread, which nothing pumps in the test binary,
      until the drive reaches a generation */
  void WaitForGeneration(const std::string& devicePath, uint64_t generation)
  {
    const auto deadline{Clock::now() + std::chrono::seconds(10)};
    while (m_manager.DiscGeneration(devicePath) != generation && Clock::now() < deadline)
    {
      CServiceBroker::GetAppMessenger()->ProcessMessages();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  /*! Hold the next probe in the drive until ReleaseProbe(), so in-flight state can be asserted */
  void HoldProbe()
  {
    m_drive->onProbe = [this] { m_probeReleased.Wait(); };
  }
  void ReleaseProbe() { m_probeReleased.Set(); }
  CEvent m_probeReleased{true};

  std::shared_ptr<FakeDiscDriveHandler> m_drive{std::make_shared<FakeDiscDriveHandler>()};
  CMediaManager m_manager;
};

// Disc identification cache

TEST_F(TestMediaManager, FailedIdentificationWithVolumeLabelExpiresSlowly)
{
  const auto before{Clock::now()};
  StoreDiscInfo({}, "Volume label", DiscInfoGeneration());

  const auto entry{ReadCachedDiscInfo()};
  EXPECT_EQ(entry.label, "Volume label");
  EXPECT_EQ(entry.info.type, UTILS::DISCS::DiscType::UNKNOWN);
  EXPECT_TRUE(entry.info.name.empty());
  EXPECT_GE(entry.expires, before + std::chrono::seconds(60));
  EXPECT_LE(entry.expires, Clock::now() + std::chrono::seconds(60));
}

TEST_F(TestMediaManager, SuccessfulIdentificationRemainsCached)
{
  UTILS::DISCS::DiscInfo info;
  info.type = UTILS::DISCS::DiscType::DVD;
  info.name = "Disc title";
  info.serial = "Disc serial";
  StoreDiscInfo(info, info.name, DiscInfoGeneration());

  const auto entry{ReadCachedDiscInfo()};
  EXPECT_EQ(entry.info.name, info.name);
  EXPECT_EQ(entry.info.serial, info.serial);
  EXPECT_EQ(entry.expires, Clock::time_point::max());
}

TEST_F(TestMediaManager, UnlabelledDiscExpiresQuickly)
{
  const auto before{Clock::now()};
  UTILS::DISCS::DiscInfo info;
  info.type = UTILS::DISCS::DiscType::DVD;
  StoreDiscInfo(info, "", DiscInfoGeneration());

  const auto entry{ReadCachedDiscInfo()};
  EXPECT_GE(entry.expires, before + std::chrono::seconds(5));
  EXPECT_LE(entry.expires, Clock::now() + std::chrono::seconds(5));
}

TEST_F(TestMediaManager, ExpiredDiscInfoIsReadAgain)
{
  SeedDiscInfo("Stale label", Clock::now() - std::chrono::seconds(1));

  // The device does not exist, so a fresh read yields no label at all
  EXPECT_NE(ReadCachedDiscInfo().label, "Stale label");
}

TEST_F(TestMediaManager, PollingAnswersFromExpiredDiscInfoWhileAJobReadsIt)
{
  SeedDiscInfo("Stale label", Clock::now() - std::chrono::seconds(1));

  EXPECT_EQ(PollCachedDiscInfo().label, "Stale label");
  WaitForFill();
  EXPECT_NE(CachedDiscLabel(), "Stale label");
}

TEST_F(TestMediaManager, ResetRejectsInFlightDiscIdentification)
{
  const auto generation{DiscInfoGeneration()};
  m_manager.ResetDriveCaches(DEVICE_PATH);
  StoreDiscInfo({}, "Old volume label", generation);

  EXPECT_FALSE(HasCachedDiscInfo());
}

#ifdef HAVE_LIBBLURAY
TEST_F(TestMediaManager, UnidentifiedDiscDoesNotCachePlaylistAbsence)
{
  StoreDiscInfo({}, "Volume label", DiscInfoGeneration());
  EXPECT_FALSE(m_manager.HasMediaBlurayPlaylist(DEVICE_PATH));
  EXPECT_EQ(PlaylistStatus(), CMediaManager::HasBlurayPlaylist::UNKNOWN);

  UTILS::DISCS::DiscInfo info;
  info.type = UTILS::DISCS::DiscType::DVD;
  StoreDiscInfo(info, "DVD title", DiscInfoGeneration());
  EXPECT_FALSE(m_manager.HasMediaBlurayPlaylist(DEVICE_PATH));
  EXPECT_EQ(PlaylistStatus(), CMediaManager::HasBlurayPlaylist::NO);
}
#endif

// TOC cache

TEST_F(TestMediaManager, OnlyPollingReusesCachedTocFailure)
{
  ReadTocWith(NoToc);
  SeedCachedFailure();
  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH, true), nullptr);
  EXPECT_EQ(m_manager.IsAudio(DEVICE_PATH, true), false);
  EXPECT_EQ(m_tocReads, 0);
  EXPECT_EQ(FailureExpiry(), Clock::time_point::max());

  // Explicit actions read the disc again and refresh the failure
  const auto before{Clock::now()};
  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH), nullptr);
  EXPECT_EQ(m_manager.IsAudio(DEVICE_PATH), false);
  EXPECT_EQ(m_tocReads, 2);
  EXPECT_GE(FailureExpiry(), before + std::chrono::seconds(30));
  EXPECT_LE(FailureExpiry(), Clock::now() + std::chrono::seconds(30));
}

TEST_F(TestMediaManager, PollingLeavesTheTocReadToAJob)
{
  std::thread::id reader;
  ReadTocWith(
      [&reader]
      {
        reader = std::this_thread::get_id();
        return SomeToc();
      });

  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH, true, true), nullptr);
  WaitForFill();
  EXPECT_EQ(m_tocReads, 1);
  EXPECT_NE(reader, std::this_thread::get_id());
  EXPECT_NE(m_manager.GetCdInfo(DEVICE_PATH, true, true), nullptr);
  EXPECT_EQ(m_tocReads, 1);
}

TEST_F(TestMediaManager, ExpiredTocFailureIsReadAgain)
{
  ReadTocWith(NoToc);
  SeedCachedFailure(DEVICE_PATH, Clock::now() - std::chrono::seconds(1));

  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH, true, true), nullptr);
  WaitForFill();
  EXPECT_EQ(m_tocReads, 1);
  EXPECT_GT(FailureExpiry(), Clock::now());
}

TEST_F(TestMediaManager, GenuineTocFailureIsNotRetried)
{
  ReadTocWith(NoToc);

  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH), nullptr);
  EXPECT_EQ(m_tocReads, 1);
  EXPECT_TRUE(HasCachedFailure());
}

TEST_F(TestMediaManager, InvalidatedTocReadIsRetriedOnce)
{
  // A storage event arrives during the first read only
  ReadTocWith(
      [this]
      {
        if (m_tocReads == 1)
          m_manager.ResetDriveCaches(DEVICE_PATH);
        return SomeToc();
      });

  const auto info{m_manager.GetCdInfo(DEVICE_PATH)};
  EXPECT_NE(info, nullptr);
  EXPECT_EQ(m_tocReads, 2);
  EXPECT_TRUE(HasCachedCdInfo());
  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH), info);
  EXPECT_EQ(m_tocReads, 2);
}

TEST_F(TestMediaManager, TocRetryStopsAfterTwoAttempts)
{
  // Every read is invalidated while in flight
  ReadTocWith(
      [this]
      {
        m_manager.ResetDriveCaches(DEVICE_PATH);
        return SomeToc();
      });

  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH), nullptr);
  EXPECT_EQ(m_tocReads, 2);
  EXPECT_FALSE(HasCachedCdInfo());
  EXPECT_FALSE(HasCachedFailure());
}

TEST_F(TestMediaManager, ResetRejectsInFlightSuccessfulTocRead)
{
  const auto generation{CdInfoGeneration()};
  m_manager.ResetDriveCaches(DEVICE_PATH);

  EXPECT_EQ(StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), generation), nullptr);
  EXPECT_FALSE(HasCachedCdInfo());
  EXPECT_FALSE(HasCachedFailure());
}

TEST_F(TestMediaManager, ResetRejectsInFlightFailedTocRead)
{
  const auto generation{CdInfoGeneration()};
  m_manager.ResetDriveCaches(DEVICE_PATH);

  EXPECT_EQ(StoreCdInfo(nullptr, generation), nullptr);
  EXPECT_FALSE(HasCachedCdInfo());
  EXPECT_FALSE(HasCachedFailure());
}

TEST_F(TestMediaManager, RemovalClearsFailureAndInvalidatesInFlightRead)
{
  SeedCachedFailure();
  const auto generation{CdInfoGeneration()};
  m_manager.RemoveCdInfo(DEVICE_PATH);

  EXPECT_FALSE(HasCachedFailure());
  EXPECT_EQ(StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), generation), nullptr);
  EXPECT_FALSE(HasCachedCdInfo());
}

TEST_F(TestMediaManager, SuccessfulTocReadClearsCachedFailure)
{
  SeedCachedFailure();
  const auto info{StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), CdInfoGeneration())};

  ASSERT_NE(info, nullptr);
  EXPECT_FALSE(HasCachedFailure());
  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH, true), info);
  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH), info);
}

TEST_F(TestMediaManager, ConcurrentTocReadsKeepPublishedSuccess)
{
  const auto first{StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), CdInfoGeneration())};
  ASSERT_NE(first, nullptr);

  EXPECT_EQ(StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), CdInfoGeneration()), first);
  EXPECT_EQ(StoreCdInfo(nullptr, CdInfoGeneration()), first);
  EXPECT_FALSE(HasCachedFailure());
}

// Drive state cache

TEST_F(TestMediaManager, UnknownDriveIsAnsweredWithoutWaiting)
{
  // Nothing is known yet, so the caller gets an answer at once and the drive is asked behind it
  HoldProbe();
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::NOT_READY);
  EXPECT_TRUE(IsRefreshing(DEVICE_PATH));

  ReleaseProbe();
  WaitForRefresh();
  EXPECT_EQ(m_drive->probes, 1);
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
}

TEST_F(TestMediaManager, AskingNowWaitsForTheDrive)
{
  EXPECT_EQ(m_manager.GetDriveStatusNow(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
  EXPECT_EQ(m_drive->probes, 1);
  EXPECT_FALSE(IsRefreshing(DEVICE_PATH));

  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
  EXPECT_EQ(m_drive->probes, 1);
}

TEST_F(TestMediaManager, PresentDiscIsProbedOnce)
{
  const auto before{Clock::now()};
  m_manager.GetDriveStatus(DEVICE_PATH);
  m_manager.GetDriveStatus(DEVICE_PATH);
  WaitForRefresh();
  EXPECT_EQ(m_drive->probes, 1);

  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
  EXPECT_TRUE(m_manager.IsDiscInDrive(DEVICE_PATH));
  EXPECT_EQ(m_drive->probes, 1);

  // A stable state is still confirmed eventually, in case Windows skipped the removal event
  const auto expires{DriveStatusExpiry(DEVICE_PATH)};
  EXPECT_GE(expires, before + std::chrono::seconds(10));
  EXPECT_LE(expires, Clock::now() + std::chrono::seconds(10));
}

TEST_F(TestMediaManager, UnreportedEjectIsNoticedAfterRefresh)
{
  m_manager.GetDriveStatus(DEVICE_PATH);
  WaitForRefresh();
  EXPECT_TRUE(m_manager.IsDiscInDrive(DEVICE_PATH));
  // The generation is keyed by the drive letter, which the test path only yields in its device form
  const uint64_t generation{m_manager.DiscGeneration(DeviceKey(DEVICE_PATH))};

  // The user presses the drive's eject button and no storage event arrives
  m_drive->state = DriveState::CLOSED_NO_MEDIA;
  EXPECT_TRUE(m_manager.IsDiscInDrive(DEVICE_PATH));
  EXPECT_EQ(m_drive->probes, 1);

  // The last state seen is given while the drive is asked again
  ExpireDriveStatus(DEVICE_PATH);
  HoldProbe();
  EXPECT_TRUE(m_manager.IsDiscInDrive(DEVICE_PATH));
  EXPECT_TRUE(IsRefreshing(DEVICE_PATH));

  ReleaseProbe();
  WaitForRefresh();
  EXPECT_FALSE(m_manager.IsDiscInDrive(DEVICE_PATH));
  EXPECT_EQ(m_drive->probes, 2);

  // Seeing the disc go is what stops a job still identifying it
  WaitForGeneration(DeviceKey(DEVICE_PATH), generation + 1);
  EXPECT_TRUE(m_manager.IsDiscCurrent(DeviceKey(DEVICE_PATH), generation + 1));
}

TEST_F(TestMediaManager, EmptyDriveIsReprobedOnlyAfterExpiry)
{
  m_drive->state = DriveState::CLOSED_NO_MEDIA;
  m_manager.GetDriveStatus(DEVICE_PATH);
  WaitForRefresh();
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_NO_MEDIA);
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_NO_MEDIA);
  EXPECT_EQ(m_drive->probes, 1);

  ExpireDriveStatus(DEVICE_PATH);
  m_drive->state = DriveState::CLOSED_MEDIA_PRESENT;
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_NO_MEDIA);
  WaitForRefresh();
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
  EXPECT_EQ(m_drive->probes, 2);
}

TEST_F(TestMediaManager, FailedProbeIsReprobedOnlyAfterExpiry)
{
  m_drive->state = DriveState::NOT_READY;
  m_manager.GetDriveStatus(DEVICE_PATH);
  WaitForRefresh();
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::NOT_READY);
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::NOT_READY);
  EXPECT_EQ(m_drive->probes, 1);

  ExpireDriveStatus(DEVICE_PATH);
  m_manager.GetDriveStatus(DEVICE_PATH);
  WaitForRefresh();
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::NOT_READY);
  EXPECT_EQ(m_drive->probes, 2);
}

TEST_F(TestMediaManager, ResetInvalidatesOnlyThatDrive)
{
  m_manager.GetDriveStatus(DEVICE_PATH);
  m_manager.GetDriveStatus(SECOND_DEVICE_PATH);
  WaitForRefresh();
  EXPECT_EQ(m_drive->probes, 2);

  m_manager.ResetDriveCaches(DEVICE_PATH);
  EXPECT_TRUE(IsDriveStatusExpired(DEVICE_PATH));
  EXPECT_FALSE(IsDriveStatusExpired(SECOND_DEVICE_PATH));

  m_manager.GetDriveStatus(DEVICE_PATH);
  m_manager.GetDriveStatus(SECOND_DEVICE_PATH);
  WaitForRefresh();
  EXPECT_EQ(m_drive->probes, 3);
}

TEST_F(TestMediaManager, ResetWithoutPathClearsEveryCache)
{
  m_manager.GetDriveStatus(DEVICE_PATH);
  m_manager.GetDriveStatus(SECOND_DEVICE_PATH);
  WaitForRefresh();
  StoreDiscInfo({}, "Label", DiscInfoGeneration(), DEVICE_PATH);
  StoreDiscInfo({}, "Label", DiscInfoGeneration(), SECOND_DEVICE_PATH);
  SeedCachedFailure(DEVICE_PATH);
  SeedCachedFailure(SECOND_DEVICE_PATH);

  m_manager.ResetDriveCaches();

  EXPECT_TRUE(IsDriveStatusExpired(DEVICE_PATH));
  EXPECT_TRUE(IsDriveStatusExpired(SECOND_DEVICE_PATH));
  EXPECT_FALSE(HasCachedDiscInfo(DEVICE_PATH));
  EXPECT_FALSE(HasCachedDiscInfo(SECOND_DEVICE_PATH));
  EXPECT_FALSE(HasCachedFailure(DEVICE_PATH));
  EXPECT_FALSE(HasCachedFailure(SECOND_DEVICE_PATH));
}

TEST_F(TestMediaManager, ResetDropsTheCachedTocWhileACallerStillHoldsIt)
{
  const auto info{StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), CdInfoGeneration())};
  ASSERT_NE(info, nullptr);

  m_manager.ResetDriveCaches(DEVICE_PATH);

  EXPECT_FALSE(HasCachedCdInfo());
  EXPECT_EQ(info.use_count(), 1);
}

TEST_F(TestMediaManager, ProbeInvalidatedInFlightIsNotStored)
{
  // A storage event arrives while the hardware is being asked
  m_drive->onProbe = [this] { m_manager.ResetDriveCaches(DEVICE_PATH); };
  m_manager.GetDriveStatus(DEVICE_PATH);
  WaitForRefresh();
  EXPECT_EQ(m_drive->probes, 1);
  EXPECT_FALSE(HasCachedDriveStatus(DEVICE_PATH));

  m_drive->onProbe = nullptr;
  m_manager.GetDriveStatus(DEVICE_PATH);
  WaitForRefresh();
  EXPECT_EQ(m_drive->probes, 2);
  EXPECT_TRUE(HasCachedDriveStatus(DEVICE_PATH));
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
}

// Path normalisation

TEST_F(TestMediaManager, DrivePathsAreNormalised)
{
  SetFirstDrive("\\\\.\\e:");

  EXPECT_EQ(m_manager.TranslateDevicePath("d:"), "D:");
  EXPECT_EQ(m_manager.TranslateDevicePath("d:\\"), "D:");
  EXPECT_EQ(m_manager.TranslateDevicePath("d:", true), "\\\\.\\D:");
  EXPECT_EQ(m_manager.TranslateDevicePath("\\\\.\\d:"), "D:");
  EXPECT_EQ(m_manager.TranslateDevicePath("\\\\.\\d:", true), "\\\\.\\D:");

  // The first drive stands in for an empty path and for the local audio CD
  EXPECT_EQ(m_manager.TranslateDevicePath(""), "E:");
  EXPECT_EQ(m_manager.TranslateDevicePath("", true), "\\\\.\\E:");
  EXPECT_EQ(m_manager.TranslateDevicePath("cdda://local/"), "E:");
}

#endif
