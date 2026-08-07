/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/Buffers/DmaBufIdentity.h"

#include <map>

#include <gtest/gtest.h>

using namespace DRMPRIME;

namespace
{

AVDRMFrameDescriptor MakeDescriptor()
{
  AVDRMFrameDescriptor descriptor{};
  descriptor.nb_objects = 1;
  descriptor.objects[0].fd = 40;
  descriptor.objects[0].format_modifier = 0x100;
  descriptor.nb_layers = 1;
  descriptor.layers[0].format = 0x3231564e; // NV12
  descriptor.layers[0].nb_planes = 2;
  descriptor.layers[0].planes[0].object_index = 0;
  descriptor.layers[0].planes[0].offset = 0;
  descriptor.layers[0].planes[0].pitch = 1920;
  descriptor.layers[0].planes[1].object_index = 0;
  descriptor.layers[0].planes[1].offset = 1920 * 1080;
  descriptor.layers[0].planes[1].pitch = 1920;
  return descriptor;
}

StatInodeFn FixedInodes(std::map<int, uint64_t> inodes)
{
  return [inodes = std::move(inodes)](int fd, uint64_t& inode)
  {
    auto it = inodes.find(fd);
    if (it == inodes.end())
      return false;
    inode = it->second;
    return true;
  };
}

} // namespace

TEST(TestDmaBufIdentity, ComputeFillsInodesPerObject)
{
  AVDRMFrameDescriptor descriptor = MakeDescriptor();
  descriptor.nb_objects = 2;
  descriptor.objects[1].fd = 41;
  descriptor.objects[1].format_modifier = 0x200;
  descriptor.layers[0].planes[1].object_index = 1;

  auto identity = ComputeDmaBufIdentity(&descriptor, 1920, 1080, FixedInodes({{40, 7}, {41, 8}}));
  ASSERT_TRUE(identity);
  EXPECT_EQ(identity->nbObjects, 2);
  EXPECT_EQ(identity->inode[0], 7u);
  EXPECT_EQ(identity->inode[1], 8u);
  EXPECT_EQ(identity->modifier[0], 0x100u);
  EXPECT_EQ(identity->modifier[1], 0x200u);
  EXPECT_EQ(identity->width, 1920u);
  EXPECT_EQ(identity->height, 1080u);
  EXPECT_EQ(identity->format, 0x3231564eu);
  EXPECT_EQ(identity->nbPlanes, 2);
  EXPECT_EQ(identity->objectIndex[1], 1);
  EXPECT_EQ(identity->offset[1], static_cast<uint64_t>(1920 * 1080));
  EXPECT_EQ(identity->pitch[1], 1920u);
}

TEST(TestDmaBufIdentity, NullDescriptorReturnsNullopt)
{
  EXPECT_FALSE(ComputeDmaBufIdentity(nullptr, 1920, 1080, FixedInodes({{40, 7}})));
}

TEST(TestDmaBufIdentity, ZeroLayersReturnsNullopt)
{
  AVDRMFrameDescriptor descriptor = MakeDescriptor();
  descriptor.nb_layers = 0;
  EXPECT_FALSE(ComputeDmaBufIdentity(&descriptor, 1920, 1080, FixedInodes({{40, 7}})));
}

TEST(TestDmaBufIdentity, StatFailureReturnsNullopt)
{
  AVDRMFrameDescriptor descriptor = MakeDescriptor();
  descriptor.nb_objects = 2;
  descriptor.objects[1].fd = 41;

  // the second object's fd is unknown to the stat fn
  EXPECT_FALSE(ComputeDmaBufIdentity(&descriptor, 1920, 1080, FixedInodes({{40, 7}})));
}

TEST(TestDmaBufIdentity, FdRecyclingProducesDifferentIdentity)
{
  AVDRMFrameDescriptor descriptor = MakeDescriptor();

  auto before = ComputeDmaBufIdentity(&descriptor, 1920, 1080, FixedInodes({{40, 7}}));
  auto after = ComputeDmaBufIdentity(&descriptor, 1920, 1080, FixedInodes({{40, 8}}));
  ASSERT_TRUE(before);
  ASSERT_TRUE(after);
  EXPECT_FALSE(*before == *after);
  EXPECT_FALSE(before->SameMemory(*after));
}

TEST(TestDmaBufIdentity, SameMemoryDifferentFdNumbersMatch)
{
  AVDRMFrameDescriptor descA = MakeDescriptor();
  AVDRMFrameDescriptor descB = MakeDescriptor();
  descB.objects[0].fd = 99;

  auto a = ComputeDmaBufIdentity(&descA, 1920, 1080, FixedInodes({{40, 7}}));
  auto b = ComputeDmaBufIdentity(&descB, 1920, 1080, FixedInodes({{99, 7}}));
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  EXPECT_TRUE(*a == *b);
  EXPECT_TRUE(a->SameMemory(*b));
}

TEST(TestDmaBufIdentity, LayoutFieldChangesBreakEquality)
{
  const auto stat = FixedInodes({{40, 7}});
  AVDRMFrameDescriptor base = MakeDescriptor();
  auto reference = ComputeDmaBufIdentity(&base, 1920, 1080, stat);
  ASSERT_TRUE(reference);

  auto differs = [&](const AVDRMFrameDescriptor& descriptor, uint32_t width, uint32_t height)
  {
    auto other = ComputeDmaBufIdentity(&descriptor, width, height, stat);
    EXPECT_TRUE(other);
    if (!other)
      return false;
    EXPECT_TRUE(reference->SameMemory(*other));
    return !(*reference == *other);
  };

  EXPECT_TRUE(differs(base, 1280, 1080));
  EXPECT_TRUE(differs(base, 1920, 720));

  AVDRMFrameDescriptor d = MakeDescriptor();
  d.objects[0].format_modifier = 0x101;
  EXPECT_TRUE(differs(d, 1920, 1080));

  d = MakeDescriptor();
  d.layers[0].format = 0x30313050; // P010
  EXPECT_TRUE(differs(d, 1920, 1080));

  d = MakeDescriptor();
  d.layers[0].planes[1].offset = 4096;
  EXPECT_TRUE(differs(d, 1920, 1080));

  d = MakeDescriptor();
  d.layers[0].planes[0].pitch = 2048;
  EXPECT_TRUE(differs(d, 1920, 1080));

  d = MakeDescriptor();
  d.layers[0].planes[1].object_index = 1;
  d.nb_objects = 2;
  d.objects[1].fd = 40;
  EXPECT_TRUE(differs(d, 1920, 1080));
}

TEST(TestDmaBufIdentity, MultiObjectSecondInodeCompared)
{
  AVDRMFrameDescriptor descriptor = MakeDescriptor();
  descriptor.nb_objects = 2;
  descriptor.objects[1].fd = 41;
  descriptor.layers[0].planes[1].object_index = 1;

  auto a = ComputeDmaBufIdentity(&descriptor, 1920, 1080, FixedInodes({{40, 7}, {41, 8}}));
  auto b = ComputeDmaBufIdentity(&descriptor, 1920, 1080, FixedInodes({{40, 7}, {41, 9}}));
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  EXPECT_FALSE(*a == *b);
  EXPECT_FALSE(a->SameMemory(*b));
}
