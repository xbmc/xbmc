#pragma once

#include "GUIDialogSettings.h"

#ifdef WITH_LINKS_BROWSER

class CGUIDialogWebBrowserSettings :
      public CGUIDialogSettings
{
public:
  CGUIDialogWebBrowserSettings(void);
  virtual ~CGUIDialogWebBrowserSettings(void);

protected:
  virtual void CreateSettings();
  virtual void OnSettingChanged(unsigned int setting);

  CStdString m_strHomepage;

  int m_iFontSize;
  int m_iScaleImages;
  int m_iMarginTop;
  int m_iMarginBottom;
  int m_iMarginLeft;
  int m_iMarginRight;

};

#endif