
#include "GUIWindowFullScreen.h"

CGUIWindowFullScreen::CGUIWindowFullScreen(void)
:CGUIWindow(0)
{
}

CGUIWindowFullScreen::~CGUIWindowFullScreen(void)
{
}


void CGUIWindowFullScreen::OnKey(const CKey& key)
{

  CGUIWindow::OnKey(key);
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{
 return CGUIWindow::OnMessage(message);
}
