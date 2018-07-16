/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogBusy.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUIWindowManager.h"
#include "threads/Thread.h"
#include "threads/IRunnable.h"

#define PROGRESS_CONTROL 10

class CBusyWaiter : public CThread
{
  std::shared_ptr<CEvent> m_done;
  IRunnable *m_runnable;
public:
  explicit CBusyWaiter(IRunnable *runnable) :
  CThread(runnable, "waiting"), m_done(new CEvent()),  m_runnable(runnable) { }

  ~CBusyWaiter()
  {
    StopThread();
  }

  bool Wait(unsigned int displaytime, bool allowCancel)
  {
    std::shared_ptr<CEvent> e_done(m_done);

    Create();
    unsigned int start = XbmcThreads::SystemClockMillis();
    if (!CGUIDialogBusy::WaitOnEvent(*e_done, displaytime, allowCancel))
    {
      m_runnable->Cancel();
      unsigned int elapsed = XbmcThreads::SystemClockMillis() - start;
      unsigned int remaining = (elapsed >= displaytime) ? 0 : displaytime - elapsed;
      CGUIDialogBusy::WaitOnEvent(*e_done, remaining, false);
      return false;
    }
    return true;
  }

  // 'this' is actually deleted from the thread where it's on the stack
  void Process() override
  {
    std::shared_ptr<CEvent> e_done(m_done);

    CThread::Process();
    (*e_done).Set();
  }

};

bool CGUIDialogBusy::Wait(IRunnable *runnable, unsigned int displaytime, bool allowCancel)
{
  if (!runnable)
    return false;
  CBusyWaiter waiter(runnable);
  if (!waiter.Wait(displaytime, allowCancel))
  {
    return false;
  }
  return true;
}

bool CGUIDialogBusy::WaitOnEvent(CEvent &event, unsigned int displaytime /* = 100 */, bool allowCancel /* = true */)
{
  bool cancelled = false;
  if (!event.WaitMSec(displaytime))
  {
    // throw up the progress
    CGUIDialogBusy* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusy>(WINDOW_DIALOG_BUSY);
    if (dialog)
    {
      if (dialog->IsDialogRunning())
      {
        throw std::logic_error("busy dialog already running");
      }

      dialog->Open();

      while(!event.WaitMSec(1))
      {
        dialog->ProcessRenderLoop(false);
        if (allowCancel && dialog->IsCanceled())
        {
          cancelled = true;
          break;
        }
      }

      dialog->Close(true);
    }
  }
  return !cancelled;
}

CGUIDialogBusy::CGUIDialogBusy(void)
  : CGUIDialog(WINDOW_DIALOG_BUSY, "DialogBusy.xml", DialogModalityType::MODAL)
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_bCanceled = false;
  m_progress = -1;
}

CGUIDialogBusy::~CGUIDialogBusy(void) = default;

void CGUIDialogBusy::Open_Internal(const std::string &param /* = "" */)
{
  m_bCanceled = false;
  m_bLastVisible = true;
  m_progress = -1;

  CGUIDialog::Open_Internal(false, param);
}


void CGUIDialogBusy::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool visible = CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(WINDOW_DIALOG_BUSY);
  if(!visible && m_bLastVisible)
    dirtyregions.push_back(CDirtyRegion(m_renderRegion));
  m_bLastVisible = visible;

  // update the progress control if available
  CGUIControl *control = GetControl(PROGRESS_CONTROL);
  if (control && control->GetControlType() == CGUIControl::GUICONTROL_PROGRESS)
  {
    CGUIProgressControl *progress = static_cast<CGUIProgressControl*>(control);
    progress->SetPercentage(m_progress);
    progress->SetVisible(m_progress > -1);
  }

  CGUIDialog::DoProcess(currentTime, dirtyregions);
}

void CGUIDialogBusy::Render()
{
  if(!m_bLastVisible)
    return;
  CGUIDialog::Render();
}

bool CGUIDialogBusy::OnBack(int actionID)
{
  m_bCanceled = true;
  return true;
}

void CGUIDialogBusy::SetProgress(float percent)
{
  m_progress = percent;
}
