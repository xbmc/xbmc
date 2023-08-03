/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <utility>

class CURL;

namespace KODI
{
namespace NETWORK
{
class CTorrentUtils
{
public:
  /*!
   * \brief Split a magnet URL, returning its original URI and the
   *        file/directory inside the torrent
   *
   * For example, the following URL will be split into the following parts:
   *
   *   magnet://Big%20Buck%20Bunny.mp4?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big%20Buck%20Bunny
   *
   * Magnet URI: magnet:?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big%20Buck%20Bunny
   * Filename: Big Buck Bunny.mp4
   *
   * \param url The magnet URL, possibly containing an optional 
   *            inside the torrent
   *
   * \return A pair containing the magnet URI and the file/directory inside the
   *         torrent
   */
  static std::pair<std::string, std::string> SplitMagnetURL(const CURL& url);
};
} // namespace NETWORK
} // namespace KODI
