
#include "stdafx.h"
#include "AutoSwitch.h"
#include "util.h"
#include "application.h"

#define VIEW_AS_LIST					0
#define VIEW_AS_ICONS					1
#define VIEW_AS_LARGEICONS		2

#define METHOD_BYFOLDERS		0
#define METHOD_BYFILES			1
#define METHOD_BYTHUMBPERCENT	2

CAutoSwitch::CAutoSwitch(void)
{
}

CAutoSwitch::~CAutoSwitch(void)
{
}

/// \brief Generic function to add a layer of transparency to the calling window
/// \param vecItems Vector of FileItems passed from the calling window
int CAutoSwitch::GetView(CFileItemList &vecItems)
{
	int iViewAs = VIEW_AS_LIST;
	int iSortMethod = -1;
	bool bBigThumbs = false;
	bool bHideParentFolderItems = false;
	int iPercent = 0;
	int iCurrentWindow = m_gWindowManager.GetActiveWindow();

	switch (iCurrentWindow)
	{
		case WINDOW_MUSIC_FILES:
		{
			iSortMethod = g_guiSettings.GetInt("MusicLists.AutoSwitchMethod");
			bBigThumbs = g_guiSettings.GetBool("MusicLists.AutoSwitchUseLargeThumbs");
			bHideParentFolderItems = g_guiSettings.GetBool("MusicLists.HideParentDirItems");
			if ( iSortMethod == METHOD_BYTHUMBPERCENT )
			{
				iPercent = g_guiSettings.GetInt("MusicLists.AutoSwitchPercentage");
			}
		}
		break;

		case WINDOW_VIDEOS:
		{
			iSortMethod = g_guiSettings.GetInt("VideoLists.AutoSwitchMethod");
			bBigThumbs = g_guiSettings.GetBool("VideoLists.AutoSwitchUseLargeThumbs");
			bHideParentFolderItems = g_guiSettings.GetBool("VideoLists.HideParentDirItems");
			if ( iSortMethod == METHOD_BYTHUMBPERCENT )
			{
				iPercent = g_guiSettings.GetInt("VideoLists.AutoSwitchPercentage");
			}
		}
		break;

		case WINDOW_PICTURES:
		{
			iSortMethod = g_guiSettings.GetInt("Pictures.AutoSwitchMethod");
			bBigThumbs = g_guiSettings.GetBool("Pictures.AutoSwitchUseLargeThumbs");
			bHideParentFolderItems = g_guiSettings.GetBool("Pictures.HideParentDirItems");
			if ( iSortMethod == METHOD_BYTHUMBPERCENT )
			{
				iPercent = g_guiSettings.GetInt("Pictures.AutoSwitchPercentage");
			}
		}
		break;

		case WINDOW_PROGRAMS:
		{
			iSortMethod = g_guiSettings.GetInt("ProgramsLists.AutoSwitchMethod");
			bBigThumbs = g_guiSettings.GetBool("ProgramsLists.AutoSwitchUseLargeThumbs");
			bHideParentFolderItems = g_guiSettings.GetBool("ProgramsLists.HideParentDirItems");
			if ( iSortMethod == METHOD_BYTHUMBPERCENT )
			{
				iPercent = g_guiSettings.GetInt("ProgramsLists.AutoSwitchPercentage");
			}
		}
		break;
	}
	// if this was called by an unknown window just return listtview
	if (iSortMethod < 0) return iViewAs;
	
	switch (iSortMethod)
	{
		case METHOD_BYFOLDERS:
			iViewAs = ByFolders(bBigThumbs, vecItems);
			break;

		case METHOD_BYFILES:
			iViewAs = ByFiles(bBigThumbs, bHideParentFolderItems, vecItems);
			break;

		case METHOD_BYTHUMBPERCENT:
			iViewAs = ByThumbPercent(bBigThumbs, bHideParentFolderItems, iPercent, vecItems);
			break;
	}

	// return the view_as type.  -1 = error.
	return iViewAs;
}

/// \brief Auto Switch method based on the current directory \e containing ALL folders and \e atleast one non-default thumb
/// \param bBigThumbs Use Big Thumbs?
/// \param vecItems Vector of FileItems
int CAutoSwitch::ByFolders(bool bBigThumbs, CFileItemList& vecItems)
{
	// is the list all folders?
	if (vecItems.GetFolderCount() == vecItems.Size())
	{
		// test for non-default thumbs
		bool bThumbs = false;
		for (int i=0; i<vecItems.Size(); i++)
		{
			CFileItem* pItem = vecItems[i];
			if (!pItem->HasDefaultThumb())
			{
				bThumbs = true;
				break;
			}
		}
		// if non-default thumbs, switch to thumb panel
		if (bThumbs)
		{
			if (bBigThumbs)
			{
				return VIEW_AS_LARGEICONS;
			}
			else
			{
				return VIEW_AS_ICONS;
			}
		}
	}

	// else switch to list view
	return VIEW_AS_LIST;
}

/// \brief Auto Switch method based on the current directory \e not containing ALL files and \e atleast one non-default thumb
/// \param bBigThumbs Use Big Thumbs?
/// \param vecItems Vector of FileItems
int CAutoSwitch::ByFiles(bool bBigThumbs, bool bHideParentDirItems, CFileItemList& vecItems)
{
	bool bThumbs = false;
	int iCompare = 0;

	// parent directorys are visible, incrememt
	if (!bHideParentDirItems)
	{
		iCompare = 1;
	}

	// confirm the list is not just files and folderback
	if (vecItems.GetFolderCount() > iCompare)
	{
		// test to see if there are thumbs other than the defaults
		for (int i=0; i<vecItems.Size(); i++)
		{
			CFileItem* pItem = vecItems[i];
			if (!pItem->HasDefaultThumb())
			{
				bThumbs = true;
				break;
			}
		}

		// if there thumbs other than defaults, switch to thumb view
		if (bThumbs)
		{
			if (bBigThumbs)
			{
				return VIEW_AS_LARGEICONS;
			}
			else
			{
				return VIEW_AS_ICONS;
			}
		}
	}

	// else use list view
	return VIEW_AS_LIST;
}


/// \brief Auto Switch method based on the percentage of non-default thumbs \e in the current directory
/// \param bBigThumbs Use Big Thumbs?
/// \param iPercent Percent of non-default thumbs to autoswitch on
/// \param vecItems Vector of FileItems
int CAutoSwitch::ByThumbPercent(bool bBigThumbs, bool bHideParentDirItems, int iPercent, CFileItemList& vecItems)
{
	bool bThumbs = false;
	int iNonDefault = 0;
	int iNumItems = vecItems.Size();
	if (!bHideParentDirItems)
	{
		iNumItems--;
	}

	for (int i=0; i<vecItems.Size(); i++)
	{
		CFileItem* pItem = vecItems[i];
		if (!pItem->HasDefaultThumb())
		{
			iNonDefault++;
			float fTempPercent = ( (float)iNonDefault / (float)iNumItems ) * (float)100;
			if (fTempPercent >= (float)iPercent)
			{
				bThumbs = true;
				break;
			}
		}
	}

	// if there enough thumbs, switch to thumb view
	if (bThumbs)
	{
		if (bBigThumbs)
		{
			return VIEW_AS_LARGEICONS;
		}
		else
		{
			return VIEW_AS_ICONS;
		}
	}

	// else use list view
	return VIEW_AS_LIST;
}