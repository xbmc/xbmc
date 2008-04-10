#pragma once
#include "GUIMediaWindow.h"

class CGUIWindowGameSaves : public CGUIMediaWindow
{
public:
  CGUIWindowGameSaves(void);
  virtual ~CGUIWindowGameSaves(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  
  //virtual bool Update(const CStdString &strDirectory);
  void GoParentFolder();
  virtual bool OnPlayMedia(int iItem);
  virtual bool GetDirectory(const CStdString& strDirectory, CFileItemList& items);
  virtual bool OnClick(int iItem);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  //bool DownloadSaves(CFileItem item);
  bool m_bViewOutput;
  VECSOURCES m_shares;
  CStdString m_strParentPath;
};
