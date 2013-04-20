/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "GUIDropPolicy.h"
#include "FileItem.h"
#include "windows/GUIWindowFileManager.h"
#include "filesystem/FavouritesDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "utils/FileOperationJob.h"
#include "GUIInfoManager.h"
#include "playlists/PlayList.h"
#include "PlayListPlayer.h"

IGUIDropPolicy* IGUIDropPolicy::Create(CArchive& ar)
{
  int dummy;
  
  ar >> dummy;
  DropPolicyType type = (DropPolicyType)dummy;
  
  CStdString str;
  switch(type)
  {
    case DPT_NONE:
      return NULL;
    case DPT_MUSIC_PLAYLIST:
      return new MusicPlaylistDropPolicy();      
    case DPT_VIDEO_PLAYLIST:
      return new VideoPlaylistDropPolicy();
    case DPT_FILE_MANAGER:
      ar >> str;
      return new FileManagerDropPolicy(str);
    case DPT_FAVOURITES:
      return new CFavouritesDropPolicy();
  }
  return NULL;
}

bool IGUIDropPolicy::IsDuplicate(const CFileItemPtr& item, const CFileItemList& list)
{
  CFileItemPtr dummy = list.Get(item->GetPath());
  return dummy;
}

CFileItemPtr IGUIDropPolicy::CreateDummy(const CFileItemPtr item) 
{ 
  return CFileItemPtr(new CFileItem(*item)); 
}

bool PlaylistDropPolicy::OnAdd(CFileItemList& list, CFileItemPtr item, int position,  int PlaylistType) 
{ 
  PLAYLIST::CPlayList& playlist = g_playlistPlayer.GetPlaylist(PlaylistType);
  playlist.Insert(item, position);
  
  if (g_playlistPlayer.GetCurrentPlaylist() == PlaylistType)
  { //if we dropped on the currently playing playlist, we might need to adjust the currently playling song
    int iCurrentSong = g_playlistPlayer.GetCurrentSong();
    if (position <= iCurrentSong)
      ++iCurrentSong;
    g_playlistPlayer.SetCurrentSong(iCurrentSong);
  }
  return true;
}
bool PlaylistDropPolicy::OnMove(CFileItemList& list, int posBefore, int posAfter,  int PlaylistType) 
{
  PLAYLIST::CPlayList& playlist = g_playlistPlayer.GetPlaylist(PlaylistType);
  playlist.Move(posBefore, posAfter-posBefore);
  
  if (g_playlistPlayer.GetCurrentPlaylist() == PlaylistType)
  { //if we dropped on the currently playing playlist, we might need to adjust the currently playling song
    int iCurrentSong = g_playlistPlayer.GetCurrentSong();
    if (posBefore == iCurrentSong)
      iCurrentSong = posAfter;
    if (posBefore > iCurrentSong && posAfter <= iCurrentSong)
      ++iCurrentSong;
    if (posBefore < iCurrentSong && posAfter > iCurrentSong)
      --iCurrentSong;
    g_playlistPlayer.SetCurrentSong(iCurrentSong);
  }
  return true;
}

bool MusicPlaylistDropPolicy::IsDropable(const CFileItemPtr& item) const 
{
  return item->IsAudio(); 
}

bool MusicPlaylistDropPolicy::OnDropAdd(CFileItemList& list, CFileItemPtr item, int position) 
{
  return OnAdd(list, item, position, PLAYLIST_MUSIC); 
}

bool MusicPlaylistDropPolicy::OnDropMove(CFileItemList& list, int posBefore, int posAfter) 
{
  return OnMove(list, posBefore, posAfter, PLAYLIST_MUSIC); 
}


bool VideoPlaylistDropPolicy::IsDropable(const CFileItemPtr& item) const 
{
  return item->IsVideo(); 
}

bool VideoPlaylistDropPolicy::OnDropAdd(CFileItemList& list, CFileItemPtr item, int position) 
{
  return OnAdd(list, item, position, PLAYLIST_VIDEO); 
}

bool VideoPlaylistDropPolicy::OnDropMove(CFileItemList& list, int posBefore, int posAfter) 
{
  return OnMove(list, posBefore, posAfter, PLAYLIST_VIDEO); 
}


bool FileManagerDropPolicy::OnDropAdd(CFileItemList& list, CFileItemPtr item, int position)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(120, 123, 0, 0))
    return false;
  
  CFileItemList toCopy;
  item->Select(true);
  toCopy.Add(item);
  
  CGUIWindowFileManager* jobManager = (CGUIWindowFileManager*)g_windowManager.GetWindow(WINDOW_FILES);
  if(!jobManager)
    return false;
  jobManager->AddJob(new CFileOperationJob(CFileOperationJob::ActionCopy,  //Should dragging mean "copy" or "move"???
                                           toCopy,
                                           list.GetPath(),
                                           true, 16201, 16202));
  item->Select(false);
  return true;
}


CFileItemPtr CFavouritesDropPolicy::CreateDummy(const CFileItemPtr item) 
{
  CFileItemPtr result(new CFileItem(*item));
  XFILE::CFavouritesDirectory favs;
  result->SetPath(favs.GetExecutePath(item.get(), g_infoManager.GetDragStartWindow()));
  return result;
}

bool CFavouritesDropPolicy::OnDrop(CFileItemList& items) 
{
  XFILE::CFavouritesDirectory favs;
  return favs.Save(items);
}

bool CFavouritesDropPolicy::IsDuplicate(const CFileItemPtr& item, const CFileItemList& list)
{
  XFILE::CFavouritesDirectory favs;
  return favs.IsFavourite(item.get(), g_infoManager.GetDragStartWindow());
}
