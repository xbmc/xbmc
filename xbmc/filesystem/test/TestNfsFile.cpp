/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/NFSFile.h"
#include "test/TestUtils.h"

#include <atomic>
#include <chrono>
#include <errno.h>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::ValuesIn;

using namespace std::chrono_literals;

struct SplitPath
{
  std::string url;
  std::string exportPath;
  std::string relativePath;
  bool expectedResultExport;
  bool expectedResultPath;
} g_TestData[] = {
                   {"nfs://192.168.0.1:2049/srv/test/tvmedia/foo.txt", "/srv/test", "//tvmedia/foo.txt", true, true},
                   {"nfs://192.168.0.1/srv/test/tv/media/foo.txt", "/srv/test/tv", "//media/foo.txt", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tvmedia", "/srv/test", "//tvmedia", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tvmedia/", "/srv/test", "//tvmedia/", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tv/media", "/srv/test/tv", "//media", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tv/media/", "/srv/test/tv", "//media/", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tv", "/srv/test/tv", "//", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/", "/srv/test", "//", true, true},
                   {"nfs://192.168.0.1:2049/", "/", "//", true, true},
                   {"nfs://192.168.0.1:2049/notexported/foo.txt", "/", "//notexported/foo.txt", true, true},

                   {"nfs://192.168.0.1:2049/notexported/foo.txt", "/notexported", "//foo.txt", false, false},
                 };

class TestNfs : public Test,
                public WithParamInterface<SplitPath>
{
};

class ExportList
{
  public:
    std::list<std::string> data;

    ExportList()
    {
      data.emplace_back("/srv/test");
      data.emplace_back("/srv/test/tv");
      data.emplace_back("/");
      data.sort();
      data.reverse();
    }
};

static ExportList exportList;

TEST_P(TestNfs, splitUrlIntoExportAndPath)
{
  CURL url(GetParam().url);
  std::string exportPath;
  std::string relativePath;
  gNfsConnection.splitUrlIntoExportAndPath(url, exportPath, relativePath, exportList.data);

  if (GetParam().expectedResultExport)
    EXPECT_STREQ(GetParam().exportPath.c_str(), exportPath.c_str());
  else
    EXPECT_STRNE(GetParam().exportPath.c_str(), exportPath.c_str());

  if (GetParam().expectedResultPath)
    EXPECT_STREQ(GetParam().relativePath.c_str(), relativePath.c_str());
  else
    EXPECT_STRNE(GetParam().relativePath.c_str(), relativePath.c_str());
}

INSTANTIATE_TEST_SUITE_P(NfsFile, TestNfs, ValuesIn(g_TestData));

namespace
{
constexpr unsigned int MAX_CONTEXTS = 4;
constexpr auto KEY = "server/export";

class TestNfsContextPool : public Test
{
protected:
  CNfsContextLease Checkout(const std::string& key = KEY, unsigned int maxContexts = MAX_CONTEXTS)
  {
    return m_pool.Checkout(key, maxContexts,
                           [this, &key]
                           {
                             m_created++;
                             return std::make_shared<CNfsContext>(key, "server", "/export",
                                                                  nullptr);
                           });
  }

  // Checks out a context on each of count threads at once, and holds them all.
  std::vector<CNfsContextLease> CheckoutConcurrently(unsigned int count)
  {
    std::vector<CNfsContextLease> leases(count);
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < count; ++i)
      threads.emplace_back([this, &leases, i] { leases[i] = Checkout(); });
    for (auto& thread : threads)
      thread.join();
    return leases;
  }

  static std::map<CNfsContext*, unsigned int> CountUsers(
      const std::vector<CNfsContextLease>& leases)
  {
    std::map<CNfsContext*, unsigned int> users;
    for (const auto& lease : leases)
      users[&*lease]++;
    return users;
  }

  CNfsContextPool m_pool;
  std::atomic<unsigned int> m_created{0};
};
} // namespace

TEST_F(TestNfsContextPool, ReusesContextWithoutUsers)
{
  CNfsContext* first = nullptr;
  {
    const CNfsContextLease lease = Checkout();
    first = &*lease;
  }
  const CNfsContextLease lease = Checkout();
  EXPECT_EQ(first, &*lease);
  EXPECT_EQ(1u, m_created);
}

TEST_F(TestNfsContextPool, ConcurrentCheckoutsTakeDistinctContexts)
{
  for (int round = 0; round < 100; ++round)
  {
    const auto leases = CheckoutConcurrently(MAX_CONTEXTS);
    EXPECT_EQ(MAX_CONTEXTS, CountUsers(leases).size());
  }
  EXPECT_EQ(MAX_CONTEXTS, m_created);
}

TEST_F(TestNfsContextPool, SaturatedPoolSharesContextsEvenly)
{
  const auto leases = CheckoutConcurrently(MAX_CONTEXTS * 3);
  const auto users = CountUsers(leases);
  EXPECT_EQ(MAX_CONTEXTS, users.size());
  for (const auto& [context, count] : users)
    EXPECT_EQ(3u, count);
  EXPECT_EQ(MAX_CONTEXTS, m_created);
}

TEST_F(TestNfsContextPool, ExportsHaveSeparateContexts)
{
  const CNfsContextLease first = Checkout("server/one", 1);
  const CNfsContextLease second = Checkout("server/two", 1);
  EXPECT_NE(&*first, &*second);
  EXPECT_EQ(2u, m_created);
}

TEST_F(TestNfsContextPool, RetiredContextIsNotHandedOut)
{
  std::vector<CNfsContextLease> leases;
  for (unsigned int i = 0; i < MAX_CONTEXTS; ++i)
    leases.push_back(Checkout());

  // held throughout, as by a file still open on it
  const CNfsContextLease retired = std::move(leases.front());
  m_pool.Retire(*retired);
  leases.clear();

  for (unsigned int i = 0; i < MAX_CONTEXTS * 2; ++i)
    leases.push_back(Checkout());
  const auto users = CountUsers(leases);
  EXPECT_EQ(MAX_CONTEXTS, users.size());
  EXPECT_FALSE(users.contains(&*retired));
  EXPECT_EQ(MAX_CONTEXTS + 1, m_created);
}

TEST_F(TestNfsContextPool, FailedCreationTakesNoPlace)
{
  const CNfsContextLease failed =
      m_pool.Checkout(KEY, 1, [] { return std::shared_ptr<CNfsContext>(); });
  EXPECT_FALSE(failed);

  const CNfsContextLease lease = Checkout(KEY, 1);
  EXPECT_TRUE(lease);
  EXPECT_EQ(1u, m_created);
}

TEST_F(TestNfsContextPool, ExtraContextsCloseBeforeTheFirst)
{
  CNfsContextLease first = Checkout();
  CNfsContextLease extra = Checkout();
  CNfsContext* const firstContext = &*first;
  CNfsContext* const extraContext = &*extra;
  first.Reset();
  extra.Reset();

  const auto now = std::chrono::steady_clock::now();
  const auto idle = m_pool.TakeUnused(now + 45s, false);
  ASSERT_EQ(1u, idle.size());
  EXPECT_EQ(extraContext, idle.front().get());

  const auto expired = m_pool.TakeUnused(now + 61s, false);
  ASSERT_EQ(1u, expired.size());
  EXPECT_EQ(firstContext, expired.front().get());
}

TEST_F(TestNfsContextPool, TakingAllUnusedLeavesContextsInUse)
{
  CNfsContextLease inUse = Checkout();
  CNfsContext* const usedContext = &*inUse;
  CNfsContext* unusedContext = nullptr;
  {
    const CNfsContextLease lease = Checkout();
    unusedContext = &*lease;
  }

  const auto now = std::chrono::steady_clock::now();
  const auto unused = m_pool.TakeUnused(now, true);
  ASSERT_EQ(1u, unused.size());
  EXPECT_EQ(unusedContext, unused.front().get());

  inUse.Reset();
  const auto released = m_pool.TakeUnused(now, true);
  ASSERT_EQ(1u, released.size());
  EXPECT_EQ(usedContext, released.front().get());
}

TEST_F(TestNfsContextPool, RetiredContextIsDestroyedWithItsLastLease)
{
  std::weak_ptr<CNfsContext> created;
  const auto create = [&created]
  {
    auto context = std::make_shared<CNfsContext>(KEY, "server", "/export", nullptr);
    created = context;
    return context;
  };

  CNfsContextLease first = m_pool.Checkout(KEY, 1, create);
  CNfsContextLease second = m_pool.Checkout(KEY, 1, create);
  m_pool.Retire(*first);

  first.Reset();
  EXPECT_FALSE(created.expired());
  second.Reset();
  EXPECT_TRUE(created.expired());
}
