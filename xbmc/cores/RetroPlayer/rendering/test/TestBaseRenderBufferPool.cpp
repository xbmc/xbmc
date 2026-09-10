/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/IRenderBuffer.h"

#include <gtest/gtest.h>

using namespace KODI::RETRO;

namespace
{
class CTestBufferPool : public CBaseRenderBufferPool
{
public:
  bool IsCompatible(const CRenderVideoSettings&) const override { return true; }

  bool allowConfiguration{true};

protected:
  bool ConfigureInternal() override { return allowConfiguration && m_format == AV_PIX_FMT_BGR0; }
  IRenderBuffer* CreateRenderBuffer(void*) override { return nullptr; }
};
} // namespace

TEST(TestBaseRenderBufferPool, RejectedFormatClearsConfiguredState)
{
  CTestBufferPool pool;
  ASSERT_TRUE(pool.Configure(AV_PIX_FMT_BGR0));

  ASSERT_FALSE(pool.Configure(AV_PIX_FMT_NONE));
  EXPECT_FALSE(pool.IsConfigured());
  EXPECT_EQ(pool.GetBuffer(160, 144), nullptr);
}

TEST(TestBaseRenderBufferPool, SuccessfulFormatCanBeConfiguredAgainAfterRejection)
{
  CTestBufferPool pool;
  ASSERT_FALSE(pool.Configure(AV_PIX_FMT_NONE));

  ASSERT_TRUE(pool.Configure(AV_PIX_FMT_BGR0));
  EXPECT_TRUE(pool.Configure(AV_PIX_FMT_BGR0));
  EXPECT_TRUE(pool.IsConfigured());
}

TEST(TestBaseRenderBufferPool, RejectedFormatIsCachedUntilFlush)
{
  CTestBufferPool pool;
  pool.allowConfiguration = false;
  ASSERT_FALSE(pool.Configure(AV_PIX_FMT_BGR0));

  pool.allowConfiguration = true;
  EXPECT_FALSE(pool.Configure(AV_PIX_FMT_BGR0));
  EXPECT_FALSE(pool.IsConfigured());

  pool.Flush();
  EXPECT_TRUE(pool.Configure(AV_PIX_FMT_BGR0));
  EXPECT_TRUE(pool.IsConfigured());
}

TEST(TestBaseRenderBufferPool, BufferCreationFailureReturnsNull)
{
  CTestBufferPool pool;
  ASSERT_TRUE(pool.Configure(AV_PIX_FMT_BGR0));

  EXPECT_EQ(pool.GetBuffer(160, 144), nullptr);
}
