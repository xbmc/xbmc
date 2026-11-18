/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"
#include "addons/IAddon.h"
#include "threads/Event.h"

#include <atomic>
#include <string>

class CScriptRunner
{
public:
  /*!
   * \brief While one lives on a thread, the thread stops waiting for an add-on script it runs, off
   * the GUI thread, once \p stop is set: the run is reported as failed at once, and the script runs
   * on to its end, what it returns dropped. A run the thread would start once the flag is set, or
   * while it waits for its kept thread, isn't started, and is reported as failed. Scopes nest, and
   * the flag of any of them stops the wait.
   */
  class CAbandonWhen
  {
  public:
    explicit CAbandonWhen(const std::atomic<bool>& stop);
    ~CAbandonWhen();
    CAbandonWhen(const CAbandonWhen&) = delete;
    CAbandonWhen& operator=(const CAbandonWhen&) = delete;

    //! Whether the flag of a scope living on the calling thread is set
    static bool IsSet();

  private:
    const std::atomic<bool>& m_stop;
    const CAbandonWhen* const m_outer;
  };

protected:
  CScriptRunner();
  virtual ~CScriptRunner() = default;

  virtual bool IsSuccessful() const = 0;
  virtual bool IsCancelled() const = 0;

  ADDON::AddonPtr GetAddon() const;

  bool RunScript(const ADDON::AddonPtr& addon, const CURL& url, int handle, bool resume);

  void SetDone();

  static int ExecuteScript(const ADDON::AddonPtr& addon, const CURL& url, bool resume);
  static int ExecuteScript(const ADDON::AddonPtr& addon, CURL url, int handle, bool resume);

private:
  bool RunScriptInternal(
      const ADDON::AddonPtr& addon, const CURL& url, int handle, bool resume, bool wait = true);
  bool WaitOnScriptResult(int scriptId, const std::string& path, const std::string& name);

  ADDON::AddonPtr m_addon;

  CEvent m_scriptDone;
};
