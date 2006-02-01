#pragma once

#include "GUIDialogSettings.h"

class CGUIDialogVideoSettings :
      public CGUIDialogSettings
{
public:
  CGUIDialogVideoSettings(void);
  virtual ~CGUIDialogVideoSettings(void);

protected:
  virtual void CreateSettings();
  virtual void OnSettingChanged(unsigned int setting);

  int m_flickerFilter;
  bool m_soften;
};
