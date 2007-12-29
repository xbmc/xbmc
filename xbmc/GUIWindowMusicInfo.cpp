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
#include "GUIWindowMusicInfo.h"
#include "utils/HTTP.h"
#include "Util.h"
#include "guiImage.h"
#include "Picture.h"
#include "GUIDialogFileBrowser.h"
#include "GUIPassword.h"

using namespace XFILE;

#define CONTROL_ALBUM  20
#define CONTROL_ARTIST 21
#define CONTROL_DATE   22
#define CONTROL_RATING 23
#define CONTROL_GENRE  24
#define CONTROL_TONE   25
#define CONTROL_STYLES 26

#define CONTROL_IMAGE   3
#define CONTROL_TEXTAREA 4

#define CONTROL_BTN_TRACKS 5
#define CONTROL_BTN_REFRESH 6
#define CONTROL_BTN_GET_THUMB 10

#define CONTROL_LIST 50

CGUIWindowMusicInfo::CGUIWindowMusicInfo(void)
    : CGUIDialog(WINDOW_MUSIC_INFO, "DialogAlbumInfo.xml")
{
  m_bRefresh = false;
}

CGUIWindowMusicInfo::~CGUIWindowMusicInfo(void)
{}

bool CGUIWindowMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(message);
      m_albumSongs.Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bViewReview = true;
      m_bRefresh = false;
      if (g_guiSettings.GetBool("network.enableinternet"))
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
        CUtil::ClearCache();

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
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowMusicInfo::SetAlbum(const CAlbum& album, const VECSONGS &songs, const CStdString &path)
{
  m_album = album;
  SetSongs(songs);
  m_albumItem = CFileItem(path, true);
  m_albumItem.GetMusicInfoTag()->SetAlbum(m_album.strAlbum);
  m_albumItem.GetMusicInfoTag()->SetAlbumArtist(m_album.strArtist);
  m_albumItem.GetMusicInfoTag()->SetArtist(m_album.strArtist);
  m_albumItem.GetMusicInfoTag()->SetYear(m_album.iYear);
  m_albumItem.GetMusicInfoTag()->SetLoaded(true);
  m_albumItem.GetMusicInfoTag()->SetRating('0' + (m_album.iRating + 1) / 2);
  m_albumItem.GetMusicInfoTag()->SetGenre(m_album.strGenre);
  m_albumItem.SetProperty("albumstyles", m_album.strStyles);
  m_albumItem.SetProperty("albumtones", m_album.strTones);
  m_albumItem.SetProperty("albumreview", m_album.strReview);
  m_albumItem.SetMusicThumb();
  m_hasUpdatedThumb = false;
}

void CGUIWindowMusicInfo::SetSongs(const VECSONGS &songs)
{
  m_albumSongs.Clear();
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    const CSong& song = songs[i];
    CFileItem *item = new CFileItem(song);
    m_albumSongs.Add(item);
  }
}

void CGUIWindowMusicInfo::Update()
{
  SetLabel(CONTROL_ALBUM, m_album.strAlbum );
  SetLabel(CONTROL_ARTIST, m_album.strArtist );
  CStdString date; date.Format("%d", m_album.iYear);
  SetLabel(CONTROL_DATE, date );

  CStdString strRating;
  if (m_album.iRating > 0)
    strRating.Format("%i/9", m_album.iRating);
  SetLabel(CONTROL_RATING, strRating );

  SetLabel(CONTROL_GENRE, m_album.strGenre);
  SetLabel(CONTROL_TONE, m_album.strTones);
  SetLabel(CONTROL_STYLES, m_album.strStyles );

  if (m_bViewReview)
  {
    SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
    SET_CONTROL_HIDDEN(CONTROL_LIST);
    SetLabel(CONTROL_TEXTAREA, m_album.strReview);
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 182);
  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_LIST);
    if (GetControl(CONTROL_LIST))
    {
      SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
      CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, &m_albumSongs);
      OnMessage(message);
    }
    else
      CLog::Log(LOGERROR, "Out of date skin - needs list with id %i", CONTROL_LIST);
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 183);
  }
  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_albumItem.GetThumbnailImage());
  }

  // disable the GetThumb button if the user isn't allowed it
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser)
}

void CGUIWindowMusicInfo::SetLabel(int iControl, const CStdString& strLabel)
{
  if (strLabel.IsEmpty())
  {
    SET_CONTROL_LABEL(iControl, (iControl == CONTROL_TEXTAREA) ? 414 : 416);
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

void CGUIWindowMusicInfo::Render()
{
  CGUIDialog::Render();
}


void CGUIWindowMusicInfo::RefreshThumb()
{
  CStdString thumbImage = m_albumItem.GetThumbnailImage();
  if (!m_albumItem.HasThumbnail())
    thumbImage = CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist);

  if (!CFile::Exists(thumbImage))
  {
    DownloadThumbnail(thumbImage);
    m_hasUpdatedThumb = true;
  }

  if (!CFile::Exists(thumbImage) )
    thumbImage.Empty();

  m_albumItem.SetThumbnailImage(thumbImage);
}

bool CGUIWindowMusicInfo::NeedRefresh() const
{
  return m_bRefresh;
}

bool CGUIWindowMusicInfo::DownloadThumbnail(const CStdString &thumbFile)
{
  // Download image and save as thumbFile
  if (m_album.strImage.IsEmpty())
    return false;

  CHTTP http;
  http.Download(m_album.strImage, thumbFile);
  return true;
}

void CGUIWindowMusicInfo::OnInitWindow()
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
void CGUIWindowMusicInfo::OnGetThumb()
{
  CFileItemList items;

  // Grab the thumbnail from the web
  CStdString thumbFromWeb;
  CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "allmusicThumb.jpg", thumbFromWeb);
  if (DownloadThumbnail(thumbFromWeb))
  {
    CFileItem *item = new CFileItem("thumb://allmusic.com", false);
    item->SetThumbnailImage(thumbFromWeb);
    item->SetLabel(g_localizeStrings.Get(20055));
    items.Add(item);
  }

  // Current thumb
  if (CFile::Exists(m_albumItem.GetThumbnailImage()))
  {
    CFileItem *item = new CFileItem("thumb://Current", false);
    item->SetThumbnailImage(m_albumItem.GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // local thumb
  CStdString cachedLocalThumb;
  CStdString localThumb(m_albumItem.GetUserMusicThumb());
  if (CFile::Exists(localThumb))
  {
    CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "localthumb.jpg", cachedLocalThumb);
    CPicture pic;
    if (pic.DoCreateThumbnail(localThumb, cachedLocalThumb))
    {
      CFileItem *item = new CFileItem("thumb://Local", false);
      item->SetThumbnailImage(cachedLocalThumb);
      item->SetLabel(g_localizeStrings.Get(20017));
      items.Add(item);
    }
  }
  else
  { // no local thumb exists, so we are just using the allmusic.com thumb or cached thumb
    // which is probably the allmusic.com thumb.  These could be wrong, so allow the user
    // to delete the incorrect thumb
    CFileItem *item = new CFileItem("thumb://None", false);
    item->SetThumbnailImage("defaultAlbumCover.png");
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
  CStdString cachedThumb(CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist));

  if (result == "thumb://None")
  { // cache the default thumb
    CPicture pic;
    pic.CacheSkinImage("defaultAlbumCover.png", cachedThumb);
  }
  else if (result == "thumb://allmusic.com")
    CFile::Cache(thumbFromWeb, cachedThumb);
  else if (result == "thumb://Local")
    CFile::Cache(cachedLocalThumb, cachedThumb);
  else if (CFile::Exists(result))
  {
    CPicture pic;
    pic.DoCreateThumbnail(result, cachedThumb);
  }

  m_albumItem.SetThumbnailImage(cachedThumb);
  m_hasUpdatedThumb = true;

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS, 0, NULL);
  g_graphicsContext.SendMessage(msg);
  // Update our screen
  Update();
}
