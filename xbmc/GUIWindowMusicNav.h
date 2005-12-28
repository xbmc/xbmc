#pragma once
#include "GUIWindowMusicBase.h"


class CGUIWindowMusicNav : public CGUIWindowMusicBase
{
public:

  CGUIWindowMusicNav(void);
  virtual ~CGUIWindowMusicNav(void);

  virtual bool OnMessage(CGUIMessage& message);
  CFileItem CurrentDirectory() const { return m_vecItems;};


protected:
  // override base class methods
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void SortItems(CFileItemList& items);
  virtual void UpdateButtons();
  virtual void OnFileItemFormatLabel(CFileItem* pItem);
  virtual void OnClick(int iItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void PlayItem(int iItem);
  virtual void OnPopupMenu(int iItem);

  void SetArtistImage(int iItem);
  bool GetSongsFromPlayList(const CStdString& strPlayList, CFileItemList &items);

  bool IsArtistDir(const CStdString& strDirectory);
  bool CanCache(const CStdString& strDirectory);
  bool HasAlbumInfo(const CStdString& strDirectory);

  void SaveDatabaseDirectoryCache(const CStdString& strDirectory, CFileItemList& items, SORT_METHOD SortMethod, SORT_ORDER Ascending, bool bSkipThe);
  bool LoadDatabaseDirectoryCache(const CStdString& strDirectory, CFileItemList& items, SORT_METHOD& SortMethod, SORT_ORDER& Ascending, bool& bSkipThe);
  void ClearDatabaseDirectoryCache(const CStdString& strDirectory);
  // directory caching
  bool m_bGotDirFromCache;
  SORT_METHOD m_SortCache;
  SORT_ORDER m_AscendCache;
  bool m_bSkipTheCache;

  VECSHARES m_shares;
};
