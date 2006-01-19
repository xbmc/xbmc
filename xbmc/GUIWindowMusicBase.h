/*!
\file GUIWindowMusicBase.h
\brief
*/

#pragma once
#include "GUIMediaWindow.h"
#include "MusicDatabase.h"
#include "PlayList.h"
#include "MusicInfoTagLoaderFactory.h"
#include "Utils/MusicInfoScraper.h"
#include "playlistplayer.h"

using namespace DIRECTORY;
using namespace PLAYLIST;

/*!
 \ingroup windows 
 \brief The base class for music windows
 
 CGUIWindowMusicBase is the base class for
 all music windows.
 */
class CGUIWindowMusicBase : public CGUIMediaWindow
{
public:
  CGUIWindowMusicBase(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowMusicBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  virtual void Render();
  virtual void OnWindowLoaded();

protected:
  /*!
  \brief Will be called when an popup context menu has been asked for
  \param iItem List/thumb control item that has been clicked on
  */
  virtual void OnPopupMenu(int iItem);
  /*!
  \brief Overwrite to update your gui buttons (visible, enable,...)
  */
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);

  virtual void OnRetrieveMusicInfo(CFileItemList& items);
  virtual void AddItemToPlayList(const CFileItem* pItem, int iPlayList = PLAYLIST_MUSIC);
  virtual void OnSearchItemFound(const CFileItem* pItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void OnScan() {};

  // new methods
  virtual void PlayItem(int iItem);

  void OnDeleteItem(int iItem);
  virtual void OnRenameItem(int iItem);

  void RetrieveMusicInfo();
  void OnInfo(int iItem);
  void OnQueueItem(int iItem);
  bool FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, CMusicAlbumInfo& album);
  void ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh);
  void ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh);
  void OnRipCD();
  void OnSearch();
  void DisplayEmptyDatabaseMessage(bool bDisplay);
  virtual void LoadPlayList(const CStdString& strPlayList);
  
  typedef vector <CFileItem*>::iterator ivecItems; ///< CFileItem* vector Iterator
  CGUIDialogProgress* m_dlgProgress; ///< Progress dialog

  bool m_bDisplayEmptyDatabaseMessage;  ///< If true we display a message informing the user to switch back to the Files view.

  // member variables to save frequently used g_guiSettings (which is slow)
  bool m_hideExtensions;
};
