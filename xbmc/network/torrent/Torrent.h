/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Event.h"

#include <mutex>

#include <libtorrent/libtorrent.hpp>

namespace KODI
{
namespace NETWORK
{
class CTorrent
{
public:
  CTorrent(lt::torrent_handle torrentHandle);
  ~CTorrent() = default;

  // Torrent API
  void Cancel();

  // Metadata API
  bool WaitForMetadata();
  void SetMetadataReceived();
  void SetMetadataFailed();

private:
  // Construction parameters
  const lt::torrent_handle m_torrentHandle;

  // Metadata parameters
  bool m_metadataReceived = false;
  bool m_metadataFailed = false;
  std::mutex m_metadataMutex;
  CEvent m_metadataEvent;
};
} // namespace NETWORK
} // namespace KODI
