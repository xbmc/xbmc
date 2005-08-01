#include "stdafx.h"
#include "GUIWindowVisualisation.h"
#include "GUIVisualisationControl.h"
#include "application.h"
#include "GUIDialogMusicOSD.h"
#include "utils/GUIInfoManager.h"
#include "ButtonTranslator.h"
#include "util.h"
#include "GUIDialogVisualisationPresetList.h"

#define TRANSISTION_COUNT   50  // 1 second
#define TRANSISTION_LENGTH 200  // 4 seconds
#define START_FADE_LENGTH  100  // 2 seconds on startup

#define CONTROL_VIS           2
#define CONTROL_PRESET_LABEL  3
#define CONTROL_LOCKED_IMAGE  4

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
    : CGUIWindow(WINDOW_VISUALISATION, "MusicVisualisation.xml")
{
  m_dwInitTimer = 0;
  m_dwLockedTimer = 0;
  m_bShowInfo = false;
  m_bShowPreset = false;
}

CGUIWindowVisualisation::~CGUIWindowVisualisation(void)
{
}

bool CGUIWindowVisualisation::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_SHOW_INFO:
    {
      // reset the timer
      CLog::Log(LOGINFO, "White pressed - current status is: InitTimer = %i, ShowInfo = %i", m_dwInitTimer, m_bShowInfo);
      if (!m_dwInitTimer || m_bShowInfo)
        m_bShowInfo = !m_bShowInfo;
      FadeControls(1, m_bShowInfo, 25);
      g_stSettings.m_bMyMusicSongThumbInVis = m_bShowInfo;
      return true;
    }
    break;

  case ACTION_SHOW_GUI:
    // save the settings
    g_settings.Save();
    m_gWindowManager.PreviousWindow();
    return true;
    break;

  case ACTION_VIS_PRESET_LOCK:
    { // show the locked icon + fall through so that the vis handles the locking
      if (!m_bShowPreset)
      {
        m_dwLockedTimer = START_FADE_LENGTH;
        g_infoManager.SetShowCodec(true);
      }
    }
    break;
  case ACTION_VIS_PRESET_SHOW:
    {
      if (!m_dwLockedTimer || m_bShowPreset)
        m_bShowPreset = !m_bShowPreset;
      g_infoManager.SetShowCodec(m_bShowPreset);
      return true;
    }
    break;

    // TODO: These should be mapped to it's own function - at the moment it's overriding
    // the global action of fastforward/rewind and OSD.
/*  case KEY_BUTTON_Y:
    g_application.m_CdgParser.Pause();
    return true;
    break;

    case ACTION_ANALOG_FORWARD:
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
  case GUI_MSG_GET_VISUALISATION:
    {
//      message.SetControlID(CONTROL_VIS);
      CGUIVisualisationControl *pVisControl = (CGUIVisualisationControl *)GetControl(CONTROL_VIS);
      if (pVisControl)
        message.SetLPVOID(pVisControl->GetVisualisation());
      return true;
    }
    break;
  case GUI_MSG_VISUALISATION_ACTION:
    {
      // message.SetControlID(CONTROL_VIS);
      CGUIVisualisationControl *pVisControl = (CGUIVisualisationControl *)GetControl(CONTROL_VIS);
      if (pVisControl)
        return pVisControl->OnMessage(message);
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      // check and close any OSD windows
      CGUIDialogMusicOSD *pOSD = (CGUIDialogMusicOSD *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
      if (pOSD && pOSD->IsRunning()) pOSD->Close();
      CGUIDialogVisualisationPresetList *pList = (CGUIDialogVisualisationPresetList *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
      if (pList && pList->IsRunning()) pList->Close();
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

      // hide or show the preset button(s)
      g_infoManager.SetShowCodec(m_bShowPreset);
      CGUIWindow::OnMessage(message);
      m_tag = g_infoManager.GetCurrentSongTag();
      if (g_stSettings.m_bMyMusicSongThumbInVis)
      { // always on
        m_bShowInfo = true;
        m_dwInitTimer = 0;
        FadeControls(1, true, 0);
      }
      else
      {
        // start display init timer (fade out after 3 secs...)
        m_bShowInfo = false;
        m_dwInitTimer = START_FADE_LENGTH;
        FadeControls(1, true, 0);
      }
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
  // check for a tag change
  const CMusicInfoTag &tag = g_infoManager.GetCurrentSongTag();
  if (tag != m_tag)
  { // need to fade in then out again
    CLog::Log(LOGINFO, "Tag changed - current status is: InitTimer = %i, ShowInfo = %i", m_dwInitTimer, m_bShowInfo);
    m_tag = tag;
    // fade in
    m_dwInitTimer = START_FADE_LENGTH;
    FadeControls(1, true, 25);
  }
  if (m_dwInitTimer)
  {
    m_dwInitTimer--;
    if (!m_dwInitTimer && !g_stSettings.m_bMyMusicSongThumbInVis)
    { // reached end of fade in, fade out again
      FadeControls(1, false, 25);
    }
  }
  // show or hide the locked texture
  if (m_dwLockedTimer)
  {
    m_dwLockedTimer--;
    if (!m_dwLockedTimer && !m_bShowPreset)
      g_infoManager.SetShowCodec(false);
  }
  CGUIWindow::Render();
}

void CGUIWindowVisualisation::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // Check if we have a vis control
  for (unsigned int i = 0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    if (pControl->GetControlType() == CGUIControl::GUICONTROL_VISUALISATION)
      return;
  }
  // not found - let's add a new one
  CGUIVisualisationControl *pVisControl = new CGUIVisualisationControl(GetID(), CONTROL_VIS, 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight());
  if (pVisControl)
  {
    m_vecControls.insert(m_vecControls.begin(), pVisControl);
  }
}

void CGUIWindowVisualisation::FadeControls(DWORD controlID, bool fadeIn, DWORD length)
{
  CGUIMessage msg(fadeIn ? GUI_MSG_VISIBLE : GUI_MSG_HIDDEN, GetID(), controlID, length);
  for (unsigned int i=0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    if (pControl->GetID() == controlID)
      pControl->OnMessage(msg);
  }
}

void CGUIWindowVisualisation::AllocResources(bool forceLoad)
{
  CGUIWindow::AllocResources(forceLoad);
  CGUIWindow *pWindow;
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
  if (pWindow) pWindow->AllocResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_VIS_SETTINGS);
  if (pWindow) pWindow->AllocResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
  if (pWindow) pWindow->AllocResources(true);
}

void CGUIWindowVisualisation::FreeResources(bool forceUnload)
{
  // Save changed settings from music OSD
  g_settings.Save();
  CGUIWindow *pWindow;
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
  if (pWindow) pWindow->FreeResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_VIS_SETTINGS);
  if (pWindow) pWindow->FreeResources(true);
  pWindow = m_gWindowManager.GetWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
  if (pWindow) pWindow->FreeResources(true);
  CGUIWindow::FreeResources(forceUnload);
}