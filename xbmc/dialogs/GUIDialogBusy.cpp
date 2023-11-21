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
#include "guilib/GUIWindowManager.h"
#include "threads/IRunnable.h"
#include "threads/Thread.h"
#include "utils/log.h"

using namespace std::chrono_literals;

class CBusyWaiter : public CThread
{
  std::shared_ptr<CEvent> m_done;
  IRunnable *m_runnable;
public:
  explicit CBusyWaiter(IRunnable *runnable) :
  CThread(runnable, "waiting"), m_done(new CEvent()),  m_runnable(runnable) { }

  ~CBusyWaiter() override { StopThread(); }

  bool Wait(unsigned int displaytime, bool allowCancel)
  {
    std::shared_ptr<CEvent> e_done(m_done);

    Create();
    auto start = std::chrono::steady_clock::now();
    if (!CGUIDialogBusy::WaitOnEvent(*e_done, displaytime, allowCancel))
    {
      m_runnable->Cancel();

      auto end = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

      unsigned int remaining =
          (duration.count() >= displaytime) ? 0 : displaytime - duration.count();
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
  if (!event.Wait(std::chrono::milliseconds(displaytime)))
  {
    // throw up the progress
    CGUIDialogBusy* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusy>(WINDOW_DIALOG_BUSY);
    if (dialog)
    {
      if (dialog->IsDialogRunning())
      {
        CLog::Log(LOGFATAL, "Logic error due to two concurrent busydialogs, this is a known issue. "
                            "The application will exit.");
        throw std::logic_error("busy dialog already running");
      }

      dialog->Open();

      while (!event.Wait(1ms))
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
}

CGUIDialogBusy::~CGUIDialogBusy(void) = default;

void CGUIDialogBusy::Open_Internal(bool bProcessRenderLoop, const std::string& param /* = "" */)
{
  m_bCanceled = false;
  m_bLastVisible = true;

  CGUIDialog::Open_Internal(false, param);
}


void CGUIDialogBusy::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool visible = CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(WINDOW_DIALOG_BUSY);
  if(!visible && m_bLastVisible)
    dirtyregions.emplace_back(m_renderRegion);
  m_bLastVisible = visible;

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
