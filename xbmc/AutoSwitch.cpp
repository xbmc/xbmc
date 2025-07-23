/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AutoSwitch.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "view/ViewState.h"

#include <algorithm>

namespace
{

enum class Method
{
  NONE,
  BY_FOLDERS,
  BY_FILES,
  BY_THUMBPERCENT,
  BY_FILECOUNT,
  BY_FOLDERTHUMBS,
};

auto hasThumb = [](const auto& item) { return item->HasArt("thumb"); };
}

/// \brief Generic function to add a layer of transparency to the calling window
/// \param vecItems Vector of FileItems passed from the calling window
int CAutoSwitch::GetView(const CFileItemList &vecItems)
{
  Method iSortMethod = Method::NONE;
  int iPercent = 0;
  int iCurrentWindow = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  bool bHideParentFolderItems = !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_SHOWPARENTDIRITEMS);

  switch (iCurrentWindow)
  {
  case WINDOW_PICTURES:
    {
      iSortMethod = Method::BY_FILECOUNT;
    }
    break;

  case WINDOW_PROGRAMS:
    {
      iSortMethod = Method::BY_THUMBPERCENT;
      iPercent = 50;  // 50% of thumbs -> use thumbs.
    }
    break;

  default:
    {
      if (MetadataPercentage(vecItems) > 0.25f)
        return DEFAULT_VIEW_INFO;
      else
        return DEFAULT_VIEW_LIST;
    }
    break;
  }

  bool bThumbs = false;

  switch (iSortMethod)
  {
    case Method::BY_FOLDERS:
      bThumbs = ByFolders(vecItems);
      break;

    case Method::BY_FILES:
      bThumbs = ByFiles(bHideParentFolderItems, vecItems);
      break;

    case Method::BY_THUMBPERCENT:
      bThumbs = ByThumbPercent(bHideParentFolderItems, iPercent, vecItems);
      break;

    case Method::BY_FILECOUNT:
      bThumbs = ByFileCount(vecItems);
      break;

    case Method::BY_FOLDERTHUMBS:
      bThumbs = ByFolderThumbPercentage(bHideParentFolderItems, iPercent, vecItems);
      break;

    case Method::NONE:
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
  if (vecItems.GetFolderCount() != vecItems.Size())
    return false;

  return std::ranges::any_of(vecItems, hasThumb);
}

/// \brief Auto Switch method based on the current directory \e not containing ALL files and \e atleast one non-default thumb
/// \param bHideParentDirItems - are we not counting the ".." item?
/// \param vecItems Vector of FileItems
bool CAutoSwitch::ByFiles(bool bHideParentDirItems, const CFileItemList& vecItems)
{
  if (vecItems.GetFolderCount() <= (bHideParentDirItems ? 0 : 1))
    return false;

  return std::ranges::any_of(vecItems, hasThumb);
}

/// \brief Auto Switch method based on the percentage of non-default thumbs \e in the current directory
/// \param iPercent Percent of non-default thumbs to autoswitch on
/// \param vecItems Vector of FileItems
bool CAutoSwitch::ByThumbPercent(bool bHideParentDirItems, int iPercent, const CFileItemList& vecItems)
{
  const int numItems = bHideParentDirItems ? vecItems.Size() : vecItems.Size() - 1;
  if (numItems <= 0)
    return false;

  const float numThumbs = std::ranges::count_if(vecItems, hasThumb);
  return numThumbs / numItems * 100.f > iPercent;
}

/// \brief Auto Switch method based on whether there is more than 25% files.
/// \param iPercent Percent of non-default thumbs to autoswitch on
bool CAutoSwitch::ByFileCount(const CFileItemList& vecItems)
{
  if (vecItems.IsEmpty())
    return false;
  return static_cast<float>(vecItems.GetFileCount()) / vecItems.Size() > 0.25f;
}

// returns true if:
// 1. Have more than 75% folders and
// 2. Have more than percent folders with thumbs
bool CAutoSwitch::ByFolderThumbPercentage(bool hideParentDirItems, int percent, const CFileItemList &vecItems)
{
  const int numItems = hideParentDirItems ? vecItems.Size() : vecItems.Size() - 1;
  if (numItems <= 0)
    return false;

  const int fileCount = vecItems.GetFileCount();
  if (fileCount > 0.25f * numItems)
    return false;

  const int numThumbs = std::ranges::count_if(
      vecItems, [](const auto& item) { return item->IsFolder() && item->HasArt("thumb"); });
  return numThumbs >= 0.01f * percent * (numItems - fileCount);
}

float CAutoSwitch::MetadataPercentage(const CFileItemList &vecItems)
{
  int total = vecItems.Size();
  const float count =
      std::ranges::count_if(vecItems,
                            [&total](const auto& item)
                            {
                              if (item->IsParentFolder())
                                --total;

                              return item->HasMusicInfoTag() || item->HasVideoInfoTag() ||
                                     item->HasPictureInfoTag() || item->HasProperty("Addon.ID");
                            });
  return total != 0 ? count / total : 0.0f;
}
