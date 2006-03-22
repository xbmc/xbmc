#include "stdafx.h"
#include "GUIWindowOSD.h"

CGUIWindowOSD::CGUIWindowOSD(void)
    : CGUIDialog(WINDOW_OSD, "VideoOSD.xml")
{
  m_loadOnDemand = false;
}

CGUIWindowOSD::~CGUIWindowOSD(void)
{
}

void CGUIWindowOSD::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_bRelativeCoords = true;
}

void CGUIWindowOSD::Render()
{
  if ( g_guiSettings.GetInt("MyVideos.OSDTimeout") &&
      !m_gWindowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS) &&
      !m_gWindowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS) &&
      !m_gWindowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS) )
  {
    if ( (timeGetTime() - m_dwOSDTimeOut) > (DWORD)(g_guiSettings.GetInt("MyVideos.OSDTimeout") * 1000))
    {
      Close();
    }
  }
  CGUIDialog::Render();  // render our controls to the screen
}

bool CGUIWindowOSD::OnAction(const CAction &action)
{
  m_dwOSDTimeOut = timeGetTime();
  // ACTION_SHOW_OSD should take the OSD away too!
  if (action.wID == ACTION_SHOW_OSD)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIWindowOSD::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_VIDEO_MENU_STARTED:
    {
      // We have gone to the DVD menu, so close the OSD.
      Close();
    }
  case GUI_MSG_WINDOW_DEINIT:  // fired when OSD is hidden
    {
      // don't save the settings here, it's bad for the hd-spindown feature (causes spinup)
      // settings are saved in FreeResources in GUIWindowFullScreen
      //g_settings.Save();
      // Remove our subdialogs if visible
      CGUIDialog *pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
      if (pDialog && pDialog->IsRunning())
        pDialog->Close(true);
      pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
      if (pDialog && pDialog->IsRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
      if (pDialog && pDialog->IsRunning()) pDialog->Close(true);
      //return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:  // fired when OSD is shown
    {
      CGUIDialog::OnMessage(message);
      // position correctly
      int iResolution = g_graphicsContext.GetVideoResolution();
      SetPosition(0, g_settings.m_ResInfo[iResolution].iOSDYOffset);

      m_dwOSDTimeOut = timeGetTime();
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}
