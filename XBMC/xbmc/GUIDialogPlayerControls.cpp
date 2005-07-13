#include "stdafx.h"
#include "GUIDialogPlayerControls.h"
#include "Application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGUIDialogPlayerControls::CGUIDialogPlayerControls(void)
    : CGUIDialog(0)
{
}

CGUIDialogPlayerControls::~CGUIDialogPlayerControls(void)
{
}

void CGUIDialogPlayerControls::Render()
{
  if (!g_application.IsPlaying() || m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
    Close();
  else
    CGUIDialog::Render();
}
