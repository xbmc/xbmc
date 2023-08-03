/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibtorrentAlerts.h"

#include "ILibtorrentAlertHandler.h"
#include "utils/log.h"

#include <chrono>
#include <sstream>
#include <vector>

using namespace KODI;
using namespace NETWORK;
using namespace std::chrono_literals;

CLibtorrentAlerts::CLibtorrentAlerts(ILibtorrentAlertHandler& alertHandler)
  : m_alertHandler(alertHandler)
{
}

CLibtorrentAlerts::~CLibtorrentAlerts() = default;

void CLibtorrentAlerts::Start(std::shared_ptr<lt::session> session)
{
  m_session = std::move(session);
  m_running = true;
}

void CLibtorrentAlerts::Abort()
{
  m_running = false;
}

void CLibtorrentAlerts::Process()
{
  while (m_running)
  {
    m_session->wait_for_alert(1s);

    std::vector<lt::alert*> alerts;

    // Get all pending requests
    m_session->pop_alerts(&alerts);

    for (auto* alert : alerts)
    {
      // Keep in order found in libtorrent's alert_types.hpp
      switch (alert->type())
      {
        case lt::torrent_removed_alert::alert_type:
        {
          // A torrent has been removed
          auto* a = lt::alert_cast<lt::torrent_removed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent removed: {}",
                    HashToString(a->info_hashes.v2));

          break;
        }
        case lt::read_piece_alert::alert_type:
        {
          // The asynchronous read operation initiated by a call to
          // torrent_handle::read_piece() is completed or has failed
          auto* a = lt::alert_cast<lt::read_piece_alert>(alert);

          if (a->buffer)
          {
            // The read operation succeeded
            m_alertHandler.OnReadPieceFinished(
                a->handle.info_hashes().v2, static_cast<int>(a->piece),
                reinterpret_cast<const uint8_t*>(a->buffer.get()), a->size);
          }
          else
          {
            // The read failed, the torrent is paused and an error state is set
            CLog::Log(LOGERROR, LOGLIBTORRENT, "Read failure: {}", a->message());
            m_alertHandler.OnReadPieceFailed(a->handle.info_hashes().v2,
                                             static_cast<int>(a->piece));
          }

          break;
        }
        case lt::file_completed_alert::alert_type:
        {
          // An individual file completes its download. i.e. all pieces
          // overlapping this file have passed their hash check
          auto* a = lt::alert_cast<lt::file_completed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "File completed: {} (file index {})",
                    HashToString(a->handle.info_hashes().v2), static_cast<int>(a->index));

          break;
        }
        case lt::file_renamed_alert::alert_type:
        {
          // A call to torrent_handle::rename_file() call has succeeded
          auto* a = lt::alert_cast<lt::file_renamed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "File renamed: {} (\"{}\" -> \"{}\")",
                    HashToString(a->handle.info_hashes().v2), a->old_name(), a->new_name());

          break;
        }
        case lt::file_rename_failed_alert::alert_type:
        {
          // A call to torrent_handle::rename_file() call has failed
          auto* a = lt::alert_cast<lt::file_rename_failed_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "File renamed failed: {} ({})",
                    HashToString(a->handle.info_hashes().v2), static_cast<int>(a->index));

          break;
        }
        case lt::performance_alert::alert_type:
        {
          // A limit is reached that might have a negative impact on upload or
          // download rate performance
          auto* a = lt::alert_cast<lt::performance_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Performance alert: {}", a->message());

          break;
        }
        case lt::state_changed_alert::alert_type:
        {
          // A torrent has changed its state
          auto* a = lt::alert_cast<lt::state_changed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "State changed: {}", a->message());

          const char* torrentName = a->torrent_name();
          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Got torrent name: {}", torrentName);

          break;
        }
        case lt::tracker_error_alert::alert_type:
        {
          // A tracker error has occurred
          auto* a = lt::alert_cast<lt::tracker_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Tracker error: {}", a->message());

          break;
        }
        case lt::tracker_warning_alert::alert_type:
        {
          // A tracker reply contains a warning field. Usually this means that
          // the tracker announce was successful, but the tracker has a
          // to the client.
          auto* a = lt::alert_cast<lt::tracker_warning_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Tracker warning: {}", a->message());

          break;
        }
        case lt::scrape_reply_alert::alert_type:
        {
          // A scrape request succeeds
          auto* a = lt::alert_cast<lt::scrape_reply_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Scrape reply: {}", a->message());

          break;
        }
        case lt::scrape_failed_alert::alert_type:
        {
          // A scrape request has failed. This might be due to the tracker timing
          // out, refusing connection or returning an http response code
          // indicating an error.
          auto* a = lt::alert_cast<lt::scrape_failed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Scraped failed: {}", a->message());

          break;
        }
        case lt::tracker_reply_alert::alert_type:
        {
          // A tracker announce has succeeded
          auto* a = lt::alert_cast<lt::tracker_reply_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Tracker reply: {}", a->message());

          break;
        }
        case lt::dht_reply_alert::alert_type:
        {
          // The DHT receives peers from a node. Typically a few of these alerts
          // are generated at a time.
          auto* a = lt::alert_cast<lt::dht_reply_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT reply: {}", a->message());

          break;
        }
        case lt::tracker_announce_alert::alert_type:
        {
          // A tracker announce has been sent (or attempted to be sent)
          auto* a = lt::alert_cast<lt::tracker_announce_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Tracker announce: {}", a->message());

          break;
        }
        case lt::hash_failed_alert::alert_type:
        {
          // A finished piece has failed its hash check
          auto* a = lt::alert_cast<lt::hash_failed_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Hash failed: {} (piece {})",
                    HashToString(a->handle.info_hashes().v2), static_cast<int>(a->piece_index));

          break;
        }
        case lt::peer_ban_alert::alert_type:
        {
          // A peer has been banned because it sent too many corrupt pieces to us
          auto* a = lt::alert_cast<lt::peer_ban_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Peer banned: {} ({}:{})",
                    HashToString(a->handle.info_hashes().v2), a->endpoint.address().to_string(),
                    a->endpoint.port());

          break;
        }
        case lt::peer_unsnubbed_alert::alert_type:
        {
          // A peer was unsnubbed. This happens when the peer was snubbed for
          // stalling sending data, and now it has started sending data again.
          auto* a = lt::alert_cast<lt::peer_unsnubbed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Peer unsnubbed: {} ({}:{})",
                    HashToString(a->handle.info_hashes().v2), a->endpoint.address().to_string(),
                    a->endpoint.port());

          break;
        }
        case lt::peer_snubbed_alert::alert_type:
        {
          // A peer was snubbed. This is when a peer stops sending data when we
          // request it.
          auto* a = lt::alert_cast<lt::peer_snubbed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Peer snubbed: {} ({}:{})",
                    HashToString(a->handle.info_hashes().v2), a->endpoint.address().to_string(),
                    a->endpoint.port());

          break;
        }
        case lt::peer_error_alert::alert_type:
        {
          // A peer has sent invalid data over the peer-peer protocol. The peer
          // will be disconnected, but you get its ip address from the alert
          // to identify it.
          auto* a = lt::alert_cast<lt::peer_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Peer error: {}", a->message());

          break;
        }
        case lt::peer_connect_alert::alert_type:
        {
          // An incoming peer connection has successfully passed the protocol
          // handshake and is associated with a torrent, or an outgoing peer
          // connection attempt succeeded.
          auto* a = lt::alert_cast<lt::peer_connect_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Peer connected: {}", a->message());

          // If the peer is added to a torrent, the handle member is valid and
          // the torrent is the torrent it was added to. If the peer is outgoing
          // and a connection attempt succeeded, the handle is invalid and the
          // torrent is the one passed to session::connect_peer().
          if (a->handle.is_valid())
          {
            // The peer was added to a torrent
            CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Peer connected: {} ({}:{})",
                      HashToString(a->handle.info_hashes().v2), a->endpoint.address().to_string(),
                      a->endpoint.port());
          }

          break;
        }
        case lt::peer_disconnected_alert::alert_type:
        {
          // A peer is disconnected for any reason (other than the ones covered
          // by peer_error_alert)
          auto* a = lt::alert_cast<lt::peer_disconnected_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Peer disconnected: {}", a->message());

          break;
        }
        case lt::invalid_request_alert::alert_type:
        {
          // A debug alert that is generated by an incoming invalid piece request
          auto* a = lt::alert_cast<lt::invalid_request_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Invalid request: {}", a->message());

          break;
        }
        case lt::torrent_finished_alert::alert_type:
        {
          // The torrent completed downloading and the torrent has switched from
          // being a downloader to a seed
          auto* a = lt::alert_cast<lt::torrent_finished_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent finished: {}",
                    HashToString(a->handle.info_hashes().v2));

          break;
        }
        case lt::piece_finished_alert::alert_type:
        {
          // A piece just completed downloading and passed its hash check. Note
          // that torrent_handle::have_piece() could still return false if the
          // piece is not fully flushed to disk.
          auto* a = lt::alert_cast<lt::piece_finished_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Piece finished: {} (piece {})",
                    HashToString(a->handle.info_hashes().v2), static_cast<int>(a->piece_index));

          m_alertHandler.OnPieceFinished(a->handle.info_hashes().v2,
                                         static_cast<int>(a->piece_index));

          break;
        }
        case lt::request_dropped_alert::alert_type:
        {
          // A peer has rejected or ignored a piece request
          auto* a = lt::alert_cast<lt::request_dropped_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Request dropped: {} (block {}, piece {})",
                    HashToString(a->handle.info_hashes().v2), a->block_index,
                    static_cast<int>(a->piece_index));

          break;
        }
        case lt::block_timeout_alert::alert_type:
        {
          // A block request has timed out
          auto* a = lt::alert_cast<lt::block_timeout_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Block timeout: {} (block {}, piece {})",
                    HashToString(a->handle.info_hashes().v2), a->block_index,
                    static_cast<int>(a->piece_index));

          break;
        }
        case lt::block_finished_alert::alert_type:
        {
          // A block request has received a response
          auto* a = lt::alert_cast<lt::block_finished_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Block finished: {} (block {}, piece {})",
                    HashToString(a->handle.info_hashes().v2), a->block_index,
                    static_cast<int>(a->piece_index));

          break;
        }
        case lt::block_downloading_alert::alert_type:
        {
          // A block request has been sent to a peer
          auto* a = lt::alert_cast<lt::block_downloading_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Block downloading: {} (block {}, piece {})",
                    HashToString(a->handle.info_hashes().v2), a->block_index,
                    static_cast<int>(a->piece_index));

          break;
        }
        case lt::unwanted_block_alert::alert_type:
        {
          // A block has been received that was not requested or whose request
          // timed out
          auto* a = lt::alert_cast<lt::unwanted_block_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Unwanted block: {} (block {}, piece {})",
                    HashToString(a->handle.info_hashes().v2), a->block_index,
                    static_cast<int>(a->piece_index));

          break;
        }
        case lt::storage_moved_alert::alert_type:
        {
          // All the disk IO has completed and the files have been moved, as an
          // effect of a call to torrent_handle::move_storage().
          auto* a = lt::alert_cast<lt::storage_moved_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Storage moved: {} (\"{}\" -> \"{}\")",
                    HashToString(a->handle.info_hashes().v2), a->old_path(), a->storage_path());

          break;
        }
        case lt::storage_moved_failed_alert::alert_type:
        {
          // An attempt to move the storage, via torrent_handle::move_storage(), failed
          auto* a = lt::alert_cast<lt::storage_moved_failed_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Storage move failed: {}", a->message());

          break;
        }
        case lt::torrent_deleted_alert::alert_type:
        {
          // A request to delete the files of a torrent complete
          auto* a = lt::alert_cast<lt::torrent_deleted_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent deleted: {}",
                    HashToString(a->info_hashes.v2));

          break;
        }
        case lt::torrent_delete_failed_alert::alert_type:
        {
          // A request to delete the files of a torrent failed
          auto* a = lt::alert_cast<lt::torrent_delete_failed_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Failed to delete torrent: {}", a->message());

          break;
        }
        case lt::save_resume_data_alert::alert_type:
        {
          // Generated as a response to a torrent_handle::save_resume_data()
          // request once the disk IO thread is done writing the state for this
          // torrent
          auto* a = lt::alert_cast<lt::save_resume_data_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Save resume data: {}", a->message());

          break;
        }
        case lt::save_resume_data_failed_alert::alert_type:
        {
          // Generated instead of ``save_resume_data_alert`` if there was an error
          // generating the resume data
          auto* a = lt::alert_cast<lt::save_resume_data_failed_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Failed to save resume data: {}", a->message());

          break;
        }
        case lt::torrent_paused_alert::alert_type:
        {
          // Generated as a response to a ``torrent_handle::pause`` request
          // once all disk IO is complete and the files in the torrent have been
          // closed. Useful for synchronizing with the disk.
          auto* a = lt::alert_cast<lt::torrent_paused_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent paused: {}",
                    HashToString(a->handle.info_hashes().v2));

          break;
        }
        case lt::torrent_resumed_alert::alert_type:
        {
          // Generated as a response to a torrent_handle::resume() request when
          // a torrent goes from a paused state to an active state
          auto* a = lt::alert_cast<lt::torrent_resumed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent resumed: {}",
                    HashToString(a->handle.info_hashes().v2));

          break;
        }
        case lt::torrent_checked_alert::alert_type:
        {
          // Posted when a torrent completes checking. i.e. when it transitions
          // out of the `checking files` state into a state where it is ready to
          // start downloading
          auto* a = lt::alert_cast<lt::torrent_checked_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent checked: {}",
                    HashToString(a->handle.info_hashes().v2));

          break;
        }
        case lt::url_seed_alert::alert_type:
        {
          // Generated when a HTTP seed name lookup fails
          auto* a = lt::alert_cast<lt::url_seed_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "URL seed error: {}", a->message());

          break;
        }
        case lt::file_error_alert::alert_type:
        {
          // The storage failed to read or write files that it needed access to,
          // and the torrent is paused
          auto* a = lt::alert_cast<lt::file_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "File error: {}", a->message());

          break;
        }
        case lt::metadata_failed_alert::alert_type:
        {
          // The metadata has been completely received and the info-hash failed
          // to match it, i.e. the metadata that was received was corrupt
          auto* a = lt::alert_cast<lt::metadata_failed_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Metadata failed: {}", a->message());

          m_alertHandler.OnMetadataFailed(a->handle.info_hashes().v2);

          break;
        }
        case lt::metadata_received_alert::alert_type:
        {
          // The metadata has been completely received and the torrent can start
          // downloading
          auto* a = lt::alert_cast<lt::metadata_received_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Metadata received: {}",
                    HashToString(a->handle.info_hashes().v2));

          m_alertHandler.OnMetadataReceived(a->handle.info_hashes().v2);

          break;
        }
        case lt::udp_error_alert::alert_type:
        {
          // Posted when there is an error on a UDP socket
          auto* a = lt::alert_cast<lt::udp_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "UDP error: {}", a->message());

          break;
        }
        case lt::external_ip_alert::alert_type:
        {
          // libtorrent learned about the machine's external IP
          auto* a = lt::alert_cast<lt::external_ip_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "External IP: {}", a->external_address.to_string());

          break;
        }
        case lt::listen_failed_alert::alert_type:
        {
          // None of the ports given in the port range to the session can be
          // opened for listening
          auto* a = lt::alert_cast<lt::listen_failed_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Listen failed: {}", a->message());

          break;
        }
        case lt::listen_succeeded_alert::alert_type:
        {
          // The listen port succeeded to be opened on a particular interface
          auto* a = lt::alert_cast<lt::listen_succeeded_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Listen succeeded: {}", a->message());

          break;
        }
        case lt::portmap_error_alert::alert_type:
        {
          // A NAT router was successfully found but some part of the port
          // mapping request failed
          auto* a = lt::alert_cast<lt::portmap_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Portmap error: {}", a->message());

          break;
        }
        case lt::portmap_alert::alert_type:
        {
          // A NAT router was successfully found and a port was successfully
          // mapped on it
          auto* a = lt::alert_cast<lt::portmap_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Portmap success: {}", a->message());

          break;
        }
        case lt::portmap_log_alert::alert_type:
        {
          // Generated to log informational events related to either UPnP or
          // NAT-PMP
          auto* a = lt::alert_cast<lt::portmap_log_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Portmap log: {}", a->message());

          break;
        }
        case lt::fastresume_rejected_alert::alert_type:
        {
          // A fast resume file has been passed to add_torrent() but the files on
          // disk did not match the fast resume file
          auto* a = lt::alert_cast<lt::fastresume_rejected_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Fastresume rejected: {}", a->message());

          break;
        }
        case lt::peer_blocked_alert::alert_type:
        {
          // An incoming peer connection, or a peer that's about to be added to
          // our peer list, is blocked for some reason
          auto* a = lt::alert_cast<lt::peer_blocked_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Peer blocked: {}", a->message());

          break;
        }
        case lt::dht_announce_alert::alert_type:
        {
          // A DHT node announces to an info-hash on our DHT node
          auto* a = lt::alert_cast<lt::dht_announce_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT announce: {}", a->message());

          break;
        }
        case lt::dht_get_peers_alert::alert_type:
        {
          // A DHT node sent a `get_peers` message to our DHT node
          auto* a = lt::alert_cast<lt::dht_get_peers_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT get peers: {}",
                    HashToString(lt::sha256_hash(a->info_hash)));

          break;
        }
        case lt::cache_flushed_alert::alert_type:
        {
          // The disk cache has been flushed for a specific torrent as a result
          // of a call to torrent_handle::flush_cache()
          auto* a = lt::alert_cast<lt::cache_flushed_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Cache flushed: {}",
                    HashToString(a->handle.info_hashes().v2));

          break;
        }
        case lt::lsd_peer_alert::alert_type:
        {
          // We received a local service discovery message from a peer for a
          // torrent we're currently participating in
          auto* a = lt::alert_cast<lt::lsd_peer_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "LSD peer: {}", a->message());

          // Log the peer's IP address, port and peer ID
          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "LSD peer: {}:{} {}",
                    a->endpoint.address().to_string(), a->endpoint.port(), a->pid.to_string());

          break;
        }
        case lt::trackerid_alert::alert_type:
        {
          // A tracker responds with a `trackerid`
          auto* a = lt::alert_cast<lt::trackerid_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Tracker ID: {}", a->tracker_id());

          break;
        }
        case lt::dht_bootstrap_alert::alert_type:
        {
          // The initial DHT bootstrap is done
          auto* a = lt::alert_cast<lt::dht_bootstrap_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT bootstrap done: {}", a->message());

          break;
        }
        case lt::torrent_error_alert::alert_type:
        {
          // A torrent has been transitioned into the error state
          auto* a = lt::alert_cast<lt::torrent_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Torrent error: {}", a->message());

          m_alertHandler.OnTorrentError(a->handle.info_hashes().v2);

          break;
        }
        case lt::torrent_need_cert_alert::alert_type:
        {
          // Always posted for SSL torrents. This is a reminder to the client that
          // the torrent won't work unless torrent_handle::set_ssl_certificate()
          // is called with a valid certificate.
          auto* a = lt::alert_cast<lt::torrent_need_cert_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent need cert: {}", a->message());

          break;
        }
        case lt::incoming_connection_alert::alert_type:
        {
          // We successfully accepted an incoming connection, through any means
          auto* a = lt::alert_cast<lt::incoming_connection_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Incoming connection: {}", a->message());

          break;
        }
        case lt::add_torrent_alert::alert_type:
        {
          // A torrent was attempted to be added and contains the return status
          // of the add operation
          auto* a = lt::alert_cast<lt::add_torrent_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent added: {}", a->message());

          break;
        }
        case lt::state_update_alert::alert_type:
        {
          // Posted when requested by the user, by calling
          // session::post_torrent_updates() on the session
          auto* a = lt::alert_cast<lt::state_update_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "State update: {}", a->message());

          break;
        }
        case lt::session_stats_alert::alert_type:
        {
          // The user requested session statistics by calling
          // post_session_stats() on the session object
          auto* a = lt::alert_cast<lt::session_stats_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Session stats: {}", a->message());

          break;
        }
        case lt::dht_error_alert::alert_type:
        {
          // Something failed in the DHT
          auto* a = lt::alert_cast<lt::dht_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "DHT error: {}", a->message());

          break;
        }
        case lt::dht_immutable_item_alert::alert_type:
        {
          // Posted as a response to a call to session::get_item(), specifically
          // the overload for looking up immutable items in the DHT
          auto* a = lt::alert_cast<lt::dht_immutable_item_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT immutable item: {}", a->message());

          break;
        }
        case lt::dht_mutable_item_alert::alert_type:
        {
          // Posted as a response to a call to session::get_item(), specifically
          // the overload for looking up mutable items in the DHT
          auto* a = lt::alert_cast<lt::dht_mutable_item_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT mutable item: {}", a->message());

          break;
        }
        case lt::dht_put_alert::alert_type:
        {
          // A DHT put operation has completed
          auto* a = lt::alert_cast<lt::dht_put_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT put: {}", a->message());

          break;
        }
        case lt::i2p_alert::alert_type:
        {
          // Used to report errors in the i2p SAM connection
          auto* a = lt::alert_cast<lt::i2p_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "I2P error: {}", a->message());

          break;
        }
        case lt::dht_outgoing_get_peers_alert::alert_type:
        {
          // Generated when we send a get_peers request
          auto* a = lt::alert_cast<lt::dht_outgoing_get_peers_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT outgoing get peers: {}", a->message());

          break;
        }
        case lt::log_alert::alert_type:
        {
          // Posted by some session wide event. Its main purpose is
          // troubleshooting and debugging.
          auto* a = lt::alert_cast<lt::log_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Log: {}", a->log_message());

          break;
        }
        case lt::torrent_log_alert::alert_type:
        {
          // Posted by torrent wide events. It's meant to be used for
          // troubleshooting and debugging.
          auto* a = lt::alert_cast<lt::torrent_log_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Torrent log: {}", a->log_message());

          break;
        }
        case lt::peer_log_alert::alert_type:
        {
          // Posted by events specific to a peer. It's meant to be used for
          // troubleshooting and debugging. Very verbose, so don't log the alert.
          break;
        }
        case lt::lsd_error_alert::alert_type:
        {
          // The local service discovery socket failed to start properly
          auto* a = lt::alert_cast<lt::lsd_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "LSD error: {}", a->message());

          break;
        }
        case lt::dht_stats_alert::alert_type:
        {
          // Contains current DHT state. Posted in response to
          // session::post_dht_stats().
          auto* a = lt::alert_cast<lt::dht_stats_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT stats: {}", a->message());

          break;
        }
        case lt::incoming_request_alert::alert_type:
        {
          // Posted every time an incoming request from a peer is accepted and
          // queued up for being serviced
          auto* a = lt::alert_cast<lt::incoming_request_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Incoming request: {}", a->message());

          break;
        }
        case lt::dht_log_alert::alert_type:
        {
          // Debug logging of the DHT. Very verbose, so don't log the alert.
          break;
        }
        case lt::dht_pkt_alert::alert_type:
        {
          // Posted every time a DHT message is sent or received. Very verbose,
          // so don't log the alert.
          break;
        }
        case lt::dht_get_peers_reply_alert::alert_type:
        {
          // Posted when we receive a response to a DHT get_peers request
          auto* a = lt::alert_cast<lt::dht_get_peers_reply_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT get peers reply: {}", a->message());

          break;
        }
        case lt::dht_direct_response_alert::alert_type:
        {
          // Posted exactly once for every call to session_handle::dht_direct_request
          auto* a = lt::alert_cast<lt::dht_direct_response_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT direct response: {}", a->message());

          break;
        }
        case lt::picker_log_alert::alert_type:
        {
          // Posted when one or more blocks are picked by the piece picker,
          // assuming the verbose piece picker logging is enabled
          auto* a = lt::alert_cast<lt::picker_log_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Picker log: {}", a->message());

          break;
        }
        case lt::session_error_alert::alert_type:
        {
          // Posted when the session encounters a serious error, potentially fatal
          auto* a = lt::alert_cast<lt::session_error_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Session error: {}", a->message());

          break;
        }
        case lt::dht_live_nodes_alert::alert_type:
        {
          // Posted in response to a call to session::dht_live_nodes()
          auto* a = lt::alert_cast<lt::dht_live_nodes_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT live nodes: {}", a->message());

          break;
        }
        case lt::session_stats_header_alert::alert_type:
        {
          // Posted the first time post_session_stats() is called
          auto* a = lt::alert_cast<lt::session_stats_header_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Session stats header: {}", a->message());

          break;
        }
        case lt::dht_sample_infohashes_alert::alert_type:
        {
          // Posted as a response to a call to session::dht_sample_infohashes()
          // with the information from the DHT response message
          auto* a = lt::alert_cast<lt::dht_sample_infohashes_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "DHT sample infohashes: {}", a->message());

          break;
        }
        case lt::block_uploaded_alert::alert_type:
        {
          // Posted when a block intended to be sent to a peer is placed in the
          // send buffer
          auto* a = lt::alert_cast<lt::block_uploaded_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Block uploaded: {}", a->message());

          break;
        }
        case lt::alerts_dropped_alert::alert_type:
        {
          // Posted to indicate to the client that some alerts were dropped
          auto* a = lt::alert_cast<lt::alerts_dropped_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Alerts dropped: {}", a->message());

          break;
        }
        case lt::socks5_alert::alert_type:
        {
          // Posted with SOCKS5 related errors, when a SOCKS5 proxy is configured
          auto* a = lt::alert_cast<lt::socks5_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "SOCKS5 error: {}", a->message());

          break;
        }
        case lt::file_prio_alert::alert_type:
        {
          // Posted when a prioritize_files() or file_priority() update of the
          // file priorities complete, which requires a round-trip to the disk
          // thread
          auto* a = lt::alert_cast<lt::file_prio_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "File priority: {}", a->message());

          break;
        }
        case lt::oversized_file_alert::alert_type:
        {
          // Posted when the initial checking of resume data and files on disk
          // completes, and a file belonging to the torrent is found on disk but
          // is larger than the file in the torrent
          auto* a = lt::alert_cast<lt::oversized_file_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Oversized file: {}", a->message());

          break;
        }
        case lt::torrent_conflict_alert::alert_type:
        {
          // Posted when two separate torrents (magnet links) resolve to the same
          // torrent, thus causing the same torrent being added twice
          auto* a = lt::alert_cast<lt::torrent_conflict_alert>(alert);

          CLog::Log(LOGERROR, LOGLIBTORRENT, "Torrent conflict: {}", a->message());

          break;
        }
        case lt::peer_info_alert::alert_type:
        {
          // Posted when torrent_handle::post_peer_info() is called
          auto* a = lt::alert_cast<lt::peer_info_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Peer info: {}", a->message());

          break;
        }
        case lt::file_progress_alert::alert_type:
        {
          // Posted when torrent_handle::post_file_progress() is called
          auto* a = lt::alert_cast<lt::file_progress_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "File progress: {}", a->message());

          break;
        }
        case lt::piece_info_alert::alert_type:
        {
          // Posted when torrent_handle::post_download_queue() is called
          auto* a = lt::alert_cast<lt::piece_info_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Piece info: {}", a->message());

          break;
        }
        case lt::piece_availability_alert::alert_type:
        {
          // Posted when torrent_handle::post_piece_availability() is called
          auto* a = lt::alert_cast<lt::piece_availability_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Piece availability: {}", a->message());

          break;
        }
        case lt::tracker_list_alert::alert_type:
        {
          // Posted when torrent_handle::post_trackers() is called
          auto* a = lt::alert_cast<lt::tracker_list_alert>(alert);

          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Tracker list: {}", a->message());

          break;
        }
        default:
        {
          CLog::Log(LOGDEBUG, LOGLIBTORRENT, "Unhandled alert: {}", alert->message());
          break;
        }
      }
    }
  }
}

std::string CLibtorrentAlerts::HashToString(const lt::sha256_hash& hash)
{
  std::ostringstream ss;
  ss << hash;
  return ss.str();
}
