/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "threads/Event.h"

#include <string>

class CScriptRunner
{
protected:
  CScriptRunner();
  virtual ~CScriptRunner() = default;

  virtual bool IsSuccessful() const = 0;
  virtual bool IsCancelled() const = 0;

  ADDON::AddonPtr GetAddon() const;

  bool StartScript(const ADDON::AddonPtr& addon, const std::string& path);
  bool RunScript(const ADDON::AddonPtr& addon, const std::string& path, int handle, bool resume);

  void SetDone();

  static int ExecuteScript(const ADDON::AddonPtr& addon, const std::string& path, bool resume);
  static int ExecuteScript(const ADDON::AddonPtr& addon,
                           const std::string& path,
                           int handle,
                           bool resume);

private:
  bool RunScriptInternal(const ADDON::AddonPtr& addon,
                         const std::string& path,
                         int handle,
                         bool resume,
                         bool wait = true);
  bool WaitOnScriptResult(int scriptId, const std::string& path, const std::string& name);

  ADDON::AddonPtr m_addon;

  CEvent m_scriptDone;
};
