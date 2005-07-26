#include "stdafx.h"
#include "GUIDialogProgress.h"
#include "GUIProgressControl.h"
#include "application.h"


#define CONTROL_PROGRESS_BAR 20

CGUIDialogProgress::CGUIDialogProgress(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_PROGRESS, "DialogProgress.xml")
{
  m_bCanceled = false;
}

CGUIDialogProgress::~CGUIDialogProgress(void)
{}


void CGUIDialogProgress::StartModal(DWORD dwParentId)
{
  m_bCanceled = false;
  m_dwParentWindowID = dwParentId;
  m_pParentWindow = m_gWindowManager.GetWindow( m_dwParentWindowID);
  if (!m_pParentWindow)
  {
    m_dwParentWindowID = 0;
    return ;
  }

  m_gWindowManager.RouteToWindow(this);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);
  ShowProgressBar(false);
  SetPercentage(0);
  m_bRunning = true;
}

void CGUIDialogProgress::Progress()
{
  if (m_bRunning)
  {
    m_gWindowManager.Process();
  }
}

void CGUIDialogProgress::ProgressKeys()
{
  if (m_bRunning)
  {
    g_application.FrameMove();
  }
}

bool CGUIDialogProgress::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {

  case GUI_MSG_CLICKED:
    {
      int iAction = message.GetParam1();
      if (1 || ACTION_SELECT_ITEM == iAction)
      {
        int iControl = message.GetSenderId();
        if (iControl == 10)
        {
          m_bCanceled = true;
          return true;
        }
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogProgress::OnAction(const CAction &action)
{
  if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
    m_bCanceled = true;
    return true;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogProgress::SetPercentage(int iPercentage)
{
  if (iPercentage < 0) iPercentage = 0;
  if (iPercentage > 100) iPercentage = 100;
  CGUIProgressControl* pControl = (CGUIProgressControl*)GetControl(CONTROL_PROGRESS_BAR);
  if (pControl) pControl->SetPercentage((float)iPercentage);
}
void CGUIDialogProgress::ShowProgressBar(bool bOnOff)
{
  if (bOnOff)
  {
    CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), CONTROL_PROGRESS_BAR);
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), CONTROL_PROGRESS_BAR);
    OnMessage(msg);
  }
}

