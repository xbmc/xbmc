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

#include "GUIPythonWindow.h"
#include "GUIMediaWindow.h"

int Py_XBMC_Event_OnClick(void* arg);
int Py_XBMC_Event_OnFocus(void* arg);
int Py_XBMC_Event_OnInit(void* arg);

class CGUIPythonWindowXML : public CGUIMediaWindow
{
public:
  CGUIPythonWindowXML(DWORD dwId, CStdString strXML, CStdString strFallBackPath);
  virtual ~CGUIPythonWindowXML(void);
  virtual bool      OnMessage(CGUIMessage& message);
  virtual bool      OnAction(const CAction &action);
  virtual void      AllocResources(bool forceLoad = false);
  virtual void      FreeResources(bool forceUnLoad = false);
  virtual void      Render();
  void              WaitForActionEvent(DWORD timeout);
  void              PulseActionEvent();
  void              AddItem(CFileItemPtr fileItem,int itemPosition);
  void              RemoveItem(int itemPosition);
  void              ClearList();
  CFileItemPtr      GetListItem(int position);
  int               GetListSize();
  int               GetCurrentListPosition();
  void              SetCurrentListPosition(int item);
  void              SetCallbackWindow(PyObject *object);
  virtual bool      OnClick(int iItem);
  void              LoadScriptStrings(const CStdString &strPath);
  void              ClearScriptStrings();

protected:
  virtual void     GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual void     Update();
  virtual void     OnInitWindow();
  void              SetupShares();
  PyObject*        pCallbackWindow;
  HANDLE           m_actionEvent;
  bool             m_bRunning;
  CStdString       m_fallbackPath;
  CStdString       m_backupMediaDir;
};
