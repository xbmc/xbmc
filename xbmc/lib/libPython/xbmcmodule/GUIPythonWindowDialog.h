#pragma once
#include "GUIPythonWindowDialog.h"
#include "GUIPythonWindow.h"
#include "..\python.h"

class CGUIPythonWindowDialog : public CGUIPythonWindow
{
public:
  CGUIPythonWindowDialog(DWORD dwId);
  virtual ~CGUIPythonWindowDialog(void);
	virtual bool    OnMessage(CGUIMessage& message);
	void            Activate(DWORD dwParentId);
	virtual void		Close();
	virtual bool		IsRunning() const { return m_bRunning; }

protected:
	DWORD						m_dwParentWindowID;
	CGUIWindow* 		m_pParentWindow;
	bool						m_bRunning;
};
