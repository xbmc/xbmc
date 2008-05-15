#pragma once

/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIPythonWindow.h"
#include "GUIViewControl.h"

class CFileItemList;

int Py_XBMC_Event_OnClick(void* arg);
int Py_XBMC_Event_OnFocus(void* arg);
int Py_XBMC_Event_OnInit(void* arg);

class CGUIPythonWindowXML : public CGUIWindow
{
public:
  CGUIPythonWindowXML(DWORD dwId, CStdString strXML, CStdString strFallBackPath);
  virtual ~CGUIPythonWindowXML(void);
  virtual bool      OnMessage(CGUIMessage& message);
  virtual bool      OnAction(const CAction &action);
  virtual void      AllocResources(bool forceLoad = false);
  virtual void      Render();
  void              WaitForActionEvent(DWORD timeout);
  void              PulseActionEvent();
  void              UpdateFileList();
  void              AddItem(CFileItem * fileItem,int itemPosition);
  void              RemoveItem(int itemPosition);
  void              ClearList();
  CFileItem*        GetListItem(int position);
  int               GetListSize();
  int               GetCurrentListPosition();
  void              SetCurrentListPosition(int item);
  virtual bool      IsMediaWindow() const { return true; };
  virtual bool      HasListItems() const { return true; };
  virtual CFileItem *GetCurrentListItem(int offset = 0);
  const CFileItemList& CurrentDirectory() const;
  int               GetViewContainerID() const { return m_viewControl.GetCurrentControl(); };

protected:
  CGUIControl      *GetFirstFocusableControl(int id);
  virtual void     UpdateButtons();
  virtual void     FormatAndSort(CFileItemList &items);
  virtual void     Update();
  virtual void     OnWindowLoaded();
  virtual void     OnInitWindow();
  virtual void     FormatItemLabels();
  virtual void     SortItems(CFileItemList &items);
  PyObject*        pCallbackWindow;
  HANDLE           m_actionEvent;
  bool             m_bRunning;
  CStdString       m_fallbackPath;
  CStdString       m_backupMediaDir;
  CGUIViewControl  m_viewControl;
  std::auto_ptr<CGUIViewState> m_guiState;
  CFileItemList*    m_vecItems;
};
