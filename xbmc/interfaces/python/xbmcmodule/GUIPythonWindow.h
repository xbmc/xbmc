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

#include "guilib/GUIWindow.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/Python.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
  #include "python/Include/Python.h"
#endif

class PyXBMCAction
{
public:
  int param;
  PyObject* pCallbackWindow;
  PyObject* pObject;
  int controlId; // for XML window
#if defined(_LINUX) || defined(_WIN32)
  int type; // 0=Action, 1=Control;
#endif

  PyXBMCAction(PyObject*& callback);
  virtual ~PyXBMCAction() ;
};

int Py_XBMC_Event_OnAction(void* arg);
int Py_XBMC_Event_OnControl(void* arg);

class CGUIPythonWindow : public CGUIWindow
{
public:
  CGUIPythonWindow(int id);
  virtual ~CGUIPythonWindow(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual bool    OnAction(const CAction &action);
  void             SetCallbackWindow(PyThreadState *state, PyObject *object);
  void             WaitForActionEvent(unsigned int timeout);
  void             PulseActionEvent();
protected:
  PyObject*        pCallbackWindow;
  PyThreadState*   m_threadState;
  HANDLE           m_actionEvent;
};
