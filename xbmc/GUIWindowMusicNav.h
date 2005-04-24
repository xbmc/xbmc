#pragma once
#include "GUIWindowMusicBase.h"
#include "GUIViewControl.h"

class CGUIWindowMusicNav : public CGUIWindowMusicBase
{
public:

  CGUIWindowMusicNav(void);
  virtual ~CGUIWindowMusicNav(void);

  virtual bool OnMessage(CGUIMessage& message);

protected:
  // override base class methods
  virtual void GoParentFolder();
  virtual void GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void OnFileItemFormatLabel(CFileItem* pItem);
  virtual void OnClick(int iItem);
  virtual void DoSort(CFileItemList& items);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void OnSearchItemFound(const CFileItem* pItem);
  virtual void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  virtual void AddItemToPlayList(const CFileItem* pItem);
  virtual void PlayItem(int iItem);
  virtual void AddItemToTempPlayList(const CFileItem* pItem);

  // state of the window
  int m_iState;
  int m_iPath;
  CStdString m_strGenre;
  CStdString m_strArtist;
  CStdString m_strAlbum;
  CStdString m_strAlbumPath;
};
