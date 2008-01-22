#ifndef GUIDIALOG_PLUGIN_SETTINGS_
#define GUIDIALOG_PLUGIN_SETTINGS_

#include "PluginSettings.h"

struct SScraperInfo;

class CGUIDialogPluginSettings : public CGUIDialogBoxBase
{
public:
  CGUIDialogPluginSettings(void);
  virtual ~CGUIDialogPluginSettings(void);
  virtual bool OnMessage(CGUIMessage& message);
  static void ShowAndGetInput(CURL& url);
  static void ShowAndGetInput(SScraperInfo& info);
    
private:
  void CreateControls();
  void FreeControls();
  void EnableControls();
  void SetDefaults();
  bool GetCondition(const CStdString &condition, const int controlId);
  
  bool SaveSettings(void);
  void ShowVirtualKeyboard(int iControl);
  static CURL m_url;
  bool TranslateSingleString(const CStdString &strCondition, vector<CStdString> &enableVec);
  CBasicSettings m_settings;
  CStdString m_strHeading;
};

#endif

