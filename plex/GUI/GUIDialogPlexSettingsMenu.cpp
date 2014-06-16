
#include "GUI/GUIDialogPlexSettingsMenu.h"
#include "GUIWindowManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexSettingsMenu::OnBack(int actionID)
{
  // upon closing menu we want to return to home Screen
  g_windowManager.PreviousWindow();

  return CGUIDialog::OnBack(actionID);
}
