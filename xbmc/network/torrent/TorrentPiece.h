/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Event.h"

#include <chrono>
#include <stdint.h>

#include <libtorrent/libtorrent.hpp>

namespace KODI
{
namespace NETWORK
{
class CTorrentPiece
{
public:
  CTorrentPiece(const lt::sha256_hash& infoHash);
  ~CTorrentPiece() = default;

  // Torrent API
  void Cancel();

  // Piece API
  bool IsPieceFinished() const { return m_pieceFinished; }
  bool WaitForPiece(std::chrono::milliseconds timeout);
  void SetPieceFinished();

  // Read piece API
  void SetReadBuffer(uint8_t* readBuffer, size_t bufferSize, size_t partStart, size_t partLength);
  bool IsReadFinished() const { return m_readFinished; }
  bool IsReadFailed() const { return m_readFailed; }
  bool WaitForRead();
  void SetReadFinished(const uint8_t* pieceBuffer, size_t pieceSize);
  void SetReadFailed();
  ssize_t GetReadSize() const { return m_readSize; }

private:
  // Construction parameters
  const lt::sha256_hash m_infoHash;

  // Piece parameters
  bool m_pieceFinished{false};
  CEvent m_pieceEvent;

  // Read piece parameters
  bool m_readFinished{false};
  bool m_readFailed{false};
  uint8_t* m_readBuffer{nullptr};
  size_t m_bufferSize{0};
  size_t m_partStart{0};
  size_t m_partLength{0};
  ssize_t m_readSize{-1};
  CEvent m_readEvent;
};
} // namespace NETWORK
} // namespace KODI
