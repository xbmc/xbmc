#pragma once
#include "GUIWindow.h"
#include "../python/Python.h"

class PyXBMCAction
{
public:
  DWORD dwParam;
  PyObject* pCallbackWindow;
  PyObject* pObject;
  int controlId; // for XML window
  PyXBMCAction() { }
  //virtual ~PyXBMCAction();
};

int Py_XBMC_Event_OnAction(void* arg);
int Py_XBMC_Event_OnControl(void* arg);

class CGUIPythonWindow : public CGUIWindow
{
public:
  CGUIPythonWindow(DWORD dwId);
  virtual ~CGUIPythonWindow(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual bool    OnAction(const CAction &action);
  void             SetCallbackWindow(PyObject *object);
  void             WaitForActionEvent(DWORD timeout);
  void             PulseActionEvent();
protected:
  PyObject*        pCallbackWindow;
  HANDLE           m_actionEvent;
};
