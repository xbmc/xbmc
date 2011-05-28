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

#include "GUIDialogMusicInfo.h"
#include "guilib/GUIWindowManager.h"
#include "Util.h"
#include "guilib/GUIImage.h"
#include "pictures/Picture.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "music/MusicDatabase.h"
#include "music/LastFmManager.h"
#include "music/tags/MusicInfoTag.h"
#include "URL.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "storage/MediaManager.h"
#include "utils/AsyncFileCopy.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "TextureCache.h"

using namespace std;
using namespace XFILE;

#define CONTROL_IMAGE            3
#define CONTROL_TEXTAREA         4

#define CONTROL_BTN_TRACKS       5
#define CONTROL_BTN_REFRESH      6
#define CONTROL_BTN_GET_THUMB   10
#define CONTROL_BTN_LASTFM      11
#define  CONTROL_BTN_GET_FANART 12

#define CONTROL_LIST            50

CGUIDialogMusicInfo::CGUIDialogMusicInfo(void)
    : CGUIDialog(WINDOW_DIALOG_MUSIC_INFO, "DialogAlbumInfo.xml")
    , m_albumItem(new CFileItem)
{
  m_bRefresh = false;
  m_albumSongs = new CFileItemList;
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
      RefreshThumb();
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
      else if (iControl == CONTROL_BTN_LASTFM)
      {
        CStdString strArtist = m_album.strArtist;
        CURL::Encode(strArtist);
        CStdString strLink;
        strLink.Format("lastfm://artist/%s/similarartists", strArtist.c_str());
        CURL url(strLink);
        CLastFmManager::GetInstance()->ChangeStation(url);
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

void CGUIDialogMusicInfo::SetAlbum(const CAlbum& album, const CStdString &path)
{
  m_album = album;
  SetSongs(m_album.songs);
  *m_albumItem = CFileItem(path, true);
  m_albumItem->GetMusicInfoTag()->SetAlbum(m_album.strAlbum);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_album.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(m_album.strArtist);
  m_albumItem->GetMusicInfoTag()->SetYear(m_album.iYear);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetRating('0' + (m_album.iRating + 1) / 2);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_album.strGenre);
  CMusicDatabase::SetPropertiesFromAlbum(*m_albumItem,m_album);
  m_albumItem->SetMusicThumb();
  // set the artist thumb
  CFileItem artist(m_album.strArtist);
  artist.SetCachedArtistThumb();
  if (CFile::Exists(artist.GetThumbnailImage()))
    m_albumItem->SetProperty("artistthumb", artist.GetThumbnailImage());
  m_hasUpdatedThumb = false;
  m_bArtistInfo = false;
  m_albumSongs->SetContent("albums");
}

void CGUIDialogMusicInfo::SetArtist(const CArtist& artist, const CStdString &path)
{
  m_artist = artist;
  SetDiscography();
  *m_albumItem = CFileItem(path, true);
  m_albumItem->SetLabel(artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_artist.strGenre);
  CMusicDatabase::SetPropertiesFromArtist(*m_albumItem,m_artist);
  CStdString strFanart = m_albumItem->GetCachedFanart();
  if (CFile::Exists(strFanart))
    m_albumItem->SetProperty("fanart_image",strFanart);
  m_albumItem->SetCachedArtistThumb();
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

  for (unsigned int i=0;i<m_artist.discography.size();++i)
  {
    CFileItemPtr item(new CFileItem(m_artist.discography[i].first));
    item->SetLabel2(m_artist.discography[i].second);
    long idAlbum = database.GetAlbumByName(item->GetLabel(),m_artist.strArtist);
    CStdString strThumb;
    if (idAlbum != -1) // we need this slight stupidity to get correct case for the album name
      database.GetAlbumThumb(idAlbum,strThumb);

    if (!strThumb.IsEmpty() && CFile::Exists(strThumb))
      item->SetThumbnailImage(strThumb);
    else
      item->SetThumbnailImage("DefaultAlbumCover.png");

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
    pImageControl->SetFileName(m_albumItem->GetThumbnailImage());
  }

  // disable the GetThumb button if the user isn't allowed it
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);

  if (!m_album.strArtist.IsEmpty() && CLastFmManager::GetInstance()->IsLastFmEnabled())
  {
    SET_CONTROL_VISIBLE(CONTROL_BTN_LASTFM);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_BTN_LASTFM);
  }

}

void CGUIDialogMusicInfo::SetLabel(int iControl, const CStdString& strLabel)
{
  if (strLabel.IsEmpty())
  {
    SET_CONTROL_LABEL(iControl, (iControl == CONTROL_TEXTAREA) ? (m_bArtistInfo?547:414) : 416);
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

void CGUIDialogMusicInfo::RefreshThumb()
{
  CStdString thumbImage = m_albumItem->GetThumbnailImage();
  if (!m_albumItem->HasThumbnail())
  {
    if (m_bArtistInfo)
      thumbImage = m_albumItem->GetCachedArtistThumb();
    else
      thumbImage = CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist);

    if (!CFile::Exists(thumbImage))
    {
      DownloadThumbnail(thumbImage);
      m_hasUpdatedThumb = true;
    }
  }
  if (!CFile::Exists(thumbImage) )
    thumbImage.Empty();

  m_albumItem->SetThumbnailImage(thumbImage);
}

bool CGUIDialogMusicInfo::NeedRefresh() const
{
  return m_bRefresh;
}

bool CGUIDialogMusicInfo::DownloadThumbnail(const CStdString &thumbFile)
{
  const CScraperUrl &url = (m_bArtistInfo) ? m_artist.thumbURL : m_album.thumbURL;
  for (unsigned int i = 0; i < url.m_url.size(); ++i)
  {
    if (CScraperUrl::DownloadThumbnail(thumbFile, url.m_url[i]))
      return true;
  }
  return false;
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
  if (CFile::Exists(m_albumItem->GetThumbnailImage()))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetThumbnailImage(m_albumItem->GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnail(s) from the web
  vector<CStdString> thumbs;
  if (m_bArtistInfo)
    m_artist.thumbURL.GetThumbURLs(thumbs);
  else
    m_album.thumbURL.GetThumbURLs(thumbs);

  for (unsigned int i = 0; i < thumbs.size(); ++i)
  {
    CStdString strItemPath;
    strItemPath.Format("thumb://Remote%i", i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetThumbnailImage(thumbs[i]);
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20015));
    
    // TODO: Do we need to clear the cached image?
    //    CTextureCache::Get().ClearCachedImage(thumb);
    items.Add(item);
  }

  // local thumb
  CStdString localThumb;
  if (m_bArtistInfo)
  {
    CMusicDatabase database;
    database.Open();
    CStdString strArtistPath;
    if (database.GetArtistPath(m_artist.idArtist,strArtistPath))
      URIUtils::AddFileToFolder(strArtistPath,"folder.jpg",localThumb);
  }
  else
    localThumb = m_albumItem->GetUserMusicThumb();
  if (CFile::Exists(localThumb))
  {
    CFileItemPtr item(new CFileItem("thumb://Local", false));
    item->SetThumbnailImage(localThumb);
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

  CStdString result;
  bool flip=false;
  VECSOURCES sources(g_settings.m_musicSources);
  g_mediaManager.GetLocalDrives(sources);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(1030), result, &flip))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  CStdString cachedThumb;
  if (m_bArtistInfo)
    cachedThumb = m_albumItem->GetCachedArtistThumb();
  else
    cachedThumb = CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist);

  CTextureCache::Get().ClearCachedImage(cachedThumb, true);
  if (result.Left(14) == "thumb://Remote")
  {
    int number = atoi(result.Mid(14));
    CFile::Cache(thumbs[number], cachedThumb);
  }
  else if (result == "thumb://Local")
    CFile::Cache(localThumb, cachedThumb);
  else if (CFile::Exists(result))
    CPicture::CreateThumbnail(result, cachedThumb);
  else
    result = "thumb://None";

  if (result == "thumb://None")
  { // clear this thumb (note - it'll likely be recached, nothing we can do about that at this point)
    CTextureCache::Get().ClearCachedImage(cachedThumb, true);
    cachedThumb = "";
  }

  m_albumItem->SetThumbnailImage(cachedThumb);
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

  CStdString cachedThumb(CFileItem::GetCachedThumb(m_artist.strArtist,g_settings.GetMusicFanartFolder()));
  if (CFile::Exists(cachedThumb))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetThumbnailImage(cachedThumb);
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // Grab the thumbnails from the web
  for (unsigned int i = 0; i < m_artist.fanart.GetNumFanarts(); i++)
  {
    CStdString strItemPath;
    strItemPath.Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    CStdString thumb = m_artist.fanart.GetPreviewURL(i);
    item->SetThumbnailImage(CTextureCache::GetWrappedThumbURL(thumb));
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20441));

    // TODO: Do we need to clear the cached image?
    //    CTextureCache::Get().ClearCachedImage(thumb);
    items.Add(item);
  }

  // Grab a local thumb
  CMusicDatabase database;
  database.Open();
  CStdString strArtistPath;
  database.GetArtistPath(m_artist.idArtist,strArtistPath);
  CFileItem item(strArtistPath,true);
  CStdString strLocal = item.GetLocalFanart();
  if (!strLocal.IsEmpty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetThumbnailImage(strLocal);
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

  CStdString result;
  VECSOURCES sources(g_settings.m_musicSources);
  g_mediaManager.GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445))
    return;   // user cancelled

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  if (result.Equals("fanart://Current"))
   return;

  if (result.Equals("fanart://Local"))
    result = strLocal;

  CTextureCache::Get().ClearCachedImage(cachedThumb, true);

  if (!result.Equals("fanart://None"))
  { // local file
    if (result.Left(15)  == "fanart://Remote")
    {
      int iFanart = atoi(result.Mid(15).c_str());
      m_artist.fanart.SetPrimaryFanart(iFanart);
      // download the fullres fanart image
      CStdString tempFile = "special://temp/fanart_download.jpg";
      CAsyncFileCopy downloader;
      if (!downloader.Copy(m_artist.fanart.GetImageURL(), tempFile, g_localizeStrings.Get(13413)))
        return;
      result = tempFile;
    }

    if (flip)
      CPicture::ConvertFile(result, cachedThumb,0,1920,-1,100,true);
    else
      CPicture::CacheFanart(result, cachedThumb);

    m_albumItem->SetProperty("fanart_image",cachedThumb);
    m_hasUpdatedThumb = true;
  }

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
  long idAlbum = database.GetAlbumByName(pItem->GetLabel(),m_artist.strArtist);
  if (idAlbum != -1)
  {
    CAlbum album;
    if (database.GetAlbumInfo(idAlbum,album,&album.songs))
    {
      CStdString strPath;
      database.GetAlbumPath(idAlbum,strPath);
      SetAlbum(album,strPath);
      Update();
    }
  }
}

CFileItemPtr CGUIDialogMusicInfo::GetCurrentListItem(int offset)
{
  return m_albumItem;
}

