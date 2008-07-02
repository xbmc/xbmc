#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindow.h"
#include "lib/libPython/Python/Include/Python.h"

class PyXBMCAction
{
public:
  DWORD dwParam;
  PyObject* pCallbackWindow;
  PyObject* pObject;
  int controlId; // for XML window
#if defined(_LINUX) || defined(_WIN32PC)  
  int type; // 0=Action, 1=Control;
#endif

  PyXBMCAction(): dwParam(0), pCallbackWindow(NULL), pObject(NULL), controlId(0), type(0) { }
  virtual ~PyXBMCAction() ;
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
