#include "include.h"
#include "GUIDialog.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "GUIAudioManager.h"


CGUIDialog::CGUIDialog(DWORD dwID, const CStdString &xmlFile)
    : CGUIWindow(dwID, xmlFile)
{
  m_dwParentWindowID = 0;
  m_pParentWindow = NULL;
  m_bModal = true;
  m_bRunning = false;
  m_dialogClosing = 0;
  m_renderOrder = 1;
}

CGUIDialog::~CGUIDialog(void)
{}

bool CGUIDialog::Load(const CStdString& strFileName, bool bContainsPath)
{
  if (!CGUIWindow::Load(strFileName, bContainsPath))
  {
    return false;
  }

  // Clip labels to extents
  if (m_vecControls.size())
  {
    CGUIControl* pBase = m_vecControls[0];

    for (ivecControls p = m_vecControls.begin() + 1; p != m_vecControls.end(); ++p)
    {
      if ((*p)->GetControlType() == CGUIControl::GUICONTROL_LABEL)
      {
        CGUILabelControl* pLabel = (CGUILabelControl*)(*p);

        if (!pLabel->GetWidth())
        {
          int spacing = (pLabel->GetXPosition() - pBase->GetXPosition()) * 2;
          pLabel->SetWidth(pBase->GetWidth() - spacing);
          pLabel->SetTruncate(true);
        }
      }
    }
  }

  return true;
}

bool CGUIDialog::OnAction(const CAction &action)
{
  if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
    Close();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIDialog::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow *pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
      if (pWindow && pWindow->GetOverlayState()!=OVERLAY_STATE_PARENT_WINDOW)
        m_gWindowManager.ShowOverlay(pWindow->GetOverlayState()==OVERLAY_STATE_SHOWN);

      CGUIWindow::OnMessage(message);
      // if we were running, make sure we remove ourselves from the window manager
      if (m_bRunning)
      {
        if (m_bModal)
        {
          m_gWindowManager.UnRoute(GetID());
        }
        else
        {
          m_gWindowManager.RemoveModeless( GetID() );
        }

        m_pParentWindow = NULL;
        m_bRunning = false;
      }
      return true;
    }
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      return true;
    }
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIDialog::Close(bool forceClose /*= false*/)
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (!m_bRunning) return;

  CLog::DebugLog("Dialog::Close called");
  // don't close if we should be animating
  if (!forceClose && HasAnimation(ANIM_TYPE_WINDOW_CLOSE))
  {
    if (!IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
      QueueAnimation(ANIM_TYPE_WINDOW_CLOSE);
    return;
  }

  //  Play the window specific deinit sound
  g_audioManager.PlayWindowSound(GetID(), SOUND_DEINIT);

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  OnMessage(msg);
}

void CGUIDialog::DoModal(DWORD dwParentId, int iWindowID /*= WINDOW_INVALID */)
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  m_dwParentWindowID = dwParentId;
  m_pParentWindow = m_gWindowManager.GetWindow( m_dwParentWindowID);

  if (!m_pParentWindow)
  {
    m_dwParentWindowID = 0;
    return ;
  }
  
  m_bModal = true;
  // set running before it's added to the window manager, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_bRunning = true;
  m_gWindowManager.RouteToWindow(this);

  //  Play the window specific init sound
  g_audioManager.PlayWindowSound(GetID(), SOUND_INIT);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, iWindowID);
  OnMessage(msg);

//  m_bRunning = true;

  lock.Leave();
  while (m_bRunning)
  {
    m_gWindowManager.Process();
  }
}

void CGUIDialog::Show(DWORD dwParentId)
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (m_bRunning && !IsAnimating(ANIM_TYPE_WINDOW_CLOSE)) return;

  m_dwParentWindowID = dwParentId;
  m_pParentWindow = m_gWindowManager.GetWindow( m_dwParentWindowID);

  if (!m_pParentWindow)
  {
    m_dwParentWindowID = 0;
    return ;
  }

  m_bModal = false;
  
  // set running before it's added to the window manager, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_bRunning = true;
  m_gWindowManager.AddModeless(this);

  //  Play the window specific init sound
  g_audioManager.PlayWindowSound(GetID(), SOUND_INIT);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);

//  m_bRunning = true;
}

void CGUIDialog::OnWindowCloseAnimation()
{
}

void CGUIDialog::UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState)
{
  // Make sure dialog is closed at the appropriate time
  if (type == ANIM_TYPE_WINDOW_OPEN)
  {
    if (currentProcess == ANIM_PROCESS_REVERSE && currentState == ANIM_STATE_APPLIED)
      Close(true);
  }
  else if (type == ANIM_TYPE_WINDOW_CLOSE)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL && currentState == ANIM_STATE_APPLIED)
      Close(true);
  }
}

bool CGUIDialog::RenderAnimation()
{
  CGUIWindow::RenderAnimation();
  // debug stuff
/*  CAnimation anim = m_showAnimation;
  if (anim.currentProcess != ANIM_PROCESS_NONE)
  {
    if (anim.effect == EFFECT_TYPE_SLIDE)
    {
      if (IsRunning())
        CLog::Log(LOGDEBUG, "Animating dialog %d with a %s slide effect %s. Amount is %2.1f, visible=%s", GetID(), anim.type == ANIM_TYPE_WINDOW_OPEN ? "show" : "close", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsRunning() ? "true" : "false");
    }
    else if (anim.effect == EFFECT_TYPE_FADE)
    {
      if (IsRunning())
        CLog::Log(LOGDEBUG, "Animating dialog %d with a %s fade effect %s. Amount is %2.1f. Visible=%s", GetID(), anim.type == ANIM_TYPE_WINDOW_OPEN ? "show" : "close", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsRunning() ? "true" : "false");
    }
  }
  anim = m_closeAnimation;
  if (anim.currentProcess != ANIM_PROCESS_NONE)
  {
    if (anim.effect == EFFECT_TYPE_SLIDE)
    {
      if (IsRunning())
        CLog::Log(LOGDEBUG, "Animating dialog %d with a %s slide effect %s. Amount is %2.1f, visible=%s", GetID(), anim.type == ANIM_TYPE_WINDOW_OPEN ? "show" : "close", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsRunning() ? "true" : "false");
    }
    else if (anim.effect == EFFECT_TYPE_FADE)
    {
      if (IsRunning())
        CLog::Log(LOGDEBUG, "Animating dialog %d with a %s fade effect %s. Amount is %2.1f. Visible=%s", GetID(), anim.type == ANIM_TYPE_WINDOW_OPEN ? "show" : "close", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsRunning() ? "true" : "false");
    }
  }*/
  return m_bRunning;
}

void CGUIDialog::Render()
{
  CGUIWindow::Render();
}