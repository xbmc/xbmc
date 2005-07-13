#include "stdafx.h"
#include "GUIWindowMusicInfo.h"
#include "Utils/HTTP.h"
#include "Util.h"
#include "GUIImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

CGUIWindowMusicInfo::CGUIWindowMusicInfo(void)
    : CGUIDialog(0)
{}

CGUIWindowMusicInfo::~CGUIWindowMusicInfo(void)
{}

bool CGUIWindowMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_pAlbum = NULL;
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

      if (iControl == CONTROL_BTN_TRACKS)
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
  m_pAlbum = &album;
}

void CGUIWindowMusicInfo::Update()
{
  if (!m_pAlbum) return ;
  CStdString strTmp;
  SetLabel(CONTROL_ALBUM, m_pAlbum->GetTitle() );
  SetLabel(CONTROL_ARTIST, m_pAlbum->GetArtist() );
  SetLabel(CONTROL_DATE, m_pAlbum->GetDateOfRelease() );

  CStdString strRating;
  if (m_pAlbum->GetRating() > 0)
    strRating.Format("%i/9", m_pAlbum->GetRating());
  SetLabel(CONTROL_RATING, strRating );

  SetLabel(CONTROL_GENRE, m_pAlbum->GetGenre() );
  {
    CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_TONE);
    OnMessage(msg1);
  }
  {
    strTmp = m_pAlbum->GetTones(); strTmp.Trim();
    CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_TONE);
    msg1.SetLabel( strTmp );
    OnMessage(msg1);
  }
  SetLabel(CONTROL_STYLES, m_pAlbum->GetStyles() );

  if (m_bViewReview)
  {
    SET_CONTROL_LABEL(CONTROL_TEXTAREA, m_pAlbum->GetReview());
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 182);
  }
  else
  {
    CStdString strLine;
    for (int i = 0; i < m_pAlbum->GetNumberOfSongs();++i)
    {
      const CMusicSong& song = m_pAlbum->GetSong(i);
      CStdString strTmp;
      strTmp.Format("%i. %-30s\n",
                    song.GetTrack(),
                    song.GetSongName());
      strLine += strTmp;
    };

    SET_CONTROL_LABEL(CONTROL_TEXTAREA, strLine);

    for (int i = 0; i < m_pAlbum->GetNumberOfSongs();++i)
    {
      const CMusicSong& song = m_pAlbum->GetSong(i);
      CStdString strTmp;

      if (song.GetDuration() > 0)
        CUtil::SecondsToHMSString(song.GetDuration(), strTmp);

      CGUIMessage msg3(GUI_MSG_LABEL2_SET, GetID(), CONTROL_TEXTAREA, i, 0);
      msg3.SetLabel(strTmp);
      g_graphicsContext.SendMessage(msg3);
    }

    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 183);

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
  if (!g_guiSettings.GetBool("Network.EnableInternet"))
  {
    Update();
    return ;
  }

  CStdString strThumb;
  CStdString strImage = m_pAlbum->GetImageURL();
  CUtil::GetAlbumThumb(m_pAlbum->GetTitle(), m_pAlbum->GetAlbumPath(), strThumb);
  if (!CFile::Exists(strThumb) && !strImage.IsEmpty() )
  {
    // Download image and save as
    // permanent thumb
    CHTTP http;
    http.Download(strImage, strThumb);
  }

  if (!CFile::Exists(strThumb) )
  {
    strThumb.Empty();
  }
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->SetKeepAspectRatio(true);
    pImageControl->SetFileName(strThumb);
  }
  Update();
}

bool CGUIWindowMusicInfo::NeedRefresh() const
{
  return m_bRefresh;
}
