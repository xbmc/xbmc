#pragma once
#include "GUIMediaWindow.h"

class CGUIWindowGameSaves : public CGUIMediaWindow
{
public:
  CGUIWindowGameSaves(void);
  virtual ~CGUIWindowGameSaves(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

protected:
  
  //virtual bool Update(const CStdString &strDirectory);
  void GoParentFolder();
  virtual bool OnPlayMedia(int iItem);
  virtual bool GetDirectory(const CStdString& strDirectory, CFileItemList& items);
  virtual bool OnClick(int iItem);
  virtual void OnPopupMenu(int iItem);
  //bool DownloadSaves(CFileItem item);
  bool m_bViewOutput;
  VECSHARES m_shares;
  CStdString m_strParentPath;
};
