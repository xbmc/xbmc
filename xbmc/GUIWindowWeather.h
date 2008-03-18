#pragma once
#include "GUIWindow.h"

class CGUIWindowWeather : public CGUIWindow
{
public:
  CGUIWindowWeather(void);
  virtual ~CGUIWindowWeather(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

protected:
  virtual void OnInitWindow();

  void UpdateButtons();
  void UpdateLocations();

  void Refresh();

  unsigned int m_iCurWeather;
};
