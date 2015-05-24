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

#include "GUIDialogMusicInfo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIImage.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "storage/MediaManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "TextureCache.h"
#include "music/MusicThumbLoader.h"
#include "filesystem/Directory.h"

using namespace std;
using namespace XFILE;

#define CONTROL_IMAGE            3
#define CONTROL_TEXTAREA         4

#define CONTROL_BTN_TRACKS       5
#define CONTROL_BTN_REFRESH      6
#define CONTROL_BTN_GET_THUMB   10
#define  CONTROL_BTN_GET_FANART 12

#define CONTROL_LIST            50

CGUIDialogMusicInfo::CGUIDialogMusicInfo(void)
    : CGUIDialog(WINDOW_DIALOG_MUSIC_INFO, "DialogAlbumInfo.xml")
    , m_albumItem(new CFileItem)
{
  m_bRefresh = false;
  m_albumSongs = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogMusicInfo::~CGUIDialogMusicInfo(void)
{
  delete m_albumSongs;
}

bool CGUIDialogMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(message);
      m_albumSongs->Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bViewReview = true;
      m_bRefresh = false;
      Update();
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_REFRESH)
      {
        m_bRefresh = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetThumb();
      }
      else if (iControl == CONTROL_BTN_TRACKS)
      {
        m_bViewReview = !m_bViewReview;
        Update();
      }
      else if (iControl == CONTROL_LIST)
      {
        int iAction = message.GetParam1();
        if (m_bArtistInfo && (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction))
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          g_windowManager.SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= (int)m_albumSongs->Size())
            break;
          CFileItemPtr item = m_albumSongs->Get(iItem);
          OnSearch(item.get());
          return true;
        }
      }
      else if (iControl == CONTROL_BTN_GET_FANART)
      {
        OnGetFanart();
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogMusicInfo::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogMusicInfo::SetAlbum(const CAlbum& album, const std::string &path)
{
  m_album = album;
  SetSongs(m_album.infoSongs);
  *m_albumItem = CFileItem(path, true);
  m_albumItem->GetMusicInfoTag()->SetAlbum(m_album.strAlbum);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(StringUtils::Join(m_album.artist, g_advancedSettings.m_musicItemSeparator));
  m_albumItem->GetMusicInfoTag()->SetArtist(m_album.artist);
  m_albumItem->GetMusicInfoTag()->SetYear(m_album.iYear);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetRating('0' + m_album.iRating);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_album.genre);
  m_albumItem->GetMusicInfoTag()->SetDatabaseId(m_album.idAlbum, MediaTypeAlbum);
  CMusicDatabase::SetPropertiesFromAlbum(*m_albumItem,m_album);

  CMusicThumbLoader loader;
  loader.LoadItem(m_albumItem.get());

  // set the artist thumb, fanart
  if (!m_album.artist.empty())
  {
    CMusicDatabase db;
    db.Open();
    map<string, string> artwork;
    if (db.GetArtistArtForItem(m_album.idAlbum, MediaTypeAlbum, artwork))
    {
      if (artwork.find("thumb") != artwork.end())
        m_albumItem->SetProperty("artistthumb", artwork["thumb"]);
      if (artwork.find("fanart") != artwork.end())
        m_albumItem->SetArt("fanart",artwork["fanart"]);
    }
  }
  m_hasUpdatedThumb = false;
  m_bArtistInfo = false;
  m_albumSongs->SetContent("albums");
}

void CGUIDialogMusicInfo::SetArtist(const CArtist& artist, const std::string &path)
{
  m_artist = artist;
  SetDiscography();
  *m_albumItem = CFileItem(path, true);
  m_albumItem->SetLabel(artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_artist.genre);
  m_albumItem->GetMusicInfoTag()->SetDatabaseId(m_artist.idArtist, MediaTypeArtist);
  CMusicDatabase::SetPropertiesFromArtist(*m_albumItem,m_artist);

  CMusicThumbLoader loader;
  loader.LoadItem(m_albumItem.get());

  m_hasUpdatedThumb = false;
  m_bArtistInfo = true;
  m_albumSongs->SetContent("artists");
}

void CGUIDialogMusicInfo::SetSongs(const VECSONGS &songs)
{
  m_albumSongs->Clear();
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    const CSong& song = songs[i];
    CFileItemPtr item(new CFileItem(song));
    m_albumSongs->Add(item);
  }
}

void CGUIDialogMusicInfo::SetDiscography()
{
  m_albumSongs->Clear();
  CMusicDatabase database;
  database.Open();

  vector<int> albumsByArtist;
  database.GetAlbumsByArtist(m_artist.idArtist, true, albumsByArtist);

  for (unsigned int i=0;i<m_artist.discography.size();++i)
  {
    CFileItemPtr item(new CFileItem(m_artist.discography[i].first));
    item->SetLabel2(m_artist.discography[i].second);

    int idAlbum = -1;
    for (vector<int>::const_iterator album = albumsByArtist.begin(); album != albumsByArtist.end(); ++album)
    {
      if (StringUtils::EqualsNoCase(database.GetAlbumById(*album), item->GetLabel()))
      {
        idAlbum = *album;
        item->GetMusicInfoTag()->SetDatabaseId(idAlbum, "album");
        break;
      }
    }

    if (idAlbum != -1) // we need this slight stupidity to get correct case for the album name
      item->SetArt("thumb", database.GetArtForItem(idAlbum, MediaTypeAlbum, "thumb"));
    else
      item->SetArt("thumb", "DefaultAlbumCover.png");

    m_albumSongs->Add(item);
  }
}

void CGUIDialogMusicInfo::Update()
{
  if (m_bArtistInfo)
  {
    CONTROL_ENABLE(CONTROL_BTN_GET_FANART);

    SetLabel(CONTROL_TEXTAREA, m_artist.strBiography);
    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
    OnMessage(message);

    if (GetControl(CONTROL_BTN_TRACKS)) // if no CONTROL_BTN_TRACKS found - allow skinner full visibility control over CONTROL_TEXTAREA and CONTROL_LIST
    {
      if (m_bViewReview)
      {
        SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
        SET_CONTROL_HIDDEN(CONTROL_LIST);
        SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 21888);
      }
      else
      {
        SET_CONTROL_VISIBLE(CONTROL_LIST);
        SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
        SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 21887);
      }
    }
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTN_GET_FANART);

    SetLabel(CONTROL_TEXTAREA, m_album.strReview);
    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
    OnMessage(message);

    if (GetControl(CONTROL_BTN_TRACKS)) // if no CONTROL_BTN_TRACKS found - allow skinner full visibility control over CONTROL_TEXTAREA and CONTROL_LIST
    {
      if (m_bViewReview)
      {
        SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
        SET_CONTROL_HIDDEN(CONTROL_LIST);
        SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 182);
      }
      else
      {
        SET_CONTROL_VISIBLE(CONTROL_LIST);
        SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
        SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 183);
      }
    }
  }
  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_albumItem->GetArt("thumb"));
  }

  // disable the GetThumb button if the user isn't allowed it
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);
}

void CGUIDialogMusicInfo::SetLabel(int iControl, const std::string& strLabel)
{
  if (strLabel.empty())
  {
    SET_CONTROL_LABEL(iControl, (iControl == CONTROL_TEXTAREA) ? (m_bArtistInfo?547:414) : 416);
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

bool CGUIDialogMusicInfo::NeedRefresh() const
{
  return m_bRefresh;
}

void CGUIDialogMusicInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  AllMusic.com thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)

// TODO: Currently no support for "embedded thumb" as there is no easy way to grab it
//       without sending a file that has this as it's album to this class
void CGUIDialogMusicInfo::OnGetThumb()
{
  CFileItemList items;

  // Current thumb
  if (CFile::Exists(m_albumItem->GetArt("thumb")))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetArt("thumb", m_albumItem->GetArt("thumb"));
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnail(s) from the web
  vector<std::string> thumbs;
  if (m_bArtistInfo)
    m_artist.thumbURL.GetThumbURLs(thumbs);
  else
    m_album.thumbURL.GetThumbURLs(thumbs);

  for (unsigned int i = 0; i < thumbs.size(); ++i)
  {
    std::string strItemPath;
    strItemPath = StringUtils::Format("thumb://Remote%i", i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetArt("thumb", thumbs[i]);
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20015));
    
    // TODO: Do we need to clear the cached image?
    //    CTextureCache::Get().ClearCachedImage(thumb);
    items.Add(item);
  }

  // local thumb
  std::string localThumb;
  if (m_bArtistInfo)
  {
    CMusicDatabase database;
    database.Open();
    std::string strArtistPath;
    if (database.GetArtistPath(m_artist.idArtist,strArtistPath))
      localThumb = URIUtils::AddFileToFolder(strArtistPath, "folder.jpg");
  }
  else
    localThumb = m_albumItem->GetUserMusicThumb();
  if (CFile::Exists(localThumb))
  {
    CFileItemPtr item(new CFileItem("thumb://Local", false));
    item->SetArt("thumb", localThumb);
    item->SetLabel(g_localizeStrings.Get(20017));
    items.Add(item);
  }
  else
  {
    CFileItemPtr item(new CFileItem("thumb://None", false));
    if (m_bArtistInfo)
      item->SetIconImage("DefaultArtist.png");
    else
      item->SetIconImage("DefaultAlbumCover.png");
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);
  }

  std::string result;
  bool flip=false;
  VECSOURCES sources(*CMediaSourceSettings::Get().GetSources("music"));
  AddItemPathToFileBrowserSources(sources, *m_albumItem);
  g_mediaManager.GetLocalDrives(sources);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(1030), result, &flip))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  std::string newThumb;
  if (StringUtils::StartsWith(result, "thumb://Remote"))
  {
    int number = atoi(result.substr(14).c_str());
    newThumb = thumbs[number];
  }
  else if (result == "thumb://Local")
    newThumb = localThumb;
  else if (CFile::Exists(result))
    newThumb = result;
  else // none
    newThumb = "-"; // force local thumbs to be ignored

  // update thumb in the database
  CMusicDatabase db;
  if (db.Open())
  {
    db.SetArtForItem(m_albumItem->GetMusicInfoTag()->GetDatabaseId(), m_albumItem->GetMusicInfoTag()->GetType(), "thumb", newThumb);
    db.Close();
  }

  m_albumItem->SetArt("thumb", newThumb);
  m_hasUpdatedThumb = true;

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}


// Allow user to select a Fanart
void CGUIDialogMusicInfo::OnGetFanart()
{
  CFileItemList items;

  if (m_albumItem->HasArt("fanart"))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetArt("thumb", m_albumItem->GetArt("fanart"));
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // Grab the thumbnails from the web
  for (unsigned int i = 0; i < m_artist.fanart.GetNumFanarts(); i++)
  {
    std::string strItemPath = StringUtils::Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    std::string thumb = m_artist.fanart.GetPreviewURL(i);
    item->SetArt("thumb", CTextureUtils::GetWrappedThumbURL(thumb));
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20441));

    // TODO: Do we need to clear the cached image?
    //    CTextureCache::Get().ClearCachedImage(thumb);
    items.Add(item);
  }

  // Grab a local thumb
  CMusicDatabase database;
  database.Open();
  std::string strArtistPath;
  database.GetArtistPath(m_artist.idArtist,strArtistPath);
  CFileItem item(strArtistPath,true);
  std::string strLocal = item.GetLocalFanart();
  if (!strLocal.empty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetArt("thumb", strLocal);
    itemLocal->SetLabel(g_localizeStrings.Get(20438));

    // TODO: Do we need to clear the cached image?
    CTextureCache::Get().ClearCachedImage(strLocal);
    items.Add(itemLocal);
  }
  else
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetIconImage("DefaultArtist.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }

  std::string result;
  VECSOURCES sources = *CMediaSourceSettings::Get().GetSources("music");
  g_mediaManager.GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445))
    return;   // user cancelled

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  if (StringUtils::EqualsNoCase(result, "fanart://Current"))
   return;

  if (StringUtils::EqualsNoCase(result, "fanart://Local"))
    result = strLocal;

  if (StringUtils::StartsWith(result, "fanart://Remote"))
  {
    int iFanart = atoi(result.substr(15).c_str());
    m_artist.fanart.SetPrimaryFanart(iFanart);
    result = m_artist.fanart.GetImageURL();
  }
  else if (StringUtils::EqualsNoCase(result, "fanart://None") || !CFile::Exists(result))
    result.clear();

  if (flip && !result.empty())
    result = CTextureUtils::GetWrappedImageURL(result, "", "flipped");

  // update thumb in the database
  CMusicDatabase db;
  if (db.Open())
  {
    db.SetArtForItem(m_albumItem->GetMusicInfoTag()->GetDatabaseId(), m_albumItem->GetMusicInfoTag()->GetType(), "fanart", result);
    db.Close();
  }

  m_albumItem->SetArt("fanart", result);
  m_hasUpdatedThumb = true;
  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}

void CGUIDialogMusicInfo::OnSearch(const CFileItem* pItem)
{
  CMusicDatabase database;
  database.Open();
  if (pItem->HasMusicInfoTag() &&
      pItem->GetMusicInfoTag()->GetDatabaseId() > 0)
  {
    CAlbum album;
    if (database.GetAlbum(pItem->GetMusicInfoTag()->GetDatabaseId(), album))
    {
      std::string strPath;
      database.GetAlbumPath(pItem->GetMusicInfoTag()->GetDatabaseId(), strPath);
      SetAlbum(album,strPath);
      Update();
    }
  }
}

CFileItemPtr CGUIDialogMusicInfo::GetCurrentListItem(int offset)
{
  return m_albumItem;
}

void CGUIDialogMusicInfo::AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item)
{
  std::string itemDir;

  if (item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == MediaTypeSong)
    itemDir = URIUtils::GetParentPath(item.GetMusicInfoTag()->GetURL());
  else
    itemDir = item.GetPath();

  if (!itemDir.empty() && CDirectory::Exists(itemDir))
  {
    CMediaSource itemSource;
    itemSource.strName = g_localizeStrings.Get(36041);
    itemSource.strPath = itemDir;
    sources.push_back(itemSource);
  }
}
