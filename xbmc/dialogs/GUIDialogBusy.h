/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <chrono>
#include <cstdint>
#include <optional>

class IRunnable;
class CEvent;

class CGUIDialogBusy : private CGUIDialog
{
  friend class CGUIWindowManager;

public:
  /*! \brief Wait for a runnable to execute off-thread.
   Creates a thread to run the given runnable, and while waiting
   it displays the busy dialog.
   \param runnable the IRunnable to run.
   \param displaytime the time in ms to wait prior to showing the busy dialog (defaults to 100ms)
   \param allowCancel whether the user can cancel the wait, defaults to true.
   \return true if the runnable completes, false if the user cancels early.
   */
  static bool Wait(IRunnable *runnable, unsigned int displaytime, bool allowCancel);

  /*! \brief Wait on an event while displaying the busy dialog.
   Throws up the busy dialog after the given time.
   \param event the CEvent to wait on.
   \param displaytime the time in ms to wait prior to showing the busy dialog (defaults to 100ms)
   \param allowCancel whether the user can cancel the wait, defaults to true.
   \return true if the event completed, false if cancelled.
   */
  static bool WaitOnEvent(CEvent& event, unsigned int displaytime = 100, bool allowCancel = true);

  //! \brief How a wait on an event ended.
  enum class WaitResult
  {
    COMPLETED, //!< the event was set
    CANCELLED, //!< the user cancelled the wait
    TIMED_OUT, //!< the timeout passed first
  };

  /*! \brief Wait on an event for at most \p timeout, displaying the busy dialog as WaitOnEvent
   does.
   \param event the CEvent to wait on.
   \param timeout how long to wait for the event.
   \param displaytime the time in ms to wait prior to showing the busy dialog (defaults to 100ms)
   \param allowCancel whether the user can cancel the wait, defaults to true.
   \return how the wait ended.
   */
  static WaitResult WaitOnEventFor(CEvent& event,
                                   std::chrono::milliseconds timeout,
                                   unsigned int displaytime = 100,
                                   bool allowCancel = true);

private:
  static WaitResult DoWaitOnEvent(CEvent& event,
                                  unsigned int displaytime,
                                  bool allowCancel,
                                  std::optional<std::chrono::milliseconds> timeout);

  CGUIDialogBusy();
  ~CGUIDialogBusy() override;

  void Open_Internal(bool bProcessRenderLoop, const std::string& param = "") override;
  bool OnBack(int actionID) override;
  void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;

  bool m_bLastVisible{false};
  bool m_cancelled{false};
  uint32_t m_waiters{0};
};
