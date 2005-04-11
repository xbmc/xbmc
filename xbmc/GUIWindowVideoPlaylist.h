#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoPlaylist : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoPlaylist(void);
  virtual ~CGUIWindowVideoPlaylist(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnAction(const CAction &action);

protected:
  virtual void LoadViewMode();
  virtual void SaveViewMode();
  virtual int SortMethod() {return 0;};
  virtual bool SortAscending() {return false;};
  virtual void SortItems(CFileItemList &items) {};
  virtual void FormatItemLabels() {};

  void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  virtual void GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void Update(const CStdString &strDirectory);
  void ClearPlayList();
  void UpdateListControl();
  void RemovePlayListItem(int iItem);
  void MoveCurrentPlayListItem(int iAction); // up or down
  void OnFileItemFormatLabel(CFileItem* pItem);
  void DoSort(CFileItemList& items);
  void UpdateButtons();
  void ShufflePlayList();
  void SavePlayList();

  virtual void OnClick(int iItem);
  void OnQueueItem(int iItem);
};
