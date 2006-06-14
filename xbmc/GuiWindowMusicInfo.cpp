#include "stdafx.h"
#include "GUIWindowMusicInfo.h"
#include "Utils/HTTP.h"
#include "Util.h"
#include "GUIImage.h"
#include "Picture.h"
#include "GUIDialogFileBrowser.h"


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

CGUIWindowMusicInfo::CGUIWindowMusicInfo(void)
    : CGUIDialog(WINDOW_MUSIC_INFO, "DialogAlbumInfo.xml")
{}

CGUIWindowMusicInfo::~CGUIWindowMusicInfo(void)
{}

bool CGUIWindowMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bViewReview = true;
      m_bRefresh = false;
      Refresh();
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

void CGUIWindowMusicInfo::SetAlbum(CMusicAlbumInfo& album)
{
  m_album = album;
  m_albumItem = CFileItem(album.GetAlbumPath(), true);
  m_albumItem.m_musicInfoTag.SetAlbum(album.GetTitle());
  m_albumItem.m_musicInfoTag.SetLoaded(true);
  m_albumItem.SetMusicThumb();
  m_hasUpdatedThumb = false;
}

void CGUIWindowMusicInfo::Update()
{
  CStdString strTmp;
  SetLabel(CONTROL_ALBUM, m_album.GetTitle() );
  SetLabel(CONTROL_ARTIST, m_album.GetArtist() );
  SetLabel(CONTROL_DATE, m_album.GetDateOfRelease() );

  CStdString strRating;
  if (m_album.GetRating() > 0)
    strRating.Format("%i/9", m_album.GetRating());
  SetLabel(CONTROL_RATING, strRating );

  SetLabel(CONTROL_GENRE, m_album.GetGenre() );
  {
    CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_TONE);
    OnMessage(msg1);
  }
  {
    strTmp = m_album.GetTones(); strTmp.Trim();
    CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_TONE);
    msg1.SetLabel( strTmp );
    OnMessage(msg1);
  }
  SetLabel(CONTROL_STYLES, m_album.GetStyles() );

  if (m_bViewReview)
  {
    SET_CONTROL_LABEL(CONTROL_TEXTAREA, m_album.GetReview());
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 182);
  }
  else
  {
    CStdString strLine;
    for (int i = 0; i < m_album.GetNumberOfSongs();++i)
    {
      const CMusicSong& song = m_album.GetSong(i);
      CStdString strTmp;
      strTmp.Format("%i. %-30s\n",
                    song.GetTrack(),
                    song.GetSongName());
      strLine += strTmp;
    };

    SET_CONTROL_LABEL(CONTROL_TEXTAREA, strLine);

    for (int i = 0; i < m_album.GetNumberOfSongs();++i)
    {
      const CMusicSong& song = m_album.GetSong(i);
      CStdString strTmp;

      if (song.GetDuration() > 0)
        StringUtils::SecondsToTimeString(song.GetDuration(), strTmp);

      CGUIMessage msg3(GUI_MSG_LABEL2_SET, GetID(), CONTROL_TEXTAREA, i, 0);
      msg3.SetLabel(strTmp);
      g_graphicsContext.SendMessage(msg3);
    }

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
}

void CGUIWindowMusicInfo::SetLabel(int iControl, const CStdString& strLabel)
{
  CStdString strLabel1 = strLabel;
  if (strLabel1.size() == 0)
    strLabel1 = g_localizeStrings.Get(416);

  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iControl);
  msg.SetLabel(strLabel1);
  OnMessage(msg);

}

void CGUIWindowMusicInfo::Render()
{
  CGUIDialog::Render();
}


void CGUIWindowMusicInfo::Refresh()
{
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("network.enableinternet"))
  {
    Update();
    return ;
  }

  CStdString thumbImage = m_albumItem.GetThumbnailImage();
  if (!m_albumItem.HasThumbnail())
    thumbImage = CUtil::GetCachedAlbumThumb(m_album.GetTitle(), m_album.GetAlbumPath());

  if (!CFile::Exists(thumbImage))
  {
    DownloadThumbnail(thumbImage);
    m_hasUpdatedThumb = true;
  }

  if (!CFile::Exists(thumbImage) )
  {
    thumbImage.Empty();
  }

  m_albumItem.SetThumbnailImage(thumbImage);
  Update();
}

bool CGUIWindowMusicInfo::NeedRefresh() const
{
  return m_bRefresh;
}

bool CGUIWindowMusicInfo::DownloadThumbnail(const CStdString &thumbFile)
{
  // Download image and save as thumbFile
  if (m_album.GetImageURL().IsEmpty())
    return false;
  
  CHTTP http;
  http.Download(m_album.GetImageURL(), thumbFile);
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
    item->SetLabel(g_localizeStrings.Get(20055)); // TODO: localize 2.0
    items.Add(item);
  }

  // Current thumb
  if (CFile::Exists(m_albumItem.GetThumbnailImage()))
  {
    CFileItem *item = new CFileItem("thumb://Current", false);
    item->SetThumbnailImage(m_albumItem.GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016)); // TODO: localize 2.0
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
      item->SetLabel(g_localizeStrings.Get(20017)); // TODO: localize 2.0
      items.Add(item);
    }
  }
  else
  { // no local thumb exists, so we are just using the allmusic.com thumb or cached thumb
    // which is probably the allmusic.com thumb.  These could be wrong, so allow the user
    // to delete the incorrect thumb
    if (0 == items.Size())
    { // no cached thumb or no allmusic.com thumb available
      // TODO: tell user and return
      return;
    }
    CFileItem *item = new CFileItem("thumb://None", false);
    item->SetThumbnailImage("defaultAlbumBig.png");
    item->SetLabel(g_localizeStrings.Get(20018)); // TODO: localize 2.0
    items.Add(item);
  }

  CStdString result;
  // TODO: localize 2.0
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, g_settings.m_vecMyMusicShares, g_localizeStrings.Get(20019), result))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  CStdString cachedThumb(CUtil::GetCachedAlbumThumb(m_album.GetTitle(), m_album.GetAlbumPath()));

  if (result == "thumb://None")
  { // delete any cached thumb
    CFile::Delete(cachedThumb);
    cachedThumb.Empty();
  }
  else if (result == "thumb://allmusic.com")
    CFile::Cache(thumbFromWeb, cachedThumb);
  else if (result == "thumb://Local")
    CFile::Cache(cachedLocalThumb, cachedThumb);
  else if (CFile::Exists(result))
    CFile::Cache(result, cachedThumb);

  m_albumItem.SetThumbnailImage(cachedThumb);
  m_hasUpdatedThumb = true;

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS, 0, NULL);
  g_graphicsContext.SendMessage(msg);
  // Update our screen
  Update();
}