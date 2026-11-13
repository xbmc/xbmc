/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <vector>

/*!
 \brief While one lives on a thread, the notifications raised by the scripts that thread starts are
 held rather than shown, and shown when it goes, unless dropped first. A caller about to try a
 script again drops what the failed try said, as the next try says how it went.

 One made while another lives on the thread hands what it holds to that one when it goes.
 */
class CHeldNotifications
{
public:
  CHeldNotifications();
  ~CHeldNotifications();
  CHeldNotifications(const CHeldNotifications&) = delete;
  CHeldNotifications& operator=(const CHeldNotifications&) = delete;

  //! \brief Lets go of the notifications held so far without showing them
  void Drop();

  /*!
   \brief Called on the thread starting the script \p scriptId, before it starts: this run's
   notifications go to the thread's holder if it has one, else are shown.
   */
  static void ScriptStarting(int scriptId);

  /*!
   \brief Holds a notification the script \p scriptId raises, if it was started by a thread with a
   holder.
   \param show shows the notification
   \return false if nothing holds it, for the caller to show it
   */
  static bool Hold(long scriptId, std::function<void()> show);

private:
  CHeldNotifications* const m_previous;
  //! Guarded by the lock on which holder each script's notifications go to, as scripts add to it
  //! from threads of their own
  std::vector<std::function<void()>> m_held;
};
