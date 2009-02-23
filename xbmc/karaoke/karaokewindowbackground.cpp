//
// C++ Implementation: karaokewindowbackground
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "stdafx.h"
#include "Settings.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIVisualisationControl.h"
#include "GUIImage.h"
#include "cores/dvdplayer/DVDPlayer.h"

#include "karaokewindowbackground.h"


#define CONTROL_ID_VIS           1
#define CONTROL_ID_IMG           2


CKaraokeWindowBackground::CKaraokeWindowBackground()
{
  m_currentMode = BACKGROUND_NONE;
  m_defaultMode = BACKGROUND_NONE;
  m_parentWindow = 0;

  m_VisControl = 0;
  m_ImgControl = 0;

  m_videoPlayer = 0;
  m_parentWindow = 0;
  m_videoLastTime = 0;
  m_playingDefaultVideo = false;
}


CKaraokeWindowBackground::~CKaraokeWindowBackground()
{
  if ( m_videoPlayer )
  {
     m_videoPlayer->CloseFile();
     delete m_videoPlayer;
  }
}


void CKaraokeWindowBackground::Init(CGUIWindow * wnd)
{
  // Init controls
  m_VisControl = (CGUIVisualisationControl*) wnd->GetControl( CONTROL_ID_VIS );
  m_ImgControl = (CGUIImage*) wnd->GetControl( CONTROL_ID_IMG );

  // Init visialisation variables
  CStdString defBkgType = g_advancedSettings.m_karaokeDefaultBackgroundType;

  if ( defBkgType.IsEmpty() || defBkgType == "none" )
  {
    CLog::Log( LOGDEBUG, "Karaoke default background is set to none" );
    m_defaultMode = BACKGROUND_NONE;
  }
  else if ( defBkgType == "vis" || defBkgType == "viz" )
  {
    CLog::Log( LOGDEBUG, "Karaoke default background is visualisation" );
    m_defaultMode = BACKGROUND_VISUALISATION;
  }
  else if ( defBkgType == "image" && !g_advancedSettings.m_karaokeDefaultBackgroundFilePath.IsEmpty() )
  {
    CLog::Log( LOGDEBUG, "Karaoke default background is image %s", g_advancedSettings.m_karaokeDefaultBackgroundFilePath.c_str() );
    m_defaultMode = BACKGROUND_IMAGE;
    m_path = g_advancedSettings.m_karaokeDefaultBackgroundFilePath;
  }
  else if ( defBkgType == "video" && !g_advancedSettings.m_karaokeDefaultBackgroundFilePath.IsEmpty() )
  {
    CLog::Log( LOGDEBUG, "Karaoke default background is video %s", g_advancedSettings.m_karaokeDefaultBackgroundFilePath.c_str() );
    m_defaultMode = BACKGROUND_VIDEO;
    m_path = g_advancedSettings.m_karaokeDefaultBackgroundFilePath;
  }
}


bool CKaraokeWindowBackground::OnAction(const CAction & action)
{
  // Send it to the visualisation if we have one
  if ( m_currentMode == BACKGROUND_VISUALISATION )
    return m_VisControl->OnAction(action);

  return false;
}


bool CKaraokeWindowBackground::OnMessage(CGUIMessage & message)
{
  // Forward visualisation control messages
  switch ( message.GetMessage() )
  {
  case GUI_MSG_PLAYBACK_STARTED:
      if ( m_currentMode == BACKGROUND_VISUALISATION )
        return m_VisControl->OnMessage(message);
    break;

  case GUI_MSG_GET_VISUALISATION:
      if ( m_currentMode == BACKGROUND_VISUALISATION )
        message.SetLPVOID( m_VisControl->GetVisualisation() );
    break;

  case GUI_MSG_VISUALISATION_ACTION:
      if ( m_currentMode == BACKGROUND_VISUALISATION )
        return m_VisControl->OnMessage(message);
    break;
  }

  return false;
}


void CKaraokeWindowBackground::Render()
{
  // Proceed with video rendering
  if ( m_currentMode == BACKGROUND_VIDEO )
  {
    if ( g_application.IsPresentFrame() )
      g_renderManager.Present();
    else
      g_renderManager.RenderUpdate(true, 0, 255);
  }

  // For other visualisations just disable the screen saver
  g_application.ResetScreenSaver();
}


void CKaraokeWindowBackground::StartEmpty()
{
  m_VisControl->SetVisible( false );
  m_ImgControl->SetVisible( false );
  m_currentMode = BACKGROUND_NONE;
  CLog::Log( LOGDEBUG, "Karaoke background started using BACKGROUND_NONE mode" );
}


void CKaraokeWindowBackground::StartVisualisation()
{
  // Showing controls
  m_ImgControl->SetVisible( false );
  m_VisControl->SetVisible( true );

  m_currentMode = BACKGROUND_VISUALISATION;
  CLog::Log( LOGDEBUG, "Karaoke background started using BACKGROUND_VISUALISATION mode" );
}


void CKaraokeWindowBackground::StartImage( const CStdString& path )
{
  // Showing controls
  m_ImgControl->SetVisible( true );
  m_VisControl->SetVisible( false );

  m_ImgControl->SetFileName( path );

  m_currentMode = BACKGROUND_IMAGE;
  CLog::Log( LOGDEBUG, "Karaoke background started using BACKGROUND_IMAGE mode using image %s", path.c_str() );
}


void CKaraokeWindowBackground::StartVideo( const CStdString& path, __int64 offset)
{
  CFileItem item( path, false);

  // Video options
  CPlayerOptions options;
  options.video_only = true;

  if ( offset > 0 )
    options.starttime = (double) (offset / 1000.0);

  if ( !item.IsVideo() )
  {
    CLog::Log(LOGERROR, "KaraokeVideoBackground: file %s is not a video file, ignoring", path.c_str() );
    return;
  }

  if ( item.IsDVD() )
  {
    CLog::Log(LOGERROR, "KaraokeVideoBackground: DVD video playback is not supported");
    return;
  }

  m_videoPlayer = new CDVDPlayer(*this);

  if ( !m_videoPlayer )
    return;

  CLog::Log(LOGDEBUG, "KaraokeVideoBackground: opening video file %s", item.m_strPath.c_str());

  if ( !m_videoPlayer->OpenFile( item, options ) )
  {
    CLog::Log(LOGERROR, "KaraokeVideoBackground: error opening video file %s", item.m_strPath.c_str());
    return;
  }

  m_ImgControl->SetVisible( false );
  m_VisControl->SetVisible( false );
  m_currentMode = BACKGROUND_VIDEO;
}


void CKaraokeWindowBackground::StartDefault()
{
  // just in case
  m_ImgControl->SetVisible( false );
  m_VisControl->SetVisible( false );

  switch ( m_defaultMode )
  {
    case BACKGROUND_VISUALISATION:
      StartVisualisation();
      break;

    case BACKGROUND_IMAGE:
      StartImage( m_path );
      break;

    case BACKGROUND_VIDEO:
      StartVideo( m_path, m_videoLastTime );

      if ( m_currentMode == BACKGROUND_VIDEO )
        m_playingDefaultVideo = true;
      break;

    default:
      StartEmpty();
      break;
  }
}


void CKaraokeWindowBackground::Stop()
{
  m_currentMode = BACKGROUND_NONE;

  // Disable and hide all control
  if ( m_videoPlayer )
  {
     if ( m_playingDefaultVideo )
       m_videoLastTime = m_videoPlayer->GetTime();

     m_videoPlayer->CloseFile();
     delete m_videoPlayer;
     m_videoPlayer = 0;
  }

  CLog::Log( LOGDEBUG, "Karaoke background stopped" );
}


void CKaraokeWindowBackground::OnPlayBackEnded()
{
}


void CKaraokeWindowBackground::OnPlayBackStarted()
{
}


void CKaraokeWindowBackground::OnPlayBackStopped()
{
}


void CKaraokeWindowBackground::OnQueueNextItem()
{
}
