#include "include.h"
#include "GUIStandardWindow.h"
#include "GUIWindowManager.h"


CGUIStandardWindow::CGUIStandardWindow(void) : CGUIWindow(0, "")
{
}

CGUIStandardWindow::~CGUIStandardWindow(void)
{
}

bool CGUIStandardWindow::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}
