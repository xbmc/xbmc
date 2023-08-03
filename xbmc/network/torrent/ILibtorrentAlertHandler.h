/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <libtorrent/libtorrent.hpp>

namespace KODI
{
namespace NETWORK
{
class ILibtorrentAlertHandler
{
public:
  virtual ~ILibtorrentAlertHandler() = default;

  // Torrent API
  virtual void OnTorrentError(const lt::sha256_hash& infoHash) = 0;

  // Metadata API
  virtual void OnMetadataReceived(const lt::sha256_hash& infoHash) = 0;
  virtual void OnMetadataFailed(const lt::sha256_hash& infoHash) = 0;

  // Piece API
  virtual void OnPieceFinished(const lt::sha256_hash& infoHash, int pieceIndex) = 0;

  // Read piece API
  virtual void OnReadPieceFinished(const lt::sha256_hash& infoHash,
                                   int pieceIndex,
                                   const uint8_t* pieceBuffer,
                                   size_t readSize) = 0;
  virtual void OnReadPieceFailed(const lt::sha256_hash& infoHash, int pieceIndex) = 0;
};
} // namespace NETWORK
} // namespace KODI
