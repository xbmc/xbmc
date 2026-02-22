/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MagnetFile.h"

#include "MagnetDirectory.h"
#include "ServiceBroker.h"
#include "SpecialProtocol.h"
#include "URL.h"
#include "network/Network.h"
#include "network/NetworkServices.h"
#include "network/torrent/Libtorrent.h"
#include "network/torrent/TorrentUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <libtorrent/libtorrent.hpp>

using namespace KODI;
using namespace NETWORK;
using namespace XFILE;

namespace
{
constexpr uint8_t PRIO_HIGHEST = 7;
constexpr uint8_t PRIO_HIGHER = 6;
constexpr uint8_t PRIO_HIGH = 5;
} // namespace

bool CMagnetFile::Open(const CURL& url)
{
  CNetworkServices& networkServices = CServiceBroker::GetNetwork().GetServices();
  CLibtorrent* libtorrent = dynamic_cast<CLibtorrent*>(networkServices.GetLibtorrent());
  if (libtorrent == nullptr)
    return false;

  std::string magnetUri;
  std::string filename;
  std::tie(magnetUri, filename) = CTorrentUtils::SplitMagnetURL(url);

  m_torrentHandle = libtorrent->AddTorrent(magnetUri);
  if (!m_torrentHandle.is_valid())
    return false;

  std::shared_ptr<const lt::torrent_info> torrentInfo = m_torrentHandle.torrent_file();
  if (!torrentInfo)
    return false;

  const lt::file_storage& files = torrentInfo->files();

  // Search through files for the matching file
  for (int i = 0; i < files.num_files(); ++i)
  {
    std::string filePath = files.file_path(static_cast<lt::file_index_t>(i));

    if (URIUtils::PathEquals(filename, filePath))
    {
      // Found the file, set file parameters
      m_fileIndex = i;
      m_fileSize = files.file_size(static_cast<lt::file_index_t>(i));
      m_modifiedTime = files.mtime(static_cast<lt::file_index_t>(i));
      m_filePosition = 0;
      break;
    }
  }

  if (m_fileIndex == -1)
    return false;

  // Set second highest priority to the first and last 0.1% or 128 KiB,
  // whichever is greater
  const size_t p01 = static_cast<size_t>(
      std::max(std::min(static_cast<int64_t>(std::numeric_limits<int>::max()), m_fileSize / 1000),
               static_cast<int64_t>(128 * 1024)));

  SetPiecePriority(m_fileIndex, 0, p01, PRIO_HIGHER);
  SetPiecePriority(m_fileIndex, static_cast<size_t>(m_fileSize - p01), p01, PRIO_HIGHER);

  // Set the initial priority for the beginning of the file
  OnFileAccess(0, 1);

  return true;
}

bool CMagnetFile::Exists(const CURL& url)
{
  struct __stat64 buffer;
  return Stat(url, &buffer) != -1;
}

int CMagnetFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CMagnetFile magnetFile;
  if (magnetFile.Open(url))
    return magnetFile.Stat(buffer);

  // Check if the URL exists as a directory
  CMagnetDirectory directory;
  if (directory.Exists(url))
  {
    // Set st_mode
    buffer->st_mode = _S_IFDIR;
    return 0;
  }

  return -1;
}

int CMagnetFile::Stat(struct __stat64* buffer)
{
  if (buffer == nullptr)
    return -1;

  if (m_fileIndex == -1)
    return -1;

  buffer->st_size = m_fileSize;
  m_modifiedTime.GetAsTime(buffer->st_mtime);

  return 0;
}

ssize_t CMagnetFile::Read(void* lpBuf, size_t uiBufSize)
{
  CNetworkServices& networkServices = CServiceBroker::GetNetwork().GetServices();
  CLibtorrent* libtorrent = dynamic_cast<CLibtorrent*>(networkServices.GetLibtorrent());
  if (libtorrent == nullptr)
    return -1;

  // Validate state
  if (!m_torrentHandle.is_valid() || m_fileIndex < 0 || m_fileSize < 0 || m_filePosition < 0)
    return -1;

  const std::shared_ptr<const lt::torrent_info> torrentInfo = m_torrentHandle.torrent_file();
  if (!torrentInfo)
    return -1;

  // Check for EOF
  if (m_filePosition >= m_fileSize)
    return 0;

  // Figure out what to read
  const int64_t readLength =
      std::min({static_cast<int64_t>(std::numeric_limits<int>::max()),
                static_cast<int64_t>(uiBufSize), m_fileSize - m_filePosition});

  const lt::peer_request part = torrentInfo->map_file(static_cast<lt::file_index_t>(m_fileIndex),
                                                      m_filePosition, static_cast<int>(readLength));
  if (part.start < 0 || part.length < 0)
    return -1;

  const int partIndex = static_cast<int>(part.piece);
  const size_t partStart = static_cast<size_t>(part.start);
  const size_t partLength = static_cast<size_t>(part.length);

  // Update piece priorities
  OnFileAccess(static_cast<size_t>(m_filePosition), partLength);

  if (!m_torrentHandle.have_piece(part.piece))
  {
    if (!libtorrent->WaitForPiece(torrentInfo->info_hashes().v2, static_cast<int>(part.piece)))
      return -1;
  }

  const ssize_t readBytes = libtorrent->ReadPiece(m_torrentHandle, partIndex, partStart, partLength,
                                                  static_cast<uint8_t*>(lpBuf), uiBufSize);
  if (readBytes > 0)
    m_filePosition += readBytes;

  return readBytes;
}

int64_t CMagnetFile::GetPosition()
{
  return m_filePosition;
}

int64_t CMagnetFile::GetLength()
{
  return m_fileSize;
}

int64_t CMagnetFile::Seek(int64_t iFilePosition, int iWhence)
{
  switch (iWhence)
  {
    case SEEK_SET:
      m_filePosition = iFilePosition;
      break;
    case SEEK_CUR:
      m_filePosition += iFilePosition;
      break;
    case SEEK_END:
      m_filePosition = m_fileSize + iFilePosition;
      break;
    default:
      return -1;
  }

  return m_filePosition;
}

void CMagnetFile::OnFileAccess(size_t fileOffset, size_t partLength)
{
  // Set highest priority to the requested range
  SetPiecePriority(m_fileIndex, static_cast<size_t>(m_filePosition), partLength, PRIO_HIGHEST);

  // Set third highest priority to the next 5% or 64 MiB, whichever is greater
  const size_t p5 = static_cast<size_t>(std::max(
      std::min(static_cast<int64_t>(std::numeric_limits<int>::max()), 5 * m_fileSize / 100),
      static_cast<int64_t>(32 * 1024 * 1024)));

  SetPiecePriority(m_fileIndex, fileOffset, p5, PRIO_HIGH);
}

void CMagnetFile::SetPiecePriority(int fileIndex,
                                   size_t fileOffset,
                                   size_t pieceSize,
                                   uint8_t priority)
{
  const std::shared_ptr<const lt::torrent_info> torrentInfo = m_torrentHandle.torrent_file();

  const lt::file_storage& fileStorage = torrentInfo->files();

  // Make sure offset + size <= file size
  const int64_t fileSize = fileStorage.file_size(static_cast<lt::file_index_t>(fileIndex));
  if (fileSize < 0)
    return;

  fileOffset = std::min(fileOffset, static_cast<size_t>(fileSize));

  // Calculate part size
  const int64_t partSize =
      std::min({static_cast<int64_t>(std::numeric_limits<int>::max()),
                static_cast<int64_t>(pieceSize), static_cast<int64_t>(fileSize - fileOffset)});

  // Map a range in the file into a range in the torrent
  lt::peer_request part = torrentInfo->map_file(static_cast<lt::file_index_t>(fileIndex),
                                                fileOffset, static_cast<int>(partSize));

  for (; part.length > 0; part.length -= torrentInfo->piece_size(part.piece++))
  {
    if (m_torrentHandle.have_piece(part.piece))
      continue;

    if (static_cast<uint8_t>(m_torrentHandle.piece_priority(part.piece)) < priority)
      continue;

    m_torrentHandle.piece_priority(part.piece, static_cast<lt::download_priority_t>(priority));
  }
}
