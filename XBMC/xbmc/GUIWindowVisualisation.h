#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "visualizations/Visualisation.h"

class CGUIWindowVisualisation :
  public CGUIWindow
{
public:
  CGUIWindowVisualisation(void);
  virtual ~CGUIWindowVisualisation(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnKey(const CKey& key);
	virtual void		Render();

private:
	CVisualisation* m_pVisualisation;
};
