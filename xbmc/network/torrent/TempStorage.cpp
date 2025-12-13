/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  This file is derived from the libtorrent project under the BSD 3-clause license.
 *  Copyright (c) 2017-2018, Steven Siloti
 *  Copyright (c) 2019-2021, Arvid Norberg
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later AND BSD-3-Clause
 *  See LICENSES/README.md for more information.
 */

#include "TempStorage.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <boost/asio/error.hpp>

using namespace KODI;
using namespace NETWORK;

CTempStorage::CTempStorage(const lt::file_storage& fs) : m_files(fs)
{
}

lt::span<const char> CTempStorage::Read(const lt::peer_request r, lt::storage_error& ec) const
{
  const auto it = m_fileData.find(r.piece);

  if (it == m_fileData.end())
  {
    ec.operation = lt::operation_t::file_read;
    ec.ec = boost::asio::error::eof;
    return {};
  }

  if (static_cast<int>(it->second.size()) <= r.start)
  {
    ec.operation = lt::operation_t::file_read;
    ec.ec = boost::asio::error::eof;
    return {};
  }

  return {it->second.data() + r.start,
          std::min(r.length, static_cast<int>(it->second.size()) - r.start)};
}

void CTempStorage::Write(const lt::span<const char> b,
                         const lt::piece_index_t piece,
                         const int offset)
{
  auto& data = m_fileData[piece];

  if (data.empty())
  {
    // Allocate the whole piece, otherwise we'll invalidate the pointers
    // we have returned back to libtorrent
    const int size = GetPieceSize(piece);
    data.resize(std::size_t(size));
  }

  TORRENT_ASSERT(offset + b.size() <= static_cast<int>(data.size()));

  std::memcpy(data.data() + offset, b.data(), std::size_t(b.size()));
}

lt::sha256_hash CTempStorage::Hash(const lt::piece_index_t piece,
                                   const lt::span<lt::sha256_hash> blockHashes,
                                   lt::storage_error& ec) const
{
  const auto it = m_fileData.find(piece);

  if (it == m_fileData.end())
  {
    ec.operation = lt::operation_t::file_read;
    ec.ec = boost::asio::error::eof;
    return {};
  }

  if (!blockHashes.empty())
  {
    const int pieceSize2 = m_files.piece_size2(piece);
    const int blocksInPiece2 = m_files.blocks_in_piece2(piece);
    const char* buf = it->second.data();
    std::int64_t offset = 0;

    for (int k = 0; k < blocksInPiece2; ++k)
    {
      lt::hasher256 h2;
      const std::ptrdiff_t len2 =
          std::min(lt::default_block_size, static_cast<int>(pieceSize2 - offset));
      h2.update({buf, len2});
      buf += len2;
      offset += len2;
      blockHashes[k] = h2.final();
    }
  }

  return lt::hasher256(it->second).final();
}

lt::sha256_hash CTempStorage::Hash2(const lt::piece_index_t piece,
                                    const int offset,
                                    lt::storage_error& ec)
{
  const auto it = m_fileData.find(piece);
  if (it == m_fileData.end())
  {
    ec.operation = lt::operation_t::file_read;
    ec.ec = boost::asio::error::eof;
    return {};
  }

  const int pieceSize = m_files.piece_size2(piece);

  const std::ptrdiff_t length = std::min(lt::default_block_size, pieceSize - offset);

  lt::span<const char> b = {it->second.data() + offset, length};
  return lt::hasher256(b).final();
}

int CTempStorage::GetPieceSize(lt::piece_index_t piece) const
{
  const int num_pieces = static_cast<int>((m_files.total_size() + m_files.piece_length() - 1) /
                                          m_files.piece_length());

  return static_cast<int>(piece) < num_pieces - 1
             ? m_files.piece_length()
             : static_cast<int>(m_files.total_size() -
                                std::int64_t(num_pieces - 1) * m_files.piece_length());
}
