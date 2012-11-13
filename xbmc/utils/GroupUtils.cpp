/*
 *      Copyright (C) 2012 Team XBMC
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

#include <map>
#include <set>

#include "GroupUtils.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"

using namespace std;

typedef map<int, set<CFileItemPtr> > SetMap;

bool GroupUtils::Group(GroupBy groupBy, const CFileItemList &items, CFileItemList &groupedItems, GroupAttribute groupAttributes /* = GroupAttributeNone */)
{
  if (items.Size() <= 0 || groupBy == GroupByNone)
    return false;

  SetMap setMap;
  for (int index = 0; index < items.Size(); index++)
  {
    bool add = true;
    const CFileItemPtr item = items.Get(index);

    // group by sets
    if ((groupBy & GroupBySet) &&
        item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iSetId > 0)
    {
      add = false;
      setMap[item->GetVideoInfoTag()->m_iSetId].insert(item);
    }

    if (add)
      groupedItems.Add(item);
  }

  if ((groupBy & GroupBySet) && setMap.size() > 0)
  {
    for (SetMap::const_iterator set = setMap.begin(); set != setMap.end(); set++)
    {
      // only one item in the set, so just re-add it
      if (set->second.size() == 1 && (groupAttributes & GroupAttributeIgnoreSingleItems))
      {
        groupedItems.Add(*set->second.begin());
        continue;
      }

      CFileItemPtr pItem(new CFileItem((*set->second.begin())->GetVideoInfoTag()->m_strSet));
      pItem->GetVideoInfoTag()->m_iDbId = set->first;
      pItem->GetVideoInfoTag()->m_type = "set";
      pItem->SetPath(StringUtils::Format("videodb://1/7/%ld/", set->first));
      pItem->m_bIsFolder = true;

      CVideoInfoTag* setInfo = pItem->GetVideoInfoTag();
      setInfo->m_strPath = pItem->GetPath();
      setInfo->m_strTitle = pItem->GetLabel();

      int ratings = 0;
      int iWatched = 0; // have all the movies been played at least once?
      for (std::set<CFileItemPtr>::const_iterator movie = set->second.begin(); movie != set->second.end(); movie++)
      {
        CVideoInfoTag* movieInfo = (*movie)->GetVideoInfoTag();
        // handle rating
        if (movieInfo->m_fRating > 0.0f)
        {
          ratings++;
          setInfo->m_fRating += movieInfo->m_fRating;
        }
        
        // handle year
        if (movieInfo->m_iYear > setInfo->m_iYear)
          setInfo->m_iYear = movieInfo->m_iYear;
        
        // handle lastplayed
        if (movieInfo->m_lastPlayed.IsValid() && movieInfo->m_lastPlayed > setInfo->m_lastPlayed)
          setInfo->m_lastPlayed = movieInfo->m_lastPlayed;
        
        // handle dateadded
        if (movieInfo->m_dateAdded.IsValid() && movieInfo->m_dateAdded > setInfo->m_dateAdded)
          setInfo->m_dateAdded = movieInfo->m_dateAdded;
        
        // handle playcount/watched
        setInfo->m_playCount += movieInfo->m_playCount;
        if (movieInfo->m_playCount > 0)
          iWatched++;
      }

      if (ratings > 1)
        pItem->GetVideoInfoTag()->m_fRating /= ratings;
        
      setInfo->m_playCount = iWatched >= (int)set->second.size() ? (setInfo->m_playCount / set->second.size()) : 0;
      pItem->SetProperty("total", (int)set->second.size());
      pItem->SetProperty("watched", iWatched);
      pItem->SetProperty("unwatched", (int)set->second.size() - iWatched);
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, setInfo->m_playCount > 0);

      groupedItems.Add(pItem);
    }
  }

  return true;
}

bool GroupUtils::GroupAndSort(GroupBy groupBy, const CFileItemList &items, const SortDescription &sortDescription, CFileItemList &groupedItems, GroupAttribute groupAttributes /* = GroupAttributeNone */)
{
  if (!Group(groupBy, items, groupedItems, groupAttributes))
    return false;

  groupedItems.Sort(sortDescription);
  return true;
}
