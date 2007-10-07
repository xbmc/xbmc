#pragma once
#include "GUIWindowVideoBase.h"
#include "ThumbLoader.h"

class CGUIWindowVideoNav : public CGUIWindowVideoBase
{
public:

  CGUIWindowVideoNav(void);
  virtual ~CGUIWindowVideoNav(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  virtual void ClearFileItems();
  virtual void OnFinalizeFileItems(CFileItemList &items);
  virtual void OnInfo(CFileItem* pItem, const SScraperInfo&info);
  static void DeleteItem(CFileItem* pItem);

protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  void OnLinkMovieToTvShow(int itemnumber);
  // override base class methods
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void PlayItem(int iItem);
  virtual void OnDeleteItem(int iItem);
  virtual void OnWindowLoaded();
  virtual void OnFilterItems();
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

  virtual CStdString GetQuickpathName(const CStdString& strPath) const;
  void FilterItems(CFileItemList &items);

  void DisplayEmptyDatabaseMessage(bool bDisplay);

  VECSHARES m_shares;

  bool m_bDisplayEmptyDatabaseMessage;  ///< If true we display a message informing the user to switch back to the Files view.

  // filtered item views
  CFileItemList m_unfilteredItems;
  CStdString m_filter;
};
