#ifndef GUIDIALOG_PLUGIN_SETTINGS_
#define GUIDIALOG_PLUGIN_SETTINGS_

#include "PluginSettings.h"

class CGUIDialogPluginSettings : public CGUIDialogBoxBase
{
public:
  CGUIDialogPluginSettings(void);
  virtual ~CGUIDialogPluginSettings(void);
  virtual bool OnMessage(CGUIMessage& message);
  static void ShowAndGetInput(CURL& url);
    
private:
  void CreateControls();
  void FreeControls();
  void EnableControls();
  
  bool SaveSettings(void);
  void ShowVirtualKeyboard(int iControl);
  static CURL m_url;
  bool TranslateSingleString(const CStdString &strCondition, vector<CStdString> &enableVec);
  CPluginSettings m_settings;
};

#endif
