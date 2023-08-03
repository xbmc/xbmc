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

#include "TempDiskIO.h"

#include "TempStorage.h"

using namespace KODI;
using namespace NETWORK;

CTempDiskIO::CTempDiskIO(lt::io_context& ioc) : m_ioc(ioc)
{
}

std::unique_ptr<lt::disk_interface> CTempDiskIO::CreateTempDisk(lt::io_context& ioc,
                                                                const lt::settings_interface&,
                                                                lt::counters&)
{
  return std::make_unique<CTempDiskIO>(ioc);
}

lt::storage_holder CTempDiskIO::new_torrent(const lt::storage_params& params,
                                            const std::shared_ptr<void>&)
{
  const lt::storage_index_t idx =
      m_freeSlots.empty() ? m_torrents.end_index() : PopSlot(m_freeSlots);

  auto storage = std::make_unique<CTempStorage>(params.files);

  if (idx == m_torrents.end_index())
    m_torrents.emplace_back(std::move(storage));
  else
    m_torrents[idx] = std::move(storage);

  return lt::storage_holder(idx, *this);
}

void CTempDiskIO::remove_torrent(const lt::storage_index_t idx)
{
  m_torrents[idx].reset();
  m_freeSlots.push_back(idx);
}

void CTempDiskIO::async_read(
    lt::storage_index_t storage,
    const lt::peer_request& r,
    std::function<void(lt::disk_buffer_holder block, const lt::storage_error& se)> handler,
    lt::disk_job_flags_t)
{
  // This buffer is owned by the storage. It will remain valid for as
  // long as the torrent remains in the session. We don't need any lifetime
  // management of it.
  lt::storage_error error;
  lt::span<const char> b = m_torrents[storage]->Read(r, error);

  post(m_ioc,
       [handler, error, b, this]
       {
         handler(
             lt::disk_buffer_holder(*this, const_cast<char*>(b.data()), static_cast<int>(b.size())),
             error);
       });
}

bool CTempDiskIO::async_write(lt::storage_index_t storage,
                              const lt::peer_request& r,
                              const char* buf,
                              std::shared_ptr<lt::disk_observer>,
                              std::function<void(const lt::storage_error&)> handler,
                              lt::disk_job_flags_t)
{
  lt::span<const char> const b = {buf, r.length};

  m_torrents[storage]->Write(b, r.piece, r.start);

  post(m_ioc, [=] { handler(lt::storage_error()); });

  return false;
}

void CTempDiskIO::async_hash(
    lt::storage_index_t storage,
    const lt::piece_index_t piece,
    lt::span<lt::sha256_hash> blockHashes,
    lt::disk_job_flags_t,
    std::function<void(lt::piece_index_t, const lt::sha1_hash&, const lt::storage_error&)> handler)
{
  lt::storage_error error;

  const lt::sha256_hash hash = m_torrents[storage]->Hash(piece, blockHashes, error);

  post(m_ioc, [=] { handler(piece, lt::sha1_hash(hash), error); });
}

void CTempDiskIO::async_hash2(
    lt::storage_index_t storage,
    const lt::piece_index_t piece,
    const int offset,
    lt::disk_job_flags_t,
    std::function<void(lt::piece_index_t, const lt::sha256_hash&, const lt::storage_error&)>
        handler)
{
  lt::storage_error error;

  const lt::sha256_hash hash = m_torrents[storage]->Hash2(piece, offset, error);

  post(m_ioc, [=] { handler(piece, hash, error); });
}

void CTempDiskIO::async_move_storage(
    lt::storage_index_t,
    std::string p,
    lt::move_flags_t,
    std::function<void(lt::status_t, const std::string&, const lt::storage_error&)> handler)

{
  post(m_ioc,
       [=]
       {
         handler(lt::disk_status::fatal_disk_error, p,
                 lt::storage_error(lt::error_code(boost::system::errc::operation_not_supported,
                                                  lt::system_category())));
       });
}

void CTempDiskIO::async_check_files(
    lt::storage_index_t,
    const lt::add_torrent_params*,
    lt::aux::vector<std::string, lt::file_index_t>,
    std::function<void(lt::status_t, const lt::storage_error&)> handler)
{
  post(m_ioc, [=] { handler(lt::status_t{}, lt::storage_error()); });
}

void CTempDiskIO::async_stop_torrent(lt::storage_index_t, std::function<void()> handler)
{
  post(m_ioc, handler);
}

void CTempDiskIO::async_rename_file(
    lt::storage_index_t,
    const lt::file_index_t idx,
    const std::string name,
    std::function<void(const std::string&, lt::file_index_t, const lt::storage_error&)> handler)

{
  post(m_ioc, [=] { handler(name, idx, lt::storage_error()); });
}

void CTempDiskIO::async_delete_files(lt::storage_index_t,
                                     lt::remove_flags_t,
                                     std::function<void(const lt::storage_error&)> handler)
{
  post(m_ioc, [=] { handler(lt::storage_error()); });
}

void CTempDiskIO::async_set_file_priority(
    lt::storage_index_t,
    lt::aux::vector<lt::download_priority_t, lt::file_index_t> prio,
    std::function<void(const lt::storage_error&,
                       lt::aux::vector<lt::download_priority_t, lt::file_index_t>)> handler)

{
  post(m_ioc,
       [=]
       {
         handler(lt::storage_error(lt::error_code(boost::system::errc::operation_not_supported,
                                                  lt::system_category())),
                 prio);
       });
}

void CTempDiskIO::async_clear_piece(lt::storage_index_t,
                                    lt::piece_index_t index,
                                    std::function<void(lt::piece_index_t)> handler)
{
  post(m_ioc, [=] { handler(index); });
}

std::vector<lt::open_file_state> CTempDiskIO::get_status(lt::storage_index_t) const
{
  return {};
}

void CTempDiskIO::free_disk_buffer(char*)
{
  // Never free any buffer. We only return buffers owned by the storage
  // object.
}

lt::storage_index_t CTempDiskIO::PopSlot(std::vector<lt::storage_index_t>& queue)
{
  TORRENT_ASSERT(!queue.empty());

  lt::storage_index_t const ret = queue.back();
  queue.pop_back();

  return ret;
}
