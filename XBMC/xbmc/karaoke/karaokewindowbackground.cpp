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
#include "GUIVisualisationControl.h"
#include "GUILargeImage.h"

#include "karaokewindowbackground.h"

#define CONTROL_ID_VIS           1
#define CONTROL_ID_IMG           2


CKaraokeWindowBackground::CKaraokeWindowBackground()
{
  m_mode = BACKGROUND_NONE;
  m_parentWindow = 0;

  m_Image = 0;
  m_VisControl = 0;
  m_ImgControl = 0;
}


CKaraokeWindowBackground::~CKaraokeWindowBackground()
{
}


void CKaraokeWindowBackground::Render()
{
}


void CKaraokeWindowBackground::Init(CGUIWindow * wnd)
{
  m_VisControl = (CGUIVisualisationControl*) wnd->GetControl( CONTROL_ID_VIS );
  m_ImgControl = (CGUIImage*) wnd->GetControl( CONTROL_ID_IMG );
}


bool CKaraokeWindowBackground::OnAction(const CAction & action)
{
  // Send it to the visualisation if we have one
  if ( m_mode == BACKGROUND_VISUALISATION )
    return m_VisControl->OnAction(action);

  return false;
}


bool CKaraokeWindowBackground::OnMessage(CGUIMessage & message)
{
  // Forward visualisation control messages
  switch ( message.GetMessage() )
  {
  case GUI_MSG_PLAYBACK_STARTED:
      if ( m_mode == BACKGROUND_VISUALISATION )
        return m_VisControl->OnMessage(message);
    break;

  case GUI_MSG_GET_VISUALISATION:
      if ( m_mode == BACKGROUND_VISUALISATION )
        message.SetLPVOID( m_VisControl->GetVisualisation() );
    break;

  case GUI_MSG_VISUALISATION_ACTION:
      if ( m_mode == BACKGROUND_VISUALISATION )
        return m_VisControl->OnMessage(message);
    break;
  }

  return false;
}


void CKaraokeWindowBackground::StartEmpty()
{
  m_VisControl->SetVisible( false );
  m_ImgControl->SetVisible( false );
  m_mode = BACKGROUND_NONE;
  CLog::Log( LOGDEBUG, "Karaoke background started using BACKGROUND_NONE mode" );
}


void CKaraokeWindowBackground::StartVisualisation()
{
  // Showing controls
  m_ImgControl->SetVisible( false );
  m_VisControl->SetVisible( true );

  m_mode = BACKGROUND_VISUALISATION;
  CLog::Log( LOGDEBUG, "Karaoke background started using BACKGROUND_VISUALISATION mode" );
}


void CKaraokeWindowBackground::StartImage()
{
  // Showing controls
  m_ImgControl->SetVisible( true );
  m_VisControl->SetVisible( false );

  m_ImgControl->SetFileName( "special://xbmc/image.jpg" );

  m_mode = BACKGROUND_IMAGE;
  CLog::Log( LOGDEBUG, "Karaoke background started using BACKGROUND_IMAGE mode" );
}


void CKaraokeWindowBackground::Stop()
{
  // Disable and hide all control
  m_mode = BACKGROUND_NONE;
  CLog::Log( LOGDEBUG, "Karaoke background stopped" );
}
