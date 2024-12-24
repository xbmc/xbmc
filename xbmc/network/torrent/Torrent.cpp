/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Torrent.h"

using namespace KODI;
using namespace NETWORK;

CTorrent::CTorrent(lt::torrent_handle torrentHandle) : m_torrentHandle(std::move(torrentHandle))
{
}

void CTorrent::Cancel()
{
  m_metadataEvent.Set();
}

bool CTorrent::WaitForMetadata()
{
  // Wait for metadata to be received or fail
  while (!m_metadataReceived && !m_metadataFailed)
    m_metadataEvent.Wait();

  return m_metadataReceived;
}

void CTorrent::SetMetadataReceived()
{
  m_metadataReceived = true;
  m_metadataEvent.Set();
}

void CTorrent::SetMetadataFailed()
{
  m_metadataFailed = true;
  m_metadataEvent.Set();
}
