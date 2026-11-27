/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayList.h"

#include "FileItemList.h"
#include "ServiceBroker.h"
#include "application/ApplicationPlayLists.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListEntryRules.h"
#include "playlists/PlayListFileItemClassify.h"
#include "utils/URIUtils.h"

using namespace KODI;

namespace XBMCAddon
{
  namespace xbmc
  {
  namespace
  {
  PLAYLIST::Type TypeFromId(int playList)
  {
    if (playList == PLAYLIST_MUSIC_ID)
      return PLAYLIST::Audio;
    if (playList == PLAYLIST_VIDEO_ID)
      return PLAYLIST::Video;
    throw PlayListException("PlayList does not exist");
  }
  } // namespace

  // a Python playlist wraps the Video or Audio playlist rather than owning one
  PlayList::PlayList(int playList) : m_type(TypeFromId(playList)), pPlayList(&CServiceBroker::GetPlayLists()->GetPlayList(m_type))
  {
  }

    PlayList::~PlayList() = default;

    int PlayList::getPlayListId() const
    {
      return m_type == PLAYLIST::Audio ? PLAYLIST_MUSIC_ID : PLAYLIST_VIDEO_ID;
    }

    void PlayList::add(const String& url, XBMCAddon::xbmcgui::ListItem* listitem, int index)
    {
      CFileItemList items;

      if (listitem != NULL)
      {
        // an optional listitem was passed
        // set m_strPath to the passed url
        listitem->item->SetPath(url);

        items.Add(listitem->item);
      }
      else
      {
        CFileItemPtr item(new CFileItem(url, false));
        item->SetLabel(url);

        items.Add(item);
      }

      CServiceBroker::GetPlayLists()->Insert(m_type, items, index);
    }

    bool PlayList::load(const char* cFileName)
    {
      const auto item = std::make_shared<CFileItem>(cFileName, false);

      if (PLAYLIST::HoldsEntries(*item))
      {
        // replace this playlist's contents with the file's entries
        PLAYLIST::CEntriesAsListed asListed;
        CFileItemList items;
        CApplicationPlayLists::ExpandToEntries(item, asListed, nullptr, items);
        if (items.IsEmpty())
          return false;

        for (const auto& entry : items)
        {
          if (entry->GetLabel().empty())
            entry->SetLabel(URIUtils::GetFileName(entry->GetPath()));
        }
        CServiceBroker::GetPlayLists()->Replace(m_type, items);
      }
      else
        // filename is not a valid playlist
        throw PlayListException("Not a valid playlist");

      return true;
    }

    void PlayList::remove(const char* filename)
    {
      CServiceBroker::GetPlayLists()->Remove(m_type, std::string{filename});
    }

    void PlayList::clear()
    {
      CServiceBroker::GetPlayLists()->Clear(m_type);
    }

    int PlayList::size()
    {
      return pPlayList->Size();
    }

    void PlayList::shuffle()
    {
      CServiceBroker::GetPlayLists()->SetShuffle(m_type, true, CApplicationPlayLists::Persist::No);
    }

    void PlayList::unshuffle()
    {
      CServiceBroker::GetPlayLists()->SetShuffle(m_type, false, CApplicationPlayLists::Persist::No);
    }

    int PlayList::getposition()
    {
      return pPlayList->GetCurrentPosition();
    }

    XBMCAddon::xbmcgui::ListItem* PlayList::operator [](long i)
    {
      int iPlayListSize = size();

      long pos = i;
      if (pos < 0) pos += iPlayListSize;

      if (pos < 0 || pos >= iPlayListSize)
        throw PlayListException("array out of bound");

      // a copy: the playlist owns its items, and nothing outside it changes one in place
      const std::shared_ptr<CFileItem> item = (*pPlayList)[pos];
      if (!item)
        throw PlayListException("array out of bound");

      return new XBMCAddon::xbmcgui::ListItem(std::make_shared<CFileItem>(*item));
    }
  }
}

