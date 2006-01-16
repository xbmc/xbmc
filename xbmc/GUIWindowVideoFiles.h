#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoFiles : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoFiles(void);
  virtual ~CGUIWindowVideoFiles(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  CFileItem CurrentDirectory() const { return m_vecItems;};

private:
  virtual void SetIMDBThumbs(CFileItemList& items);
protected:
  virtual void OnPlayMedia(int iItem);
  virtual void AddFileToDatabase(const CFileItem* pItem);
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);
  virtual bool OnClick(int iItem);
  virtual void OnPopupMenu(int iItem);
  virtual void OnInfo(int iItem);
  virtual void OnQueueItem(int iItem);
  virtual void OnScan();
  bool DoScan(const CStdString& strPath, CFileItemList& items);
  void GetStackedDirectory(const CStdString &strPath, CFileItemList &items);
  void OnRetrieveVideoInfo(CFileItemList& items);
  virtual void LoadPlayList(const CStdString& strFileName);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  void GetIMDBDetails(CFileItem *pItem, CIMDBUrl &url);
  void PlayFolder(const CFileItem* pItem);
};
