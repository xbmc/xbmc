/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogBoxBase.h"
#include "IProgressCallback.h"

class CGUIDialogProgress :
      public CGUIDialogBoxBase, public IProgressCallback
{
public:
  CGUIDialogProgress(void);
  ~CGUIDialogProgress(void) override;

  void Open(const std::string &param = "");
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;
  void OnWindowLoaded() override;
  void Progress();
  bool IsCanceled() const { return m_bCanceled; }
  void SetPercentage(int iPercentage);
  int GetPercentage() const { return m_percentage; };
  void ShowProgressBar(bool bOnOff);

  /*! \brief Wait for the progress dialog to be closed or canceled, while regularly
   rendering to allow for pointer movement or progress to be shown. Used when showing
   the progress of a process that is taking place on a separate thread and may be
   reporting progress infrequently.
   \param progresstime the time in ms to wait between rendering the dialog (defaults to 10ms)
   \return true if the dialog is closed, false if the user cancels early.
   */
  bool Wait(int progresstime = 10);

  /*! \brief Wait on an event or for the progress dialog to be canceled, while
  regularly rendering to allow for pointer movement or progress to be shown.
  \param event the CEvent to wait on.
  \return true if the event completed, false if cancelled.
  */
  bool WaitOnEvent(CEvent& event);

  // Implements IProgressCallback
  void SetProgressMax(int iMax) override;
  void SetProgressAdvance(int nSteps=1) override;
  bool Abort() override;

  void SetCanCancel(bool bCanCancel);

protected:
  void OnInitWindow() override;
  int GetDefaultLabelID(int controlId) const override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;

  bool m_bCanCancel;
  bool m_bCanceled;

  int  m_iCurrent;
  int  m_iMax;
  int m_percentage;
  bool m_showProgress;

private:
  void Reset();
};
