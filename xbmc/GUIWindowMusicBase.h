/*!
	\file GUIWindowMusicBase.h
	\brief
	*/

#pragma once
#include "GUIWindow.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "MusicDatabase.h"
#include "FileItem.h"
#include "PlayList.h"
#include "MusicInfoTagLoaderFactory.h"
#include "Utils/MusicInfoScraper.h"

using namespace DIRECTORY;
using namespace PLAYLIST;

/*!
	\brief View as list
	*/
#define VIEW_AS_LIST							0
/*!
	\brief View as icons
	*/
#define VIEW_AS_ICONS							1
/*!
	\brief View as large icons
	*/
#define VIEW_AS_LARGEICONS				2

/*!
	\ingroup windows 
	\brief The base class for music windows

	CGUIWindowMusicBase is the base class for
	all music windows.
	*/
class CGUIWindowMusicBase : 	public CGUIWindow
{
public:
	CGUIWindowMusicBase(void);
	virtual ~CGUIWindowMusicBase(void);
  virtual bool							OnMessage(CGUIMessage& message);
  virtual void							OnAction(const CAction& action);
	virtual void							Render();
protected:
	/*!
 		\brief Overwrite to fill fileitems from a source
		\param strDirectory Path to read
		\param items Fill with items specified in \e strDirectory
		*/
	virtual void							GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)=0;
 	/*!
		\brief Will be called when an item in list/thumb control has been clicked
		\param iItem List/thumb control item that has been clicked on
		*/
  virtual void							OnClick(int iItem)=0;
 	/*!
		\brief Will be called when an popup context menu has been asked for
		\param iItem List/thumb control item that has been clicked on
		*/
	virtual void							OnPopupMenu(int iItem);
 	/*!
		\brief Will be called for every item to set the label and thumbs
		\param pItem Item to be formated
		*/
	virtual void							OnFileItemFormatLabel(CFileItem* pItem)=0;
	/*!
		\brief Will be called to sort items. Provide a sort method.
		\param items Items to sort
		*/
	virtual	void							DoSort(VECFILEITEMS& items)=0;
 	/*!
		\brief Overwrite to update your gui buttons (visible, enable,...)
		*/
	virtual	void							UpdateButtons();
	virtual	void							OnQueueItem(int iItem);
	virtual void							OnRetrieveMusicInfo(VECFILEITEMS& items);
	virtual void							GoParentFolder();
					void							RetrieveMusicInfo();
					void							OnInfo(int iItem);
					bool							FindAlbumInfo(const CStdString& strAlbum, CMusicAlbumInfo& album);
					void							ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh);
	virtual void							ClearFileItems();
	virtual void							Update(const CStdString &strDirectory);
					void							UpdateListControl();
	virtual void							AddItemToPlayList(const CFileItem* pItem);
					int								GetSelectedItem();
          void							SetSelectedItem(int index);
					void							OnRipCD();
	virtual void							OnScan() {};
					void							OnSearch();
	virtual	void							OnSearchItemFound(const CFileItem* pItem);
	virtual void							DoSearch(const CStdString& strSearch,VECFILEITEMS& items);
					bool							HaveDiscOrConnection( CStdString& strPath, int iDriveType );
					bool							GetKeyboard(CStdString& strInput);
	virtual	void							GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
	virtual	void							ShowThumbPanel();
					bool							ViewByIcon();
					bool							ViewByLargeIcon();
					void							DisplayEmptyDatabaseMessage(bool bDisplay);
					void							SetLabelFromTag(CFileItem *pItem);

	CFileItem									m_Directory;	///< Holds the current direcotry path after calling Update()
	CVirtualDirectory					m_rootDir;	///< Used to get directories from shares and the shares itself
  VECFILEITEMS							m_vecItems;	///< Represents the current items listed in the list/thumb control
	typedef vector <CFileItem*>::iterator ivecItems;	///< CFileItem* vector Iterator
	CGUIDialogProgress*				m_dlgProgress;	///< Progress dialog
	CDirectoryHistory					m_history;	///< Previous items selected as string for list/thumb control
	/*!
		\brief Is list, thumb or thumb control with large icons shown.

			Value can be:
			- #VIEW_AS_LIST\n
				List control is visible
			- #VIEW_AS_ICONS\n
				Thumb control is visible and in normal icon mode
			- #VIEW_AS_LARGEICONS\n
				Thumb control is visible and in large icon mode
		*/
	int												m_iViewAsIcons;
	/*!
		\brief Is list, thumb or thumb control with large icons for root items shown.

			Value can be:
			- #VIEW_AS_LIST\n
				List control is visible
			- #VIEW_AS_ICONS\n
				Thumb control is visible and in normal icon mode
			- #VIEW_AS_LARGEICONS\n
				Thumb control is visible and in large icon mode
		*/
	int												m_iViewAsIconsRoot;
	static int								m_nTempPlayListWindow;	///< Window the temporary playlist was started
	static CStdString					m_strTempPlayListDirectory;	///< The directory the temporary playlist was started
	int												m_nSelectedItem;	///< Backups the last selected item before window is deinitialized
	int												m_iLastControl;	///< Backups the last selected control before window is deinitialized
	bool											m_bDisplayEmptyDatabaseMessage;		///< If true we display a message informing the user to switch back to the Files view.
	CStdString								m_strParentPath;	///< Parent path to handle going up a dir
};
