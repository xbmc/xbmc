/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"

#include <map>

#include <gtest/gtest.h>

extern "C"
{
#include <libavutil/frame.h>
}

using namespace DRMPRIME;

namespace
{

AVDRMFrameDescriptor MakeDescriptor(int fd)
{
  AVDRMFrameDescriptor descriptor{};
  descriptor.nb_objects = 1;
  descriptor.objects[0].fd = fd;
  descriptor.nb_layers = 1;
  descriptor.layers[0].format = 0x3231564e; // NV12
  descriptor.layers[0].nb_planes = 2;
  descriptor.layers[0].planes[1].offset = 1920 * 1080;
  descriptor.layers[0].planes[0].pitch = 1920;
  descriptor.layers[0].planes[1].pitch = 1920;
  return descriptor;
}

// hand a descriptor-carrying frame to the buffer, as the decoder does
void SetFrame(CVideoBufferDRMPRIMEFFmpeg& buffer, AVDRMFrameDescriptor* descriptor)
{
  AVFrame* frame = av_frame_alloc();
  frame->data[0] = reinterpret_cast<uint8_t*>(descriptor);
  buffer.SetRef(frame);
  av_frame_free(&frame);
}

struct CountingStat
{
  std::map<int, uint64_t> inodes;
  int calls{0};

  StatInodeFn Fn()
  {
    return [this](int fd, uint64_t& inode)
    {
      calls++;
      auto it = inodes.find(fd);
      if (it == inodes.end())
        return false;
      inode = it->second;
      return true;
    };
  }
};

} // namespace

TEST(TestVideoBufferDRMPRIMEMemo, MemoComputedOnce)
{
  auto pool = std::make_shared<CVideoBufferPoolDRMPRIMEFFmpeg>();
  auto* buffer = dynamic_cast<CVideoBufferDRMPRIMEFFmpeg*>(pool->Get());
  ASSERT_NE(buffer, nullptr);

  AVDRMFrameDescriptor descriptor = MakeDescriptor(40);
  SetFrame(*buffer, &descriptor);

  CountingStat stat{{{40, 7}}};
  ASSERT_TRUE(buffer->GetIdentity(stat.Fn()));
  ASSERT_TRUE(buffer->GetIdentity(stat.Fn()));
  EXPECT_EQ(stat.calls, 1);

  buffer->Release();
}

TEST(TestVideoBufferDRMPRIMEMemo, SetRefInvalidatesMemo)
{
  auto pool = std::make_shared<CVideoBufferPoolDRMPRIMEFFmpeg>();
  auto* buffer = dynamic_cast<CVideoBufferDRMPRIMEFFmpeg*>(pool->Get());
  ASSERT_NE(buffer, nullptr);

  AVDRMFrameDescriptor descA = MakeDescriptor(40);
  AVDRMFrameDescriptor descB = MakeDescriptor(41);

  CountingStat stat{{{40, 7}, {41, 8}}};
  SetFrame(*buffer, &descA);
  auto identityA = buffer->GetIdentity(stat.Fn());
  ASSERT_TRUE(identityA);

  SetFrame(*buffer, &descB);
  auto identityB = buffer->GetIdentity(stat.Fn());
  ASSERT_TRUE(identityB);

  EXPECT_EQ(stat.calls, 2);
  EXPECT_FALSE(*identityA == *identityB);

  buffer->Release();
}

TEST(TestVideoBufferDRMPRIMEMemo, UnrefInvalidatesMemo)
{
  auto pool = std::make_shared<CVideoBufferPoolDRMPRIMEFFmpeg>();
  auto* buffer = dynamic_cast<CVideoBufferDRMPRIMEFFmpeg*>(pool->Get());
  ASSERT_NE(buffer, nullptr);

  AVDRMFrameDescriptor descriptor = MakeDescriptor(40);
  SetFrame(*buffer, &descriptor);

  CountingStat stat{{{40, 7}}};
  ASSERT_TRUE(buffer->GetIdentity(stat.Fn()));

  buffer->Unref();
  EXPECT_FALSE(buffer->GetIdentity(stat.Fn()));
  EXPECT_EQ(stat.calls, 1); // null descriptor never reaches the stat fn

  buffer->Release();
}

TEST(TestVideoBufferDRMPRIMEMemo, PoolReturnReissueCycleNoFalseCarry)
{
  auto pool = std::make_shared<CVideoBufferPoolDRMPRIMEFFmpeg>();
  auto* buffer = dynamic_cast<CVideoBufferDRMPRIMEFFmpeg*>(pool->Get());
  ASSERT_NE(buffer, nullptr);

  AVDRMFrameDescriptor descriptor = MakeDescriptor(40);

  CountingStat statA{{{40, 100}}};
  SetFrame(*buffer, &descriptor);
  auto identityA = buffer->GetIdentity(statA.Fn());
  ASSERT_TRUE(identityA);
  const DmaBufIdentity savedA = *identityA;

  // back to the pool (Return runs Unref), then reissued carrying a recycled
  // fd number that now names different memory
  buffer->Release();
  auto* reissued = dynamic_cast<CVideoBufferDRMPRIMEFFmpeg*>(pool->Get());
  ASSERT_EQ(reissued, buffer);

  CountingStat statB{{{40, 200}}};
  SetFrame(*reissued, &descriptor);
  auto identityB = reissued->GetIdentity(statB.Fn());
  ASSERT_TRUE(identityB);

  EXPECT_FALSE(savedA == *identityB);
  EXPECT_FALSE(savedA.SameMemory(*identityB));

  reissued->Release();
}
