#include "stdafx.h"
#include "GUIDialogPlayerControls.h"
#include "Application.h"

CGUIDialogPlayerControls::CGUIDialogPlayerControls(void)
    : CGUIDialog(0)
{
}

CGUIDialogPlayerControls::~CGUIDialogPlayerControls(void)
{
}

void CGUIDialogPlayerControls::OnAction(const CAction &action)
{
  if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
    Close();
    return ;
  }
  CGUIDialog::OnAction(action);
}

void CGUIDialogPlayerControls::Render()
{
  if (!g_application.IsPlaying() || m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
    Close();
  else
    CGUIDialog::Render();
}
