#include "stdafx.h"
#include "GUIWindowVisualisation.h"
#include "application.h"
#include "GUIVisualisationControl.h"
#include "utils/GUIInfoManager.h"

#define TRANSISTION_COUNT   50  // 1 second
#define TRANSISTION_LENGTH 200  // 4 seconds
#define START_FADE_LENGTH  100  // 2 seconds on startup

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
    : CGUIWindow(0)
{
  m_dwFrameCounter = 0;
  m_dwInitTimer = 0;
  m_bShowInfo = false;
  m_bFadingAtStart = false;
}

CGUIWindowVisualisation::~CGUIWindowVisualisation(void)
{
}

bool CGUIWindowVisualisation::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_SHOW_INFO:
    //send the action to the overlay
    if (g_graphicsContext.IsOverlayAllowed())
      return g_application.m_guiMusicOverlay.OnAction(action);
    else
    {
      // reset the timer
      m_dwInitTimer = 0;
      if (m_dwFrameCounter)
      { // already in the process of a fade - reverse it
        m_dwFrameCounter = TRANSISTION_COUNT - m_dwFrameCounter;  // takes care of a switch half way
        m_bShowInfo = !m_bShowInfo;
      }
      else
        m_dwFrameCounter = TRANSISTION_COUNT;
      // toggle our settings
      g_stSettings.m_bMyMusicSongThumbInVis = !m_bShowInfo;
      return true;
    }
    break;

  case ACTION_SHOW_GUI:
    //send the action to the overlay so we can reset
    //the bool m_bShowInfoAlways
    g_application.m_guiMusicOverlay.OnAction(action);
    // save the settings
    g_settings.Save();
    m_gWindowManager.PreviousWindow();
    return true;
    break;

  case KEY_BUTTON_Y:
    g_application.m_CdgParser.Pause();
    return true;
    break;

    // TODO: This should be mapped to it's own function - at the moment it's overriding
    // the global action of fastforward/rewind.
/*  case ACTION_ANALOG_FORWARD:
    // calculate the speed based on the amount the button is held down
    if (action.fAmount1)
    {
      float AVDelay = g_application.m_CdgParser.GetAVDelay();
      g_application.m_CdgParser.SetAVDelay(AVDelay - action.fAmount1 / 4.0f);
      return true;
    }
    break;*/
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing music
      if (message.GetParam1() == WINDOW_INVALID && !g_application.IsPlayingAudio())
      { // why are we here if nothing is playing???
        m_gWindowManager.PreviousWindow();
        return true;
      }

      if (g_stSettings.m_bMyMusicSongThumbInVis)
      { // always on
        m_bShowInfo = true;
        m_dwFrameCounter = 0;
        m_dwInitTimer = 0;
        SetAlpha(255);
      }
      else
      {
        // start display init timer (fade out after 3 secs...)
        m_bShowInfo = true;
        m_dwInitTimer = START_FADE_LENGTH;
        m_bFadingAtStart = true;
        SetAlpha(255);
      }

      CGUIWindow::OnMessage(message);
      return true;
    }
  }
  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowVisualisation::OnMouse()
{
  if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // no control found to absorb this click - go back to GUI
    CAction action;
    action.wID = ACTION_SHOW_GUI;
    OnAction(action);
    return true;
  }
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // no control found to absorb this click - toggle the track INFO
    CAction action;
    action.wID = ACTION_SHOW_INFO;
    OnAction(action);
  }
  return true;
}

void CGUIWindowVisualisation::Render()
{
  g_application.ResetScreenSaver();
  if (!g_stSettings.m_bMyMusicSongThumbInVis)
  {
    if (m_dwInitTimer)
    {
      m_dwInitTimer--;
      if (!m_dwInitTimer)
      { // time has elapsed - switch view modes!
        m_dwFrameCounter = TRANSISTION_COUNT;
      }
    }
    else if (!m_dwFrameCounter)
    {  // check our current time, as we may have to fade in
      int timeStarted = (int)(g_infoManager.GetPlayTime()/1000);
      if (timeStarted < TRANSISTION_LENGTH/50 && !m_bShowInfo && !m_bFadingAtStart)
      { // fade in at the start
        m_dwFrameCounter = TRANSISTION_COUNT;
        m_bFadingAtStart = true;
      }
      else if (timeStarted >= TRANSISTION_LENGTH/50 && m_bShowInfo)
      { // fade out after 5 seconds
        m_dwFrameCounter = TRANSISTION_COUNT;
        m_bFadingAtStart = false;
      }
      else if (timeStarted > TRANSISTION_LENGTH/50)
        m_bFadingAtStart = false; // reset so that at next track we're good to go again.
    }
  }
  if (m_dwFrameCounter)
  {
    m_dwFrameCounter--;
    if (m_bShowInfo)
    { // fading out
      float fAlpha = (float)m_dwFrameCounter/((float)TRANSISTION_COUNT)*255.0f;
      SetAlpha((DWORD)fAlpha);
    }
    else
    { // fading in
      float fAlpha = (float)(TRANSISTION_COUNT - m_dwFrameCounter)/((float)TRANSISTION_COUNT)*255.0f;
      SetAlpha((DWORD)fAlpha);
    }
    // toggle condition
    if (!m_dwFrameCounter)
    {
      m_bShowInfo = !m_bShowInfo;
    }
  }
  CGUIWindow::Render();
}

void CGUIWindowVisualisation::SetAlpha(DWORD dwAlpha)
{
  for (unsigned int i = 0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    if (pControl->GetControlType() != CGUIControl::GUICONTROL_VISUALISATION)
    { // set the alpha
      pControl->SetAlpha(dwAlpha);
    }
  }
}

void CGUIWindowVisualisation::FreeResources()
{
  // Save changed settings from music OSD
  g_settings.Save();
  CGUIWindow::FreeResources();
}

void CGUIWindowVisualisation::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // Check if we have a vis control...
  for (unsigned int i = 0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    if (pControl->GetControlType() == CGUIControl::GUICONTROL_VISUALISATION)
      return;
  }
  // not found - let's add a new one
  CGUIVisualisationControl *pVisControl = new CGUIVisualisationControl(GetID(), 0, 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight());
  if (pVisControl)
  {
    m_vecControls.insert(m_vecControls.begin(), pVisControl);
  }
}

