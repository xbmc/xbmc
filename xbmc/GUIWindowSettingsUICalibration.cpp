#include "stdafx.h"
#include "GUIWindowSettingsUICalibration.h"
#include "GUIMoverControl.h"
#include "Application.h"
#include "GUIWindowSettingsScreenCalibration.h"

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#define CONTROL_TOPLEFT 8
#define CONTROL_BOTTOMRIGHT 9
#define CONTROL_LABEL 3

CGUIWindowSettingsUICalibration::CGUIWindowSettingsUICalibration(void)
    : CGUIWindow(WINDOW_UI_CALIBRATION, "SettingsUICalibration.xml")
{
}

CGUIWindowSettingsUICalibration::~CGUIWindowSettingsUICalibration(void)
{}

bool CGUIWindowSettingsUICalibration::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    g_settings.Save();
    m_gWindowManager.PreviousWindow();
    return true;
  }
  else if (action.wID == ACTION_CALIBRATE_SWAP_ARROWS)
  {
    if (m_control == CONTROL_TOPLEFT)
      m_control = CONTROL_BOTTOMRIGHT;
    else
      m_control = CONTROL_TOPLEFT;
    SET_CONTROL_FOCUS(m_control, 0);
    return true;
  }
  else if (action.wID == ACTION_CALIBRATE_RESET)
  {
    g_graphicsContext.ResetOverscan(g_guiSettings.m_LookAndFeelResolution, g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].GUIOverscan);
    ResetControls();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSettingsUICalibration::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);

      ResetControls();

      m_control = CONTROL_TOPLEFT;
      SET_CONTROL_FOCUS(m_control, 0);

      return true;
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsUICalibration::Render()
{
  // Get the information from the control
  CStdStringW strStatus;
  RESOLUTION res = g_guiSettings.m_LookAndFeelResolution;
  CGUIMoverControl *pControl = (CGUIMoverControl *)GetControl(m_control);
  if (pControl)
  {
    if (m_control == CONTROL_TOPLEFT)
    {
      g_settings.m_ResInfo[res].GUIOverscan.left = pControl->GetXLocation();
      g_settings.m_ResInfo[res].GUIOverscan.top = pControl->GetYLocation();
      strStatus.Format(L"%s (%i,%i)", g_localizeStrings.Get(272).c_str(), pControl->GetXLocation(), pControl->GetYLocation());
    }
    else //if (m_control == CONTROL_BOTTOMRIGHT)
    {
      g_settings.m_ResInfo[res].GUIOverscan.right = pControl->GetXLocation();
      g_settings.m_ResInfo[res].GUIOverscan.bottom = pControl->GetYLocation();
      int iXOff1 = g_settings.m_ResInfo[res].iWidth - pControl->GetXLocation();
      int iYOff1 = g_settings.m_ResInfo[res].iHeight - pControl->GetYLocation();
      strStatus.Format(L"%s (%i,%i)", g_localizeStrings.Get(273).c_str(), iXOff1, iYOff1);
    }
  }
  // Set the label and hide our render controls
  SET_CONTROL_LABEL(CONTROL_LABEL, strStatus);
  // set all controls visible except for our movers
  for (unsigned int i=0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    bool hidden = pControl->GetID() == CONTROL_TOPLEFT || pControl->GetID() == CONTROL_BOTTOMRIGHT;
    pControl->SetVisible(!hidden);
  }
  // And render
  m_needsScaling = true;
  CGUIWindow::Render();
  // set all controls hidden except for our movers
  for (unsigned int i=0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    bool hidden = pControl->GetID() == CONTROL_TOPLEFT || pControl->GetID() == CONTROL_BOTTOMRIGHT;
    pControl->SetVisible(hidden);
  }
  // And render
  m_needsScaling = false;
  CGUIWindow::Render();
}

void CGUIWindowSettingsUICalibration::ResetControls()
{
  CGUIMoverControl *pControl = (CGUIMoverControl *)GetControl(CONTROL_TOPLEFT);
  RESOLUTION res = g_guiSettings.m_LookAndFeelResolution;
  if (pControl)
  {
    pControl->SetLimits(-g_settings.m_ResInfo[res].iWidth / 4,
                        -g_settings.m_ResInfo[res].iWidth / 4,
                        g_settings.m_ResInfo[res].iWidth / 4,
                        g_settings.m_ResInfo[res].iHeight / 4);
    pControl->SetPosition(g_settings.m_ResInfo[res].GUIOverscan.left,
                          g_settings.m_ResInfo[res].GUIOverscan.top);
    pControl->SetLocation(g_settings.m_ResInfo[res].GUIOverscan.left,
                          g_settings.m_ResInfo[res].GUIOverscan.top, false);
  }
  pControl = (CGUIMoverControl *)GetControl(CONTROL_BOTTOMRIGHT);
  if (pControl)
  {
    pControl->SetLimits(g_settings.m_ResInfo[res].iWidth*3 / 4,
                        g_settings.m_ResInfo[res].iHeight*3 / 4,
                        g_settings.m_ResInfo[res].iWidth*5 / 4,
                        g_settings.m_ResInfo[res].iHeight*5 / 4);
    pControl->SetPosition(g_settings.m_ResInfo[res].GUIOverscan.right - (int)pControl->GetWidth(),
                          g_settings.m_ResInfo[res].GUIOverscan.bottom - (int)pControl->GetHeight());
    pControl->SetLocation(g_settings.m_ResInfo[res].GUIOverscan.right,
                          g_settings.m_ResInfo[res].GUIOverscan.bottom, false);
  }
}

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#define CONTROL_OLD_MOVER 2
void CGUIWindowSettingsUICalibration::OnWindowLoaded()
{
  if (g_SkinInfo.GetVersion() < 1.85)
  {
    // check if we have 2 mover controls - if not, load the ones
    // for the screen calibration and use those instead.
    CGUIMoverControl *pTopLeft = (CGUIMoverControl *)GetControl(CONTROL_TOPLEFT);
    CGUIMoverControl *pBottomRight = (CGUIMoverControl *)GetControl(CONTROL_BOTTOMRIGHT);
    if (pTopLeft && pBottomRight)
      return; // we are up to date

    // ok, first off, load the SettingsScreenCalibration file and find the mover control information
    CGUIWindowSettingsScreenCalibration *pScreen = (CGUIWindowSettingsScreenCalibration*)m_gWindowManager.GetWindow(WINDOW_MOVIE_CALIBRATION);
    if (pScreen)
    {
      pScreen->Initialize(); // loads itself
      CGUIMoverControl *pMover = (CGUIMoverControl *)pScreen->GetControl(CONTROL_TOPLEFT);
      if (pMover) pTopLeft = new CGUIMoverControl(*pMover);  
      pMover = (CGUIMoverControl *)pScreen->GetControl(CONTROL_BOTTOMRIGHT);
      if (pMover) pBottomRight = new CGUIMoverControl(*pMover);
      pScreen->ClearAll();  // clear itself
    }
    // remove any current mover controls
    const CGUIControl *pOldMover = GetControl(CONTROL_OLD_MOVER);
    if (pOldMover)
    {
      Remove(CONTROL_OLD_MOVER);
      delete pOldMover;
    }
    if (!pTopLeft || !pBottomRight) return;
    // add the new movers
    Add(pTopLeft);
    Add(pBottomRight);
  }
  CGUIWindow::OnWindowLoaded();
}
#endif