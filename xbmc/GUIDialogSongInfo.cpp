/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogSongInfo.h"
#include "Util.h"
#include "Picture.h"
#include "GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "MusicDatabase.h"
#include "GUIWindowMusicBase.h"
#include "MusicInfoTag.h"
#include "GUIWindowManager.h"
#include "FileSystem/File.h"
#include "FileSystem/FileCurl.h"
#include "FileItem.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "LocalizeStrings.h"
#include "TextureCache.h"
#include "Album.h"

using namespace XFILE;

#define CONTROL_OK        10
#define CONTROL_CANCEL    11
#define CONTROL_ALBUMINFO 12
#define CONTROL_GETTHUMB  13

CGUIDialogSongInfo::CGUIDialogSongInfo(void)
    : CGUIDialog(WINDOW_DIALOG_SONG_INFO, "DialogSongInfo.xml")
    , m_song(new CFileItem)
{
  m_cancelled = false;
  m_needsUpdate = false;
  m_startRating = -1;
}

CGUIDialogSongInfo::~CGUIDialogSongInfo(void)
{
}

bool CGUIDialogSongInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (!m_cancelled && m_startRating != m_song->GetMusicInfoTag()->GetRating())
      {
        CMusicDatabase db;
        if (db.Open())      // OpenForWrite() ?
        {
          db.SetSongRating(m_song->m_strPath, m_song->GetMusicInfoTag()->GetRating());
          db.Close();
        }
        m_needsUpdate = true;
      }
      else
      { // cancelled - reset the song rating
        SetRating(m_startRating);
        m_needsUpdate = false;
      }
      break;
    }
  case GUI_MSG_WINDOW_INIT:
    m_cancelled = false;
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_CANCEL)
      {
        m_cancelled = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_OK)
      {
        m_cancelled = false;
        Close();
        return true;
      }
      else if (iControl == CONTROL_ALBUMINFO)
      {
        CGUIWindowMusicBase *window = (CGUIWindowMusicBase *)g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
        if (window)
        {
          CFileItem item(*m_song);
          item.m_strPath.Format("musicdb://3/%li",m_albumId);
          item.m_bIsFolder = true;
          window->OnInfo(&item, true);
        }
        return true;
      }
      else if (iControl == CONTROL_GETTHUMB)
      {
        OnGetThumb();
        return true;
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogSongInfo::OnAction(const CAction &action)
{
  char rating = m_song->GetMusicInfoTag()->GetRating();
  if (action.GetID() == ACTION_INCREASE_RATING)
  {
    if (rating < '5')
      SetRating(rating + 1);
    return true;
  }
  else if (action.GetID() == ACTION_DECREASE_RATING)
  {
    if (rating > '0')
      SetRating(rating - 1);
    return true;
  }
  else if (action.GetID() == ACTION_PREVIOUS_MENU)
    m_cancelled = true;
  return CGUIDialog::OnAction(action);
}

void CGUIDialogSongInfo::OnInitWindow()
{
  CMusicDatabase db;
  db.Open();

  // no known db info - check if parent dir is an album
  if (m_song->GetMusicInfoTag()->GetDatabaseId() == -1)
  {
    CStdString path;
    CUtil::GetDirectory(m_song->m_strPath,path);
    m_albumId = db.GetAlbumIdByPath(path);
  }
  else
  {
    CAlbum album;
    db.GetAlbumFromSong(m_song->GetMusicInfoTag()->GetDatabaseId(),album);
    m_albumId = album.idAlbum;
  }
  CONTROL_ENABLE_ON_CONDITION(CONTROL_ALBUMINFO, m_albumId > -1);

  CGUIDialog::OnInitWindow();
}

void CGUIDialogSongInfo::SetRating(char rating)
{
  if (rating < '0') rating = '0';
  if (rating > '5') rating = '5';
  m_song->GetMusicInfoTag()->SetRating(rating);
  // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_song);
  g_windowManager.SendMessage(msg);
}

void CGUIDialogSongInfo::SetSong(CFileItem *item)
{
  *m_song = *item;
  m_song->LoadMusicTag();
  m_startRating = m_song->GetMusicInfoTag()->GetRating();
  MUSIC_INFO::CMusicInfoLoader::LoadAdditionalTagInfo(m_song.get());
  // set artist thumb as well
  if (m_song->HasMusicInfoTag())
  {
    CFileItem artist(m_song->GetMusicInfoTag()->GetArtist());
    artist.SetCachedArtistThumb();
    if (CFile::Exists(artist.GetThumbnailImage()))
      m_song->SetProperty("artistthumb", artist.GetThumbnailImage());
  }
  m_needsUpdate = false;
}

CFileItemPtr CGUIDialogSongInfo::GetCurrentListItem(int offset)
{
  return m_song;
}

bool CGUIDialogSongInfo::DownloadThumbnail(const CStdString &thumbFile)
{
  // TODO: Obtain the source...
  CStdString source;
  CFileCurl http;
  http.Download(source, thumbFile);
  return true;
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  AllMusic.com thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)

// TODO: Currently no support for "embedded thumb" as there is no easy way to grab it
//       without sending a file that has this as it's album to this class
void CGUIDialogSongInfo::OnGetThumb()
{
  CFileItemList items;


  // Grab the thumbnail from the web
  CStdString thumbFromWeb;
  /*
  CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "allmusicThumb.jpg", thumbFromWeb);
  if (DownloadThumbnail(thumbFromWeb))
  {
    CFileItemPtr item(new CFileItem("thumb://allmusic.com", false));
    item->SetThumbnailImage(thumbFromWeb);
    item->SetLabel(g_localizeStrings.Get(20055));
    items.Add(item);
  }*/

  // Current thumb
  if (CFile::Exists(m_song->GetThumbnailImage()))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetThumbnailImage(m_song->GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // local thumb
  CStdString cachedLocalThumb;
  CStdString localThumb(m_song->GetUserMusicThumb(true));
  if (m_song->IsMusicDb())
  {
    CFileItem item(m_song->GetMusicInfoTag()->GetURL(), false);
    localThumb = item.GetUserMusicThumb(true);
  }
  if (CFile::Exists(localThumb))
  {
    CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "localthumb.jpg", cachedLocalThumb);
    if (CPicture::CreateThumbnail(localThumb, cachedLocalThumb))
    {
      CFileItemPtr item(new CFileItem("thumb://Local", false));
      item->SetThumbnailImage(cachedLocalThumb);
      item->SetLabel(g_localizeStrings.Get(20017));
      items.Add(item);
    }
  }
  else
  { // no local thumb exists, so we are just using the allmusic.com thumb or cached thumb
    // which is probably the allmusic.com thumb.  These could be wrong, so allow the user
    // to delete the incorrect thumb
    CFileItemPtr item(new CFileItem("thumb://None", false));
    item->SetThumbnailImage("DefaultAlbumCover.png");
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);
  }

  CStdString result;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, g_settings.m_musicSources, g_localizeStrings.Get(1030), result))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail

  CStdString cachedThumb(CUtil::GetCachedAlbumThumb(m_song->GetMusicInfoTag()->GetAlbum(), m_song->GetMusicInfoTag()->GetArtist()));

  CTextureCache::Get().ClearCachedImage(cachedThumb, true);
  if (result == "thumb://None")
  {
    CFile::Delete(cachedThumb);
    cachedThumb = "";
  }
  else if (result == "thumb://allmusic.com")
    CFile::Cache(thumbFromWeb, cachedThumb);
  else if (result == "thumb://Local")
    CFile::Cache(cachedLocalThumb, cachedThumb);
  else if (CFile::Exists(result))
    CPicture::CreateThumbnail(result, cachedThumb);

  m_song->SetThumbnailImage(cachedThumb);

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);

//  m_hasUpdatedThumb = true;
}
