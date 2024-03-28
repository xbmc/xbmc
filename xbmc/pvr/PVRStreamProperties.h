/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace PVR
{

class CPVRStreamProperties : public std::vector<std::pair<std::string, std::string>>
{
public:
  CPVRStreamProperties() = default;
  virtual ~CPVRStreamProperties() = default;

  /*!
   * @brief Obtain the URL of the stream.
   * @return The stream URL or empty string, if not found.
   */
  std::string GetStreamURL() const;

  /*!
   * @brief Obtain the MIME type of the stream.
   * @return The stream's MIME type or empty string, if not found.
   */
  std::string GetStreamMimeType() const;

  /*!
   * @brief If props are from an EPG tag indicates if playback should be as live playback would be
   * @return true if it should be played back as live, false otherwise.
   */
  bool EPGPlaybackAsLive() const;

  /*!
   * @brief If props are from an channel indicates if playback should be as a video playback would be
   * @return true if it should be played back as live, false otherwise.
   */
  bool LivePlaybackAsEPG() const;
};

} // namespace PVR
