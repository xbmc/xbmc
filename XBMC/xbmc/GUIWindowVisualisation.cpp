#include "stdafx.h"
#include "GUIWindowVisualisation.h"
#include "application.h"
#include "GUIVisualisationControl.h"

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

#define CONTROL_VISUALISATION 20

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
    : CGUIWindow(0)
{
}

CGUIWindowVisualisation::~CGUIWindowVisualisation(void)
{
}

void CGUIWindowVisualisation::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_SHOW_INFO:
    //send the action to the overlay
    g_application.m_guiMusicOverlay.OnAction(action);
    break;

  case ACTION_SHOW_GUI:
    //send the action to the overlay so we can reset
    //the bool m_bShowInfoAlways
    g_application.m_guiMusicOverlay.OnAction(action);
    m_gWindowManager.PreviousWindow();
    break;

  case KEY_BUTTON_Y:
    g_application.m_CdgParser.Pause();
    break;

  case ACTION_ANALOG_FORWARD:
    // calculate the speed based on the amount the button is held down
    float AVDelay = g_application.m_CdgParser.GetAVDelay();
    g_application.m_CdgParser.SetAVDelay(AVDelay - action.fAmount1 / 4.0f);
    break;
  }
  CGUIWindow::OnAction(action);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      // remove z-buffer
      RESOLUTION res = g_graphicsContext.GetVideoResolution();
      g_graphicsContext.SetVideoResolution(res, FALSE);
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

      CGUIWindow::OnMessage(message);
      // setup a z-buffer
      RESOLUTION res = g_graphicsContext.GetVideoResolution();
      g_graphicsContext.SetVideoResolution(res, TRUE);
      return true;
    }
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowVisualisation::OnMouse()
{
  if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // no control found to absorb this click - go back to GUI
    CAction action;
    action.wID = ACTION_SHOW_GUI;
    OnAction(action);
    return ;
  }
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // no control found to absorb this click - toggle the track INFO
    CAction action;
    action.wID = ACTION_SHOW_INFO;
    OnAction(action);
  }
}

void CGUIWindowVisualisation::Render()
{
  g_application.ResetScreenSaver();
  CGUIWindow::Render();
}

void CGUIWindowVisualisation::FreeResources()
{
  // Save changed settings from music OSD
  g_settings.Save();
  CGUIWindow::FreeResources();
}

void CGUIWindowVisualisation::OnWindowLoaded()
{
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
    Add(pVisControl);
  }
}