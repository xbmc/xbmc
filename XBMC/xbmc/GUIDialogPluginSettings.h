#ifndef GUIDIALOG_PLUGIN_SETTINGS_
#define GUIDIALOG_PLUGIN_SETTINGS_

#include "tinyXML/tinyxml.h" 
#include "GUIDialogBoxBase.h"
#include "PluginSettings.h"
#include "GUIButtonControl.h"
#include "GUIRadioButtonControl.h"
#include "GUISpinControlEx.h"

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
  
  bool SaveSettings(void);
  void ShowVirtualKeyboard(int iControl);
  static CURL m_url;
  CPluginSettings m_settings;

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalButton;
};

#endif
