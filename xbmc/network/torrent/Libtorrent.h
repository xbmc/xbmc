/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ILibtorrent.h"
#include "ILibtorrentAlertHandler.h"
#include "threads/Thread.h"

#include <map>
#include <memory>
#include <mutex>
#include <stddef.h>
#include <string>
#include <vector>

#include <libtorrent/libtorrent.hpp>

#include "PlatformDefs.h" // for ssize_t

class CURL;

namespace KODI
{
namespace NETWORK
{
class CTorrent;
class CLibtorrentAlerts;
class CLibtorrentCache;
class CTorrentPiece;

class CLibtorrent : public ILibtorrent, public ILibtorrentAlertHandler, private CThread
{
public:
  CLibtorrent();
  ~CLibtorrent() override;

  // Implementation of ILibtorrent
  void Start() override;
  bool IsOnline() const override;
  void Stop(bool bWait) override;

  // Implementation of CThread
  void Process() override;

  // Libtorrent API
  lt::torrent_handle AddTorrent(const std::string& magnetUri);
  bool WaitForMetadata(const lt::sha256_hash& infoHash);
  bool WaitForPiece(const lt::sha256_hash& infoHash, int pieceIndex);
  ssize_t ReadPiece(const lt::torrent_handle& torrentHandle,
                    int pieceIndex,
                    size_t partStart,
                    size_t partLength,
                    uint8_t* buffer,
                    size_t bufferSize);

  // Implementation of ILibtorrentAlertHandler
  void OnTorrentError(const lt::sha256_hash& infoHash) override;
  void OnMetadataReceived(const lt::sha256_hash& infoHash) override;
  void OnMetadataFailed(const lt::sha256_hash& infoHash) override;
  void OnPieceFinished(const lt::sha256_hash& infoHash, int pieceIndex) override;
  void OnReadPieceFinished(const lt::sha256_hash& infoHash,
                           int pieceIndex,
                           const uint8_t* pieceBuffer,
                           size_t readSize) override;
  void OnReadPieceFailed(const lt::sha256_hash& infoHash, int pieceIndex) override;

private:
  // Private utility functions
  lt::torrent_handle AddTorrent(const lt::add_torrent_params& params, lt::error_code& ec);
  std::shared_ptr<CTorrent> GetTorrent(const lt::sha256_hash& infoHash);

  // libtorrent parameters
  std::shared_ptr<lt::session> m_session;

  // libtorrent subsystems
  std::unique_ptr<CLibtorrentAlerts> m_alerts;
  std::unique_ptr<CLibtorrentCache> m_cache;

  // Torrent parameters
  std::map<lt::sha256_hash, std::shared_ptr<CTorrent>> m_torrents;
  std::mutex m_torrentsMutex;

  // Piece parameters
  std::map<std::pair<lt::sha256_hash, int>, std::shared_ptr<CTorrentPiece>> m_waitingPieces;
  std::mutex m_piecesMutex;

  // Read piece parameters
  std::map<std::pair<lt::sha256_hash, int>, std::vector<std::shared_ptr<CTorrentPiece>>>
      m_readingPieces;
  std::mutex m_readingMutex;
};
} // namespace NETWORK
} // namespace KODI
