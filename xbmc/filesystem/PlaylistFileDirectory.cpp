/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "PlaylistFileDirectory.h"
#include "playlists/PlayListFactory.h"
#include "File.h"
#include "URL.h"
#include "playlists/PlayList.h"

using namespace PLAYLIST;

namespace XFILE
{
  CPlaylistFileDirectory::CPlaylistFileDirectory() = default;

  CPlaylistFileDirectory::~CPlaylistFileDirectory() = default;

  bool CPlaylistFileDirectory::GetDirectory(const CURL& url, CFileItemList& items)
  {
    const std::string pathToUrl = url.Get();
    std::unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(pathToUrl));
    if ( NULL != pPlayList.get())
    {
      // load it
      if (!pPlayList->Load(pathToUrl))
        return false; //hmmm unable to load playlist?

      CPlayList playlist = *pPlayList;
      // convert playlist items to songs
      for (int i = 0; i < (int)playlist.size(); ++i)
      {
        CFileItemPtr item = playlist[i];
        item->m_iprogramCount = i;  // hack for playlist order
        items.Add(item);
      }
    }
    return true;
  }

  bool CPlaylistFileDirectory::ContainsFiles(const CURL& url)
  {
    const std::string pathToUrl = url.Get();
    std::unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(pathToUrl));
    if ( NULL != pPlayList.get())
    {
      // load it
      if (!pPlayList->Load(pathToUrl))
        return false; //hmmm unable to load playlist?

      return (pPlayList->size() > 1);
    }
    return false;
  }

  bool CPlaylistFileDirectory::Remove(const CURL& url)
  {
    return XFILE::CFile::Delete(url);
  }
}

