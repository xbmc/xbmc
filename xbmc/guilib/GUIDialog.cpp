/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "GUIDialog.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "GUIAudioManager.h"
#include "GUIInfoManager.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "Application.h"
#include "ApplicationMessenger.h"

CGUIDialog::CGUIDialog(int id, const CStdString &xmlFile)
    : CGUIWindow(id, xmlFile)
{
  m_bModal = true;
  m_wasRunning = false;
  m_renderOrder = 1;
  m_autoClosing = false;
  m_showStartTime = 0;
  m_showDuration = 0;
  m_enableSound = true;
  m_bAutoClosed = false;
}

CGUIDialog::~CGUIDialog(void)
{}

void CGUIDialog::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();

  // Clip labels to extents
  if (m_children.size())
  {
    CGUIControl* pBase = m_children[0];

    for (iControls p = m_children.begin() + 1; p != m_children.end(); ++p)
    {
      if ((*p)->GetControlType() == CGUIControl::GUICONTROL_LABEL)
      {
        CGUILabelControl* pLabel = (CGUILabelControl*)(*p);

        if (!pLabel->GetWidth())
        {
          float spacing = (pLabel->GetXPosition() - pBase->GetXPosition()) * 2;
          pLabel->SetWidth(pBase->GetWidth() - spacing);
        }
      }
    }
  }
}

bool CGUIDialog::OnAction(const CAction &action)
{
  // keyboard or controller movement should prevent autoclosing
  if (!action.IsMouse() && m_autoClosing)
    SetAutoClose(m_showDuration);

  return CGUIWindow::OnAction(action);
}

bool CGUIDialog::OnBack(int actionID)
{
  Close();
  return true;
}

bool CGUIDialog::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
      if (pWindow)
        g_windowManager.ShowOverlay(pWindow->GetOverlayState());

      CGUIWindow::OnMessage(message);
      return true;
    }
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      m_showStartTime = 0;
      return true;
    }
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIDialog::OnDeinitWindow(int nextWindowID)
{
  if (m_active)
  {
    g_windowManager.RemoveDialog(GetID());
    m_autoClosing = false;
  }
  CGUIWindow::OnDeinitWindow(nextWindowID);
}

void CGUIDialog::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  UpdateVisibility();

  // if we were running but now we're not, mark us dirty
  if (!m_active && m_wasRunning)
    dirtyregions.push_back(m_renderRegion);

  if (m_active)
    CGUIWindow::DoProcess(currentTime, dirtyregions);

  m_wasRunning = m_active;
}

void CGUIDialog::UpdateVisibility()
{
  if (m_visibleCondition)
  {
    if (g_infoManager.GetBoolValue(m_visibleCondition))
      Show();
    else
      Close();
  }
}

void CGUIDialog::DoModal_Internal(int iWindowID /*= WINDOW_INVALID */, const CStdString &param /* = "" */)
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (!g_windowManager.Initialized())
    return; // don't do anything

  m_closing = false;
  m_bModal = true;
  // set running before it's added to the window manager, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_active = true;
  g_windowManager.RouteToWindow(this);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, iWindowID);
  msg.SetStringParam(param);
  OnMessage(msg);

  if (!m_windowLoaded)
    Close(true);

  lock.Leave();

  while (m_active && !g_application.m_bStop)
  {
    g_windowManager.ProcessRenderLoop();
  }
}

void CGUIDialog::Show_Internal()
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (m_active && !m_closing && !IsAnimating(ANIM_TYPE_WINDOW_CLOSE)) return;

  if (!g_windowManager.Initialized())
    return; // don't do anything

  m_bModal = false;

  // set running before it's added to the window manager, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_active = true;
  m_closing = false;
  g_windowManager.AddModeless(this);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);
}

void CGUIDialog::DoModal(int iWindowID /*= WINDOW_INVALID */, const CStdString &param)
{
  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    CSingleExit leaveIt(g_graphicsContext);
    CApplicationMessenger::Get().DoModal(this, iWindowID, param);
  }
  else
    DoModal_Internal(iWindowID, param);
}

void CGUIDialog::Show()
{
  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    CSingleExit leaveIt(g_graphicsContext);
    CApplicationMessenger::Get().Show(this);
  }
  else
    Show_Internal();
}

void CGUIDialog::FrameMove()
{
  if (m_autoClosing)
  { // check if our timer is running
    if (!m_showStartTime)
    {
      if (HasRendered()) // start timer
        m_showStartTime = CTimeUtils::GetFrameTime();
    }
    else
    {
      if (m_showStartTime + m_showDuration < CTimeUtils::GetFrameTime() && !m_closing)
      {
        m_bAutoClosed = true;
        Close();
      }
    }
  }
  CGUIWindow::FrameMove();
}

void CGUIDialog::Render()
{
  if (!m_active)
    return;

  CGUIWindow::Render();
}

void CGUIDialog::SetDefaults()
{
  CGUIWindow::SetDefaults();
  m_renderOrder = 1;
}

void CGUIDialog::SetAutoClose(unsigned int timeoutMs)
{
   m_autoClosing = true;
   m_showDuration = timeoutMs;
   ResetAutoClose();
}

void CGUIDialog::ResetAutoClose(void)
{
  if (m_autoClosing && m_active)
    m_showStartTime = CTimeUtils::GetFrameTime();
}
