#include "include.h"
#include "GUIDialog.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "GUIAudioManager.h"
#include "../xbmc/utils/SingleLock.h"
#include "../xbmc/Application.h"


CGUIDialog::CGUIDialog(DWORD dwID, const CStdString &xmlFile)
    : CGUIWindow(dwID, xmlFile)
{
  m_bModal = true;
  m_bRunning = false;
  m_dialogClosing = false;
  m_renderOrder = 1;
}

CGUIDialog::~CGUIDialog(void)
{}

bool CGUIDialog::Load(const CStdString& strFileName, bool bContainsPath)
{
  m_renderOrder = 1;
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
          float spacing = (pLabel->GetXPosition() - pBase->GetXPosition()) * 2;
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
      if (pWindow)
        m_gWindowManager.ShowOverlay(pWindow->GetOverlayState());

      CGUIWindow::OnMessage(message);
      // if we were running, make sure we remove ourselves from the window manager
      if (m_bRunning)
      {
        m_gWindowManager.RemoveDialog(GetID());
        m_bRunning = false;
        m_dialogClosing = false;
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

  //  Play the window specific deinit sound
  g_audioManager.PlayWindowSound(GetID(), SOUND_DEINIT);

  // don't close if we should be animating
  if (!forceClose && HasAnimation(ANIM_TYPE_WINDOW_CLOSE))
  {
    if (!m_dialogClosing && !IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
    {
      QueueAnimation(ANIM_TYPE_WINDOW_CLOSE);
      m_dialogClosing = true;
    }
    return;
  }

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  OnMessage(msg);
}

void CGUIDialog::DoModal(int iWindowID /*= WINDOW_INVALID */)
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  m_dialogClosing = false;
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

  if (!m_windowLoaded)
    Close(true);

  lock.Leave();

  while (m_bRunning && !g_application.m_bStop)
  {
    m_gWindowManager.Process();
  }
}

void CGUIDialog::Show()
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (m_bRunning && !m_dialogClosing && !IsAnimating(ANIM_TYPE_WINDOW_CLOSE)) return;

  m_bModal = false;
  
  // set running before it's added to the window manager, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_bRunning = true;
  m_dialogClosing = false;
  m_gWindowManager.AddModeless(this);

  //  Play the window specific init sound
  g_audioManager.PlayWindowSound(GetID(), SOUND_INIT);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);

//  m_bRunning = true;
}

bool CGUIDialog::RenderAnimation(DWORD time)
{
  CGUIWindow::RenderAnimation(time);
  return m_bRunning;
}

void CGUIDialog::Render()
{
  CGUIWindow::Render();
  // Check to see if we should close at this point
  // We check after the controls have finished rendering, as we may have to close due to
  // the controls rendering after the window has finished it's animation
  // we call the base class instead of this class so that we can find the change
  if (m_dialogClosing && !CGUIWindow::IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
  {
    Close(true);
  }
}

bool CGUIDialog::IsAnimating(ANIMATION_TYPE animType)
{
  if (animType == ANIM_TYPE_WINDOW_CLOSE)
    return m_dialogClosing;
  return CGUIWindow::IsAnimating(animType);
}

void CGUIDialog::SetDefaults()
{
  CGUIWindow::SetDefaults();
  m_renderOrder = 1;
}