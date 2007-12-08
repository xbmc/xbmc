#pragma once
#include "GUIWindowMusicBase.h"
#include "ThumbLoader.h"

class CGUIWindowMusicNav : public CGUIWindowMusicBase, public IBackgroundLoaderObserver
{
public:

  CGUIWindowMusicNav(void);
  virtual ~CGUIWindowMusicNav(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  virtual void ClearFileItems();
  virtual void OnFinalizeFileItems(CFileItemList &items);

protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  // override base class methods
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void PlayItem(int iItem);
  virtual void OnWindowLoaded();
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void OnFilterItems();
  virtual bool OnClick(int iItem);
  void OnSearch();
  void FilterItems(CFileItemList &items);

  void SetThumb(int iItem, CONTEXT_BUTTON button);
  bool GetSongsFromPlayList(const CStdString& strPlayList, CFileItemList &items);
  void DisplayEmptyDatabaseMessage(bool bDisplay);
  CStdString GetQuickpathName(const CStdString& strPath) const;

  VECSHARES m_shares;

  bool m_bDisplayEmptyDatabaseMessage;  ///< If true we display a message informing the user to switch back to the Files view.

  CMusicThumbLoader m_thumbLoader;      ///< used for the loading of thumbs in the special://musicplaylist folder

  // filtered item views
  CFileItemList m_unfilteredItems;
  CStdString m_filter;

  CStdString m_search;  // current search
};
