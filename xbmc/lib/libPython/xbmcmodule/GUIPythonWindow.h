#pragma once
#include "GUIDialog.h"
#include "guiwindowmanager.h"
#include "..\python.h"

class PyXBMCAction
{
public:
	DWORD dwParam;
	PyObject* pActionCallback;  
};

int Py_XBMC_Event_OnAction(void* arg);
int Py_XBMC_Event_OnControl(void* arg);

class CGUIPythonWindow :
  public CGUIWindow
{
public:
  CGUIPythonWindow(DWORD dwId);
  virtual ~CGUIPythonWindow(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
	void						SetActionCallback(PyObject *object);
	void						WaitForActionEvent(DWORD timeout);
	void						PulseActionEvent();
protected:
	PyObject*		pActionCallback;
	HANDLE			m_actionEvent;
};
