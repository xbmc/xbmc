#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"

class IRunnable;
class CEvent;

class CGUIDialogBusy: public CGUIDialog
{
public:
  CGUIDialogBusy(void);
  ~CGUIDialogBusy(void) override;
  bool OnBack(int actionID) override;
  void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  /*! \brief set the current progress of the busy operation
   \param progress a percentage of progress
   */
  void SetProgress(float progress);

  bool IsCanceled() { return m_bCanceled; }

  /*! \brief Wait for a runnable to execute off-thread.
   Creates a thread to run the given runnable, and while waiting
   it displays the busy dialog.
   \param runnable the IRunnable to run.
   \param displaytime the time in ms to wait prior to showing the busy dialog (defaults to 100ms)
   \param allowCancel whether the user can cancel the wait, defaults to true.
   \return true if the runnable completes, false if the user cancels early.
   */
  static bool Wait(IRunnable *runnable, unsigned int displaytime = 100, bool allowCancel = true);

  /*! \brief Wait on an event while displaying the busy dialog.
   Throws up the busy dialog after the given time.
   \param event the CEvent to wait on.
   \param displaytime the time in ms to wait prior to showing the busy dialog (defaults to 100ms)
   \param allowCancel whether the user can cancel the wait, defaults to true.
   \return true if the event completed, false if cancelled.
   */
  static bool WaitOnEvent(CEvent &event, unsigned int displaytime = 100, bool allowCancel = true);
protected:
  void Open_Internal(const std::string &param = "") override;
  bool m_bCanceled;
  bool m_bLastVisible;
  float m_progress; ///< current progress
};
