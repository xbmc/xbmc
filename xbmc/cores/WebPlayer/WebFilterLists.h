/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <map>
#include <set>
#include <string>

namespace KODI
{
namespace WAYLANDHOST
{
class CWaylandHost;
}

namespace WEB
{

/*!
 * \brief Keeps the shared content filter lists named in the advanced settings
 * in the profile and hands them to the web client. The lists installed web
 * apps name are fetched in the background when Kodi starts, a list named
 * later when a web app first asks for it. Every six hours a HEAD request
 * compares each list's published ETag or Last-Modified date with the stored
 * one, and a newer list is fetched once the stored one is older than the
 * interval its "Expires" header line states, measured between the server's
 * Date headers. The client is handed each list once
 * a run, and again when its content changes; a failed fetch is retried early
 * when the system clock is set.
 */
class CWebFilterLists : private CThread
{
public:
  enum class ListState
  {
    UNKNOWN,
    FETCHING,
    FETCH_FAILED,
    PREPARING,
    PREPARED,
    PREPARE_FAILED,
  };

  /*!
   * \brief changed is set whenever a list's state changes
   */
  CWebFilterLists(WAYLANDHOST::CWaylandHost& host, CEvent& changed);
  ~CWebFilterLists() override;

  void Start(const std::set<std::string>& used);

  /*!
   * \brief A web app asks for a list; one not yet known is fetched; any thread
   */
  void Want(const std::string& name);

  /*!
   * \brief How far a list is on its way to the web client; any thread
   */
  ListState State(const std::string& name) const;

  /*!
   * \brief The web client has prepared a list, or cannot
   */
  void OnPrepared(const std::string& name, bool prepared);

  /*!
   * \brief The web client is gone; the next one prepares the lists again
   */
  void ClientGone();

protected:
  void Process() override;

private:
  enum class Wakeup
  {
    STOP,
    DUE,
    WANTED,
    CLOCK,
  };

  enum class Result
  {
    FAILED,
    UNCHANGED,
    UPDATED,
  };

  void RemoveUnconfigured();
  bool Track(const std::string& name);
  bool FetchAll(bool all);
  Result Fetch(const std::string& name, const std::string& url);
  bool Hand(const std::string& name);
  void ArmClockChange();
  Wakeup Wait(bool retry);
  void Wake();

  WAYLANDHOST::CWaylandHost& m_host;
  CEvent& m_changed;
  std::map<std::string, std::string> m_lists;
  std::string m_dir;
  int m_wakeFd{-1};
  int m_clockFd{-1};
  mutable CCriticalSection m_lock;
  std::set<std::string> m_wanted;
  std::set<std::string> m_pending;
  std::set<std::string> m_handed;
  std::set<std::string> m_fetchFailed;
  std::set<std::string> m_prepared;
  std::set<std::string> m_prepareFailed;
  bool m_started{false};
};

} // namespace WEB
} // namespace KODI
