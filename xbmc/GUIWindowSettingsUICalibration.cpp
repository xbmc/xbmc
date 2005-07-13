
#include "stdafx.h"
#include "GUIWindowSettingsUICalibration.h"
#include "GUIMoverControl.h"
#include "Application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CONTROL_MOVER 2
#define CONTROL_LABEL 3

CGUIWindowSettingsUICalibration::CGUIWindowSettingsUICalibration(void)
    : CGUIWindow(0)
{}

CGUIWindowSettingsUICalibration::~CGUIWindowSettingsUICalibration(void)
{}

bool CGUIWindowSettingsUICalibration::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_iLastControl = -1;
    m_gWindowManager.PreviousWindow();
    return true;
  }
  else if (action.wID == ACTION_CALIBRATE_RESET)
  {
    CGUIMoverControl *pControl = (CGUIMoverControl *)GetControl(CONTROL_MOVER);
    if (pControl) pControl->SetLocation(0, 0);
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
      g_settings.Save();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      CGUIMoverControl *pControl = (CGUIMoverControl *)GetControl(CONTROL_MOVER);
      if (pControl)
      {
        pControl->EnableCalibration(false);
        pControl->SetLimits( -100, -100, 100, 100);
        pControl->SetLocation(g_guiSettings.GetInt("UIOffset.X"), g_guiSettings.GetInt("UIOffset.Y"));
        pControl->SetFocus(true);
      }

      if (m_iLastControl > -1)
      {
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

      return true;
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      m_iLastControl = message.GetControlId();
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsUICalibration::Render()
{
  // Get the information from the control
  CGUIMoverControl *pControl = (CGUIMoverControl *)GetControl(CONTROL_MOVER);
  if (pControl)
  {
    if (g_guiSettings.GetInt("UIOffset.X") != pControl->GetXLocation() ||
        g_guiSettings.GetInt("UIOffset.Y") != pControl->GetYLocation())
    {
      g_guiSettings.SetInt("UIOffset.X", pControl->GetXLocation());
      g_guiSettings.SetInt("UIOffset.Y", pControl->GetYLocation());
      g_graphicsContext.SetOffset(g_guiSettings.GetInt("UIOffset.X"), g_guiSettings.GetInt("UIOffset.Y"));
      ResetAllControls();
      g_application.ResetAllControls();
    }
  }
  // Set the label
  CStdString strOffset;
  strOffset.Format("%i,%i", g_guiSettings.GetInt("UIOffset.X"), g_guiSettings.GetInt("UIOffset.Y"));
  SET_CONTROL_LABEL(CONTROL_LABEL, strOffset);
  // And render
  CGUIWindow::Render();
}
