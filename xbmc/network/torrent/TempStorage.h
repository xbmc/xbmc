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

#pragma once

#include <map>
#include <vector>

#include <libtorrent/libtorrent.hpp>

namespace KODI
{
namespace NETWORK
{

class CTempStorage
{
public:
  explicit CTempStorage(const lt::file_storage& fs);

  lt::span<const char> Read(const lt::peer_request r, lt::storage_error& ec) const;

  void Write(const lt::span<const char> b, const lt::piece_index_t piece, const int offset);

  lt::sha256_hash Hash(const lt::piece_index_t piece,
                       const lt::span<lt::sha256_hash> blockHashes,
                       lt::storage_error& ec) const;

  lt::sha256_hash Hash2(const lt::piece_index_t piece, const int offset, lt::storage_error& ec);

private:
  // Utility functions
  int GetPieceSize(lt::piece_index_t piece) const;

  // Construction parameters
  const lt::file_storage& m_files;

  // Storage
  std::map<lt::piece_index_t, std::vector<char>> m_fileData;
};

} // namespace NETWORK
} // namespace KODI
