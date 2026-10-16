/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UDFBlockInput.h"

#include "filesystem/File.h"

#include <algorithm>
#include <cstring>
#include <mutex>

#include <udfread/udfread.h>

namespace
{
//! Blocks read at once for a smaller read. The structures of a disc (descriptors, file entries,
//! directories, playlists) mostly lie close together, and every read is a round trip when the
//! image is on a network share, so the reads that follow are mostly answered from memory.
constexpr uint32_t READ_AHEAD_BLOCKS{32};

ssize_t ReadBlocks(XFILE::CFile& file, uint32_t lba, void* buf, uint32_t blocks)
{
  const int64_t pos{static_cast<int64_t>(lba) * UDF_BLOCK_SIZE};
  if (file.Seek(pos, SEEK_SET) != pos)
    return -1;

  return file.Read(buf, static_cast<size_t>(blocks) * UDF_BLOCK_SIZE);
}
} // namespace

int CUDFBlockInput::Close(udfread_block_input* bi)
{
  auto m_bi = reinterpret_cast<UDF_BI*>(bi);

  m_bi->fp->Close();

  return 0;
}

uint32_t CUDFBlockInput::Size(udfread_block_input* bi)
{
  auto m_bi = reinterpret_cast<UDF_BI*>(bi);

  return static_cast<uint32_t>(m_bi->fp->GetLength() / UDF_BLOCK_SIZE);
}

int CUDFBlockInput::Read(
    udfread_block_input* bi, uint32_t lba, void* buf, uint32_t blocks, int flags)
{
  auto m_bi = reinterpret_cast<UDF_BI*>(bi);
  std::unique_lock lock(m_bi->lock);

  // A large read gains nothing from reading ahead
  if (blocks >= READ_AHEAD_BLOCKS)
  {
    const ssize_t read{ReadBlocks(*m_bi->fp, lba, buf, blocks)};
    if (read > 0)
      return static_cast<int>(read / UDF_BLOCK_SIZE);

    return static_cast<int>(read);
  }

  if (lba < m_bi->readAheadLba ||
      uint64_t{lba} + blocks > uint64_t{m_bi->readAheadLba} + m_bi->readAheadBlocks)
  {
    m_bi->readAhead.resize(READ_AHEAD_BLOCKS * UDF_BLOCK_SIZE);
    const ssize_t read{ReadBlocks(*m_bi->fp, lba, m_bi->readAhead.data(), READ_AHEAD_BLOCKS)};
    m_bi->readAheadLba = lba;
    m_bi->readAheadBlocks = read > 0 ? static_cast<uint32_t>(read / UDF_BLOCK_SIZE) : 0;
    if (read < 0)
      return static_cast<int>(read);
  }

  // Fewer blocks than asked for are available at the end of the image
  const uint32_t available{std::min(blocks, m_bi->readAheadLba + m_bi->readAheadBlocks - lba)};
  std::memcpy(
      buf, m_bi->readAhead.data() + static_cast<size_t>(lba - m_bi->readAheadLba) * UDF_BLOCK_SIZE,
      static_cast<size_t>(available) * UDF_BLOCK_SIZE);

  return static_cast<int>(available);
}

udfread_block_input* CUDFBlockInput::GetBlockInput(const std::string& file)
{
  auto fp = std::make_shared<XFILE::CFile>();

  if (fp->Open(file))
  {
    m_bi = std::make_unique<UDF_BI>();
    if (m_bi)
    {
      m_bi->fp = fp;
      m_bi->bi.close = CUDFBlockInput::Close;
      m_bi->bi.read = CUDFBlockInput::Read;
      m_bi->bi.size = CUDFBlockInput::Size;

      return &m_bi->bi;
    }

    fp->Close();
  }

  return nullptr;
}
