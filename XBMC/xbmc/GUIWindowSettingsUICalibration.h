#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
class CGUIWindowSettingsUICalibration :
  public CGUIWindow
{
public:
  CGUIWindowSettingsUICalibration(void);
  virtual ~CGUIWindowSettingsUICalibration(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
protected:
	int			m_iCountU;
	int			m_iCountD;
	int			m_iCountL;
	int			m_iCountR;
	int			m_iSpeed;
	DWORD   m_dwLastTime;
};
