#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoFiles : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoFiles(void);
  virtual ~CGUIWindowVideoFiles(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool OnPlayMedia(int iItem);
  virtual void AddFileToDatabase(const CFileItem* pItem);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void UpdateButtons();
  virtual bool OnClick(int iItem);
  virtual void OnPopupMenu(int iItem, bool bContextDriven = true);
  virtual void OnInfo(int iItem);
  virtual void OnQueueItem(int iItem);
  virtual void OnScan(const CStdString& strPath, const CStdString& strScraper, const CStdString& strContent);
  virtual void OnAssignContent(int iItem);
  virtual void OnUnAssignContent(int iItem);
  bool DoScan(const CStdString& strPath, CFileItemList& items, const CStdString& strScraper, const CStdString& strContent, bool bDirNames);
  void GetStackedDirectory(const CStdString &strPath, CFileItemList &items);
  void OnRetrieveVideoInfo(CFileItemList& items, const CStdString& strScraper, const CStdString& strContent, bool bDirNames);
  virtual void LoadPlayList(const CStdString& strFileName);
  void GetIMDBDetails(CFileItem *pItem, CIMDBUrl &url, const CStdString& strScaper);
  void PlayFolder(const CFileItem* pItem);
};
