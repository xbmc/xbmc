/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ILibtorrent.h"

#include <atomic>
#include <memory>
#include <string>

#include <libtorrent/libtorrent.hpp>

namespace KODI
{
namespace NETWORK
{
class ILibtorrentAlertHandler;

/*!
 * \brief Subsystem for handling libtorrent alerts
 */
class CLibtorrentAlerts
{
public:
  CLibtorrentAlerts(ILibtorrentAlertHandler& alertHandler);
  ~CLibtorrentAlerts();

  // Lifecycle functions
  void Start(std::shared_ptr<lt::session> session);
  void Process();
  void Abort();

private:
  // Utility functions
  static std::string HashToString(const lt::sha256_hash& hash);

  // Construction parameters
  ILibtorrentAlertHandler& m_alertHandler;

  // Libtorrent parameters
  std::shared_ptr<lt::session> m_session;

  // Lifecycle parameters
  std::atomic<bool> m_running{false};
};
} // namespace NETWORK
} // namespace KODI
