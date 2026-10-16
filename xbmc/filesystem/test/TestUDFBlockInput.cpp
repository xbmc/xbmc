/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "filesystem/UDFBlockInput.h"
#include "test/TestUtils.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>
#include <udfread/udfread.h>

namespace
{
constexpr uint32_t IMAGE_BLOCKS{70};

//! Every byte of a block holds the block's number, so that a block read from the wrong place shows
bool HoldsBlocks(const std::vector<uint8_t>& buf, uint32_t firstBlock, uint32_t blocks)
{
  for (uint32_t block{0}; block < blocks; ++block)
  {
    for (uint32_t byte{0}; byte < UDF_BLOCK_SIZE; ++byte)
    {
      if (buf[block * UDF_BLOCK_SIZE + byte] != static_cast<uint8_t>(firstBlock + block))
        return false;
    }
  }
  return true;
}

class TestUDFBlockInput : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_image = XBMC_CREATETEMPFILE("");
    ASSERT_NE(nullptr, m_image);
    m_image->Close();
    ASSERT_TRUE(m_image->OpenForWrite(XBMC_TEMPFILEPATH(m_image), true));
    std::vector<uint8_t> block(UDF_BLOCK_SIZE);
    for (uint32_t i{0}; i < IMAGE_BLOCKS; ++i)
    {
      std::ranges::fill(block, static_cast<uint8_t>(i));
      ASSERT_EQ(static_cast<ssize_t>(block.size()), m_image->Write(block.data(), block.size()));
    }
    m_image->Close();

    m_blockInput = m_input.GetBlockInput(XBMC_TEMPFILEPATH(m_image));
    ASSERT_NE(nullptr, m_blockInput);
  }

  void TearDown() override
  {
    if (m_blockInput)
      m_blockInput->close(m_blockInput);
    XBMC_DELETETEMPFILE(m_image);
  }

  //! Read blocks and check that each holds what is at that place in the image
  void ExpectRead(uint32_t lba, uint32_t blocks, int expectedBlocks)
  {
    std::vector<uint8_t> buf(blocks * UDF_BLOCK_SIZE);
    EXPECT_EQ(expectedBlocks, m_blockInput->read(m_blockInput, lba, buf.data(), blocks, 0))
        << "reading " << blocks << " block(s) at " << lba;
    if (expectedBlocks > 0)
    {
      EXPECT_TRUE(HoldsBlocks(buf, lba, static_cast<uint32_t>(expectedBlocks)))
          << "reading " << blocks << " block(s) at " << lba;
    }
  }

  XFILE::CFile* m_image{nullptr};
  CUDFBlockInput m_input;
  udfread_block_input* m_blockInput{nullptr};
};
} // namespace

TEST_F(TestUDFBlockInput, ReadsEveryBlockInTurn)
{
  for (uint32_t lba{0}; lba < IMAGE_BLOCKS; ++lba)
    ExpectRead(lba, 1, 1);
}

TEST_F(TestUDFBlockInput, ReadsOutOfOrder)
{
  ExpectRead(50, 1, 1);
  ExpectRead(3, 2, 2);
  ExpectRead(4, 1, 1); // within the blocks just read ahead
  ExpectRead(2, 1, 1); // just before them
  ExpectRead(33, 3, 3); // overlapping the end of them
  ExpectRead(49, 1, 1);
}

TEST_F(TestUDFBlockInput, ReadsLargeReadsDirectly)
{
  ExpectRead(5, 40, 40);
  ExpectRead(6, 1, 1);
}

TEST_F(TestUDFBlockInput, ReadsUpToTheEndOfTheImage)
{
  ExpectRead(IMAGE_BLOCKS - 2, 4, 2);
  ExpectRead(IMAGE_BLOCKS - 1, 1, 1);
  ExpectRead(IMAGE_BLOCKS, 1, 0);
}
