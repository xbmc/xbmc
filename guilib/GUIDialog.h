#pragma once
#include "guiwindow.h"

class CGUIDialog :
	public CGUIWindow
{
public:
	CGUIDialog(DWORD dwID);
	virtual ~CGUIDialog(void);
  virtual void    Render();
	void						DoModal(DWORD dwParentId);
	virtual void		Close();
	
protected:
	DWORD						m_dwParentWindowID;
	CGUIWindow* 		m_pParentWindow;
	bool						m_bRunning;
};
