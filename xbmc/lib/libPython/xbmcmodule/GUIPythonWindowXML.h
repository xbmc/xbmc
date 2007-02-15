#pragma once
#include "GUIPythonWindow.h"


//int Py_XBMC_Event_OnAction(void* arg);
int Py_XBMC_Event_OnClick(void* arg);
int Py_XBMC_Event_OnFocus(void* arg);
int Py_XBMC_Event_OnInit(void* arg);
class CGUIPythonWindowXML : public CGUIWindow
{
public:
  CGUIPythonWindowXML(DWORD dwId, CStdString strXML);
  virtual ~CGUIPythonWindowXML(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual bool    OnAction(const CAction &action);
  void            Activate(DWORD dwParentId);
	void						SetCallbackWindow(PyObject *object);
	void						WaitForActionEvent(DWORD timeout);
	void						PulseActionEvent();

protected:
	PyObject*		pCallbackWindow;
	HANDLE			m_actionEvent;
	DWORD						m_dwParentWindowID;
	CGUIWindow* 		m_pParentWindow;
	bool						m_bRunning;
};
