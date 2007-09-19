#pragma once
#include "GUIWindowVideoBase.h"
#include "VideoInfoScanner.h"

class CGUIWindowVideoFiles : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoFiles(void);
  virtual ~CGUIWindowVideoFiles(void);
  virtual bool OnMessage(CGUIMessage& message);

  void GetStackedDirectory(const CStdString &strPath, CFileItemList &items);
  virtual void OnInfo(CFileItem* pItem, const SScraperInfo& info);
protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool OnPlayMedia(int iItem);
  virtual void AddFileToDatabase(const CFileItem* pItem);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void UpdateButtons();
  virtual bool OnClick(int iItem);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

  virtual void OnQueueItem(int iItem);
  virtual void OnAssignContent(int iItem, int iFound, SScraperInfo& info, VIDEO::SScanSettings& settings);
  virtual void OnUnAssignContent(int iItem);
  
  virtual void LoadPlayList(const CStdString& strFileName);
  void PlayFolder(const CFileItem* pItem);
};
