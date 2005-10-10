
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
    : CGUIDialog(WINDOW_DIALOG_SEEK_BAR, "DialogSeekbar.xml")
{
  m_fSeekPercentage = 0.0f;
  m_bRequireSeek = false;
  m_loadOnDemand = false;    // the application class handles our resources
}

CGUIDialogSeekBar::~CGUIDialogSeekBar(void)
{
}

bool CGUIDialogSeekBar::OnAction(const CAction &action)
{
  if (action.wID == ACTION_ANALOG_SEEK_FORWARD || action.wID == ACTION_ANALOG_SEEK_BACK)
  {
    if (!g_infoManager.GetBool(PLAYER_SEEKING), m_dwParentWindowID)
    { // start of seeking
      if (g_infoManager.GetTotalPlayTime())
        m_fSeekPercentage = (float)g_infoManager.GetPlayTime() / g_infoManager.GetTotalPlayTime() * 0.1f;
      // start timer
      m_dwTimer = timeGetTime();
      m_bRequireSeek = false;
      g_infoManager.SetSeeking(true);
    }
    // calculate our seek amount
    if (g_application.m_pPlayer && !g_infoManager.m_performingSeek)
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
      ResetTimer();
    }
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSeekBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  case GUI_MSG_WINDOW_DEINIT:
    return CGUIDialog::OnMessage(message);

  case GUI_MSG_LABEL_SET:
    {
      if (message.GetSenderId() == GetID() && message.GetControlId() == POPUP_SEEK_LABEL)
        CGUIDialog::OnMessage(message);
    }
    break;
  case GUI_MSG_PLAYBACK_STARTED:
    { // new song started while our window is up - update our details
      if (g_infoManager.GetTotalPlayTime())
        m_fSeekPercentage = (float)g_infoManager.GetPlayTime() / g_infoManager.GetTotalPlayTime() * 0.1f;
      m_bRequireSeek = false;
    }
    break;

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
    Close(true);
    return;
  }

  // check if we should seek or exit
  if (!g_infoManager.m_performingSeek && timeGetTime() - m_dwTimer > SEEK_BAR_DISPLAY_TIME)
    g_infoManager.SetSeeking(false);

  // render our controls
  if (!m_bRequireSeek && !g_infoManager.m_performingSeek)
  { // position the bar at our current time
    CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_SEEK_SLIDER);
    if (pSlider) pSlider->SetPercentage((int)((float)g_infoManager.GetPlayTime()/g_infoManager.GetTotalPlayTime() * 0.1f));
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    msg.SetLabel(g_infoManager.GetCurrentPlayTime());
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    msg.SetLabel(GetSeekTimeLabel());
    OnMessage(msg);
  }
  CGUIDialog::Render();

  // Check for seek timeout, and perform the seek
  if (m_bRequireSeek && timeGetTime() - m_dwTimer > SEEK_BAR_SEEK_TIME)
  {
    g_infoManager.m_performingSeek = true;
    float time = g_infoManager.GetTotalPlayTime() * m_fSeekPercentage * 10.0f;
    g_application.m_pPlayer->SeekTime((__int64)time);
    m_bRequireSeek = false;
  }
}

CStdString CGUIDialogSeekBar::GetSeekTimeLabel()
{
  int time = (int)(g_infoManager.GetTotalPlayTime() * m_fSeekPercentage * 0.01f);
  CStdString strHMS;
  CUtil::SecondsToHMSString(time, strHMS, g_application.IsPlayingVideo());
  return strHMS;
}
