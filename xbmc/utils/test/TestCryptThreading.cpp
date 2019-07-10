/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/CryptThreading.h"
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
#include "threads/SingleLock.h"

#include <atomic>
#include <set>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

TEST(TestCryptThreadingInitializer, General)
{
  std::cout << "g_cryptThreadingInitializer address: " <<
    testing::PrintToString(&g_cryptThreadingInitializer) << "\n";
}

#define PVTID_NUM_THREADS 10

TEST(TestCryptThreadingInitializer, ProducesValidThreadIds)
{
  std::thread testThreads[PVTID_NUM_THREADS];

  std::vector<unsigned long> gatheredIds;
  CCriticalSection gatheredIdsMutex;

  std::atomic<unsigned long> threadsWaiting{0};
  std::atomic<bool> gate{false};

  for (int i = 0; i < PVTID_NUM_THREADS; i++)
  {
    testThreads[i] = std::thread([&gatheredIds, &gatheredIdsMutex, &threadsWaiting, &gate]() {
      threadsWaiting++;

      while (!gate);

      unsigned long myTid = g_cryptThreadingInitializer.GetCurrentCryptThreadId();

      {
        CSingleLock gatheredIdsLock(gatheredIdsMutex);
        gatheredIds.push_back(myTid);
      }
    });        
  }

  gate = true;

  for (int i = 0; i < PVTID_NUM_THREADS; i++)
    // This is somewhat dangerous but C++ doesn't have a join with timeout or a way to check
    // if a thread is still running.
    testThreads[i].join();

  // Verify that all of the thread id's are unique, and that there are 10 of them, and that none
  // of them is zero
  std::set<unsigned long> checkIds;
  for (std::vector<unsigned long>::const_iterator i = gatheredIds.begin(); i != gatheredIds.end(); ++i)
  {
    unsigned long curId = *i;
    // Thread ID isn't zero (since the sequence is pre-incremented and starts at 0)
    ASSERT_TRUE(curId != 0);

    // Make sure the ID isn't duplicated
    ASSERT_TRUE(checkIds.find(curId) == checkIds.end());
    checkIds.insert(curId);
  }

  // Make sure there's exactly PVTID_NUM_THREADS of them
  ASSERT_EQ(PVTID_NUM_THREADS, gatheredIds.size());
  ASSERT_EQ(PVTID_NUM_THREADS, checkIds.size());
}
#endif
