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

#include <memory>
#include <vector>

#include <libtorrent/libtorrent.hpp>

namespace KODI
{
namespace NETWORK
{

class CTempStorage;

struct CTempDiskIO final : lt::disk_interface, lt::buffer_allocator_interface
{
public:
  explicit CTempDiskIO(lt::io_context& ioc);

  static std::unique_ptr<lt::disk_interface> CreateTempDisk(lt::io_context& ioc,
                                                            const lt::settings_interface&,
                                                            lt::counters&);

  // Implementation of lt::disk_interface
  lt::storage_holder new_torrent(const lt::storage_params& params,
                                 const std::shared_ptr<void>&) override;
  void remove_torrent(const lt::storage_index_t idx) override;
  void async_read(
      lt::storage_index_t storage,
      const lt::peer_request& r,
      std::function<void(lt::disk_buffer_holder block, const lt::storage_error& se)> handler,
      lt::disk_job_flags_t) override;
  bool async_write(lt::storage_index_t storage,
                   const lt::peer_request& r,
                   const char* buf,
                   std::shared_ptr<lt::disk_observer>,
                   std::function<void(const lt::storage_error&)> handler,
                   lt::disk_job_flags_t) override;
  void async_hash(
      lt::storage_index_t storage,
      const lt::piece_index_t piece,
      lt::span<lt::sha256_hash> blockHashes,
      lt::disk_job_flags_t,
      std::function<void(lt::piece_index_t, const lt::sha1_hash&, const lt::storage_error&)>
          handler) override;
  void async_hash2(
      lt::storage_index_t storage,
      const lt::piece_index_t piece,
      const int offset,
      lt::disk_job_flags_t,
      std::function<void(lt::piece_index_t, const lt::sha256_hash&, const lt::storage_error&)>
          handler) override;
  void async_move_storage(
      lt::storage_index_t,
      std::string p,
      lt::move_flags_t,
      std::function<void(lt::status_t, const std::string&, const lt::storage_error&)> handler)
      override;
  void async_release_files(lt::storage_index_t, std::function<void()>) override {}
  void async_check_files(
      lt::storage_index_t,
      const lt::add_torrent_params*,
      lt::aux::vector<std::string, lt::file_index_t>,
      std::function<void(lt::status_t, const lt::storage_error&)> handler) override;
  void async_stop_torrent(lt::storage_index_t, std::function<void()> handler) override;
  void async_rename_file(
      lt::storage_index_t,
      const lt::file_index_t idx,
      const std::string name,
      std::function<void(const std::string&, lt::file_index_t, const lt::storage_error&)> handler)
      override;
  void async_delete_files(lt::storage_index_t,
                          lt::remove_flags_t,
                          std::function<void(const lt::storage_error&)> handler) override;
  void async_set_file_priority(
      lt::storage_index_t,
      lt::aux::vector<lt::download_priority_t, lt::file_index_t> prio,
      std::function<void(const lt::storage_error&,
                         lt::aux::vector<lt::download_priority_t, lt::file_index_t>)> handler)
      override;
  void async_clear_piece(lt::storage_index_t,
                         lt::piece_index_t index,
                         std::function<void(lt::piece_index_t)> handler) override;
  void update_stats_counters(lt::counters&) const override {}
  std::vector<lt::open_file_state> get_status(lt::storage_index_t) const override;
  void abort(bool) override {}
  void submit_jobs() override {}
  void settings_updated() override {}

  // Implementation of lt::buffer_allocator_interface
  void free_disk_buffer(char*) override;

private:
  // Utility functions
  static lt::storage_index_t PopSlot(std::vector<lt::storage_index_t>& queue);

  // Callbacks are posted on this
  lt::io_context& m_ioc;

  // Storage for torrents
  lt::aux::vector<std::shared_ptr<CTempStorage>, lt::storage_index_t> m_torrents;

  // Slots that are unused in the m_torrents vector
  std::vector<lt::storage_index_t> m_freeSlots;
};

} // namespace NETWORK
} // namespace KODI
