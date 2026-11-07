/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <functional>

namespace KODI::VIDEO
{
/*!
 \brief Looks a batch of videos up on a few threads, and hands each back to the calling thread in
 the order it was given, as soon as it and those before it have been looked up.

 A look-up mostly waits: on the scraper, on the file system, on the file's streams. The calling
 thread writes the videos to the library as they are handed back, so a look-up neither writes to
 the database nor shows a dialog. A bounded number of look-ups run ahead of the video being handed
 back, so that a slow one holds no more than those back.
 */
class CVideoLookups
{
public:
  /*!
   \brief Look up a batch of videos
   \param count the number of videos
   \param workers the number of threads to look them up on; with one, each video is looked up and
   handed back before the next is looked up
   \param lookUp looks video i up, on any thread
   \param handBack takes video i once it and those before it have been looked up, on the calling
   thread, and returns false to look up no more
   \param stop whether to look up no more, as the scan is cancelled
   */
  static void Run(size_t count,
                  size_t workers,
                  const std::function<void(size_t)>& lookUp,
                  const std::function<bool(size_t)>& handBack,
                  const std::atomic<bool>& stop);
};
} // namespace KODI::VIDEO
