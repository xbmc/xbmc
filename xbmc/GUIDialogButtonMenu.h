#pragma once
#include "GUIDialog.h"

class CGUIDialogButtonMenu :
	public CGUIDialog
{
public:
	CGUIDialogButtonMenu(void);
	virtual ~CGUIDialogButtonMenu(void);
  virtual void    OnAction(const CAction &action);
	virtual bool		OnMessage(CGUIMessage &message);
	virtual void		Render();
};
