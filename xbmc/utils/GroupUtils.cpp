/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GroupUtils.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/MultiPathDirectory.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDbUrl.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <map>
#include <set>

using namespace KODI;

using SetMap = std::map<int, std::set<CFileItemPtr> >;

bool GroupUtils::Group(GroupBy groupBy, const std::string &baseDir, const CFileItemList &items, CFileItemList &groupedItems, GroupAttribute groupAttributes /* = GroupAttributeNone */)
{
  CFileItemList ungroupedItems;
  return Group(groupBy, baseDir, items, groupedItems, ungroupedItems, groupAttributes);
}

bool GroupUtils::Group(GroupBy groupBy, const std::string &baseDir, const CFileItemList &items, CFileItemList &groupedItems, CFileItemList &ungroupedItems, GroupAttribute groupAttributes /* = GroupAttributeNone */)
{
  if (groupBy == GroupByNone)
    return false;

  // nothing to do if there are no items to group
  if (items.Size() <= 0)
    return true;

  SetMap setMap;
  for (int index = 0; index < items.Size(); index++)
  {
    bool ungrouped = true;
    const CFileItemPtr item = items.Get(index);

    // group by sets
    if ((groupBy & GroupBySet) &&
      item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_set.id > 0)
    {
      ungrouped = false;
      setMap[item->GetVideoInfoTag()->m_set.id].insert(item);
    }

    if (ungrouped)
      ungroupedItems.Add(item);
  }

  if ((groupBy & GroupBySet) && !setMap.empty())
  {
    CVideoDbUrl itemsUrl;
    if (!itemsUrl.FromString(baseDir))
      return false;

    for (SetMap::const_iterator set = setMap.begin(); set != setMap.end(); ++set)
    {
      // only one item in the set, so add it to the ungrouped items
      if (set->second.size() == 1 && (groupAttributes & GroupAttributeIgnoreSingleItems))
      {
        ungroupedItems.Add(*set->second.begin());
        continue;
      }

      CFileItemPtr pItem(new CFileItem((*set->second.begin())->GetVideoInfoTag()->m_set.title));
      pItem->GetVideoInfoTag()->m_iDbId = set->first;
      pItem->GetVideoInfoTag()->m_type = MediaTypeVideoCollection;

      std::string basePath = StringUtils::Format("videodb://movies/sets/{}/", set->first);
      CVideoDbUrl videoUrl;
      if (!videoUrl.FromString(basePath))
        pItem->SetPath(basePath);
      else
      {
        videoUrl.AddOptions((*set->second.begin())->GetURL().GetOptions());
        pItem->SetPath(videoUrl.ToString());
      }
      pItem->m_bIsFolder = true;

      CVideoInfoTag* setInfo = pItem->GetVideoInfoTag();
      setInfo->m_strPath = pItem->GetPath();
      setInfo->m_strTitle = pItem->GetLabel();
      setInfo->m_strPlot = (*set->second.begin())->GetVideoInfoTag()->m_set.overview;

      int ratings = 0;
      float totalRatings = 0;
      int iWatched = 0; // have all the movies been played at least once?
      int inProgress = 0;
      std::set<std::string> pathSet;
      for (std::set<CFileItemPtr>::const_iterator movie = set->second.begin(); movie != set->second.end(); ++movie)
      {
        CVideoInfoTag* movieInfo = (*movie)->GetVideoInfoTag();
        // handle rating
        if (movieInfo->GetRating().rating > 0.0f)
        {
          ratings++;
          totalRatings += movieInfo->GetRating().rating;
        }

        // handle year
        if (movieInfo->GetYear() > setInfo->GetYear())
          setInfo->SetYear(movieInfo->GetYear());

        // handle lastplayed
        if (movieInfo->m_lastPlayed.IsValid() && movieInfo->m_lastPlayed > setInfo->m_lastPlayed)
          setInfo->m_lastPlayed = movieInfo->m_lastPlayed;

        // handle dateadded
        if (movieInfo->m_dateAdded.IsValid() && movieInfo->m_dateAdded > setInfo->m_dateAdded)
          setInfo->m_dateAdded = movieInfo->m_dateAdded;

        // handle playcount/watched
        setInfo->SetPlayCount(setInfo->GetPlayCount() + movieInfo->GetPlayCount());
        if (movieInfo->GetPlayCount() > 0)
          iWatched++;

        // handle resume points
        CBookmark bookmark = movieInfo->GetResumePoint();
        if (bookmark.IsSet())
          inProgress++;

        //accumulate the path for a multipath construction
        CFileItem video(movieInfo->m_basePath, false);
        if (VIDEO::IsVideo(video))
          pathSet.insert(URIUtils::GetParentPath(movieInfo->m_basePath));
        else
          pathSet.insert(movieInfo->m_basePath);
      }
      setInfo->m_basePath = XFILE::CMultiPathDirectory::ConstructMultiPath(pathSet);

      if (ratings > 0)
        pItem->GetVideoInfoTag()->SetRating(totalRatings / ratings);

      setInfo->SetPlayCount(iWatched >= static_cast<int>(set->second.size()) ? (setInfo->GetPlayCount() / set->second.size()) : 0);
      pItem->SetProperty("total", (int)set->second.size());
      pItem->SetProperty("watched", iWatched);
      pItem->SetProperty("unwatched", (int)set->second.size() - iWatched);
      pItem->SetProperty("inprogress", inProgress);
      pItem->SetOverlayImage(setInfo->GetPlayCount() > 0 ? CGUIListItem::ICON_OVERLAY_WATCHED
                                                         : CGUIListItem::ICON_OVERLAY_UNWATCHED);

      groupedItems.Add(pItem);
    }
  }

  return true;
}

bool GroupUtils::GroupAndMix(GroupBy groupBy, const std::string &baseDir, const CFileItemList &items, CFileItemList &groupedItemsMixed, GroupAttribute groupAttributes /* = GroupAttributeNone */)
{
  CFileItemList ungroupedItems;
  if (!Group(groupBy, baseDir, items, groupedItemsMixed, ungroupedItems, groupAttributes))
    return false;

  // add all the ungrouped items as well
  groupedItemsMixed.Append(ungroupedItems);

  return true;
}
