/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "storage/MediaManager.h"

#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)

#include "storage/cdioSupport.h"
#include "storage/discs/IDiscDriveHandler.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
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
    if (onProbe)
      onProbe();
    return state;
  }
  TrayState GetTrayState(const std::string& devicePath) override { return TrayState::UNDEFINED; }
  void EjectDriveTray(const std::string& devicePath) override {}
  void CloseDriveTray(const std::string& devicePath) override {}
  void ToggleDriveTray(const std::string& devicePath) override {}

  DriveState state{DriveState::CLOSED_MEDIA_PRESENT};
  int probes{0};
  /*! Runs inside the probe, ie. while GetDriveStatus() holds no lock */
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
    m_manager.m_bOpticalDrivePresent = true;
    m_manager.m_strFirstAvailDrive = DEVICE_PATH;
    m_manager.m_platformDiscDriveHander = m_drive;
  }

  void TearDown() override
  {
    m_manager.RemoveCdInfo(DEVICE_PATH);
    m_manager.RemoveCdInfo(SECOND_DEVICE_PATH);
  }

  uint64_t CdInfoGeneration() const { return m_manager.m_cdInfoGeneration; }
  uint64_t DiscInfoGeneration() const { return m_manager.m_discInfoGeneration; }

  MEDIA_DETECT::CCdInfo* StoreCdInfo(std::unique_ptr<MEDIA_DETECT::CCdInfo> info,
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

TEST_F(TestMediaManager, ExpiredTocFailureIsReadAgain)
{
  ReadTocWith(NoToc);
  SeedCachedFailure(DEVICE_PATH, Clock::now() - std::chrono::seconds(1));

  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH, true), nullptr);
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

  auto* info{m_manager.GetCdInfo(DEVICE_PATH)};
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
  auto* info{StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), CdInfoGeneration())};

  ASSERT_NE(info, nullptr);
  EXPECT_FALSE(HasCachedFailure());
  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH, true), info);
  EXPECT_EQ(m_manager.GetCdInfo(DEVICE_PATH), info);
}

TEST_F(TestMediaManager, ConcurrentTocReadsKeepPublishedSuccess)
{
  auto* first{StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), CdInfoGeneration())};
  ASSERT_NE(first, nullptr);

  EXPECT_EQ(StoreCdInfo(std::make_unique<MEDIA_DETECT::CCdInfo>(), CdInfoGeneration()), first);
  EXPECT_EQ(StoreCdInfo(nullptr, CdInfoGeneration()), first);
  EXPECT_FALSE(HasCachedFailure());
}

// Drive state cache

TEST_F(TestMediaManager, PresentDiscIsProbedOnce)
{
  const auto before{Clock::now()};
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
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
  EXPECT_TRUE(m_manager.IsDiscInDrive(DEVICE_PATH));

  // The user presses the drive's eject button and no storage event arrives
  m_drive->state = DriveState::CLOSED_NO_MEDIA;
  EXPECT_TRUE(m_manager.IsDiscInDrive(DEVICE_PATH));
  EXPECT_EQ(m_drive->probes, 1);

  ExpireDriveStatus(DEVICE_PATH);
  EXPECT_FALSE(m_manager.IsDiscInDrive(DEVICE_PATH));
  EXPECT_EQ(m_drive->probes, 2);
}

TEST_F(TestMediaManager, EmptyDriveIsReprobedOnlyAfterExpiry)
{
  m_drive->state = DriveState::CLOSED_NO_MEDIA;
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_NO_MEDIA);
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_NO_MEDIA);
  EXPECT_EQ(m_drive->probes, 1);

  ExpireDriveStatus(DEVICE_PATH);
  m_drive->state = DriveState::CLOSED_MEDIA_PRESENT;
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
  EXPECT_EQ(m_drive->probes, 2);
}

TEST_F(TestMediaManager, FailedProbeIsReprobedOnlyAfterExpiry)
{
  m_drive->state = DriveState::NOT_READY;
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::NOT_READY);
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::NOT_READY);
  EXPECT_EQ(m_drive->probes, 1);

  ExpireDriveStatus(DEVICE_PATH);
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::NOT_READY);
  EXPECT_EQ(m_drive->probes, 2);
}

TEST_F(TestMediaManager, ResetInvalidatesOnlyThatDrive)
{
  m_manager.GetDriveStatus(DEVICE_PATH);
  m_manager.GetDriveStatus(SECOND_DEVICE_PATH);
  EXPECT_EQ(m_drive->probes, 2);

  m_manager.ResetDriveCaches(DEVICE_PATH);
  EXPECT_FALSE(HasCachedDriveStatus(DEVICE_PATH));
  EXPECT_TRUE(HasCachedDriveStatus(SECOND_DEVICE_PATH));

  m_manager.GetDriveStatus(DEVICE_PATH);
  m_manager.GetDriveStatus(SECOND_DEVICE_PATH);
  EXPECT_EQ(m_drive->probes, 3);
}

TEST_F(TestMediaManager, ResetWithoutPathClearsEveryCache)
{
  m_manager.GetDriveStatus(DEVICE_PATH);
  m_manager.GetDriveStatus(SECOND_DEVICE_PATH);
  StoreDiscInfo({}, "Label", DiscInfoGeneration(), DEVICE_PATH);
  StoreDiscInfo({}, "Label", DiscInfoGeneration(), SECOND_DEVICE_PATH);
  SeedCachedFailure(DEVICE_PATH);
  SeedCachedFailure(SECOND_DEVICE_PATH);

  m_manager.ResetDriveCaches();

  EXPECT_FALSE(HasCachedDriveStatus(DEVICE_PATH));
  EXPECT_FALSE(HasCachedDriveStatus(SECOND_DEVICE_PATH));
  EXPECT_FALSE(HasCachedDiscInfo(DEVICE_PATH));
  EXPECT_FALSE(HasCachedDiscInfo(SECOND_DEVICE_PATH));
  EXPECT_FALSE(HasCachedFailure(DEVICE_PATH));
  EXPECT_FALSE(HasCachedFailure(SECOND_DEVICE_PATH));
}

TEST_F(TestMediaManager, ProbeInvalidatedInFlightIsNotStored)
{
  // A storage event arrives while the hardware is being asked
  m_drive->onProbe = [this] { m_manager.ResetDriveCaches(DEVICE_PATH); };
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
  EXPECT_FALSE(HasCachedDriveStatus(DEVICE_PATH));

  m_drive->onProbe = nullptr;
  EXPECT_EQ(m_manager.GetDriveStatus(DEVICE_PATH), DriveState::CLOSED_MEDIA_PRESENT);
  EXPECT_EQ(m_drive->probes, 2);
  EXPECT_TRUE(HasCachedDriveStatus(DEVICE_PATH));
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
