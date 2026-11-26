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
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
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

  /*!
   \brief The video looked up on a thread, for as long as one lives there: the scraper's calls and
   the probes made for it wait for those of the video being added and of the videos before it
   */
  class CPlace
  {
  public:
    //! \param place the video's index in its run
    explicit CPlace(size_t place);
    ~CPlace();
    CPlace(const CPlace&) = delete;
    CPlace& operator=(const CPlace&) = delete;

  private:
    const size_t m_previous;
  };
  //! \brief The place in its run, from 1, of the video the calling thread looks up, or 0 for the
  //! video being added
  static size_t Place();
};

//! \brief Whether a file is a disc, by its name alone, as telling a bluray image from another mounts it
bool IsDiscByName(const std::string& path);

/*!
 \brief Reads discs ahead of a scan, on threads of its own, so that the scan finds what it reads of
 a disc in the disc cache rather than waiting on the disc.
 */
class CDiscReadAhead
{
public:
  /*!
   \param ahead how many discs at most are read, or being read, and not yet released
   \param readers how many are read at once
   */
  CDiscReadAhead(size_t ahead, size_t readers);
  ~CDiscReadAhead();
  CDiscReadAhead(const CDiscReadAhead&) = delete;
  CDiscReadAhead& operator=(const CDiscReadAhead&) = delete;

  /*!
   \brief Read a disc as \p read does, unless it is queued already
   \param held whether it waits to be read until Allow() or Drop() says whether the scan needs it
   */
  void Queue(const std::string& disc, std::function<void()> read, bool held = false);
  //! \brief Read a disc held until the scan was known to need it
  void Allow(const std::string& disc);
  //! \brief Read no disc the scan turns out not to need, unless it is being read already
  void Drop(const std::string& disc);
  /*!
   \brief Take a disc, as the scan comes to it. One being read is waited for, and one not yet read
   is left to the scan. Those queued before it are let go of, as the scan has passed them.
   \return whether the disc was read ahead, to be released once the scan is done with it
   */
  bool Take(const std::string& disc);
  //! \brief The scan is done with a disc it took, which may leave the cache
  void Release(const std::string& disc);
  //! \brief Read no more, without waiting for the disc being read
  void Abandon();
  //! \brief Read no more, and wait for the disc being read
  void Stop();

  //! \brief A disc taken for as long as it lives
  class CTaken
  {
  public:
    CTaken(CDiscReadAhead& reads, std::string disc)
      : m_reads(reads),
        m_disc(std::move(disc)),
        m_taken(m_reads.Take(m_disc))
    {
    }
    ~CTaken()
    {
      if (m_taken)
        m_reads.Release(m_disc);
    }
    CTaken(const CTaken&) = delete;
    CTaken& operator=(const CTaken&) = delete;

  private:
    CDiscReadAhead& m_reads;
    std::string m_disc;
    bool m_taken;
  };

private:
  void Work();

  enum class State
  {
    HELD,
    QUEUED,
    READING,
    READ,
  };
  struct Disc
  {
    std::string path;
    std::function<void()> read;
    State state{State::QUEUED};
    //! Passed by the scan while being read, so let go of once read
    bool passed{false};

    bool Waiting() const { return state == State::HELD || state == State::QUEUED; }
  };

  const size_t m_ahead;
  const size_t m_readers;
  std::mutex m_lock;
  std::condition_variable m_changed;
  //! In the order queued, until taken and released or let go of
  std::list<Disc> m_discs;
  //! The discs being read or read, and not yet let go of
  size_t m_held{0};
  bool m_stopping{false};
  std::vector<std::unique_ptr<CThread>> m_threads;
};
} // namespace KODI::VIDEO
