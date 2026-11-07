/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HeldNotifications.h"

#include <iterator>
#include <map>
#include <mutex>
#include <utility>

namespace
{
thread_local CHeldNotifications* current{nullptr};

std::mutex& Lock()
{
  static std::mutex lock;
  return lock;
}

//! The holder each script's notifications go to, by the script's id
std::map<long, CHeldNotifications*>& Scripts()
{
  static std::map<long, CHeldNotifications*> scripts;
  return scripts;
}
} // namespace

CHeldNotifications::CHeldNotifications() : m_previous(current)
{
  current = this;
}

CHeldNotifications::~CHeldNotifications()
{
  current = m_previous;

  std::vector<std::function<void()>> held;
  {
    std::unique_lock lock(Lock());
    std::erase_if(Scripts(), [this](const auto& script) { return script.second == this; });
    if (m_previous)
    {
      m_previous->m_held.insert(m_previous->m_held.end(), std::make_move_iterator(m_held.begin()),
                                std::make_move_iterator(m_held.end()));
      return;
    }
    held = std::move(m_held);
  }

  for (const auto& show : held)
    show();
}

void CHeldNotifications::Drop()
{
  std::unique_lock lock(Lock());
  m_held.clear();
}

void CHeldNotifications::ScriptStarting(int scriptId)
{
  std::unique_lock lock(Lock());
  // A thread kept to run a script again keeps its id, so a run replaces what an earlier one held
  if (current)
    Scripts()[scriptId] = current;
  else
    Scripts().erase(scriptId);
}

bool CHeldNotifications::Hold(long scriptId, std::function<void()> show)
{
  std::unique_lock lock(Lock());
  const auto script = Scripts().find(scriptId);
  if (script == Scripts().end())
    return false;

  script->second->m_held.push_back(std::move(show));
  return true;
}
