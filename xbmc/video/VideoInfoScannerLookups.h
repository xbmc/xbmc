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
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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

//! \brief Whether a file is a disc, by its name alone, as telling a bluray image from another mounts it
bool IsDiscByName(const std::string& path);

/*!
 \brief Reads discs ahead of a scan, on threads of its own, so that the scan finds what it reads of
 a disc in the disc cache rather than waiting on the disc. The discs are read a few at a time, as
 more read at once from one source slow each other down, in the order they are queued, and only a
 few ahead of those the scan has taken, as the cache holds no more.
 */
class CDiscReadAhead
{
public:
  /*!
   \param ahead how many discs at most are read, or being read, and not yet released
   \param readers how many are read at once
   */
  CDiscReadAhead(size_t ahead, size_t readers) : m_ahead(ahead), m_readers(readers) {}
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
  std::vector<std::thread> m_threads;
};
} // namespace KODI::VIDEO
