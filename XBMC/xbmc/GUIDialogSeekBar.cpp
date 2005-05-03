
#include "stdafx.h"
#include "GUIDialogSeekBar.h"
#include "GUISliderControl.h"
#include "Application.h"
#include "utils/GUIInfoManager.h"
#include "Util.h"

// May need to change this so that it is "modeless" rather than Modal,
// though it works reasonably well as is...

#define SEEK_BAR_DISPLAY_TIME 1000L
#define SEEK_BAR_SEEK_TIME    500L

#define SEEK_SPEED            2.0f

#define POPUP_SEEK_SLIDER     401
#define POPUP_SEEK_LABEL      402

CGUIDialogSeekBar::CGUIDialogSeekBar(void)
    : CGUIDialog(0)
{
  m_bNeedsScaling = true; // make sure we scale this window, as it appears on different resolutions
  m_fSeekPercentage = 0.0f;
  m_bRequireSeek = false;
}

CGUIDialogSeekBar::~CGUIDialogSeekBar(void)
{
}

void CGUIDialogSeekBar::OnAction(const CAction &action)
{
  if (action.wID == ACTION_ANALOG_SEEK_FORWARD || action.wID == ACTION_ANALOG_SEEK_BACK)
  {
    // calculate our seek amount
    if (g_application.m_pPlayer)
    {
      if (action.wID == ACTION_ANALOG_SEEK_FORWARD)
        m_fSeekPercentage += action.fAmount1 * SEEK_SPEED;
      else
        m_fSeekPercentage -= action.fAmount1 * SEEK_SPEED;
      if (m_fSeekPercentage > 100.0f) m_fSeekPercentage = 100.0f;
      if (m_fSeekPercentage < 0.0f) m_fSeekPercentage = 0.0f;
      CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_SEEK_SLIDER);
      if (pSlider) pSlider->SetPercentage((int)m_fSeekPercentage);   // Update our seek bar accordingly
      m_bRequireSeek = true;
    }
    ResetTimer();
    return ;
  }
  if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
    Close();
    return ;
  }
  CGUIDialog::OnAction(action);
}

bool CGUIDialogSeekBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      //resources are allocated in g_application
      //CGUIDialog::OnMessage(message);
      if (g_application.m_pPlayer)
        m_fSeekPercentage = g_application.m_pPlayer->GetPercentage();
      // start timer
      m_dwTimer = timeGetTime();
      m_bRequireSeek = false;
      // levels are set in Render(), so no need to do them here...
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      //don't deinit, g_application handles it
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == POPUP_SEEK_SLIDER) // who else is it going to be??
      {
        CGUISliderControl* pControl = (CGUISliderControl*)GetControl(message.GetSenderId());
        if (pControl)
        {
          // Set the global volume setting to the percentage requested
          int iPercentage = pControl->GetPercentage();
          g_application.SetVolume(iPercentage);
          // Label and control will auto-update when Render() is called.
        }
        // reset the timer
        m_dwTimer = timeGetTime();
        return true;
      }
    }
    break;
  case GUI_MSG_LABEL_SET:
    {
      if (message.GetSenderId() == GetID() && message.GetControlId() == POPUP_SEEK_LABEL)
        CGUIDialog::OnMessage(message);
    }
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogSeekBar::ResetTimer()
{
  m_dwTimer = timeGetTime();
}

void CGUIDialogSeekBar::Render()
{
  if (!g_application.m_pPlayer)
  {
    Close();
    return;
  }

  if (!m_bRequireSeek)
  { // position the bar at our current time
    CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_SEEK_SLIDER);
    if (pSlider) pSlider->SetPercentage((int)g_application.m_pPlayer->GetPercentage());
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    msg.SetLabel(g_infoManager.GetCurrentPlayTime());
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    int time = (int)(g_application.m_pPlayer->GetTotalTime() * m_fSeekPercentage * 0.01f);
    CStdString strHMS;
    CUtil::SecondsToHMSString(time, strHMS, g_application.IsPlayingVideo());
    msg.SetLabel(strHMS);
    OnMessage(msg);
  }
  // render the controls
  CGUIDialog::Render();
  // and check if we should seek or exit
  if (timeGetTime() - m_dwTimer > SEEK_BAR_DISPLAY_TIME)
  {
    Close();
  }
  else if (m_bRequireSeek && timeGetTime() - m_dwTimer > SEEK_BAR_SEEK_TIME)
  {
    g_application.m_pPlayer->SeekPercentage(m_fSeekPercentage);
    m_bRequireSeek = false;
  }
}
