#include "stdafx.h"
#include "GUIDialogPlayerControls.h"
#include "Application.h"


CGUIDialogPlayerControls::CGUIDialogPlayerControls(void)
    : CGUIDialog(WINDOW_DIALOG_PLAYER_CONTROLS, "PlayerControls.xml")
{
}

CGUIDialogPlayerControls::~CGUIDialogPlayerControls(void)
{
}

void CGUIDialogPlayerControls::Render()
{
  if (!g_application.IsPlaying() || m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
    Close(true);
  else
    CGUIDialog::Render();
}
