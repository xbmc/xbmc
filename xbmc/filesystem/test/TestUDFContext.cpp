/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/UDFContext.h"
#include "test/TestUtils.h"

#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

// An image is mounted without holding up the mounts of others, so threads asking for it at once may
// each mount it, and still all get the one volume
TEST(TestUDFContext, AnImageAskedForByThreadsAtOnceIsOneVolume)
{
  const std::string image{XBMC_REF_FILE_PATH(
      "xbmc/video/test/testdata/moviestack_blurayiso/Movie_(2001)/Movie_(2001)_part1.iso")};
  constexpr size_t count{8};
  std::promise<void> go;
  const std::shared_future<void> started{go.get_future().share()};
  std::vector<std::shared_ptr<CUDFContext>> volumes(count);
  std::vector<std::thread> threads;
  for (size_t i = 0; i < count; ++i)
  {
    threads.emplace_back(
        [&volumes, &image, started, i]
        {
          started.wait();
          volumes[i] = CUDFContext::Get(image);
        });
  }
  go.set_value();
  for (std::thread& thread : threads)
    thread.join();

  ASSERT_NE(nullptr, volumes[0]);
  for (const std::shared_ptr<CUDFContext>& volume : volumes)
    EXPECT_EQ(volumes[0], volume);
}
