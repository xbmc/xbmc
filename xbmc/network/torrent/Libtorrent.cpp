/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Libtorrent.h"

#include "LibtorrentAlerts.h"
#include "LibtorrentCache.h"
#include "TempDiskIO.h"
#include "Torrent.h"
#include "TorrentPiece.h"
#include "URL.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include <chrono>
#include <string>
#include <vector>

using namespace KODI;
using namespace NETWORK;
using namespace std::chrono_literals;

namespace
{
// clang-format off
const std::vector<std::string> LIBTORRENT_DHT_NODES = {
    "router.bittorrent.com:6881",
    "router.utorrent.com:6881",
    "dht.transmissionbt.com:6881",
};
// clang-format on
} // namespace

CLibtorrent::CLibtorrent()
  : CThread("libtorrent"),
    m_alerts(std::make_unique<CLibtorrentAlerts>(*this)),
    m_cache(std::make_unique<CLibtorrentCache>())
{
}

CLibtorrent::~CLibtorrent() = default;

void CLibtorrent::Start()
{
  if (IsRunning())
    return;

  lt::session_params sessionParams(lt::default_settings());

  //! @todo Load DHT session state

  //! @todo Implement custom persistent disk IO
  //sessionParams.disk_io_constructor = CTempDiskIO::CreateTempDisk;

  lt::settings_pack& settings = sessionParams.settings;

  // Set user agent
  settings.set_str(lt::settings_pack::user_agent, CSysInfo::GetUserAgent());

  // Set DHT bootstrap nodes
  settings.set_str(lt::settings_pack::dht_bootstrap_nodes,
                   StringUtils::Join(LIBTORRENT_DHT_NODES, ","));

  // Set alert mask
  settings.set_int(lt::settings_pack::alert_mask, lt::alert_category::all);

  // Really aggressive settings to optimize time-to-play
  settings.set_bool(lt::settings_pack::strict_end_game_mode, false);
  settings.set_bool(lt::settings_pack::announce_to_all_trackers, true);
  settings.set_bool(lt::settings_pack::announce_to_all_tiers, true);
  settings.set_int(lt::settings_pack::stop_tracker_timeout, 1);
  settings.set_int(lt::settings_pack::request_timeout, 2);
  settings.set_int(lt::settings_pack::whole_pieces_threshold, 5);
  settings.set_int(lt::settings_pack::request_queue_time, 1);
  settings.set_int(lt::settings_pack::urlseed_pipeline_size, 2);
  settings.set_int(lt::settings_pack::urlseed_max_request_bytes, 100 * 1024);

  // Create libtorrent session
  m_session = std::make_shared<lt::session>(std::move(sessionParams));

  // Create subsystems
  m_alerts->Start(m_session);

  // Create thread
  Create(false);
}

bool CLibtorrent::IsOnline() const
{
  return IsRunning();
}

void CLibtorrent::Stop(bool bWait)
{
  StopThread(false);

  m_alerts->Abort();

  // Abort active session
  if (m_session)
    m_session->abort();

  // Cancel torrent events
  for (const auto& torrent : m_torrents)
    torrent.second->Cancel();

  if (bWait)
  {
    // Wait for thread to end
    StopThread(true);

    // Reset libtorrent state
    m_session.reset();
    m_torrents.clear();
  }
}

void CLibtorrent::Process()
{
  while (!m_bStop)
    m_alerts->Process();
}

lt::torrent_handle CLibtorrent::AddTorrent(const std::string& magnetUri)
{
  lt::add_torrent_params addTorrentParams;
  addTorrentParams.save_path = m_cache->GetCachePath();
  addTorrentParams.flags &= ~lt::torrent_flags::auto_managed;
  addTorrentParams.flags &= ~lt::torrent_flags::paused;

  lt::error_code errorCode;

  lt::parse_magnet_uri(magnetUri, addTorrentParams, errorCode);
  if (errorCode)
  {
    addTorrentParams.ti = std::make_shared<lt::torrent_info>(magnetUri, errorCode);
    if (errorCode)
    {
      CLog::Log(LOGERROR, "Failed to parse magnet metadata: {}", errorCode.message());
      return {};
    }
  }
  else
  {
    const std::string metadataPath = m_cache->GetMetadataPath(addTorrentParams.info_hashes.v2);

    // Try to read from cache
    addTorrentParams.ti = std::make_shared<lt::torrent_info>(metadataPath, errorCode);

    if (errorCode)
    {
      // The torrent_info is invalid
      addTorrentParams.ti.reset();

      // Dowload metadata
      const lt::sha256_hash infoHash = addTorrentParams.info_hashes.v2;

      // Doesn't matter if it's duplicate since we never remove torrents
      lt::torrent_handle torrentHandle = AddTorrent(addTorrentParams, errorCode);
      if (errorCode)
      {
        CLog::Log(LOGERROR, "Failed to add torrent: {}", errorCode.message());
        return {};
      }

      // Wait for metadata to download
      if (!WaitForMetadata(infoHash))
      {
        CLog::Log(LOGERROR, "Failed to download metadata");
        return {};
      }

      // Save metadata to cache
      addTorrentParams.ti = torrentHandle.torrent_file_with_hashes();
      m_cache->SaveMetadata(addTorrentParams);

      return torrentHandle;
    }
  }

  // Doesn't matter if it's a duplicate since we never remove torrents
  lt::torrent_handle torrentHandle = AddTorrent(addTorrentParams, errorCode);
  if (errorCode)
  {
    CLog::Log(LOGERROR, "Failed to add torrent: {}", errorCode.message());
    return {};
  }

  return torrentHandle;
}

bool CLibtorrent::WaitForMetadata(const lt::sha256_hash& infoHash)
{
  std::shared_ptr<CTorrent> torrent = GetTorrent(infoHash);

  if (torrent)
    return torrent->WaitForMetadata();

  return false;
}

bool CLibtorrent::WaitForPiece(const lt::sha256_hash& infoHash, int pieceIndex)
{
  if (!m_session)
    return false;

  // Check m_session if piece is already available
  lt::torrent_handle torrentHandle = m_session->find_torrent(lt::sha1_hash(infoHash));
  if (torrentHandle.have_piece(static_cast<lt::piece_index_t>(pieceIndex)))
    return true;

  const std::pair<lt::sha256_hash, int> key = std::make_pair(infoHash, pieceIndex);

  std::shared_ptr<CTorrentPiece> torrentPiece;

  {
    std::lock_guard<std::mutex> lock(m_piecesMutex);

    auto it = m_waitingPieces.find(key);
    if (it == m_waitingPieces.end())
      it = m_waitingPieces.emplace(key, std::make_shared<CTorrentPiece>(infoHash)).first;

    torrentPiece = it->second;
  }

  while (!torrentHandle.have_piece(static_cast<lt::piece_index_t>(pieceIndex)))
  {
    if (torrentPiece->WaitForPiece(1000ms))
      return true;
  }

  return torrentHandle.have_piece(static_cast<lt::piece_index_t>(pieceIndex));
}

ssize_t CLibtorrent::ReadPiece(const lt::torrent_handle& torrentHandle,
                               int pieceIndex,
                               size_t partStart,
                               size_t partLength,
                               uint8_t* buffer,
                               size_t bufferSize)
{
  // Validate parameters
  if (pieceIndex < 0)
    return -1;

  const lt::sha256_hash infoHash = torrentHandle.info_hashes().v2;

  // Create torrent piece with the read buffer
  const std::shared_ptr<CTorrentPiece> torrentPiece = std::make_shared<CTorrentPiece>(infoHash);

  torrentPiece->SetReadBuffer(buffer, bufferSize, partStart, partLength);

  // Add piece to waiting list
  {
    std::lock_guard<std::mutex> lock(m_readingMutex);

    const std::pair<lt::sha256_hash, int> key = std::make_pair(infoHash, pieceIndex);

    auto it = m_readingPieces.find(key);
    if (it == m_readingPieces.end())
      it = m_readingPieces.emplace(key, std::vector<std::shared_ptr<CTorrentPiece>>{}).first;

    it->second.push_back(torrentPiece);
  }

  // Trigger read
  torrentHandle.read_piece(static_cast<lt::piece_index_t>(pieceIndex));

  // Wait for read to complete
  if (!torrentPiece->WaitForRead())
    return -1;

  return torrentPiece->GetReadSize();
}

void CLibtorrent::OnTorrentError(const lt::sha256_hash& infoHash)
{
  OnMetadataFailed(infoHash);
}

void CLibtorrent::OnMetadataReceived(const lt::sha256_hash& infoHash)
{
  std::lock_guard<std::mutex> lock(m_torrentsMutex);

  // Notify the torrent
  auto it = m_torrents.find(infoHash);
  if (it != m_torrents.end())
    it->second->SetMetadataReceived();
}

void CLibtorrent::OnMetadataFailed(const lt::sha256_hash& infoHash)
{
  std::lock_guard<std::mutex> lock(m_torrentsMutex);

  // Notify the torrent
  auto it = m_torrents.find(infoHash);
  if (it != m_torrents.end())
    it->second->SetMetadataFailed();
}

void CLibtorrent::OnPieceFinished(const lt::sha256_hash& infoHash, int pieceIndex)
{
  const std::pair<lt::sha256_hash, int> key = std::make_pair(infoHash, pieceIndex);

  std::lock_guard<std::mutex> lock(m_piecesMutex);

  // Notify the piece
  auto it = m_waitingPieces.find(key);
  if (it != m_waitingPieces.end())
  {
    it->second->SetPieceFinished();
    m_waitingPieces.erase(it);
  }
}

void CLibtorrent::OnReadPieceFinished(const lt::sha256_hash& infoHash,
                                      int pieceIndex,
                                      const uint8_t* pieceBuffer,
                                      size_t readSize)
{
  const std::pair<lt::sha256_hash, int> key = std::make_pair(infoHash, pieceIndex);

  std::lock_guard<std::mutex> lock(m_readingMutex);

  // Notify the pieces
  auto it = m_readingPieces.find(key);
  if (it != m_readingPieces.end())
  {
    const auto& readingPieces = it->second;
    for (const std::shared_ptr<CTorrentPiece>& readingPiece : readingPieces)
      readingPiece->SetReadFinished(pieceBuffer, readSize);

    m_readingPieces.erase(it);
  }
}

void CLibtorrent::OnReadPieceFailed(const lt::sha256_hash& infoHash, int pieceIndex)
{
  const std::pair<lt::sha256_hash, int> key = std::make_pair(infoHash, pieceIndex);

  std::lock_guard<std::mutex> lock(m_readingMutex);

  // Notify the pieces
  auto it = m_readingPieces.find(key);
  if (it != m_readingPieces.end())
  {
    const auto& readingPieces = it->second;
    for (const std::shared_ptr<CTorrentPiece>& readingPiece : readingPieces)
      readingPiece->SetReadFailed();

    m_readingPieces.erase(it);
  }
}

lt::torrent_handle CLibtorrent::AddTorrent(const lt::add_torrent_params& params, lt::error_code& ec)
{
  if (m_session)
  {
    lt::torrent_handle torrentHandle = m_session->add_torrent(params, ec);

    std::lock_guard<std::mutex> lock(m_torrentsMutex);
    if (m_torrents.find(params.info_hashes.v2) == m_torrents.end())
      m_torrents.emplace(params.info_hashes.v2, std::make_shared<CTorrent>(torrentHandle));

    return torrentHandle;
  }

  ec = lt::errors::invalid_session_handle;
  return {};
}

std::shared_ptr<CTorrent> CLibtorrent::GetTorrent(const lt::sha256_hash& infoHash)
{
  std::lock_guard<std::mutex> lock(m_torrentsMutex);

  auto it = m_torrents.find(infoHash);
  if (it != m_torrents.end())
    return it->second;

  return {};
}
