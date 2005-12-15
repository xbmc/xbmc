#pragma once
#include "guiwindow.h"

class CGUIWindowSettingsUICalibration :
      public CGUIWindow
{
public:
  CGUIWindowSettingsUICalibration(void);
  virtual ~CGUIWindowSettingsUICalibration(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void ResetControls();
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  virtual void OnWindowLoaded();
#endif

protected:
  int m_control;
};
