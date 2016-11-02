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

#include "GUIDialogBusy.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUIWindowManager.h"
#include "threads/Thread.h"

#define PROGRESS_CONTROL 10

class CBusyWaiter : public CThread
{
  std::shared_ptr<CEvent>  m_done;
public:
  CBusyWaiter(IRunnable *runnable) : CThread(runnable, "waiting"), m_done(new CEvent()) {  }
  
  bool Wait()
  {
    std::shared_ptr<CEvent> e_done(m_done);

    Create();
    return CGUIDialogBusy::WaitOnEvent(*e_done);
  }

  // 'this' is actually deleted from the thread where it's on the stack
  virtual void Process()
  {
    std::shared_ptr<CEvent> e_done(m_done);

    CThread::Process();
    (*e_done).Set();
  }

};

bool CGUIDialogBusy::Wait(IRunnable *runnable)
{
  if (!runnable)
    return false;
  CBusyWaiter waiter(runnable);
  return waiter.Wait();
}

bool CGUIDialogBusy::WaitOnEvent(CEvent &event, unsigned int displaytime /* = 100 */, bool allowCancel /* = true */)
{
  bool cancelled = false;
  if (!event.WaitMSec(displaytime))
  {
    // throw up the progress
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    if (dialog)
    {
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
      
      dialog->Close();
    }
  }
  return !cancelled;
}

CGUIDialogBusy::CGUIDialogBusy(void)
  : CGUIDialog(WINDOW_DIALOG_BUSY, "DialogBusy.xml", DialogModalityType::PARENTLESS_MODAL),
    m_bLastVisible(false)
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_bCanceled = false;
  m_progress = -1;
}

CGUIDialogBusy::~CGUIDialogBusy(void)
{
}

void CGUIDialogBusy::Open_Internal(const std::string &param /* = "" */)
{
  m_bCanceled = false;
  m_bLastVisible = true;
  m_progress = -1;

  CGUIDialog::Open_Internal(false, param);
}


void CGUIDialogBusy::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool visible = g_windowManager.GetTopMostModalDialogID() == WINDOW_DIALOG_BUSY;
  if(!visible && m_bLastVisible)
    dirtyregions.push_back(m_renderRegion);
  m_bLastVisible = visible;

  // update the progress control if available
  const CGUIControl *control = GetControl(PROGRESS_CONTROL);
  if (control && control->GetControlType() == CGUIControl::GUICONTROL_PROGRESS)
  {
    CGUIProgressControl *progress = (CGUIProgressControl *)control;
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
