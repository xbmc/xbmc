/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/generic/ILanguageInvoker.h"
#include "threads/Thread.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

class CScriptInvocationManager;

class CLanguageInvokerThread : public ILanguageInvoker, protected CThread
{
public:
  CLanguageInvokerThread(std::shared_ptr<ILanguageInvoker> invoker,
                         CScriptInvocationManager* invocationManager,
                         bool reuseable);
  ~CLanguageInvokerThread() override;

  virtual InvokerState GetState() const;

  std::shared_ptr<ILanguageInvoker> GetInvoker() const { return m_invoker; }
  bool Reuseable(const std::string& script) const
  {
    // Cheap, independently-synchronised checks first, so the common "not
    // reusable" answer does not contend with a running script for m_mutex.
    if (m_bStop || IsClaimed() || IsProcessDone() || GetState() != InvokerStateScriptDone)
      return false;

    // m_script and m_reusable are written by execute() and Process() under
    // m_mutex. Reading them unlocked from the manager thread was a data race on
    // a std::string: a concurrent assignment can reallocate the buffer while
    // the comparison walks it.
    std::unique_lock<std::mutex> lck(m_mutex);
    return m_reusable && m_script == script;
  }

  /*!
   * \brief Try to take the claim on this thread.
   *
   * A claim marks the thread as spoken for from the moment it is handed out
   * until its owner drops it, which is what covers the gap between Reset() and
   * the script actually running. Only one claim can be held at a time.
   *
   * \return True if the claim was taken, false if somebody else already holds it
   */
  bool Claim();

  //! \brief Drop the claim taken by Claim().
  void ReleaseClaim();

  bool IsClaimed() const { return m_claimed; }

  //! \brief True once Release()/stop() has asked this thread to finish.
  //! Deliberately not IsStopping(): that is an ILanguageInvoker virtual meaning
  //! "the invoker reached InvokerStateStopping", which is a different question.
  bool IsStopRequested() const { return m_bStop; }

  /*!
   * \brief True once the Process() loop has left and will accept no more work.
   *
   * CThread::IsRunning() stays true until OnExit() (and Py_EndInterpreter) have
   * finished, so it cannot answer this on its own.
   */
  bool IsProcessDone() const { return m_processDone; }

  /*!
   * \brief True unless this thread is idle and safe to retire.
   *
   * IsActive() cannot answer this on its own: it is false for
   * InvokerStateUninitialized, which is where a thread sits both when a claimer
   * has Reset() it and when it has been published as the reusable one but has
   * not started. Releasing either loses a script that nothing will run again.
   * The state alone cannot answer it either - a claim dropped without ever
   * running leaves the same Uninitialized, and that one must stay releasable or
   * it pins the slot for the session.
   */
  bool IsBusy() const
  {
    if (IsClaimed())
      return true;

    // The loop has left. Nothing more can run here whatever the invoker says,
    // and the slot has to be given up or it is pinned for good.
    if (IsProcessDone())
      return false;

    // Published as the reusable thread, but execute() has not reached Create()
    // yet. A Release() landing here sets m_bStop, Create() clears it again, and
    // the loop then parks for ever on a thread nothing can reach: the
    // interpreter is retained and the script stays marked running.
    if (!CThread::IsRunning())
      return true;

    // The loop is parked. Busy only if a script is already running or queued -
    // execute() returns as soon as it has set the restart, and the handle-less
    // path drops its claim at that point, so the queued script is all that is
    // left marking the thread. An invoker left Uninitialized by a claim that
    // was dropped without running is idle and must stay releasable.
    return m_invoker->IsActive() || HasQueuedScript();
  }

  virtual void Release();

protected:
  bool execute(const std::string &script, const std::vector<std::string> &arguments) override;
  bool stop(bool wait) override;

  void OnStartup() override;
  void Process() override;
  void OnExit() override;
  void OnException() override;

private:
  //! \brief True once execute() has queued a script the Process() loop has not taken yet.
  bool HasQueuedScript() const
  {
    std::unique_lock<std::mutex> lck(m_mutex);
    return m_restart;
  }

  std::shared_ptr<ILanguageInvoker> m_invoker;
  CScriptInvocationManager *m_invocationManager;
  std::string m_script;
  std::vector<std::string> m_args;

  //! Guards m_script, m_args, m_restart and m_reusable. Taken while m_critSection is held
  //! (CScriptInvocationManager -> Reuseable()/Release()); never the other way
  //! round, which is why Process() drops it across Execute().
  mutable std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_restart = false;
  bool m_reusable = false;
  std::atomic<bool> m_claimed{false};
  std::atomic<bool> m_processDone{false};
};
