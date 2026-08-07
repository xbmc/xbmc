/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/Buffers/DmaBufIdentity.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDMA.h"
#include "utils/BufferObject.h"
#include "utils/BufferObjectFactory.h"

#include <map>
#include <memory>

#include <gtest/gtest.h>

using namespace DRMPRIME;

namespace
{

constexpr int FAKE_FD = 4242;
constexpr uint32_t FOURCC_NV12 = 0x3231564e;

// heap-backed stand-in so CVideoBufferDMA is usable without a kernel
class FakeBufferObject : public CBufferObject
{
public:
  bool CreateBufferObject(uint32_t format, uint32_t width, uint32_t height) override
  {
    return false;
  }
  bool CreateBufferObject(uint64_t size) override
  {
    m_memory = std::make_unique<uint8_t[]>(size);
    m_fd = FAKE_FD;
    return true;
  }
  void DestroyBufferObject() override
  {
    m_memory.reset();
    m_fd = -1;
  }
  uint8_t* GetMemory() override { return m_memory.get(); }
  void ReleaseMemory() override {}
  std::string GetName() const override { return "FakeBufferObject"; }

private:
  std::unique_ptr<uint8_t[]> m_memory;
};

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

class TestVideoBufferDMA : public ::testing::Test
{
protected:
  void SetUp() override
  {
    CBufferObjectFactory::ClearBufferObjects();
    CBufferObjectFactory::RegisterBufferObject([] { return std::make_unique<FakeBufferObject>(); });
  }
  void TearDown() override { CBufferObjectFactory::ClearBufferObjects(); }
};

} // namespace

// identity is derived fresh from the descriptor; layout changes must land in it
TEST_F(TestVideoBufferDMA, DescriptorIdentityTracksLayout)
{
  auto buffer = std::make_unique<CVideoBufferDMA>(0, FOURCC_NV12, 2, 1920 * 1080 * 3 / 2);
  ASSERT_TRUE(buffer->Alloc());

  const int strides[YuvImage::MAX_PLANES] = {1920, 1920, 0};
  const int offsets[YuvImage::MAX_PLANES] = {0, 1920 * 1080, 0};
  buffer->SetDimensions(1920, 1080, strides, offsets);

  CountingStat stat{{{FAKE_FD, 7}}};
  const auto first = ComputeDmaBufIdentity(buffer->GetDescriptor(), buffer->GetWidth(),
                                           buffer->GetHeight(), stat.Fn());
  ASSERT_TRUE(first);

  const auto repeat = ComputeDmaBufIdentity(buffer->GetDescriptor(), buffer->GetWidth(),
                                            buffer->GetHeight(), stat.Fn());
  ASSERT_TRUE(repeat);
  EXPECT_TRUE(*first == *repeat);

  const int changed[YuvImage::MAX_PLANES] = {2048, 1920, 0};
  buffer->SetDimensions(1920, 1080, changed, offsets);
  const auto relaid = ComputeDmaBufIdentity(buffer->GetDescriptor(), buffer->GetWidth(),
                                            buffer->GetHeight(), stat.Fn());
  ASSERT_TRUE(relaid);
  EXPECT_FALSE(*first == *relaid);
  EXPECT_TRUE(first->SameMemory(*relaid));
}

TEST_F(TestVideoBufferDMA, IsValidGateSurvivesCollapse)
{
  auto buffer = std::make_unique<CVideoBufferDMA>(0, FOURCC_NV12, 2, 1920 * 1080 * 3 / 2);
  ASSERT_TRUE(buffer->Alloc());

  // descriptor has no layers until dimensions are set
  EXPECT_FALSE(buffer->IsValid());

  const int strides[YuvImage::MAX_PLANES] = {1920, 1920, 0};
  const int offsets[YuvImage::MAX_PLANES] = {0, 1920 * 1080, 0};
  buffer->SetDimensions(1920, 1080, strides, offsets);
  EXPECT_TRUE(buffer->IsValid());
}
