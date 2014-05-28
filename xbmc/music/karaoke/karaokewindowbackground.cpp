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

// C++ Implementation: karaokewindowbackground

#ifndef KARAOKE_SYSTEM_H_INCLUDED
#define KARAOKE_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#ifndef KARAOKE_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#define KARAOKE_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#include "settings/AdvancedSettings.h"
#endif

#ifndef KARAOKE_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#define KARAOKE_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#include "guilib/GUIWindowManager.h"
#endif

#ifndef KARAOKE_APPLICATION_H_INCLUDED
#define KARAOKE_APPLICATION_H_INCLUDED
#include "Application.h"
#endif

#ifndef KARAOKE_GUIUSERMESSAGES_H_INCLUDED
#define KARAOKE_GUIUSERMESSAGES_H_INCLUDED
#include "GUIUserMessages.h"
#endif

#ifndef KARAOKE_GUILIB_GUIVISUALISATIONCONTROL_H_INCLUDED
#define KARAOKE_GUILIB_GUIVISUALISATIONCONTROL_H_INCLUDED
#include "guilib/GUIVisualisationControl.h"
#endif

#ifndef KARAOKE_GUILIB_GUIIMAGE_H_INCLUDED
#define KARAOKE_GUILIB_GUIIMAGE_H_INCLUDED
#include "guilib/GUIImage.h"
#endif

#ifndef KARAOKE_THREADS_SINGLELOCK_H_INCLUDED
#define KARAOKE_THREADS_SINGLELOCK_H_INCLUDED
#include "threads/SingleLock.h"
#endif

#ifndef KARAOKE_UTILS_LOG_H_INCLUDED
#define KARAOKE_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif


#ifndef KARAOKE_KARAOKEWINDOWBACKGROUND_H_INCLUDED
#define KARAOKE_KARAOKEWINDOWBACKGROUND_H_INCLUDED
#include "karaokewindowbackground.h"
#endif

#ifndef KARAOKE_KARAOKEVIDEOBACKGROUND_H_INCLUDED
#define KARAOKE_KARAOKEVIDEOBACKGROUND_H_INCLUDED
#include "karaokevideobackground.h"
#endif



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
}


CKaraokeWindowBackground::~CKaraokeWindowBackground()
{
  if ( m_videoPlayer )
    delete m_videoPlayer;
}


void CKaraokeWindowBackground::Init(CGUIWindow * wnd)
{
  // Init controls
  m_VisControl = (CGUIVisualisationControl*) wnd->GetControl( CONTROL_ID_VIS );
  m_ImgControl = (CGUIImage*) wnd->GetControl( CONTROL_ID_IMG );

  // Init visialisation variables
  CStdString defBkgType = g_advancedSettings.m_karaokeDefaultBackgroundType;

  if ( defBkgType.empty() || defBkgType == "none" )
  {
    CLog::Log( LOGDEBUG, "Karaoke default background is set to none" );
    m_defaultMode = BACKGROUND_NONE;
  }
  else if ( defBkgType == "vis" || defBkgType == "viz" )
  {
    CLog::Log( LOGDEBUG, "Karaoke default background is visualisation" );
    m_defaultMode = BACKGROUND_VISUALISATION;
  }
  else if ( defBkgType == "image" && !g_advancedSettings.m_karaokeDefaultBackgroundFilePath.empty() )
  {
    CLog::Log( LOGDEBUG, "Karaoke default background is image %s", g_advancedSettings.m_karaokeDefaultBackgroundFilePath.c_str() );
    m_defaultMode = BACKGROUND_IMAGE;
    m_path = g_advancedSettings.m_karaokeDefaultBackgroundFilePath;
  }
  else if ( defBkgType == "video" && !g_advancedSettings.m_karaokeDefaultBackgroundFilePath.empty() )
  {
    CLog::Log( LOGDEBUG, "Karaoke default background is video %s", g_advancedSettings.m_karaokeDefaultBackgroundFilePath.c_str() );
    m_defaultMode = BACKGROUND_VIDEO;
  }
}


bool CKaraokeWindowBackground::OnAction(const CAction & action)
{
  CSingleLock lock (m_CritSectionShared);

  // Send it to the visualisation if we have one
  if ( m_currentMode == BACKGROUND_VISUALISATION )
    return m_VisControl->OnAction(action);

  return false;
}


bool CKaraokeWindowBackground::OnMessage(CGUIMessage & message)
{
  CSingleLock lock (m_CritSectionShared);

  // Forward visualisation control messages
  switch ( message.GetMessage() )
  {
  case GUI_MSG_PLAYBACK_STARTED:
      if ( m_currentMode == BACKGROUND_VISUALISATION )
        return m_VisControl->OnMessage(message);
    break;

  case GUI_MSG_GET_VISUALISATION:
      if ( m_currentMode == BACKGROUND_VISUALISATION )
        return m_VisControl->OnMessage(message);
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
  CSingleLock lock (m_CritSectionShared);

  // Proceed with video rendering
  if ( m_currentMode == BACKGROUND_VIDEO && m_videoPlayer )
  {
    m_videoPlayer->Render();
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


void CKaraokeWindowBackground::StartVideo( const CStdString& path )
{
  if ( !m_videoPlayer )
	m_videoPlayer = new KaraokeVideoBackground();

  if ( !m_videoPlayer->Start( path ) )
  {
    delete m_videoPlayer;
    m_videoPlayer = 0;
    m_currentMode = BACKGROUND_NONE;
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
      StartVideo();
      break;

    default:
      StartEmpty();
      break;
  }
}


void CKaraokeWindowBackground::Stop()
{
  CSingleLock lock (m_CritSectionShared);
  m_currentMode = BACKGROUND_NONE;

  if ( m_videoPlayer )
    m_videoPlayer->Stop();

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

void CKaraokeWindowBackground::Pause(bool now_paused)
{
}
