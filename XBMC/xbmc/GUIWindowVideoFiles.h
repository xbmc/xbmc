#pragma once
#include "GUIWindowVideoBase.h"
#include "VideoInfoScanner.h"

class CGUIWindowVideoFiles : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoFiles(void);
  virtual ~CGUIWindowVideoFiles(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);  

  virtual void OnScan(const CStdString& strPath, const SScraperInfo& info) { OnScan(strPath, info, -1, -1); }  
  void GetStackedDirectory(const CStdString &strPath, CFileItemList &items);

  void OnScan(const CStdString& strPath, const SScraperInfo& info, int iDirNames, int iScanRecursively);
protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool OnPlayMedia(int iItem);
  virtual void AddFileToDatabase(const CFileItem* pItem);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void UpdateButtons();
  virtual bool OnClick(int iItem);
  virtual void OnPopupMenu(int iItem, bool bContextDriven = true);
  virtual void OnInfo(int iItem, const SScraperInfo& info);
  virtual void OnQueueItem(int iItem);
  virtual void OnAssignContent(int iItem, int iFound, SScraperInfo& info);
  virtual void OnUnAssignContent(int iItem);
  
  virtual void LoadPlayList(const CStdString& strFileName);
  void PlayFolder(const CFileItem* pItem);
};
