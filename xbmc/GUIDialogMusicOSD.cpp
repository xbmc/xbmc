#include "stdafx.h"
#include "GUIDialogMusicOSD.h"
#include "GUIWindowSettingsCategory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CONTROL_VIS_BUTTON       500
#define CONTROL_LOCK_BUTTON      501
#define CONTROL_VIS_CHOOSER      503

CGUIDialogMusicOSD::CGUIDialogMusicOSD(void)
    : CGUIDialog(0)
{
  m_iLastControl = -1;
  m_pVisualisation = NULL;
}

CGUIDialogMusicOSD::~CGUIDialogMusicOSD(void)
{
}

bool CGUIDialogMusicOSD::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == CONTROL_VIS_CHOOSER)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
        OnMessage(msg);
        CStdString strLabel = msg.GetLabel();
        if (msg.GetParam1() == 0)
          g_guiSettings.SetString("MyMusic.Visualisation", strLabel);
        else
          g_guiSettings.SetString("MyMusic.Visualisation", strLabel + ".vis");
        // hide the control and reset focus
        SET_CONTROL_HIDDEN(CONTROL_VIS_CHOOSER);
        SET_CONTROL_FOCUS(CONTROL_VIS_BUTTON, 0);
        return true;
      }
      else if (iControl == CONTROL_VIS_BUTTON)
      {
        SET_CONTROL_VISIBLE(CONTROL_VIS_CHOOSER);
        SET_CONTROL_FOCUS(CONTROL_VIS_CHOOSER, 0);
        // fire off an event that we've pressed this button...
        CAction action;
        action.wID = ACTION_SELECT_ITEM;
        OnAction(action);
      }
      else if (iControl == CONTROL_LOCK_BUTTON)
      {
        CGUIMessage msg(GUI_MSG_VISUALISATION_ACTION, 0, 0, ACTION_VIS_PRESET_LOCK);
        g_graphicsContext.SendMessage(msg);
      }
      return true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      CSetting *pSetting = g_guiSettings.GetSetting("MyMusic.Visualisation");
      CGUIWindowSettingsCategory::FillInVisualisations(pSetting, CONTROL_VIS_CHOOSER);

      SET_CONTROL_HIDDEN(CONTROL_VIS_CHOOSER);
      SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      m_pVisualisation = NULL;
    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
      if (message.GetLPVOID())
        m_pVisualisation = (CVisualisation *)message.GetLPVOID();
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogMusicOSD::Render()
{
  CGUIDialog::Render();
}
