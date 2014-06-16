#ifndef GUIDIALOGPLEXSETTINGSMENU_H
#define GUIDIALOGPLEXSETTINGSMENU_H

#include "xbmc/guilib/GUIDialog.h"

class CGUIDialogPlexSettingsMenu : public CGUIDialog
{
public:
  CGUIDialogPlexSettingsMenu() : CGUIDialog(WINDOW_DIALOG_SETTINGS_MENU, "Custom2_SettingsNav.xml")
  {
  }
  virtual bool OnBack(int actionID);
};

#endif // GUIDIALOGPLEXSETTINGSMENU_H
