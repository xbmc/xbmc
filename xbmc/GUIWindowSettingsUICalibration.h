#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
class CGUIWindowSettingsUICalibration :
  public CGUIWindow
{
public:
	CGUIWindowSettingsUICalibration(void);
	virtual ~CGUIWindowSettingsUICalibration(void);
	virtual bool	OnMessage(CGUIMessage& message);
	virtual void	OnAction(const CAction &action);
	virtual void	Render();
//private:
//	int	m_iOffsetX;
//	int m_iOffsetY;
};
