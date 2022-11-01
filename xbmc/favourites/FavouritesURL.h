/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/ExecString.h"

#include <string>
#include <vector>

class CFileItem;

class CFavouritesURL
{
public:
  enum class Action
  {
    UNKNOWN,
    ACTIVATE_WINDOW,
    PLAY_MEDIA,
    SHOW_PICTURE,
    RUN_SCRIPT,
    RUN_ADDON,
    START_ANDROID_ACTIVITY,
  };

  explicit CFavouritesURL(const std::string& favouritesURL);
  explicit CFavouritesURL(const CExecString& execString);
  CFavouritesURL(Action action, const std::vector<std::string>& params);
  CFavouritesURL(const CFileItem& item, int contextWindow);

  virtual ~CFavouritesURL() = default;

  std::string GetURL() const { return m_path; }

  bool IsValid() const { return m_valid && m_exec.IsValid(); }

  bool IsDir() const { return m_isDir; }

  std::string GetExecString() const { return m_exec.GetExecString(); }
  Action GetAction() const { return m_action; }
  std::vector<std::string> GetParams() const { return m_exec.GetParams(); }
  std::string GetTarget() const { return m_target; }
  int GetWindowID() const { return m_windowId; }
  std::string GetActionLabel() const { return m_actionLabel; }
  std::string GetProviderLabel() const { return m_providerLabel; }

private:
  bool Parse(CFavouritesURL::Action action, const std::vector<std::string>& params);

  CExecString m_exec;

  bool m_valid{false};
  std::string m_path;
  Action m_action{Action::UNKNOWN};
  std::string m_target;
  int m_windowId{-1};
  bool m_isDir{false};
  std::string m_actionLabel;
  std::string m_providerLabel;
};
