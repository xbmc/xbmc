/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AutoSwitch.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "FileItem.h"
#include "guilib/Key.h"

#define METHOD_BYFOLDERS  0
#define METHOD_BYFILES   1
#define METHOD_BYTHUMBPERCENT 2
#define METHOD_BYFILECOUNT 3
#define METHOD_BYFOLDERTHUMBS 4

CAutoSwitch::CAutoSwitch(void)
{}

CAutoSwitch::~CAutoSwitch(void)
{}

/// \brief Generic function to add a layer of transparency to the calling window
/// \param vecItems Vector of FileItems passed from the calling window
int CAutoSwitch::GetView(const CFileItemList &vecItems)
{
  int iSortMethod = -1;
  int iPercent = 0;
  int iCurrentWindow = g_windowManager.GetActiveWindow();
  bool bHideParentFolderItems = !g_guiSettings.GetBool("filelists.showparentdiritems");

  switch (iCurrentWindow)
  {
  case WINDOW_MUSIC_FILES:
    {
      iSortMethod = METHOD_BYFOLDERTHUMBS;
      iPercent = 50;
    }
    break;

  case WINDOW_VIDEO_FILES:
    {
      iSortMethod = METHOD_BYTHUMBPERCENT;
      iPercent = 50;  // 50% of thumbs -> use thumbs.
    }
    break;

  case WINDOW_PICTURES:
    {
      iSortMethod = METHOD_BYFILECOUNT;
    }
    break;

  case WINDOW_PROGRAMS:
    {
      iSortMethod = METHOD_BYTHUMBPERCENT;
      iPercent = 50;  // 50% of thumbs -> use thumbs.
    }
    break;

  default:
    {
      if(MetadataPercentage(vecItems) > 0.25)
        return DEFAULT_VIEW_INFO;
      else
        return DEFAULT_VIEW_LIST;
    }
    break;
  }

  bool bThumbs = false;

  switch (iSortMethod)
  {
  case METHOD_BYFOLDERS:
    bThumbs = ByFolders(vecItems);
    break;

  case METHOD_BYFILES:
    bThumbs = ByFiles(bHideParentFolderItems, vecItems);
    break;

  case METHOD_BYTHUMBPERCENT:
    bThumbs = ByThumbPercent(bHideParentFolderItems, iPercent, vecItems);
    break;
  case METHOD_BYFILECOUNT:
    bThumbs = ByFileCount(vecItems);
    break;
  case METHOD_BYFOLDERTHUMBS:
    bThumbs = ByFolderThumbPercentage(bHideParentFolderItems, iPercent, vecItems);
    break;
  }

  // the GUIViewControl object will default down to small icons if a big icon
  // view is not available.
  return bThumbs ? DEFAULT_VIEW_BIG_ICONS : DEFAULT_VIEW_LIST;
}

/// \brief Auto Switch method based on the current directory \e containing ALL folders and \e atleast one non-default thumb
/// \param vecItems Vector of FileItems
bool CAutoSwitch::ByFolders(const CFileItemList& vecItems)
{
  bool bThumbs = false;
  // is the list all folders?
  if (vecItems.GetFolderCount() == vecItems.Size())
  {
    // test for thumbs
    for (int i = 0; i < vecItems.Size(); i++)
    {
      const CFileItemPtr pItem = vecItems[i];
      if (pItem->HasArt("thumb"))
      {
        bThumbs = true;
        break;
      }
    }
  }
  return bThumbs;
}

/// \brief Auto Switch method based on the current directory \e not containing ALL files and \e atleast one non-default thumb
/// \param bHideParentDirItems - are we not counting the ".." item?
/// \param vecItems Vector of FileItems
bool CAutoSwitch::ByFiles(bool bHideParentDirItems, const CFileItemList& vecItems)
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
    // test for thumbs
    for (int i = 0; i < vecItems.Size(); i++)
    {
      const CFileItemPtr pItem = vecItems[i];
      if (pItem->HasArt("thumb"))
      {
        bThumbs = true;
        break;
      }
    }
  }
  return bThumbs;
}


/// \brief Auto Switch method based on the percentage of non-default thumbs \e in the current directory
/// \param iPercent Percent of non-default thumbs to autoswitch on
/// \param vecItems Vector of FileItems
bool CAutoSwitch::ByThumbPercent(bool bHideParentDirItems, int iPercent, const CFileItemList& vecItems)
{
  bool bThumbs = false;
  int iNumThumbs = 0;
  int iNumItems = vecItems.Size();
  if (!bHideParentDirItems)
  {
    iNumItems--;
  }

  if (iNumItems <= 0) return false;

  for (int i = 0; i < vecItems.Size(); i++)
  {
    const CFileItemPtr pItem = vecItems[i];
    if (pItem->HasArt("thumb"))
    {
      iNumThumbs++;
      float fTempPercent = ( (float)iNumThumbs / (float)iNumItems ) * (float)100;
      if (fTempPercent >= (float)iPercent)
      {
        bThumbs = true;
        break;
      }
    }
  }

  return bThumbs;
}

/// \brief Auto Switch method based on whether there is more than 25% files.
/// \param iPercent Percent of non-default thumbs to autoswitch on
bool CAutoSwitch::ByFileCount(const CFileItemList& vecItems)
{
  if (vecItems.Size() == 0) return false;
  float fPercent = (float)vecItems.GetFileCount() / vecItems.Size();
  return (fPercent > 0.25);
}

// returns true if:
// 1. Have more than 75% folders and
// 2. Have more than percent folders with thumbs
bool CAutoSwitch::ByFolderThumbPercentage(bool hideParentDirItems, int percent, const CFileItemList &vecItems)
{
  int numItems = vecItems.Size();
  if (!hideParentDirItems)
    numItems--;
  if (numItems <= 0) return false;

  int fileCount = vecItems.GetFileCount();
  if (fileCount > 0.25f * numItems) return false;

  int numThumbs = 0;
  for (int i = 0; i < vecItems.Size(); i++)
  {
    const CFileItemPtr item = vecItems[i];
    if (item->m_bIsFolder && item->HasArt("thumb"))
    {
      numThumbs++;
      if (numThumbs >= 0.01f * percent * (numItems - fileCount))
        return true;
    }
  }

  return false;
}

float CAutoSwitch::MetadataPercentage(const CFileItemList &vecItems)
{
  int count = 0;
  int total = vecItems.Size();
  for (int i = 0; i < vecItems.Size(); i++)
  {
    const CFileItemPtr item = vecItems[i];
    if(item->HasMusicInfoTag()
    || item->HasVideoInfoTag()
    || item->HasPictureInfoTag()
    || item->HasProperty("Addon.ID"))
      count++;
    if(item->IsParentFolder())
      total--;
  }
  return (float)count / total;
}
