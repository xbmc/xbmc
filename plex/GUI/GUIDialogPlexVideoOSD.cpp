#include "GUIDialogPlexVideoOSD.h"
#include "guilib/GUIWindowManager.h"
#include "PlexApplication.h"
#include "Application.h"

#define OSD_CONTROL_GROUP 1111
#define OSD_TIMEOUT 5 * 1000

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIDialogPlexVideoOSD::CGUIDialogPlexVideoOSD()
  : m_openedFromPause(false)
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexVideoOSD::OnAction(const CAction &action)
{
  int id = action.GetID();

  switch (id)
  {
    case ACTION_NEXT_ITEM:
    case ACTION_PREV_ITEM:
      return false;

    case ACTION_NAV_BACK:
    {
      if (m_openedFromPause || g_application.IsPaused())
      {
        g_application.StopPlaying();
        return true;
      }
      break;
    }

    case ACTION_SHOW_CODEC:
    case ACTION_SHOW_INFO:
      return true;

    case ACTION_SHOW_GUI:
      g_windowManager.PreviousWindow();
      return true;

    case ACTION_SHOW_OSD:
    {
      if (m_openedFromPause)
        return true;

      break;
    }

    case ACTION_MOVE_LEFT:
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_DOWN:
    case ACTION_MOVE_UP:
    case ACTION_SELECT_ITEM:
    {
      const CGUIControl* control = GetControl(OSD_CONTROL_GROUP);
      if (control && !control->IsVisible())
        SET_CONTROL_VISIBLE(OSD_CONTROL_GROUP);

      g_plexApplication.timer->RestartTimeout(OSD_TIMEOUT, this);
    }
  }

  return CGUIDialogVideoOSD::OnAction(action);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexVideoOSD::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    SET_CONTROL_VISIBLE(OSD_CONTROL_GROUP);

    m_openedFromPause = (message.GetStringParam(0) == "pauseOpen");
    if (m_openedFromPause)
    {
      g_plexApplication.timer->SetTimeout(OSD_TIMEOUT, this);
    }
  }

  return CGUIDialogVideoOSD::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexVideoOSD::OnTimeout()
{
  SET_CONTROL_HIDDEN(OSD_CONTROL_GROUP);
}
