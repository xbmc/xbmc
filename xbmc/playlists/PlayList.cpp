/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "PlayList.h"
#include "PlayListFactory.h"
#include <sstream>
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/Random.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "interfaces/AnnouncementManager.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>


using namespace MUSIC_INFO;
using namespace XFILE;
using namespace PLAYLIST;

CPlayList::CPlayList(int id)
  : m_id(id)
{
  m_strPlayListName = "";
  m_iPlayableItems = -1;
  m_bShuffled = false;
  m_bWasPlayed = false;
}

void CPlayList::AnnounceRemove(int pos)
{
  if (m_id < 0)
    return;

  CVariant data;
  data["playlistid"] = m_id;
  data["position"] = pos;
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Playlist, "xbmc", "OnRemove", data);
}

void CPlayList::AnnounceClear()
{
  if (m_id < 0)
    return;

  CVariant data;
  data["playlistid"] = m_id;
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Playlist, "xbmc", "OnClear", data);
}

void CPlayList::AnnounceAdd(const CFileItemPtr& item, int pos)
{
  if (m_id < 0)
    return;

  CVariant data;
  data["playlistid"] = m_id;
  data["position"] = pos;
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Playlist, "xbmc", "OnAdd", item, data);
}

void CPlayList::Add(const CFileItemPtr &item, int iPosition, int iOrder)
{
  int iOldSize = size();
  if (iPosition < 0 || iPosition >= iOldSize)
    iPosition = iOldSize;
  if (iOrder < 0 || iOrder >= iOldSize)
    item->m_iprogramCount = iOldSize;
  else
    item->m_iprogramCount = iOrder;

  // videodb files are not supported by the filesystem as yet
  if (item->IsVideoDb())
    item->SetPath(item->GetVideoInfoTag()->m_strFileNameAndPath);

  // increment the playable counter
  item->ClearProperty("unplayable");
  if (m_iPlayableItems < 0)
    m_iPlayableItems = 1;
  else
    m_iPlayableItems++;

  // set 'IsPlayable' property - needed for properly handling plugin:// URLs
  item->SetProperty("IsPlayable", true);

  //CLog::Log(LOGDEBUG,"%s item:(%02i/%02i)[%s]", __FUNCTION__, iPosition, item->m_iprogramCount, item->GetPath().c_str());
  if (iPosition == iOldSize)
    m_vecItems.push_back(item);
  else
  {
    ivecItems it = m_vecItems.begin() + iPosition;
    m_vecItems.insert(it, 1, item);
    // correct any duplicate order values
    if (iOrder < iOldSize)
      IncrementOrder(iPosition + 1, iOrder);
  }
  AnnounceAdd(item, iPosition);
}

void CPlayList::Add(const CFileItemPtr &item)
{
  Add(item, -1, -1);
}

void CPlayList::Add(CPlayList& playlist)
{
  for (int i = 0; i < (int)playlist.size(); i++)
    Add(playlist[i], -1, -1);
}

void CPlayList::Add(CFileItemList& items)
{
  for (int i = 0; i < (int)items.Size(); i++)
    Add(items[i]);
}

void CPlayList::Insert(CPlayList& playlist, int iPosition /* = -1 */)
{
  // out of bounds so just add to the end
  int iSize = size();
  if (iPosition < 0 || iPosition >= iSize)
  {
    Add(playlist);
    return;
  }
  for (int i = 0; i < (int)playlist.size(); i++)
  {
    int iPos = iPosition + i;
    Add(playlist[i], iPos, iPos);
  }
}

void CPlayList::Insert(CFileItemList& items, int iPosition /* = -1 */)
{
  // out of bounds so just add to the end
  int iSize = size();
  if (iPosition < 0 || iPosition >= iSize)
  {
    Add(items);
    return;
  }
  for (int i = 0; i < (int)items.Size(); i++)
  {
    Add(items[i], iPosition + i, iPosition + i);
  }
}

void CPlayList::Insert(const CFileItemPtr &item, int iPosition /* = -1 */)
{
  // out of bounds so just add to the end
  int iSize = size();
  if (iPosition < 0 || iPosition >= iSize)
  {
    Add(item);
    return;
  }
  Add(item, iPosition, iPosition);
}

void CPlayList::DecrementOrder(int iOrder)
{
  if (iOrder < 0) return;

  // it was the last item so do nothing
  if (iOrder == size()) return;

  // fix all items with an order greater than the removed iOrder
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end())
  {
    CFileItemPtr item = *it;
    if (item->m_iprogramCount > iOrder)
    {
      //CLog::Log(LOGDEBUG,"%s fixing item at order %i", __FUNCTION__, item->m_iprogramCount);
      item->m_iprogramCount--;
    }
    ++it;
  }
}

void CPlayList::IncrementOrder(int iPosition, int iOrder)
{
  if (iOrder < 0) return;

  // fix all items with an order equal or greater to the added iOrder at iPos
  ivecItems it;
  it = m_vecItems.begin() + iPosition;
  while (it != m_vecItems.end())
  {
    CFileItemPtr item = *it;
    if (item->m_iprogramCount >= iOrder)
    {
      //CLog::Log(LOGDEBUG,"%s fixing item at order %i", __FUNCTION__, item->m_iprogramCount);
      item->m_iprogramCount++;
    }
    ++it;
  }
}

void CPlayList::Clear()
{
  bool announce = false;
  if (!m_vecItems.empty())
  {
    m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
    announce = true;
  }
  m_strPlayListName = "";
  m_iPlayableItems = -1;
  m_bWasPlayed = false;

  if (announce)
    AnnounceClear();
}

int CPlayList::size() const
{
  return (int)m_vecItems.size();
}

const CFileItemPtr CPlayList::operator[] (int iItem) const
{
  if (iItem < 0 || iItem >= size())
  {
    assert(false);
    CLog::Log(LOGERROR, "Error trying to retrieve an item that's out of range");
    return CFileItemPtr();
  }
  return m_vecItems[iItem];
}

CFileItemPtr CPlayList::operator[] (int iItem)
{
  if (iItem < 0 || iItem >= size())
  {
    assert(false);
    CLog::Log(LOGERROR, "Error trying to retrieve an item that's out of range");
    return CFileItemPtr();
  }
  return m_vecItems[iItem];
}

void CPlayList::Shuffle(int iPosition)
{
  if (size() == 0)
    // nothing to shuffle, just set the flag for later
    m_bShuffled = true;
  else
  {
    if (iPosition >= size())
      return;
    if (iPosition < 0)
      iPosition = 0;
    CLog::Log(LOGDEBUG,"%s shuffling at pos:%i", __FUNCTION__, iPosition);

    ivecItems it = m_vecItems.begin() + iPosition;
    KODI::UTILS::RandomShuffle(it, m_vecItems.end());

    // the list is now shuffled!
    m_bShuffled = true;
  }
}

struct SSortPlayListItem
{
  static bool PlaylistSort(const CFileItemPtr &left, const CFileItemPtr &right)
  {
    return (left->m_iprogramCount < right->m_iprogramCount);
  }
};

void CPlayList::UnShuffle()
{
  std::sort(m_vecItems.begin(), m_vecItems.end(), SSortPlayListItem::PlaylistSort);
  // the list is now unshuffled!
  m_bShuffled = false;
}

const std::string& CPlayList::GetName() const
{
  return m_strPlayListName;
}

void CPlayList::Remove(const std::string& strFileName)
{
  int iOrder = -1;
  int position = 0;
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end() )
  {
    CFileItemPtr item = *it;
    if (item->GetPath() == strFileName)
    {
      iOrder = item->m_iprogramCount;
      it = m_vecItems.erase(it);
      AnnounceRemove(position);
      //CLog::Log(LOGDEBUG,"PLAYLIST, removing item at order %i", iPos);
    }
    else
    {
      ++position;
      ++it;
    }
  }
  DecrementOrder(iOrder);
}

int CPlayList::FindOrder(int iOrder) const
{
  for (int i = 0; i < size(); i++)
  {
    if (m_vecItems[i]->m_iprogramCount == iOrder)
      return i;
  }
  return -1;
}

// remove item from playlist by position
void CPlayList::Remove(int position)
{
  int iOrder = -1;
  if (position >= 0 && position < (int)m_vecItems.size())
  {
    iOrder = m_vecItems[position]->m_iprogramCount;
    m_vecItems.erase(m_vecItems.begin() + position);
  }
  DecrementOrder(iOrder);

  AnnounceRemove(position);
}

int CPlayList::RemoveDVDItems()
{
  std::vector <std::string> vecFilenames;

  // Collect playlist items from DVD share
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end() )
  {
    CFileItemPtr item = *it;
    if ( item->IsCDDA() || item->IsOnDVD() )
    {
      vecFilenames.push_back( item->GetPath() );
    }
    ++it;
  }

  // Delete them from playlist
  int nFileCount = vecFilenames.size();
  if ( nFileCount )
  {
    std::vector <std::string>::iterator it;
    it = vecFilenames.begin();
    while (it != vecFilenames.end() )
    {
      std::string& strFilename = *it;
      Remove( strFilename );
      ++it;
    }
    vecFilenames.erase( vecFilenames.begin(), vecFilenames.end() );
  }
  return nFileCount;
}

bool CPlayList::Swap(int position1, int position2)
{
  if (
    (position1 < 0) ||
    (position2 < 0) ||
    (position1 >= size()) ||
    (position2 >= size())
  )
  {
    return false;
  }

  if (!IsShuffled())
  {
    // swap the ordinals before swapping the items!
    //CLog::Log(LOGDEBUG,"PLAYLIST swapping items at orders (%i, %i)",m_vecItems[position1]->m_iprogramCount,m_vecItems[position2]->m_iprogramCount);
    std::swap(m_vecItems[position1]->m_iprogramCount, m_vecItems[position2]->m_iprogramCount);
  }

  // swap the items
  std::swap(m_vecItems[position1], m_vecItems[position2]);
  return true;
}

void CPlayList::SetUnPlayable(int iItem)
{
  if (iItem < 0 || iItem >= size())
  {
    CLog::Log(LOGWARNING, "Attempt to set unplayable index %d", iItem);
    return;
  }

  CFileItemPtr item = m_vecItems[iItem];
  if (!item->GetProperty("unplayable").asBoolean())
  {
    item->SetProperty("unplayable", true);
    m_iPlayableItems--;
  }
}


bool CPlayList::Load(const std::string& strFileName)
{
  Clear();
  m_strBasePath = URIUtils::GetDirectory(strFileName);

  CFileStream file;
  if (!file.Open(strFileName))
    return false;

  if (file.GetLength() > 1024*1024)
  {
    CLog::Log(LOGWARNING, "%s - File is larger than 1 MB, most likely not a playlist", __FUNCTION__);
    return false;
  }

  return LoadData(file);
}

bool CPlayList::LoadData(std::istream &stream)
{
  // try to read as a string
  std::ostringstream ostr;
  ostr << stream.rdbuf();
  return LoadData(ostr.str());
}

bool CPlayList::LoadData(const std::string& strData)
{
  return false;
}


bool CPlayList::Expand(int position)
{
  CFileItemPtr item = m_vecItems[position];
  std::unique_ptr<CPlayList> playlist (CPlayListFactory::Create(*item.get()));
  if ( NULL == playlist.get())
    return false;

  if(!playlist->Load(item->GetPath()))
    return false;

  // remove any item that points back to itself
  for(int i = 0;i<playlist->size();i++)
  {
    if(StringUtils::EqualsNoCase((*playlist)[i]->GetPath(), item->GetPath()))
    {
      playlist->Remove(i);
      i--;
    }
  }

  if(playlist->size() <= 0)
    return false;

  Remove(position);
  Insert(*playlist, position);
  return true;
}

void CPlayList::UpdateItem(const CFileItem *item)
{
  if (!item) return;

  for (ivecItems it = m_vecItems.begin(); it != m_vecItems.end(); ++it)
  {
    CFileItemPtr playlistItem = *it;
    if (playlistItem->IsSamePath(item))
    {
      std::string temp = playlistItem->GetPath(); // save path, it may have been altered
      *playlistItem = *item;
      playlistItem->SetPath(temp);
      break;
    }
  }
}

const std::string& CPlayList::ResolveURL(const CFileItemPtr &item ) const
{
  if (item->IsMusicDb() && item->HasMusicInfoTag())
    return item->GetMusicInfoTag()->GetURL();
  else
    return item->GetPath();
}
