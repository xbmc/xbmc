/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowMusicPlaylistEditor.h"
#include "Util.h"
#include "Utils/GUIInfoManager.h"
#include "Application.h"
#include "GUIDialogFileBrowser.h"
#include "Filesystem/PlaylistFileDirectory.h"
#include "PlaylistM3U.h"

#define CONTROL_LIST              50
#define CONTROL_LABELFILES        12

#define CONTROL_LOAD_PLAYLIST      6
#define CONTROL_SAVE_PLAYLIST      7
#define CONTROL_CLEAR_PLAYLIST     8

#define CONTROL_PLAYLIST         100
#define CONTROL_LABEL_PLAYLIST   101

CGUIWindowMusicPlaylistEditor::CGUIWindowMusicPlaylistEditor(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_PLAYLIST_EDITOR, "MyMusicPlaylistEditor.xml")
{
  m_thumbLoader.SetObserver(this);
  m_playlistThumbLoader.SetObserver(this);
}

CGUIWindowMusicPlaylistEditor::~CGUIWindowMusicPlaylistEditor(void)
{
}

bool CGUIWindowMusicPlaylistEditor::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR && !m_viewControl.HasControl(GetFocusedControlID()))
  { // don't go to parent folder unless we're on the list in question
    m_gWindowManager.PreviousWindow();
    return true;
  }
  if (action.wID == ACTION_CONTEXT_MENU && GetFocusedControlID() == CONTROL_PLAYLIST)
  {
    int item = GetCurrentPlaylistItem();
    if (item >= 0)
      m_playlist[item]->Select(true);
    if (!OnPopupMenu(-1) && item >= 0 && item < m_playlist.Size())
      m_playlist[item]->Select(false);
    return true;
  }
  return CGUIWindowMusicBase::OnAction(action);
}

bool CGUIWindowMusicPlaylistEditor::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    if (m_playlistThumbLoader.IsLoading())
      m_playlistThumbLoader.StopThread();
    CGUIWindowMusicBase::OnMessage(message);
    ClearPlaylist();
    return true;

  case GUI_MSG_WINDOW_INIT:
    {
      if (m_vecItems.m_strPath == "?")
        m_vecItems.m_strPath.Empty();
      CGUIWindowMusicBase::OnMessage(message);

      LoadPlaylist(message.GetStringParam());
      m_strLoadedPlaylist = message.GetStringParam();

      return true;
    }
    break;

  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1()==GUI_MSG_REMOVED_MEDIA)
        DeleteRemoveableMediaDirectoryCache();
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int control = message.GetSenderId();
      if (control == CONTROL_PLAYLIST)
      {
        int item = GetCurrentPlaylistItem();
        int action = message.GetParam1();
        if (action == ACTION_QUEUE_ITEM || action == ACTION_DELETE_ITEM || action == ACTION_MOUSE_MIDDLE_CLICK)
          OnDeletePlaylistItem(item);
        else if (action == ACTION_MOVE_ITEM_UP)
          OnMovePlaylistItem(item, -1);
        else if (action == ACTION_MOVE_ITEM_DOWN)
          OnMovePlaylistItem(item, 1);
        return true;
      }
      else if (control == CONTROL_LOAD_PLAYLIST)
      { // load a playlist
        OnLoadPlaylist();
        return true;
      }
      else if (control == CONTROL_SAVE_PLAYLIST)
      { // save the playlist
        OnSavePlaylist();
        return true;
      }
      else if (control == CONTROL_CLEAR_PLAYLIST)
      { // clear the playlist
        ClearPlaylist();
        return true;
      }
    }
    break;
  }

  return CGUIWindowMusicBase::OnMessage(message);
}

bool CGUIWindowMusicPlaylistEditor::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  items.Clear();
  if (strDirectory.IsEmpty())
  { // root listing - list files:// and musicdb://
    CFileItem *files = new CFileItem("files://", true);
    files->SetLabel(g_localizeStrings.Get(744));
    files->SetLabelPreformated(true);
    files->m_bIsShareOrDrive = true;
    items.Add(files);
    CFileItem *db = new CFileItem("musicdb://", true);
    db->SetLabel(g_localizeStrings.Get(14022));
    db->SetLabelPreformated(true);
    db->m_bIsShareOrDrive = true;
    items.m_strPath = "";
    items.Add(db);
    return true;
  }

  if (!CGUIWindowMusicBase::GetDirectory(strDirectory, items))
    return false;

  // check for .CUE files here.
  items.FilterCueItems();

  return true;
}

void CGUIWindowMusicPlaylistEditor::OnPrepareFileItems(CFileItemList &items)
{
  RetrieveMusicInfo();

  items.SetCachedMusicThumbs();
}

void CGUIWindowMusicPlaylistEditor::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  // Update object count label
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->IsParentFolder()) iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str()); // " 14 Objects"
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);
}

void CGUIWindowMusicPlaylistEditor::DeleteRemoveableMediaDirectoryCache()
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile("Z:\\r-*.fi", &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      CStdString strFile = "Z:\\";
      strFile += wfd.cFileName;
      DeleteFile(strFile.c_str());
    }
  }
  while (FindNextFile(hFind, &wfd));
}

void CGUIWindowMusicPlaylistEditor::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // we're at the root source listing
  if (m_vecItems.IsVirtualDirectoryRoot() && !m_vecItems[iItem]->IsDVD())
    return;

  if (m_vecItems[iItem]->IsDVD())
    CAutorun::PlayDisc();
  else
    CGUIWindowMusicBase::PlayItem(iItem);
}

void CGUIWindowMusicPlaylistEditor::OnQueueItem(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems.Size())
    return;

  // add this item to our playlist
  CFileItem *item = m_vecItems[iItem];
  CFileItemList newItems;
  AddItemToPlayList(item, newItems);
  AppendToPlaylist(newItems);
}

bool CGUIWindowMusicPlaylistEditor::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  g_infoManager.m_content = "files";
  m_thumbLoader.Load(m_vecItems);

  // update our playlist control
  UpdatePlaylist();
  return true;
}

void CGUIWindowMusicPlaylistEditor::ClearPlaylist()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PLAYLIST, 0, 0, NULL);
  OnMessage(msg);

  m_playlist.Clear();
}

void CGUIWindowMusicPlaylistEditor::UpdatePlaylist()
{
  if (m_playlistThumbLoader.IsLoading())
    m_playlistThumbLoader.StopThread();

  // deselect all items
  for (int i = 0; i < m_playlist.Size(); i++)
    m_playlist[i]->Select(false);

  // bind them to the list
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_PLAYLIST, 0, 0, &m_playlist);
  OnMessage(msg);

  // indicate how many songs we have
  CStdString items;
  items.Format("%i %s", m_playlist.Size(), g_localizeStrings.Get(134).c_str()); // "123 Songs"
  SET_CONTROL_LABEL(CONTROL_LABEL_PLAYLIST, items);

  m_playlistThumbLoader.Load(m_playlist);
}

int CGUIWindowMusicPlaylistEditor::GetCurrentPlaylistItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PLAYLIST);
  OnMessage(msg);
  int item = msg.GetParam1();
  if (item > m_playlist.Size())
    return -1;
  return item;
}

void CGUIWindowMusicPlaylistEditor::OnDeletePlaylistItem(int item)
{
  if (item < 0) return;
  m_playlist.Remove(item);
  UpdatePlaylist();
  // select the next item
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_PLAYLIST, item);
  OnMessage(msg);
}

void CGUIWindowMusicPlaylistEditor::OnMovePlaylistItem(int item, int direction)
{
  if (item < 0) return;
  if (item + direction >= m_playlist.Size() || item + direction < 0)
    return;
  m_playlist.Swap(item, item + direction);
  UpdatePlaylist();
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_PLAYLIST, item + direction);
  OnMessage(msg);
}

void CGUIWindowMusicPlaylistEditor::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItem *item = (itemNumber >= 0 && itemNumber < m_vecItems.Size()) ? m_vecItems[itemNumber] : NULL;

  if (GetFocusedControlID() == CONTROL_PLAYLIST)
  {
    int playlistItem = GetCurrentPlaylistItem();
    if (playlistItem > 0)
      buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_UP, 13332);
    if (playlistItem >= 0 && playlistItem < m_playlist.Size())
      buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_DOWN, 13333);
    if (playlistItem >= 0)
      buttons.Add(CONTEXT_BUTTON_DELETE, 1210);
  }
  else if (item && !item->IsParentFolder() && !m_vecItems.IsVirtualDirectoryRoot())
    buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 15019);

  if (m_playlist.Size())
  {
    buttons.Add(CONTEXT_BUTTON_SAVE, 190);
    buttons.Add(CONTEXT_BUTTON_CLEAR, 192);
  }
  buttons.Add(CONTEXT_BUTTON_LOAD, 21385);
}

bool CGUIWindowMusicPlaylistEditor::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  switch (button)
  {
  case CONTEXT_BUTTON_MOVE_ITEM_UP:
    OnMovePlaylistItem(GetCurrentPlaylistItem(), -1);
    return true;

  case CONTEXT_BUTTON_MOVE_ITEM_DOWN:
    OnMovePlaylistItem(GetCurrentPlaylistItem(), 1);
    return true;

  case CONTEXT_BUTTON_SAVE:
    OnSavePlaylist();
    return true;

  case CONTEXT_BUTTON_CLEAR:
    ClearPlaylist();
    return true;

  case CONTEXT_BUTTON_LOAD:
    OnLoadPlaylist();
    return true;
  case CONTEXT_BUTTON_DELETE:
    OnDeletePlaylistItem(GetCurrentPlaylistItem());
    return true;
  }
  return CGUIWindowMusicBase::OnContextButton(itemNumber, button);
}

void CGUIWindowMusicPlaylistEditor::OnLoadPlaylist()
{
  // prompt user for file to load
  CStdString playlist;
  VECSHARES shares;
  m_rootDir.GetShares(shares);
  // add the playlist share
  CShare share;
  share.strName = g_localizeStrings.Get(20011);
  share.strPath = "special://musicplaylists/";
  shares.push_back(share);
  if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".m3u|.pls|.b4s|.wpl", g_localizeStrings.Get(656), playlist))
    LoadPlaylist(playlist);
}

void CGUIWindowMusicPlaylistEditor::LoadPlaylist(const CStdString &playlist)
{
  DIRECTORY::CPlaylistFileDirectory dir;
  CFileItemList items;
  if (dir.GetDirectory(playlist, items))
  {
    ClearPlaylist();
    AppendToPlaylist(items);
    m_strLoadedPlaylist = playlist;
  }
}

void CGUIWindowMusicPlaylistEditor::OnSavePlaylist()
{
  // saves playlist to the playlist folder
  CStdString name = CUtil::GetFileName(m_strLoadedPlaylist);
  CStdString strExt = CUtil::GetExtension(name);
  name = name.Mid(0,name.size()-strExt.size());
  if (CGUIDialogKeyboard::ShowAndGetInput(name, g_localizeStrings.Get(16012), false))
  { // save playlist as an .m3u
    PLAYLIST::CPlayListM3U playlist;
    playlist.Add(m_playlist);
    CStdString path, strBase;
    CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "music", strBase);
    CUtil::AddFileToFolder(strBase, name + ".m3u", path);
    playlist.Save(path);
    m_strLoadedPlaylist = name;
  }
}

// NOTE: Destroys it's input argument
void CGUIWindowMusicPlaylistEditor::AppendToPlaylist(CFileItemList &newItems)
{
  OnRetrieveMusicInfo(newItems);
  FormatItemLabels(newItems, LABEL_MASKS(g_guiSettings.GetString("musicfiles.trackformat"), g_guiSettings.GetString("musicfiles.trackformatright"), "%L", ""));
  m_playlist.AppendPointer(newItems);
  newItems.ClearKeepPointer();
  UpdatePlaylist();
}