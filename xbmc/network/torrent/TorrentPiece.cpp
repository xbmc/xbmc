/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TorrentPiece.h"

#include <cstring>

using namespace KODI;
using namespace NETWORK;

CTorrentPiece::CTorrentPiece(const lt::sha256_hash& infoHash) : m_infoHash(infoHash)
{
}

void CTorrentPiece::Cancel()
{
  m_pieceEvent.Set();
  m_readEvent.Set();
}

bool CTorrentPiece::WaitForPiece(std::chrono::milliseconds timeout)
{
  // Wait for piece to be received
  if (!m_pieceFinished)
    m_pieceEvent.Wait(timeout);

  return m_pieceFinished;
}

void CTorrentPiece::SetPieceFinished()
{
  m_pieceFinished = true;
  m_pieceEvent.Set();
}

void CTorrentPiece::SetReadBuffer(uint8_t* readBuffer,
                                  size_t bufferSize,
                                  size_t partStart,
                                  size_t partLength)
{
  // Set read piece parameters
  m_readBuffer = readBuffer;
  m_bufferSize = bufferSize;
  m_partStart = partStart;
  m_partLength = partLength;
  m_readSize = -1;
}

bool CTorrentPiece::WaitForRead()
{
  // Wait for read to be performed or fail
  if (!m_readFinished && !m_readFailed)
    m_readEvent.Wait();

  return m_readFinished;
}

void CTorrentPiece::SetReadFinished(const uint8_t* pieceBuffer, size_t readSize)
{
  m_readFinished = true;

  // Validate state
  if (m_readBuffer == nullptr || m_bufferSize == 0)
    return;

  // Calculate read length
  const int readLength = std::min({static_cast<int>(readSize) - static_cast<int>(m_partStart),
                                   static_cast<int>(m_bufferSize), static_cast<int>(m_partLength)});
  if (readLength < 0)
    return;

  // Copy from libtorrent buffer to our buffer
  std::memcpy(m_readBuffer, pieceBuffer + m_partStart, static_cast<size_t>(readLength));

  // Record read size
  m_readSize = static_cast<ssize_t>(readLength);

  m_readEvent.Set();
}

void CTorrentPiece::SetReadFailed()
{
  m_readFailed = true;
  m_readEvent.Set();
}
