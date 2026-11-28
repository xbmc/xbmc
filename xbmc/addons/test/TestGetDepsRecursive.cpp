/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <set>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace ADDON;

namespace
{
constexpr const char* REPO_ID = "test.deps.repo";

AddonInfoPtr MakeAddon(const std::string& id, std::vector<DependencyInfo> deps = {})
{
  CAddonInfoBuilderFromDB builder;
  builder.SetId(id);
  builder.SetVersion(CAddonVersion("1.0.0"));
  builder.SetDependencies(std::move(deps));
  return builder.get();
}

DependencyInfo Dep(const std::string& id,
                   const char* versionMin,
                   const char* version,
                   bool optional = false)
{
  return DependencyInfo{id, CAddonVersion(versionMin), CAddonVersion(version), optional};
}
} // namespace

class TestGetDepsRecursive : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_TRUE(m_database.Open());

    std::set<std::string, std::less<>> installed{REPO_ID};
    m_database.SyncInstalled(installed, installed, {});
    m_database.SetRepoUpdateData(REPO_ID, {});
  }

  void TearDown() override { m_database.Close(); }

  void PublishRepoContent(std::vector<AddonInfoPtr> addons)
  {
    ASSERT_TRUE(m_database.UpdateRepositoryContent(REPO_ID, CAddonVersion("1.0.0"), "test",
                                                   std::move(addons)));
  }

  CAddonDatabase m_database;
};

TEST_F(TestGetDepsRecursive, CycleTerminates)
{
  PublishRepoContent({MakeAddon("cycle.a", {Dep("cycle.b", "", "1.0.0")}),
                      MakeAddon("cycle.b", {Dep("cycle.a", "", "1.0.0")})});

  // On unfixed code this loops forever; run it on a thread so the test can fail
  // instead of hanging the suite. The result outlives the test on the timeout
  // path, where the detached worker may still be running.
  struct Result
  {
    std::atomic<bool> done{false};
    std::vector<DependencyInfo> deps;
  };
  const auto result = std::make_shared<Result>();
  std::thread worker(
      [result]()
      {
        result->deps = CServiceBroker::GetAddonMgr().GetDepsRecursive(
            "cycle.a", OnlyEnabledRootAddon::CHOICE_NO);
        result->done = true;
      });
  worker.detach();

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (!result->done && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  ASSERT_TRUE(result->done);
  EXPECT_NE(result->deps.end(), std::ranges::find_if(result->deps, [](const DependencyInfo& d)
                                                     { return d.id == "cycle.b"; }));
}

TEST_F(TestGetDepsRecursive, DiamondKeepsHighestVersionMinAndRequired)
{
  PublishRepoContent(
      {MakeAddon("diamond.root", {Dep("x.dep", "", "1.0.0"), Dep("y.dep", "", "1.0.0")}),
       MakeAddon("x.dep", {Dep("shared.dep", "1.0.0", "1.0.0")}),
       MakeAddon("y.dep", {Dep("shared.dep", "2.0.0", "1.1.0", /*optional=*/true)}),
       MakeAddon("shared.dep")});

  const auto deps = CServiceBroker::GetAddonMgr().GetDepsRecursive("diamond.root",
                                                                   OnlyEnabledRootAddon::CHOICE_NO);

  const auto it =
      std::ranges::find_if(deps, [](const DependencyInfo& d) { return d.id == "shared.dep"; });
  ASSERT_NE(deps.end(), it);
  EXPECT_EQ("1.1.0", it->version.asString());
  EXPECT_EQ("2.0.0", it->versionMin.asString());
  // required through x.dep even though y.dep marks it optional
  EXPECT_FALSE(it->optional);
}
