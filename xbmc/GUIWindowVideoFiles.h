#pragma once
#include "GUIWindowVideoBase.h"

typedef struct SScanSettings
{
  bool parent_name;       /* use the parent dirname as name of lookup */
  bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
  int  recurse;           /* recurse into sub folders (indicate levels) */
} SScanSettings;

class CGUIWindowVideoFiles : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoFiles(void);
  virtual ~CGUIWindowVideoFiles(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);  
  bool IsScanning() { return m_bIsScanning; };

  virtual void OnScan(const CStdString& strPath, const SScraperInfo& info) { OnScan(strPath, info, -1, -1); }  

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
  
  bool DoScan(CFileItemList& items, const SScraperInfo& info, const SScanSettings &settings, int depth = 0);
  void GetStackedDirectory(const CStdString &strPath, CFileItemList &items);
  void OnRetrieveVideoInfo(CFileItemList& items, const SScraperInfo& info, bool bDirNames);
  virtual void LoadPlayList(const CStdString& strFileName);
  void GetIMDBDetails(CFileItem *pItem, CIMDBUrl &url, const SScraperInfo& info);
  void AddMovieAndGetThumb(CFileItem *pItem, const CIMDBMovie &movieDetails);
  void PlayFolder(const CFileItem* pItem);

  bool m_bIsScanning;
};
