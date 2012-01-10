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
#include "threads/Event.h"

class PyXBMCAction
{
public:
  int param;
  void* pCallbackWindow;
  void* pObject;
  int controlId; // for XML window
#if defined(_LINUX) || defined(_WIN32)
  int type; // 0=Action, 1=Control;
#endif

  PyXBMCAction(void*& callback);
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
  virtual bool    OnBack(int actionID);
  void             SetCallbackWindow(void* state, void *object);
  void             WaitForActionEvent(unsigned int timeout);
  void             PulseActionEvent();
  void             SetDestroyAfterDeinit(bool destroy = true);
protected:
  virtual void     OnDeinitWindow(int nextWindowID = 0);
  void* pCallbackWindow;
  void* m_threadState;
  CEvent           m_actionEvent;
  bool             m_destroyAfterDeinit;
};
