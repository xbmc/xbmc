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

#include "GUIDialog.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "input/Key.h"

using namespace KODI::MESSAGING;

CGUIDialog::CGUIDialog(int id, const std::string &xmlFile, DialogModalityType modalityType /* = DialogModalityType::MODAL */)
    : CGUIWindow(id, xmlFile)
{
  m_modalityType = modalityType;
  m_wasRunning = false;
  m_renderOrder = RENDER_ORDER_DIALOG;
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
    if (m_visibleCondition->Get())
      Open();
    else
      Close();
  }
  
  if (m_autoClosing)
  { // check if our timer is running
    if (!m_showStartTime)
    {
      if (HasProcessed()) // start timer
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
}

void CGUIDialog::Open_Internal(const std::string &param /* = "" */)
{
  CGUIDialog::Open_Internal(m_modalityType != DialogModalityType::MODELESS, param);
}

void CGUIDialog::Open_Internal(bool bProcessRenderLoop, const std::string &param /* = "" */)
{
  // Lock graphic context here as it is sometimes called from non rendering threads
  // maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (!g_windowManager.Initialized() ||
      (m_active && !m_closing && !IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
    return;

  // set running before it's added to the window manager, else the auto-show code
  // could show it as well if we are in a different thread from the main rendering
  // thread (this should really be handled via a thread message though IMO)
  m_active = true;
  m_closing = false;
  g_windowManager.RegisterDialog(this);

  // active this window
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  msg.SetStringParam(param);
  OnMessage(msg);

  // process render loop
  if (bProcessRenderLoop)
  {
    if (!m_windowLoaded)
      Close(true);

    lock.Leave();

    while (m_active && !g_application.m_bStop)
    {
      g_windowManager.ProcessRenderLoop();
    }
  }
}

void CGUIDialog::Open(const std::string &param /* = "" */)
{
  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    CSingleExit leaveIt(g_graphicsContext);
    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_DIALOG_OPEN, -1, -1, static_cast<void*>(this), param);
  }
  else
    Open_Internal(param);
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
  m_renderOrder = RENDER_ORDER_DIALOG;
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

void CGUIDialog::CancelAutoClose(void)
{
  m_autoClosing = false;
}

void CGUIDialog::ProcessRenderLoop(bool renderOnly)
{
  g_windowManager.ProcessRenderLoop(renderOnly);
}
