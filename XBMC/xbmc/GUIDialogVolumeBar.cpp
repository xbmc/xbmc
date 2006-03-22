
#include "stdafx.h"
#include "GUIDialogVolumeBar.h"
#include "GUISliderControl.h"
#include "Application.h"
#include "utils/GUIInfoManager.h"

#define VOLUME_BAR_DISPLAY_TIME 1000L

CGUIDialogVolumeBar::CGUIDialogVolumeBar(void)
    : CGUIDialog(WINDOW_DIALOG_VOLUME_BAR, "DialogVolumeBar.xml")
{
  m_loadOnDemand = false;
}

CGUIDialogVolumeBar::~CGUIDialogVolumeBar(void)
{}

bool CGUIDialogVolumeBar::OnAction(const CAction &action)
{
  if (action.wID == ACTION_VOLUME_UP || action.wID == ACTION_VOLUME_DOWN)
  { // reset the timer, as we've changed the volume level
    ResetTimer();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogVolumeBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      //resources are allocated in g_application
      CGUIDialog::OnMessage(message);
      ResetTimer();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      //don't deinit, g_application handles it
      return CGUIDialog::OnMessage(message);
    }
    break;
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogVolumeBar::ResetTimer()
{
  m_dwTimer = timeGetTime();
}

void CGUIDialogVolumeBar::Render()
{
  // and render the controls
  CGUIDialog::Render();
  // now check if we should exit
  if (timeGetTime() - m_dwTimer > VOLUME_BAR_DISPLAY_TIME)
  {
    Close();
  }
}
