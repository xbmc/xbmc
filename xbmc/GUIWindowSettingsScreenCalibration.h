#pragma once
#include "GUIWindow.h"

class CGUIWindowSettingsScreenCalibration : public CGUIWindow
{
public:
  CGUIWindowSettingsScreenCalibration(void);
  virtual ~CGUIWindowSettingsScreenCalibration(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);

protected:
  virtual void OnMouseAction();
  void NextControl();
  void ResetControls();
  void EnableControl(int iControl);
  void UpdateFromControl(int iControl);
  UINT m_iCurRes;
  std::vector<RESOLUTION> m_Res;
  int m_iControl;
  float m_fPixelRatioBoxHeight;
};
