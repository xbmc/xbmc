#include "include.h"
#include "GUIDialog.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "GUIAudioManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGUIDialog::CGUIDialog(DWORD dwID)
    : CGUIWindow(dwID)
{
  m_dwParentWindowID = 0;
  m_pParentWindow = NULL;
  m_bModal = true;
  m_bRunning = false;
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
          pLabel->m_dwTextAlign |= XBFONT_TRUNCATED;
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
      if (pWindow && pWindow->OverlayAllowed() >= 0)
        g_graphicsContext.SetOverlay(pWindow->OverlayAllowed() == 1);
      break;
    }
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIDialog::Close()
{
  if (!m_bRunning) return;

  //  Play the window specific deinit sound
  g_audioManager.PlayWindowSound(GetID(), SOUND_DEINIT);

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  OnMessage(msg);

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

void CGUIDialog::DoModal(DWORD dwParentId)
{
  m_dwParentWindowID = dwParentId;
  m_pParentWindow = m_gWindowManager.GetWindow( m_dwParentWindowID);

  if (!m_pParentWindow)
  {
    m_dwParentWindowID = 0;
    return ;
  }

  m_bModal = true;
  m_gWindowManager.RouteToWindow(this);

  //  Play the window specific init sound
  g_audioManager.PlayWindowSound(GetID(), SOUND_INIT);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);

  m_bRunning = true;
  while (m_bRunning)
  {
    m_gWindowManager.Process();
  }
}

void CGUIDialog::Show(DWORD dwParentId)
{
  if (m_bRunning) return;

  m_dwParentWindowID = dwParentId;
  m_pParentWindow = m_gWindowManager.GetWindow( m_dwParentWindowID);

  if (!m_pParentWindow)
  {
    m_dwParentWindowID = 0;
    return ;
  }

  m_bModal = false;
  m_gWindowManager.AddModeless(this);

  //  Play the window specific init sound
  g_audioManager.PlayWindowSound(GetID(), SOUND_INIT);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);

  m_bRunning = true;
}
