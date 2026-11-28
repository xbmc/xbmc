/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonBuilder.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "jobs/JobManager.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "windowing/WinSystem.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#if defined(TARGET_POSIX)
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace ADDON;
using namespace std::chrono_literals;

namespace
{
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

// CGUIWindowManager::GetWindow() locks the windowing system's gfx context, so
// a minimal WinSystem must be registered for background installs.
class CTestWinSystem : public CWinSystemBase
{
public:
  bool CreateNewWindow(const std::string&, bool, RESOLUTION_INFO&) override { return true; }
  bool ResizeWindow(int, int, int, int) override { return true; }
  bool SetFullScreen(bool, RESOLUTION_INFO&, bool) override { return true; }
  void Register(IDispResource*) override {}
  void Unregister(IDispResource*) override {}
};

const char* ADDON_XML =
    "<addon id=\"{}\" name=\"{}\" version=\"1.0.0\">"
    "<requires>{}</requires>"
    "<extension point=\"kodi.addon.metadata\"><platform>all</platform></extension>"
    "</addon>";

AddonInfoPtr BuildAddonInfo(const std::string& id, const std::string& deps)
{
  CXBMCTinyXML2 xml;
  const std::string doc = StringUtils::Format(ADDON_XML, id, id, deps);
  if (!xml.Parse(doc))
    return nullptr;
  return CAddonInfoBuilder::Generate(xml.RootElement(), RepositoryDirInfo{});
}

AddonPtr BuildAddon(const std::string& id, const std::string& deps, const std::string& path)
{
  auto info = BuildAddonInfo(id, deps);
  if (!info)
    return nullptr;
  info->SetPath(path);
  return CAddonBuilder::Generate(info, AddonType::UNKNOWN);
}

bool WaitFor(const std::function<bool()>& pred, std::chrono::milliseconds timeout)
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline)
  {
    if (pred())
      return true;
    std::this_thread::sleep_for(10ms);
  }
  return pred();
}
} // namespace

class TestAddonInstaller : public testing::Test
{
protected:
  void SetUp() override
  {
    gui = std::make_unique<TestGUI>();
    CServiceBroker::RegisterWinSystem(&winSystem);
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());

    // Seed repository content so CheckDependencies() accepts the test
    // dependencies as installable without a real repository. The repo must be
    // marked installed+enabled: FindByAddonId only returns content of enabled
    // repositories.
    CAddonDatabase db;
    ASSERT_TRUE(db.Open());
    std::set<std::string, std::less<>> installed{"test.repo"};
    db.SyncInstalled(installed, installed, {});
    db.SetRepoUpdateData("test.repo", {});
    std::vector<AddonInfoPtr> repoAddons;
    for (const char* id : {"test.dep", "test.dep2"})
    {
      CAddonInfoBuilderFromDB builder;
      builder.SetId(id);
      builder.SetVersion(CAddonVersion("1.0.0"));
      repoAddons.push_back(builder.get());
    }
    ASSERT_TRUE(
        db.UpdateRepositoryContent("test.repo", CAddonVersion("1.0.0"), "test", repoAddons));
    db.Close();

    tempFile = XBMC_CREATETEMPFILE("addoninst");
    ASSERT_NE(tempFile, nullptr);
    tempFile->Close();
    workDir = XBMC_TEMPFILEPATH(tempFile) + "_dir";
    std::filesystem::create_directories(workDir);
  }

  void TearDown() override
  {
    auto& installer = CAddonInstaller::GetInstance();
    installer.Cancel("test.parent");
    installer.Cancel("test.dep");
    installer.Cancel("test.dep2");
    installer.Cancel("test.slow");

#if defined(TARGET_POSIX)
    UnblockFifo();
    // The install threads still touch the GUI on their way out; they must be
    // done before it is destroyed below, not just before the fixture members.
    if (depThread.joinable())
      depThread.join();
    if (blockingThread.joinable())
      blockingThread.join();
    if (syncThread.joinable())
      syncThread.join();
#endif

    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::UnregisterJobManager();
    CServiceBroker::UnregisterWinSystem();
    gui.reset();
    std::filesystem::remove_all(workDir);
    if (tempFile)
      XBMC_DELETETEMPFILE(tempFile);
  }

#if defined(TARGET_POSIX)
  // Releases the installs blocked on the fifo so the worker threads can unwind.
  void UnblockFifo()
  {
    if (fifoPath.empty() || fifoReaderDone)
      return;
    // A reader that closes between our open() and write() would raise SIGPIPE
    // and kill the test binary; block it so write() just fails with EPIPE.
    sigset_t sigpipeSet, prevSet;
    sigemptyset(&sigpipeSet);
    sigaddset(&sigpipeSet, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sigpipeSet, &prevSet);
    // A writer only pairs with a reader already waiting in open(2), so keep
    // offering one until the install thread has finished: a reader arriving
    // after a fixed retry budget would block in open() and hang the join.
    while (!fifoReaderDone)
    {
      int fd = ::open(fifoPath.c_str(), O_WRONLY | O_NONBLOCK);
      if (fd >= 0)
      {
        const char data = 'x';
        const ssize_t written = ::write(fd, &data, 1);
        (void)written;
        ::close(fd);
      }
      std::this_thread::sleep_for(10ms);
    }
    // An EPIPE write leaves SIGPIPE pending; consume it before restoring the
    // mask or the restore delivers it and kills the test binary. sigwait only
    // once sigpending confirms a SIGPIPE is there (macOS has no sigtimedwait).
    sigset_t pending;
    if (::sigpending(&pending) == 0 && sigismember(&pending, SIGPIPE) == 1)
    {
      int caught = 0;
      (void)::sigwait(&sigpipeSet, &caught);
    }
    pthread_sigmask(SIG_SETMASK, &prevSet, nullptr);
  }
#endif

  std::string workDir;
  std::string fifoPath;
  XFILE::CFile* tempFile = nullptr;
  std::unique_ptr<TestGUI> gui;
  CTestWinSystem winSystem;
#if defined(TARGET_POSIX)
  std::thread depThread;
  std::thread blockingThread;
  std::thread syncThread;
  std::atomic<bool> fifoReaderDone{true};
#endif
};

#if defined(TARGET_POSIX)
TEST_F(TestAddonInstaller, CancelDuringSyncInstallLeavesNoStaleJob)
{
  auto& installer = CAddonInstaller::GetInstance();

  // The install blocks in DownloadPackage() on a fifo until UnblockFifo(),
  // so Cancel() always lands while the synchronous job is still registered.
  fifoPath = workDir + "/slow.fifo";
  ASSERT_EQ(::mkfifo(fifoPath.c_str(), 0600), 0);
  auto addon = BuildAddon("test.slow", "", "file://" + fifoPath);
  ASSERT_NE(addon, nullptr);

  fifoReaderDone = false;
  syncThread = std::thread(
      [this, addon, &installer]()
      {
        installer.InstallOrUpdateDependency(addon, RepositoryPtr());
        fifoReaderDone = true;
      });

  ASSERT_TRUE(WaitFor([&]() { return installer.HasJob("test.slow"); }, 10s));

  // Cancel while DoWork() is still blocked on the fifo. The synchronous job
  // has no JobManager id, so Cancel() only drops the download job entry.
  EXPECT_TRUE(installer.Cancel("test.slow"));
  EXPECT_FALSE(installer.HasJob("test.slow"));

  UnblockFifo();
  syncThread.join();

  // The install finished after its job entry was cancelled: no stale entry may
  // remain and the installer must not report itself as busy.
  EXPECT_FALSE(installer.HasJob("test.slow"));
  EXPECT_FALSE(installer.IsDownloading());
}
#endif

#if defined(TARGET_POSIX)
TEST_F(TestAddonInstaller, AbortedJobStopsWaitingForDependency)
{
  auto& installer = CAddonInstaller::GetInstance();
  auto& jobManager = *CServiceBroker::GetJobManager();

  // test.dep2 blocks forever in DownloadPackage(): its "zip" source is a fifo
  // that no writer ever opens until TearDown, so its install never finishes
  // and HasJob("test.dep2") stays true.
  fifoPath = workDir + "/dep2.fifo";
  ASSERT_EQ(::mkfifo(fifoPath.c_str(), 0600), 0);
  auto dep2 = BuildAddon("test.dep2", "", "file://" + fifoPath);
  ASSERT_NE(dep2, nullptr);
  fifoReaderDone = false;
  blockingThread = std::thread(
      [this, dep2, &installer]()
      {
        installer.InstallOrUpdateDependency(dep2, RepositoryPtr());
        fifoReaderDone = true;
      });
  ASSERT_TRUE(WaitFor([&]() { return installer.HasJob("test.dep2"); }, 10s));

  // test.dep waits in the dependency loop while test.dep2 "installs".
  const std::string depRequires =
      "<import addon=\"test.dep2\" minversion=\"1.0.0\" version=\"1.0.0\"/>";
  auto dep =
      BuildAddon("test.dep", depRequires, XBMC_REF_FILE_PATH("xbmc/addons/test/test.parent.zip"));
  ASSERT_NE(dep, nullptr);
  depThread = std::thread([dep, &installer]()
                           { installer.InstallOrUpdateDependency(dep, RepositoryPtr()); });
  ASSERT_TRUE(WaitFor([&]() { return installer.HasJob("test.dep"); }, 10s));

  // test.parent installs as a background job; its DoWork waits on test.dep.
  ASSERT_TRUE(installer.InstallFromZip(XBMC_REF_FILE_PATH("xbmc/addons/test/test.parent.zip")));
  ASSERT_TRUE(WaitFor([&]() { return jobManager.IsProcessing(CJob::PRIORITY_DEDICATED); }, 10s));

  // Wait until the parent's install phase begins: the dependency wait loop is
  // only entered after the download phase reports install-type progress, so
  // cancelling now exercises the loop's own cancellation check.
  ASSERT_TRUE(WaitFor(
      [&]()
      {
        unsigned int percent = 0;
        bool downloadFinished = false;
        return installer.GetProgress("test.parent", percent, downloadFinished) && downloadFinished;
      },
      10s));

  // Cancelling the parent must abort its dependency wait promptly instead of
  // spinning until the never-ending test.dep2 install resolves.
  ASSERT_TRUE(installer.Cancel("test.parent"));
  EXPECT_TRUE(WaitFor([&]() { return !jobManager.IsProcessing(CJob::PRIORITY_DEDICATED); }, 10s));
}
#endif
