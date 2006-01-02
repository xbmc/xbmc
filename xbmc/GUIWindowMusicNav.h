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
  virtual void UpdateButtons();
  virtual void OnClick(int iItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void PlayItem(int iItem);
  virtual void OnPopupMenu(int iItem);

  void SetArtistImage(int iItem);
  bool GetSongsFromPlayList(const CStdString& strPlayList, CFileItemList &items);

  VECSHARES m_shares;
};
