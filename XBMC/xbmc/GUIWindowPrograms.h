#pragma once
#include "guiwindow.h"
#include "FileItem.h"

class CGUIWindowPrograms :
  public CGUIWindow
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    Render();
  virtual void    OnKey(const CKey& key);
protected:
  
  
  void            Update(const CStdString& strDirectory);
  void            LoadDirectory(const CStdString& strDirectory);
  void            OnClick(int iItem);
  void            OnSort();
  void            UpdateButtons();
  void            Clear();
  
  vector <CFileItem*> m_vecItems;
  CStdString          m_strDirectory;
  
};
