
#include "stdafx.h"
#include "GUIDialogVolumeBar.h"
#include "GUISliderControl.h"
#include "Application.h"
#include "utils/GUIInfoManager.h"

#define VOLUME_BAR_DISPLAY_TIME 1000L

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#define POPUP_VOLUME_SLIDER     401
#define POPUP_VOLUME_LEVEL_TEXT 402
#endif

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

  case GUI_MSG_LABEL_SET:
    {
      if (g_SkinInfo.GetVersion() < 1.86)
      {
        if (message.GetSenderId() == GetID() && message.GetControlId() == POPUP_VOLUME_LEVEL_TEXT)
          CGUIDialog::OnMessage(message);
      }
    }
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogVolumeBar::ResetTimer()
{
  m_dwTimer = timeGetTime();
}

void CGUIDialogVolumeBar::Render()
{
  // set the level on our slider
  if (g_SkinInfo.GetVersion() < 1.86)
  {
    CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_VOLUME_SLIDER);
    if (pSlider) pSlider->SetPercentage(g_application.GetVolume());   // Update our volume bar accordingly
    // and set the level in our text label
    SET_CONTROL_LABEL(POPUP_VOLUME_LEVEL_TEXT, g_infoManager.GetLabel(32));
  }
  // and render the controls
  CGUIDialog::Render();
  // now check if we should exit
  if (timeGetTime() - m_dwTimer > VOLUME_BAR_DISPLAY_TIME)
  {
    Close();
  }
}
