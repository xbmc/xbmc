/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

class CThread;

namespace KODI::VIDEO
{
/*!
 \brief Looks a batch of videos up on a few threads, and hands each back to the calling thread in
 the order it was given, as soon as it and those before it have been looked up.

 A look-up neither writes to the database nor shows a dialog: the calling thread writes the videos
 as they are handed back.
 */
class CVideoLookups
{
public:
  /*!
   \param workers the number of threads to look them up on; with one, each video is looked up and
   handed back before the next is looked up
   \param lookUp looks video i up, on any thread
   \param handBack takes video i once it and those before it have been looked up, on the calling
   thread, and returns false to look up no more
   \param stop whether to look up no more, as the scan is cancelled. A look-up waiting for an
   add-on's script, such as a python scraper's call, stops waiting once it is set.
   */
  static void Run(size_t count,
                  size_t workers,
                  const std::function<void(size_t)>& lookUp,
                  const std::function<bool(size_t)>& handBack,
                  const std::atomic<bool>& stop);

  /*!
   \brief A run that videos are added to while those added before are handed back, so that the
   next videos are looked up while the last of those before them are. Its threads are made once,
   and the videos are numbered on from one addition to the next. Run() is one addition, all handed
   back.
   */
  class CRun
  {
  public:
    //! \param workers, stop as Run()'s
    CRun(size_t workers, const std::atomic<bool>& stop);
    //! Looks up no more, and waits for the look-ups running
    ~CRun();
    CRun(const CRun&) = delete;
    CRun& operator=(const CRun&) = delete;

    /*!
     \brief Add videos, to be looked up after those added before
     \param lookUp looks up video i of these, given its index in the run too, on any thread
     \return the index in the run of the first of them
     */
    size_t Add(size_t count, std::function<void(size_t, size_t)> lookUp);
    /*!
     \brief Hand the videos of the run back in order, as Run() does, up to the one at \p end
     \param handBack as Run()'s, given the video's index in the run
     \return false where the run ended first, as a video was refused, or not looked up as the scan
     stopped: it then looks up and hands back no more
     */
    bool HandBack(size_t end, const std::function<bool(size_t)>& handBack);

  private:
    void Work();
    void LookUp(size_t index);
    //! Looks up and hands back no more, once the look-ups running are done
    void End();

    struct Addition
    {
      size_t first;
      std::function<void(size_t, size_t)> lookUp;
    };

    const size_t m_workers;
    const std::atomic<bool>& m_stop;
    const size_t m_window;
    std::mutex m_lock;
    std::condition_variable m_changed;
    //! In the order added, so by the index of their first videos. A deque, so that a look-up
    //! running keeps its addition while another is added.
    std::deque<Addition> m_additions;
    //! Whether each video added has been looked up
    std::vector<char> m_done;
    size_t m_next{0};
    size_t m_handedBack{0};
    bool m_ended{false};
    std::vector<std::unique_ptr<CThread>> m_threads;
  };
};
} // namespace KODI::VIDEO
