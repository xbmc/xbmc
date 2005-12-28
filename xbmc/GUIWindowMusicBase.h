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
    \brief Overwrite to fill fileitems from a source
   \param strDirectory Path to read
   \param items Fill with items specified in \e strDirectory
   */
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  /*!
  \brief Will be called when an item in list/thumb control has been clicked
  \param iItem List/thumb control item that has been clicked on
  */
  virtual void OnClick(int iItem) {};
  /*!
  \brief Will be called when an popup context menu has been asked for
  \param iItem List/thumb control item that has been clicked on
  */
  virtual void OnPopupMenu(int iItem);
  /*!
  \brief Will be called for every item to set the label and thumbs
  \param pItem Item to be formatted
  */
  virtual void OnFileItemFormatLabel(CFileItem* pItem) = 0;
  /*!
   \brief Will be called to sort items. Provide a sort method.
   \param items Items to sort
   */
  virtual void SortItems(CFileItemList& items);
  /*!
  \brief Overwrite to update your gui buttons (visible, enable,...)
  */
  virtual void GoParentFolder();
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);

  virtual void OnRetrieveMusicInfo(CFileItemList& items);
  virtual void AddItemToPlayList(const CFileItem* pItem, int iPlayList = PLAYLIST_MUSIC);
  virtual void OnSearchItemFound(const CFileItem* pItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void OnScan() {};
  virtual void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);

  // new methods
  virtual void PlayItem(int iItem);
  virtual void AddItemToTempPlayList(const CFileItem* pItem);

  void OnDeleteItem(int iItem);
  virtual void OnRenameItem(int iItem);

  void RetrieveMusicInfo();
  void OnInfo(int iItem);
  void OnQueueItem(int iItem);
  bool FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, CMusicAlbumInfo& album);
  void ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh);
  void ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh);
  void UpdateListControl();
  void OnRipCD();
  void OnSearch();
  bool ViewByIcon();
  bool ViewByLargeIcon();
  void DisplayEmptyDatabaseMessage(bool bDisplay);
  void SetLabelFromTag(CFileItem *pItem);
  CStdString ParseFormat(CFileItem *pItem, const CStdString& strFormat);
  void LoadPlayList(const CStdString& strPlayList, int iPlayList = PLAYLIST_MUSIC);
  void ShowShareErrorMessage(CFileItem* pItem);
  void SetHistoryForPath(const CStdString& strDirectory);
  
  typedef vector <CFileItem*>::iterator ivecItems; ///< CFileItem* vector Iterator
  CGUIDialogProgress* m_dlgProgress; ///< Progress dialog

  static int m_nTempPlayListWindow; ///< Window the temporary playlist was started
  static CStdString m_strTempPlayListDirectory; ///< The directory the temporary playlist was started

  bool m_bDisplayEmptyDatabaseMessage;  ///< If true we display a message informing the user to switch back to the Files view.
  vector<CStdString> m_vecPathHistory; ///< History of traversed directories

  // member variables to save frequently used g_guiSettings (which is slow)
  CStdString m_formatLeft;
  CStdString m_formatRight;
  bool m_hideExtensions;
};
