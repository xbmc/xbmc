#pragma once
#include "GUIWindow.h"

class CGUIWindowSettingsScreenCalibration : public CGUIWindow
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
	void			NextControl();
	void			ResetControls();
	void			EnableControl(int iControl);
	void			UpdateFromControl(int iControl);
	UINT				m_iCurRes;
	vector<RESOLUTION>	m_Res;
	int					m_iControl;
	float				m_fPixelRatioBoxHeight;
};
