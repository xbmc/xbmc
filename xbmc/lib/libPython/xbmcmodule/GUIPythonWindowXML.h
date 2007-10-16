#pragma once
#include "GUIPythonWindow.h"
#include "../../../GUIViewControl.h"

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
  int               GetViewContainerID() const { return m_viewControl.GetCurrentControl(); };

protected:
  CGUIControl      *GetFirstFocusableControl(int id);
  virtual void     UpdateButtons();
  virtual void     OnSort();
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
  auto_ptr<CGUIViewState> m_guiState;
  CFileItemList    m_vecItems;
};
