#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
class CGUIWindowHome :
  public CGUIWindow
{
public:
  CGUIWindowHome(void);
  virtual ~CGUIWindowHome(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void	  OnAction(const CAction &action);
  virtual void    Render();

protected:

	void OnClickShutdown(CGUIMessage& aMessage);
	void OnClickDashboard(CGUIMessage& aMessage);
	void OnClickReboot(CGUIMessage& aMessage);
	void OnClickCredits(CGUIMessage& aMessage);
	void OnClickOnlineGaming(CGUIMessage& aMessage);

	VOID GetDate(WCHAR* szDate, LPSYSTEMTIME pTime);
	VOID GetTime(WCHAR* szTime, LPSYSTEMTIME pTime);

	IDirect3DTexture8* m_pTexture;
	int m_iLastControl;
	int m_iLastMenuOption;
};
