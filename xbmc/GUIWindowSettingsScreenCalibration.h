#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "settings.h"

class CGUIWindowSettingsScreenCalibration :
  public CGUIWindow
{
public:
  CGUIWindowSettingsScreenCalibration(void);
  virtual ~CGUIWindowSettingsScreenCalibration(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void    Render();
  virtual void    AllocResources();
  virtual void    FreeResources();

protected:
	UINT				m_iCurRes;
	vector<RESOLUTION>	m_Res;
	int			m_iCountU;
	int			m_iCountD;
	int			m_iCountL;
	int			m_iCountR;
	int			m_iSpeed;
	DWORD		m_dwLastTime;
	int			m_iControl;
	float		m_fPixelRatioBoxHeight;
};
