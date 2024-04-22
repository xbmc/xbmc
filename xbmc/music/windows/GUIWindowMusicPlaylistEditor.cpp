/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowMusicPlaylistEditor.h"

#include "Autorun.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/PlaylistFileDirectory.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "music/MusicUtils.h"
#include "playlists/PlayListM3U.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#define CONTROL_LABELFILES        12

#define CONTROL_LOAD_PLAYLIST      6
#define CONTROL_SAVE_PLAYLIST      7
#define CONTROL_CLEAR_PLAYLIST     8

#define CONTROL_LIST              50
#define CONTROL_PLAYLIST         100
#define CONTROL_LABEL_PLAYLIST   101

CGUIWindowMusicPlaylistEditor::CGUIWindowMusicPlaylistEditor(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_PLAYLIST_EDITOR, "MyMusicPlaylistEditor.xml")
{
  m_playlistThumbLoader.SetObserver(this);
  m_playlist = new CFileItemList;
}

CGUIWindowMusicPlaylistEditor::~CGUIWindowMusicPlaylistEditor(void)
{
  delete m_playlist;
}

bool CGUIWindowMusicPlaylistEditor::OnBack(int actionID)
{
  if (actionID == ACTION_NAV_BACK && !m_viewControl.HasControl(GetFocusedControlID()))
    return CGUIWindow::OnBack(actionID); // base class goes up a folder, but none to go up
  return CGUIWindowMusicBase::OnBack(actionID);
}

bool CGUIWindowMusicPlaylistEditor::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_CONTEXT_MENU)
  {
    int iControl = GetFocusedControlID();
    if (iControl == CONTROL_PLAYLIST)
    {
      OnPlaylistContext();
      return true;
    }
    else if (iControl == CONTROL_LIST)
    {
      OnSourcesContext();
      return true;
    }
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowMusicPlaylistEditor::OnClick(int iItem, const std::string& player /* = "" */)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return false;
  CFileItemPtr item = m_vecItems->Get(iItem);

  // Expand .m3u files in sources list when clicked on regardless of <playlistasfolders>
  if (item->IsFileFolder(EFILEFOLDER_MASK_ONBROWSE))
    return Update(item->GetPath());
  // Avoid playback (default click behaviour) of media files
  if (!item->m_bIsFolder)
    return false;

  return CGUIWindowMusicBase::OnClick(iItem, player);
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
    return true;

  case GUI_MSG_WINDOW_INIT:
    {
      if (m_vecItems->GetPath() == "?")
        m_vecItems->SetPath("");
      CGUIWindowMusicBase::OnMessage(message);

      if (message.GetNumStringParams())
        LoadPlaylist(message.GetStringParam());

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
        if (action == ACTION_CONTEXT_MENU || action == ACTION_MOUSE_RIGHT_CLICK)
          OnPlaylistContext();
        else if (action == ACTION_QUEUE_ITEM || action == ACTION_DELETE_ITEM ||
                 action == ACTION_MOUSE_MIDDLE_CLICK)
          OnDeletePlaylistItem(item);
        else if (action == ACTION_MOVE_ITEM_UP)
          OnMovePlaylistItem(item, -1);
        else if (action == ACTION_MOVE_ITEM_DOWN)
          OnMovePlaylistItem(item, 1);
        return true;
      }
      else if (control == CONTROL_LIST)
      {
        int action = message.GetParam1();
        if (action == ACTION_CONTEXT_MENU || action == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnSourcesContext();
          return true;
        }
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

bool CGUIWindowMusicPlaylistEditor::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  items.Clear();
  if (strDirectory.empty())
  { // root listing - list files:// and musicdb://
    CFileItemPtr files(new CFileItem("sources://music/", true));
    files->SetLabel(g_localizeStrings.Get(744));
    files->SetLabelPreformatted(true);
    files->m_bIsShareOrDrive = true;
    items.Add(files);

    CFileItemPtr mdb(new CFileItem("library://music/", true));
    mdb->SetLabel(g_localizeStrings.Get(14022));
    mdb->SetLabelPreformatted(true);
    mdb->m_bIsShareOrDrive = true;
    items.SetPath("");
    items.Add(mdb);

    CFileItemPtr vdb(new CFileItem("videodb://musicvideos/", true));
    vdb->SetLabel(g_localizeStrings.Get(20389));
    vdb->SetLabelPreformatted(true);
    vdb->m_bIsShareOrDrive = true;
    items.SetPath("");
    items.Add(vdb);

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
  CGUIWindowMusicBase::OnPrepareFileItems(items);

  RetrieveMusicInfo();
}

void CGUIWindowMusicPlaylistEditor::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  // Update object count label
  std::string items = StringUtils::Format("{} {}", m_vecItems->GetObjectCount(),
                                          g_localizeStrings.Get(127)); // " 14 Objects"
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);
}

void CGUIWindowMusicPlaylistEditor::DeleteRemoveableMediaDirectoryCache()
{
  CUtil::DeleteDirectoryCache("r-");
}

void CGUIWindowMusicPlaylistEditor::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // we're at the root source listing
  if (m_vecItems->IsVirtualDirectoryRoot() && !m_vecItems->Get(iItem)->IsDVD())
    return;

#ifdef HAS_OPTICAL_DRIVE
  if (m_vecItems->Get(iItem)->IsDVD())
    MEDIA_DETECT::CAutorun::PlayDiscAskResume(m_vecItems->Get(iItem)->GetPath());
  else
#endif
    CGUIWindowMusicBase::PlayItem(iItem);
}

void CGUIWindowMusicPlaylistEditor::OnQueueItem(int iItem, bool)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return;

  // add this item to our playlist.  We make a new copy here as we may be rendering them side by side,
  // and thus want a different layout for each item
  CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));
  CFileItemList newItems;
  MUSIC_UTILS::GetItemsForPlayList(item, newItems);
  AppendToPlaylist(newItems);
}

bool CGUIWindowMusicPlaylistEditor::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  m_vecItems->SetContent("files");
  m_thumbLoader.Load(*m_vecItems);

  // update our playlist control
  UpdatePlaylist();
  return true;
}

void CGUIWindowMusicPlaylistEditor::ClearPlaylist()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PLAYLIST);
  OnMessage(msg);

  m_playlist->Clear();
}

void CGUIWindowMusicPlaylistEditor::UpdatePlaylist()
{
  if (m_playlistThumbLoader.IsLoading())
    m_playlistThumbLoader.StopThread();

  // deselect all items
  for (int i = 0; i < m_playlist->Size(); i++)
    m_playlist->Get(i)->Select(false);

  // bind them to the list
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_PLAYLIST, 0, 0, m_playlist);
  OnMessage(msg);

  // indicate how many songs we have
  std::string items = StringUtils::Format("{} {}", m_playlist->Size(),
                                          g_localizeStrings.Get(134)); // "123 Songs"
  SET_CONTROL_LABEL(CONTROL_LABEL_PLAYLIST, items);

  m_playlistThumbLoader.Load(*m_playlist);
}

int CGUIWindowMusicPlaylistEditor::GetCurrentPlaylistItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PLAYLIST);
  OnMessage(msg);
  int item = msg.GetParam1();
  if (item > m_playlist->Size())
    return -1;
  return item;
}

void CGUIWindowMusicPlaylistEditor::OnDeletePlaylistItem(int item)
{
  if (item < 0) return;
  m_playlist->Remove(item);
  UpdatePlaylist();
  // select the next item
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_PLAYLIST, item);
  OnMessage(msg);
}

void CGUIWindowMusicPlaylistEditor::OnMovePlaylistItem(int item, int direction)
{
  if (item < 0) return;
  if (item + direction >= m_playlist->Size() || item + direction < 0)
    return;
  m_playlist->Swap(item, item + direction);
  UpdatePlaylist();
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_PLAYLIST, item + direction);
  OnMessage(msg);
}

void CGUIWindowMusicPlaylistEditor::OnLoadPlaylist()
{
  // Prompt user for file to load from music playlists folder
  std::string playlist;
  if (CGUIDialogFileBrowser::ShowAndGetFile("special://musicplaylists/",
                                            ".m3u|.m3u8|.pls|.b4s|.wpl|.xspf", g_localizeStrings.Get(656),
                                            playlist))
    LoadPlaylist(playlist);
}

void CGUIWindowMusicPlaylistEditor::LoadPlaylist(const std::string &playlist)
{
  const CURL pathToUrl(playlist);
  if (pathToUrl.IsProtocol("newplaylist"))
  {
    ClearPlaylist();
    m_strLoadedPlaylist.clear();
    return;
  }

  XFILE::CPlaylistFileDirectory dir;
  CFileItemList items;
  if (dir.GetDirectory(pathToUrl, items))
  {
    ClearPlaylist();
    AppendToPlaylist(items);
    m_strLoadedPlaylist = playlist;
  }
}

void CGUIWindowMusicPlaylistEditor::OnSavePlaylist()
{
  // saves playlist to the playlist folder
  std::string name = URIUtils::GetFileName(m_strLoadedPlaylist);
  std::string extension = URIUtils::GetExtension(m_strLoadedPlaylist);
  if (extension.empty())
    extension = ".m3u8";
  else
    URIUtils::RemoveExtension(name);

  if (CGUIKeyboardFactory::ShowAndGetInput(name, CVariant{g_localizeStrings.Get(16012)}, false))
  {
    PLAYLIST::CPlayListM3U playlist;
    playlist.Add(*m_playlist);
    std::string path = URIUtils::AddFileToFolder(
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH),
      "music",
      name + extension);

    playlist.Save(path);
    m_strLoadedPlaylist = name;
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                          g_localizeStrings.Get(559), // "Playlist"
                                          g_localizeStrings.Get(35259)); // "Saved"
  }
}

void CGUIWindowMusicPlaylistEditor::AppendToPlaylist(CFileItemList &newItems)
{
  OnRetrieveMusicInfo(newItems);
  FormatItemLabels(newItems, LABEL_MASKS(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT), "%D", "%L", ""));
  m_playlist->Append(newItems);
  UpdatePlaylist();
}

void CGUIWindowMusicPlaylistEditor::OnSourcesContext()
{
  static constexpr int CONTEXT_BUTTON_QUEUE_ITEM = 0;
  static constexpr int CONTEXT_BUTTON_BROWSE_INTO = 1;

  CFileItemPtr item = GetCurrentListItem();
  CContextButtons buttons;
  if (item->IsFileFolder(EFILEFOLDER_MASK_ONBROWSE))
    buttons.Add(CONTEXT_BUTTON_BROWSE_INTO, 37015); //Browse into
  if (item && !item->IsParentFolder() && !m_vecItems->IsVirtualDirectoryRoot())
    buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 15019); // Add (to playlist)

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(buttons);
  if (btnid == CONTEXT_BUTTON_QUEUE_ITEM)
    OnQueueItem(m_viewControl.GetSelectedItem(), false);
  else if (btnid == CONTEXT_BUTTON_BROWSE_INTO)
    Update(item->GetPath());
}

void CGUIWindowMusicPlaylistEditor::OnPlaylistContext()
{
  int item = GetCurrentPlaylistItem();
  CContextButtons buttons;
  if (item > 0)
    buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_UP, 13332);
  if (item >= 0 && item < m_playlist->Size() - 1)
    buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_DOWN, 13333);
  if (item >= 0)
      buttons.Add(CONTEXT_BUTTON_DELETE, 1210);

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(buttons);
  if (btnid == CONTEXT_BUTTON_MOVE_ITEM_UP)
    OnMovePlaylistItem(item, -1);
  else if (btnid == CONTEXT_BUTTON_MOVE_ITEM_DOWN)
    OnMovePlaylistItem(item, 1);
  else if (btnid == CONTEXT_BUTTON_DELETE)
    OnDeletePlaylistItem(item);
}
