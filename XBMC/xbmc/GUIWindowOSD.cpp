#include "stdafx.h"
#include "GUIWindowOSD.h"
#include "Application.h"

CGUIWindowOSD::CGUIWindowOSD(void)
    : CGUIDialog(WINDOW_OSD, "VideoOSD.xml")
{
}

CGUIWindowOSD::~CGUIWindowOSD(void)
{
}

void CGUIWindowOSD::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_bRelativeCoords = true;
}

bool CGUIWindowOSD::OnAction(const CAction &action)
{
  // ACTION_SHOW_OSD should take the OSD away too!
  if (action.wID == ACTION_SHOW_OSD)
  {
    Close();
    return true;
  }
  else if (action.wID == ACTION_NEXT_ITEM || action.wID == ACTION_PREV_ITEM)
  {
    // these could indicate next chapter if video supports it
    if (g_application.m_pPlayer != NULL && g_application.m_pPlayer->OnAction(action))
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
      // Remove our subdialogs if visible
      CGUIDialog *pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
      if (pDialog && pDialog->IsRunning())
        pDialog->Close(true);
      pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
      if (pDialog && pDialog->IsRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
      if (pDialog && pDialog->IsRunning()) pDialog->Close(true);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}
