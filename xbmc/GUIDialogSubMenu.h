#pragma once
#include "GUIDialog.h"

class CGUIDialogSubMenu :
	public CGUIDialog
{
public:
	CGUIDialogSubMenu(void);
	virtual ~CGUIDialogSubMenu(void);
  virtual void    OnAction(const CAction &action);
	virtual bool		OnMessage(CGUIMessage &message);

protected:
	void OnClickShutdown(CGUIMessage& aMessage);
	void OnClickDashboard(CGUIMessage& aMessage);
	void OnClickReboot(CGUIMessage& aMessage);
	void OnClickCredits(CGUIMessage& aMessage);
	void OnClickOnlineGaming(CGUIMessage& aMessage);
};
